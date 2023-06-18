/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "L3DMesh.h"

#include <stdexcept>

#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <L3DFile.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <glm/matrix.hpp>
#include <spdlog/spdlog.h>

#include "FileSystem/FileSystem.h"
#include "FileSystem/IStream.h"
#include "Game.h"
#include "Graphics/VertexBuffer.h"

using namespace openblack;
using namespace openblack::graphics;

L3DMesh::L3DMesh(std::string debugName)
    : _flags(static_cast<l3d::L3DMeshFlags>(0))
    , _debugName(std::move(debugName))
{
}

L3DMesh::~L3DMesh() = default;

void L3DMesh::Load(const l3d::L3DFile& l3d)
{
	_flags = static_cast<l3d::L3DMeshFlags>(l3d.GetHeader().flags);
	_nameData = l3d.GetNameData();
	for (const auto& skin : l3d.GetSkins())
	{
		_skins[skin.id] = std::make_unique<Texture2D>(_debugName.c_str());
		_skins[skin.id]->Create(l3d::L3DTexture::k_Width, l3d::L3DTexture::k_Height, 1, Format::BGRA4, Wrapping::Repeat,
		                        Filter::Linear, skin.texels.data(),
		                        static_cast<uint32_t>(skin.texels.size() * sizeof(skin.texels[0])));
	}

	if (HasDoorPosition() && !l3d.GetExtraPoints().empty())
	{
		_doorPos = glm::vec3(l3d.GetExtraPoints()[0].x, l3d.GetExtraPoints()[0].y, l3d.GetExtraPoints()[0].z);
	}

	if (ContainsLandscapeFeature() && l3d.GetFootprint().has_value())
	{
		struct FootprintVertex
		{
			glm::vec2 pos;
			glm::vec2 texCoord;
		};
		VertexDecl decl;
		decl.reserve(1);
		decl.emplace_back(VertexAttrib::Attribute::Position, static_cast<uint8_t>(2), VertexAttrib::Type::Float);
		decl.emplace_back(VertexAttrib::Attribute::TexCoord0, static_cast<uint8_t>(2), VertexAttrib::Type::Float);

		const auto& footprint = l3d.GetFootprint().value();
		for (uint32_t i = 1; const auto& entry : footprint.entries)
		{
			auto texture = std::make_unique<Texture2D>("footprints/texture/" + _debugName + "/" + std::to_string(i));
			++i;
			texture->Create(static_cast<uint16_t>(footprint.header.width), static_cast<uint16_t>(footprint.header.height), 1,
			                graphics::Format::BGRA4, Wrapping::ClampEdge, Filter::Linear, entry.pixels.data(),
			                static_cast<uint32_t>(entry.pixels.size() * sizeof(entry.pixels[0])));

			const bgfx::Memory* verticesMem =
			    bgfx::alloc(static_cast<uint32_t>(sizeof(FootprintVertex) * entry.triangles.size() * 3));
			auto* vertices = reinterpret_cast<FootprintVertex*>(verticesMem->data);
			for (uint8_t j = 0; const auto& t : entry.triangles)
			{
				for (uint8_t k = 0; k < 3; ++k)
				{
					// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index): access is bound to size
					auto& vertex = vertices[j];
					++j;

					// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index): access is bound to size
					const auto& world = t.world[k];
					// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index): access is bound to size
					const auto& uv = t.texture[k];

					vertex.pos.x = world.x;
					vertex.pos.y = world.y;
					vertex.texCoord.x = uv.x / footprint.header.width;
					vertex.texCoord.y = uv.y / footprint.header.height;
				}
			}

			auto* vertexBuffer = new VertexBuffer("footprints/quad/" + _debugName + "/" + std::to_string(i), verticesMem, decl);
			auto mesh = std::make_unique<Mesh>(vertexBuffer);
			_footprints.emplace_back(Footprint {std::move(texture), std::move(mesh)});
		}
	}

	if (ContainsExtraMetrics() && !l3d.GetExtraMetrics().empty())
	{
		const auto& extraMetrics = l3d.GetExtraMetrics();
		_extraMetrics.reserve(extraMetrics.size());
		for (const auto& e : extraMetrics)
		{
			_extraMetrics.emplace_back(static_cast<glm::mat4>(glm::make_mat4x3(e.data())));
		}
	}

	std::map<uint32_t, glm::mat4> matrices;
	const auto& bones = l3d.GetBones();
	_bonesParents.resize(bones.size());
	for (uint32_t i = 0; i < bones.size(); ++i)
	{
		const auto& bone = bones[i];
		// clang-format off
		auto matrix = glm::mat4(bone.orientation[0], bone.orientation[1], bone.orientation[2], 0.0f,
		                        bone.orientation[3], bone.orientation[4], bone.orientation[5], 0.0f,
		                        bone.orientation[6], bone.orientation[7], bone.orientation[8], 0.0f,
		                        bone.position.x, bone.position.y, bone.position.z, 1.0f);
		// clang-format on
		_bonesParents[i] = bone.parent;
		if (bone.parent != std::numeric_limits<uint32_t>::max())
		{
			matrix = matrices[bone.parent] * matrix;
		}
		_bonesDefaultMatrices.emplace_back(matrix);
		matrices.emplace(i, matrix);
	}

	auto submeshCount = l3d.GetSubmeshHeaders().size();
	for (uint32_t i = 0; i < submeshCount; ++i)
	{
		auto subMesh = std::make_unique<L3DSubMesh>(*this);
		if (!subMesh->Load(l3d, i))
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open L3DSubMesh from file: {}", l3d.GetFilename());
			continue;
		}
		if (subMesh->GetFlags().isPhysics)
		{
			const auto& verticesSpan = l3d.GetVertexSpan(i);
			auto* physicsMesh =
			    new btConvexHullShape(reinterpret_cast<const btScalar*>(verticesSpan.data()),
			                          static_cast<int>(verticesSpan.size()), static_cast<int>(sizeof(verticesSpan[0])));
			physicsMesh->optimizeConvexHull();
			_physicsMesh.reset(physicsMesh);
			// FIXME(bwrsandman): Some meshes have multiple physics meshes
		}
		const auto& bb = subMesh->GetBoundingBox();
		_boundingBox.minima = glm::min(_boundingBox.minima, bb.minima);
		_boundingBox.maxima = glm::max(_boundingBox.maxima, bb.maxima);

		_subMeshes.emplace_back(std::move(subMesh));
	}
	// TODO(bwrsandman): if no physics mesh was found, make physics mesh the bounding box

	// TODO(bwrsandman): store vertex and index buffers at mesh level
	bgfx::frame();
}

bool L3DMesh::LoadFromFile(const std::filesystem::path& path)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading L3DMesh from file: {}", path.generic_string());
	l3d::L3DFile l3d;

	try
	{
		l3d.Open(Game::Instance()->GetFileSystem().FindPath(path));
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open l3d mesh from filesystem {}: {}", path.generic_string(),
		                    err.what());
		return false;
	}

	Load(l3d);
	return true;
}

bool L3DMesh::LoadFromBuffer(const std::vector<uint8_t>& data)
{
	l3d::L3DFile l3d;

	try
	{
		l3d.Open(data);
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open l3d mesh from buffer: {}", err.what());
		return false;
	}

	Load(l3d);
	return true;
}

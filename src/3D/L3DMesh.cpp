/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "L3DMesh.h"

#include <cassert>

#include <algorithm>
#include <filesystem>
#include <stdexcept>

#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <L3DFile.h>
#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <glm/matrix.hpp>
#include <spdlog/spdlog.h>

#include "3D/L3DSubMesh.h"
#include "FileSystem/FileSystemInterface.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexBuffer.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::graphics;

L3DMesh::L3DMesh(std::string debugName, bool dynamic) noexcept
    : _flags(static_cast<l3d::L3DMeshFlags>(0))
    , _debugName(std::move(debugName))
    , _dynamic(dynamic)
{
}

L3DMesh::~L3DMesh() noexcept = default;

bool L3DMesh::Load(const l3d::L3DFile& l3d) noexcept
{
	bool result = true;

	_flags = static_cast<l3d::L3DMeshFlags>(l3d.GetHeader().flags);
	_nameData = l3d.GetNameData();
	for (const auto& skin : l3d.GetSkins())
	{
		_skinOrder.push_back(skin.id);
		_skins[skin.id] = std::make_unique<Texture2D>(_debugName.c_str());
		const auto size = static_cast<uint32_t>(skin.texels.size() * sizeof(skin.texels[0]));
		// bgfx only lets a texture created without texels have them changed
		_skins[skin.id]->Create(l3d::L3DTexture::k_Width, l3d::L3DTexture::k_Height, 1, TextureFormat::BGRA4, Wrapping::Repeat,
		                        Filter::Linear, _dynamic ? nullptr : bgfx::makeRef(skin.texels.data(), size));
		if (_dynamic)
		{
			_skins[skin.id]->Update(skin.texels.data(), size);
		}
	}

	if (HasDoorPosition() && !l3d.GetExtraPoints().empty())
	{
		_doorPos = glm::vec3(l3d.GetExtraPoints()[0].x, l3d.GetExtraPoints()[0].y, l3d.GetExtraPoints()[0].z);
	}

	// A chimney is always the second extra point, the first being the door
	if (HasChimney() && l3d.GetExtraPoints().size() >= 2)
	{
		_chimneyPos = glm::vec3(l3d.GetExtraPoints()[1].x, l3d.GetExtraPoints()[1].y, l3d.GetExtraPoints()[1].z);
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

		const auto& footprint = *l3d.GetFootprint();

		// TODO (#749) use use std::views::enumerate
		for (uint32_t i = 1; const auto& entry : footprint.entries)
		{
			auto texture = std::make_unique<Texture2D>("footprints/texture/" + _debugName + "/" + std::to_string(i));
			++i;
			texture->Create(
			    static_cast<uint16_t>(footprint.header.width), static_cast<uint16_t>(footprint.header.height), 1,
			    graphics::TextureFormat::BGRA4, Wrapping::ClampEdge, Filter::Linear,
			    bgfx::makeRef(entry.pixels.data(), static_cast<uint32_t>(entry.pixels.size() * sizeof(entry.pixels[0]))));

			const bgfx::Memory* verticesMem =
			    bgfx::alloc(static_cast<uint32_t>(sizeof(FootprintVertex) * entry.triangles.size() * 3));
			auto* vertices = reinterpret_cast<FootprintVertex*>(verticesMem->data);
			// TODO (#749) Maybe use std::views::enumerate
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
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open L3DSubMesh");
			result = false;
			continue;
		}
		{
			// Its surface, as the game picks points on it
			auto& surface = _surfaces.emplace_back();
			const auto vertices = l3d.GetVertexSpan(i);
			surface.positions.reserve(vertices.size());
			for (const auto& vertex : vertices)
			{
				surface.positions.emplace_back(vertex.position.x, vertex.position.y, vertex.position.z);
			}
			const auto indices = l3d.GetIndexSpan(i);
			surface.indices.assign(indices.begin(), indices.end());
			uint32_t vertexBase = 0;
			uint32_t indexBase = 0;
			for (const auto& primitive : l3d.GetPrimitiveSpan(i))
			{
				surface.primitives.push_back(
				    {.vertexBase = vertexBase, .indexBase = indexBase, .numTriangles = primitive.numTriangles});
				vertexBase += primitive.numVertices;
				indexBase += primitive.numTriangles * 3;
			}
		}
		if (subMesh->GetFlags().isPhysics)
		{
			const auto& verticesSpan = l3d.GetVertexSpan(i);
			auto* physicsMesh =
			    new btConvexHullShape(reinterpret_cast<const btScalar*>(verticesSpan.data()),
			                          static_cast<int>(verticesSpan.size()), static_cast<int>(sizeof(verticesSpan[0])));
			physicsMesh->optimizeConvexHull();
			_physicsMesh.reset(physicsMesh);
			// Its triangles, each primitive's indices counting from its own first vertex
			uint32_t vertexBase = 0;
			uint32_t indexBase = 0;
			const auto indices = l3d.GetIndexSpan(i);
			for (const auto& primitive : l3d.GetPrimitiveSpan(i))
			{
				for (uint32_t t = 0; t < primitive.numTriangles && indexBase + (t * 3) + 2 < indices.size(); ++t)
				{
					std::array<glm::vec3, 3> triangle {};
					for (uint32_t c = 0; c < 3; ++c)
					{
						const auto index = vertexBase + indices[indexBase + (t * 3) + c];
						if (index < verticesSpan.size())
						{
							const auto& p = verticesSpan[index].position;
							triangle.at(c) = glm::vec3(p.x, p.y, p.z);
						}
					}
					_physicsTriangles.push_back(triangle);
				}
				vertexBase += primitive.numVertices;
				indexBase += primitive.numTriangles * 3;
			}
			// FIXME(bwrsandman): Some meshes have multiple physics meshes
		}
		const auto& bb = subMesh->GetBoundingBox();
		_boundingBox.minima = glm::min(_boundingBox.minima, bb.minima);
		_boundingBox.maxima = glm::max(_boundingBox.maxima, bb.maxima);

		_subMeshes.emplace_back(std::move(subMesh));
	}

	// The windows of the temple shed volumes of light, with the texture of their first primitive. The temple's windows
	// are each a single primitive.
	const auto& names = l3d.GetSubmeshNames();
	for (uint32_t i = 0; i < names.size() && i < submeshCount; ++i)
	{
		const auto& name = names[i];
		const auto& primitives = l3d.GetPrimitiveSpan(i);
		if ((name.flags & l3d::L3DSubmeshName::VolumeLight) == 0 || primitives.empty())
		{
			continue;
		}
		const auto& primitive = primitives.front();
		const auto vertices = l3d.GetVertexSpan(i).first(std::min<size_t>(primitive.numVertices, l3d.GetVertexSpan(i).size()));
		const auto indices =
		    l3d.GetIndexSpan(i).first(std::min<size_t>(primitive.numTriangles * 3, l3d.GetIndexSpan(i).size()));
		const glm::vec3 source {name.volumeLightSource.x, name.volumeLightSource.y, name.volumeLightSource.z};
		_volumeLights.push_back({
		    .skinID = primitive.material.skinID,
		    .mesh = MakeVolumeLight(vertices, indices, source, name.volumeLightLength),
		});
	}
	// TODO(bwrsandman): if no physics mesh was found, make physics mesh the bounding box

	// TODO(bwrsandman): store vertex and index buffers at mesh level
	bgfx::frame();

	return result;
}

std::optional<L3DMesh::PickHit> L3DMesh::Pick(glm::vec3 origin, glm::vec3 direction, bool onlyJoints, bool withoutJoints,
                                              std::span<const uint32_t> hidden) const
{
	std::optional<PickHit> nearest;
	for (uint32_t i = 0; i < _subMeshes.size(); ++i)
	{
		const auto& subMesh = _subMeshes[i];
		// The submeshes drawn: no physics, statuses or low levels of detail
		if (subMesh->IsPhysics() || subMesh->GetFlags().status != 0 || (subMesh->GetFlags().lodMask & 1) != 1)
		{
			continue;
		}
		if (onlyJoints && !subMesh->GetJoint().has_value())
		{
			continue;
		}
		if ((withoutJoints && subMesh->GetJoint().has_value()) || std::ranges::find(hidden, i) != hidden.end())
		{
			continue;
		}
		if (const auto distance = subMesh->Pick(origin, direction);
		    distance.has_value() && (!nearest.has_value() || *distance < nearest->distance))
		{
			nearest = PickHit {.distance = *distance, .subMesh = i};
		}
	}
	return nearest;
}

void L3DMesh::UpdateVertices(const l3d::L3DFile& l3d) noexcept
{
	assert(_dynamic);
	_boundingBox = {
	    glm::vec3(std::numeric_limits<float>::max()),
	    glm::vec3(std::numeric_limits<float>::lowest()),
	};
	for (const auto& subMesh : _subMeshes)
	{
		subMesh->UpdateVertices(l3d);
		const auto& bb = subMesh->GetBoundingBox();
		_boundingBox.minima = glm::min(_boundingBox.minima, bb.minima);
		_boundingBox.maxima = glm::max(_boundingBox.maxima, bb.maxima);
	}
}

void L3DMesh::UpdateSkin(SkinId skin, std::span<const uint16_t> texels) noexcept
{
	assert(_dynamic);
	if (const auto found = _skins.find(skin); found != _skins.end())
	{
		found->second->Update(texels.data(), static_cast<uint32_t>(texels.size_bytes()));
	}
}

bool L3DMesh::LoadFromFilesystem(const std::filesystem::path& path) noexcept
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading L3DMesh from file: {}", path.generic_string());
	l3d::L3DFile l3d;

	try
	{
		l3d.ReadFile(*Locator::filesystem::value().GetData(path));
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

bool L3DMesh::LoadFromFile(const std::filesystem::path& path) noexcept
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading L3DMesh from file: {}", path.generic_string());
	l3d::L3DFile l3d;

	const auto result = l3d.Open(Locator::filesystem::value().FindPath(path));
	if (result != l3d::L3DResult::Success)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open l3d mesh from filesystem {}: {}", path.generic_string(),
		                    l3d::ResultToStr(result));
		return false;
	}

	if (!Load(l3d))
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "Some issues were seen while loading l3d mesh from from file: {}.",
		                   path.generic_string());
	}

	return true;
}

bool L3DMesh::LoadFromBuffer(const std::vector<uint8_t>& data) noexcept
{
	l3d::L3DFile l3d;

	const auto result = l3d.Open(data);
	if (result != l3d::L3DResult::Success)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open l3d mesh from buffer: {}", l3d::ResultToStr(result));
		return false;
	}

	if (!Load(l3d))
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "Some issues were seen while loading l3d mesh from buffer.");
	}

	return true;
}

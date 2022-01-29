/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "L3DMesh.h"

#include "3D/MeshPack.h"
#include "Common/FileSystem.h"
#include "Common/IStream.h"
#include "Game.h"

#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <L3DFile.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/matrix.hpp>
#include <spdlog/spdlog.h>

#include <stdexcept>

using namespace openblack;
using namespace openblack::graphics;

L3DMesh::L3DMesh(const std::string& debugName)
    : _flags(static_cast<l3d::L3DMeshFlags>(0))
    , _debugName(debugName)
    , _physicsMass(1.0f) // TODO(bwrsandman): Find somewhere in file a value
    , _boundingBox {glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX), glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX)}
{
}

L3DMesh::~L3DMesh() = default;

void L3DMesh::Load(const l3d::L3DFile& l3d)
{
	_flags = static_cast<l3d::L3DMeshFlags>(l3d.GetHeader().flags);
	for (auto& skin : l3d.GetSkins())
	{
		_skins[skin.id] = std::make_unique<Texture2D>(_debugName.c_str());
		_skins[skin.id]->Create(skin.width, skin.height, 1, Format::RGBA4, Wrapping::Repeat, skin.texels.data(),
		                        static_cast<uint32_t>(skin.texels.size() * sizeof(skin.texels[0])));
	}

	if (static_cast<uint32_t>(_flags) & static_cast<uint32_t>(l3d::L3DMeshFlags::HasDoorPosition) &&
	    l3d.GetExtraPoints().size() > 0)
	{
		_doorPos = glm::vec3(l3d.GetExtraPoints()[0].x, l3d.GetExtraPoints()[0].y, l3d.GetExtraPoints()[0].z);
	}

	std::map<uint32_t, glm::mat4> matrices;
	auto& bones = l3d.GetBones();
	_bonesParents.resize(bones.size());
	for (uint32_t i = 0; i < bones.size(); ++i)
	{
		auto& bone = bones[i];
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
			auto& fileName = l3d.GetFilename();
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open L3DSubMesh from file: {}", fileName);
			continue;
		}
		if (subMesh->GetFlags().isPhysics)
		{
			auto& verticesSpan = l3d.GetVertexSpan(i);
			auto physicsMesh =
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
		l3d.Open(Game::instance()->GetFileSystem().FindPath(path).string());
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

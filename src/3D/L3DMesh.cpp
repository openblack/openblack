/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "L3DMesh.h"

#include "3D/MeshPack.h"
#include "Common/FileSystem.h"
#include "Common/IOStream.h"
#include "Game.h"

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
{
}

void L3DMesh::Load(const l3d::L3DFile& l3d)
{
	_flags = static_cast<l3d::L3DMeshFlags>(l3d.GetHeader().flags);
	for (auto& skin : l3d.GetSkins())
	{
		_skins[skin.id] = std::make_unique<Texture2D>(_debugName.c_str());
		_skins[skin.id]->Create(skin.width, skin.height, 1, Format::RGBA4, Wrapping::ClampEdge, skin.texels.data(),
		                        skin.texels.size() * sizeof(skin.texels[0]));
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

	_subMeshes.resize(l3d.GetSubmeshHeaders().size());
	for (size_t i = 0; i < _subMeshes.size(); ++i)
	{
		_subMeshes[i] = std::make_unique<L3DSubMesh>(*this);
		_subMeshes[i]->Load(l3d, i);
	}
	// TODO(bwrsandman): store vertex and index buffers at mesh level
	bgfx::frame();
}

bool L3DMesh::LoadFromFile(const fs::path& path)
{
	spdlog::debug("Loading L3DMesh from file: {}", path.generic_string());
	l3d::L3DFile l3d;

	try
	{
		l3d.Open(Game::instance()->GetFileSystem().FindPath(path).u8string());
	}
	catch (std::runtime_error& err)
	{
		spdlog::error("Failed to open l3d mesh from filesystem {}: {}", path.generic_string(), err.what());
		return false;
	}

	Load(l3d);

	return true;
}

void L3DMesh::LoadFromBuffer(const std::vector<uint8_t>& data)
{
	l3d::L3DFile l3d;

	try
	{
		l3d.Open(data);
	}
	catch (std::runtime_error& err)
	{
		spdlog::error("Failed to open l3d mesh from buffer: {}", err.what());
		return;
	}

	Load(l3d);
}

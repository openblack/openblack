/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "MeshPack.h"

#include "3D/L3DMesh.h"
#include "AllMeshes.h"
#include "Common/FileSystem.h"
#include "Common/MemoryStream.h"
#include "ECS/Components/Hand.h"
#include "Game.h"
#include "Graphics/Texture2D.h"

#include <PackFile.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <stdexcept>

namespace openblack
{

MeshPack::MeshPack(bool enableUnknownMeshes)
    : _enableUnknownMeshes(enableUnknownMeshes)
{
}

bool MeshPack::LoadFromFile(const fs::path& path)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading Mesh Pack from file: {}", path.generic_string());
	pack::PackFile pack;

	try
	{
		pack.Open(Game::instance()->GetFileSystem().FindPath(path).u8string());
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open {}: {}", path.generic_string(), err.what());
		return false;
	}

	loadTextures(pack.GetTextures());
	loadMeshes(pack.GetMeshes());

	return true;
}

const L3DMesh& MeshPack::GetMesh(MeshId id) const
{
	// TODO(raffclar): Handle non-mesh pack IDs via a new mechanism
	if (static_cast<int>(id) == ecs::components::Hand::meshId)
	{
		return Game::instance()->GetHandModel();
	}
	else
	{
		return *_meshes.at(static_cast<size_t>(id));
	}
}

L3DMesh& MeshPack::GetMesh(MeshId id)
{
	// TODO(raffclar): Handle non-mesh pack IDs via a new mechanism
	if (static_cast<int>(id) == ecs::components::Hand::meshId)
	{
		return Game::instance()->GetHandModel();
	}
	else
	{
		return *_meshes.at(static_cast<size_t>(id));
	}
}

void MeshPack::loadTextures(const std::map<std::string, pack::G3DTexture>& textures)
{
	// textures start at 1 - 0 would be an error texture
	_textures.resize(static_cast<std::size_t>(textures.size() + 1));
	_textures[0] = std::make_unique<graphics::Texture2D>("Error Texture");

	for (auto const& [name, g3dTexture] : textures)
	{
		// some assumptions:
		// - no mipmaps
		// - no cubemap or volume textures
		// - always dxt1 or dxt3
		// - all are compressed

		graphics::Format internalFormat;
		if (g3dTexture.ddsHeader.format.fourCC == std::string("DXT1"))
			internalFormat = graphics::Format::BlockCompression1;
		else if (g3dTexture.ddsHeader.format.fourCC == std::string("DXT3"))
			internalFormat = graphics::Format::BlockCompression2;
		else if (g3dTexture.ddsHeader.format.fourCC == std::string("DXT5"))
			internalFormat = graphics::Format::BlockCompression3;
		else
			throw std::runtime_error("Unsupported compressed texture format");

		_textures[g3dTexture.header.id] = std::make_unique<graphics::Texture2D>(name);
		_textures[g3dTexture.header.id]->Create(static_cast<uint16_t>(g3dTexture.ddsHeader.width),
		                                        static_cast<uint16_t>(g3dTexture.ddsHeader.height), 1, internalFormat,
		                                        graphics::Wrapping::Repeat, g3dTexture.ddsData.data(),
		                                        static_cast<uint32_t>(g3dTexture.ddsData.size()));
	}

	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "MeshPack loaded {0} textures", textures.size());
}

void MeshPack::loadMeshes(const std::vector<std::vector<uint8_t>>& meshes)
{
	if (meshes.size() > MeshNames.size() && !_enableUnknownMeshes)
	{
		SPDLOG_LOGGER_ERROR(
		    spdlog::get("game"),
		    "Cannot load meshes. The number of meshes to load ({}) does not match the number of stored mesh names ({}).",
		    meshes.size(), MeshNames.size());
		return;
	}

	_meshes.resize(meshes.size());

	for (uint32_t i = 0; i < _meshes.size(); i++)
	{
		auto name = "Unknown_" + std::to_string(i);

		// We grab a name from the Mesh Names structure
		// Names and meshes are associated via their positions (indices)
		// If we load a mesh pack with more meshes than we have names then we will
		// assign a name for it, or alternatively throw an error
		if (i < MeshNames.size())
		{
			name = MeshNames[i].data();
		}
		else
		{
			SPDLOG_LOGGER_WARN(spdlog::get("game"), "There are more meshes than mesh names; Using mesh name \"{}\" instead.",
			                   name);
		}

		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "L3DMesh {} {}", i, name);
		_meshes[i] = std::make_unique<L3DMesh>(name);
		_meshes[i]->LoadFromBuffer(meshes[i]);
	}

	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "MeshPack loaded {0} meshes", meshes.size());
}

} // namespace openblack

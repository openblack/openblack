/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "MeshPack.h"

#include "3D/L3DMesh.h"
#include "3D/MeshLocator.h"
#include "AllMeshes.h"
#include "Entities/Components/Hand.h"
#include "Game.h"

#include <PackFile.h>

#include <algorithm>
#include <stdexcept>

namespace openblack
{

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
	if (id == entities::components::Hand::meshId)
	{
		return Game::instance()->GetHandModel();
	}
	else
	{
		return *_meshes.at(id);
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
		else
			throw std::runtime_error("Unsupported compressed texture format");

		_textures[g3dTexture.header.id] = std::make_unique<graphics::Texture2D>(name);
		_textures[g3dTexture.header.id]->Create(g3dTexture.ddsHeader.width, g3dTexture.ddsHeader.height, 1, internalFormat,
		                                        graphics::Wrapping::ClampEdge, g3dTexture.ddsData.data(),
		                                        g3dTexture.ddsData.size());
	}

	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "MeshPack loaded {0} textures", textures.size());
}

void MeshPack::loadMeshes(const std::vector<std::vector<uint8_t>>& meshes)
{
	_meshes.resize(meshes.size());
	for (uint32_t i = 0; i < _meshes.size(); i++)
	{
		// SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "L3DMesh {} {}", i, MeshNames[i].data());
		_meshes[i] = std::make_unique<L3DMesh>(MeshNames[i].data());
		_meshes[i]->LoadFromBuffer(meshes[i]);
	}

	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "MeshPack loaded {0} meshes", meshes.size());
}

} // namespace openblack

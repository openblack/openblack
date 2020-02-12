/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
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
#include "Game.h"
#include "Graphics/Texture2D.h"

#include <g3d_file.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <stdexcept>

namespace openblack
{

void MeshPack::LoadFromFile(const std::string& filename)
{
	spdlog::debug("Loading Mesh Pack from file: {}", filename);
	g3d::G3DFile g3d;

	try
	{
		g3d.Open(Game::instance()->GetFileSystem().FindPath(filename).u8string());
	}
	catch (std::runtime_error& err)
	{
		spdlog::error("Failed to open {}: {}", filename, err.what());
		return;
	}

	loadTextures(g3d.GetTextures());
	loadMeshes(g3d.GetMeshes());
}

void MeshPack::loadTextures(const std::map<std::string, g3d::G3DTexture>& textures)
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

	spdlog::debug("MeshPack loaded {0} textures", textures.size());
}

void MeshPack::loadMeshes(const std::vector<std::vector<uint8_t>>& meshes)
{
	_meshes.resize(meshes.size());
	for (uint32_t i = 0; i < _meshes.size(); i++)
	{
		// spdlog::debug("L3DMesh {} {}", i, MeshNames[i].data());
		_meshes[i] = std::make_unique<L3DMesh>(MeshNames[i].data());
		_meshes[i]->LoadFromBuffer(meshes[i]);
	}

	spdlog::debug("MeshPack loaded {0} meshes", meshes.size());
}

} // namespace openblack

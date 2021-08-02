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

extern "C" {
#include "miniz.c"
}

#include <PackFile.h>

#include <algorithm>
#include <stdexcept>

namespace openblack
{

MeshPack::MeshPack(bool enableUnknownMeshes)
    : _enableUnknownMeshes(enableUnknownMeshes)
{
}

MeshId MeshPack::LoadFromFile(const fs::path& path)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading Mesh Pack from file: {}", path.generic_string());
	auto pathExt = path.extension().string();
	LowerCase(pathExt);

	try
	{
		if (pathExt == ".g3d")
		{
			pack::PackFile pack;
			pack.Open(Game::instance()->GetFileSystem().FindPath(path).u8string());
			loadTextures(pack.GetTextures());
			loadMeshes(pack.GetMeshes());
			return MeshId(_meshes.size() - 1);
		}
		else if (pathExt == ".l3d")
		{
			auto mesh = std::make_unique<L3DMesh>();
			mesh->LoadFromFile(path);
			auto meshId = MeshId(_meshes.size());
			_meshes.emplace_back(std::move(mesh));
			MeshNames.emplace_back(path.stem().string());
			return meshId;
		}
		else if (pathExt == ".zzz")
		{
			auto stream = Game::instance()->GetFileSystem().Open(path, FileMode::Read);
			uint32_t decompressedSize = 0;
			auto buffer = std::vector<uint8_t>(stream->Size() - sizeof(decompressedSize));
			stream->Read(reinterpret_cast<uint32_t*>(&decompressedSize), sizeof(decompressedSize));
			stream->Read(reinterpret_cast<char*>(buffer.data()), buffer.size());
			auto decompressedBuffer = std::vector<uint8_t>(decompressedSize);
			auto r = uncompress(decompressedBuffer.data(), reinterpret_cast<mz_ulong*>(&decompressedSize), buffer.data(), buffer.size());

			if (r != MZ_OK)
				throw new std::runtime_error("Unable to decompress file");

			// Load the decompressed mesh
			auto mesh = std::make_unique<L3DMesh>();
			mesh->LoadFromBuffer(decompressedBuffer);
			auto meshId = MeshId(_meshes.size());
			_meshes.emplace_back(std::move(mesh));
			MeshNames.emplace_back(path.stem().string());
			return meshId;
		}
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open {}: {}", path.generic_string(), err.what());
		return false;
	}

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
		else if (g3dTexture.ddsHeader.format.fourCC == std::string("DXT5"))
			internalFormat = graphics::Format::BlockCompression3;
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

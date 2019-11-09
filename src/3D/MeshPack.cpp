/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

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

// todo: move this code to it's own image parser
void createCompressedDDS(graphics::Texture2D* texture, const g3d::G3DTexture& g3dTexture)
{
	// some assumptions:
	// - no mipmaps
	// - no cubemap or volume textures
	// - always dxt1 or dxt3
	// - all are compressed

	graphics::Format internalFormat;
	int32_t width = g3dTexture.ddsHeader.width;
	int32_t height = g3dTexture.ddsHeader.height;

	if (g3dTexture.ddsHeader.format.fourCC == std::string("DXT1"))
		internalFormat = graphics::Format::BlockCompression1;
	else if (g3dTexture.ddsHeader.format.fourCC == std::string("DXT3"))
		internalFormat = graphics::Format::BlockCompression2;
	else
		throw std::runtime_error("Unsupported compressed texture format");

	// DXT1 = 8bpp or DXT3 = 16bpp
	int bpp = internalFormat == graphics::Format::BlockCompression2 ? 16 : 8;
	size_t size = std::max(1, ((int)width + 3) >> 2) * std::max(1, ((int)height + 3) >> 2) * bpp;

	// TODO(bwrsandman): Remove above calculation if the following holds true
	assert(g3dTexture.ddsData.size() == size);

	texture->Create(width, height, 1, internalFormat, graphics::Wrapping::ClampEdge, g3dTexture.ddsData.data(),
	                g3dTexture.ddsData.size());
}

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

	for (auto const& [name, tex] : textures)
	{
		_textures[tex.header.id] = std::make_unique<graphics::Texture2D>(name);
		createCompressedDDS(_textures[tex.header.id].get(), tex);
	}

	spdlog::debug("MeshPack loaded {0} textures", textures.size());
}

void MeshPack::loadMeshes(const std::vector<std::vector<uint8_t>>& meshes)
{
	_meshes.resize(meshes.size());
	for (uint32_t i = 0; i < _meshes.size(); i++)
	{
		MemoryStream modelStream(meshes[i].data(), meshes[i].size() * sizeof(meshes[i][0]));
		// spdlog::debug("L3DMesh {} {}", i, MeshNames[i].data());
		_meshes[i] = std::make_unique<L3DMesh>(MeshNames[i].data());
		_meshes[i]->LoadFromBuffer(meshes[i]);
	}

	spdlog::debug("MeshPack loaded {0} meshes", meshes.size());
}

} // namespace openblack

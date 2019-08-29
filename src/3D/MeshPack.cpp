/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <3D/MeshPack.h>
#include <Common/File.h>
#include <Graphics/OpenGL.h>
#include <Graphics/Texture2D.h>
#include <algorithm>
#include <stdexcept>
#include <stdint.h>

namespace OpenBlack
{

// todo: move this code to it's own image parser
void createCompressedDDS(Graphics::Texture2D* texture, uint8_t* buffer)
{
	// DDS file structures.
	struct dds_pixel_format
	{
		uint32_t dwSize;
		uint32_t dwFlags;
		uint32_t dwFourCC;
		uint32_t dwRGBBitCount;
		uint32_t dwRBitMask;
		uint32_t dwGBitMask;
		uint32_t dwBBitMask;
		uint32_t dwABitMask;
	};

	struct dds_header
	{
		uint32_t dwSize;
		uint32_t dwFlags;
		uint32_t dwHeight;
		uint32_t dwWidth;
		uint32_t dwPitchOrLinearSize;
		uint32_t dwDepth;
		uint32_t dwMipMapCount;
		uint32_t dwReserved1[11];
		dds_pixel_format ddspf;
		uint32_t dwCaps;
		uint32_t dwCaps2;
		uint32_t dwCaps3;
		uint32_t dwCaps4;
		uint32_t dwReserved2;
	};

	struct dds_mip_level
	{
		GLubyte* data;
		GLsizei width;
		GLsizei height;
		GLsizei size;
	};

	dds_header* header = reinterpret_cast<dds_header*>(buffer);

	// some assumptions:
	// - no mipmaps
	// - no cubemap or volume textures
	// - always dxt1 or dxt3
	// - all are compressed

	Graphics::InternalFormat internalFormat;
	GLsizei width         = header->dwWidth;
	GLsizei height        = header->dwHeight;
	int bytesPerBlock;

	switch (header->ddspf.dwFourCC)
	{
	case ('D' | ('X' << 8) | ('T' << 16) | ('1' << 24)):
		internalFormat = Graphics::InternalFormat::CompressedRGBAS3TCDXT1;
		break;
	case ('D' | ('X' << 8) | ('T' << 16) | ('3' << 24)):
		internalFormat = Graphics::InternalFormat::CompressedRGBAS3TCDXT3;
		break;
	default:
		throw std::runtime_error("Unsupported compressed texture format");
		break;
	}

	texture->CreateCompressed(buffer + header->dwSize, width, height, internalFormat);
}

char kLionheadMagic[] = "LiOnHeAd";

void MeshPack::LoadFromFile(File& file)
{
	// check magic header
	char magic[8];
	file.ReadBytes<char>(magic, 8);
	if (!std::equal(kLionheadMagic, kLionheadMagic + 8, magic))
		throw std::runtime_error("invalid MeshPack file magic");

	std::size_t size = file.Size();
	while (file.Position() < size)
	{
		LHBlockHeader header;
		file.ReadBytes<LHBlockHeader>(&header, 36);

		header.position = file.Position();
		file.Seek(header.blockSize, FileSeekMode::Current);

		_blocks[header.blockName] = header;
	}

	if (_blocks.find("MESHES") == _blocks.end())
		throw std::runtime_error("no MESHES block in mesh pack");

	loadTextures(file);
	// loadMeshes(file);
}

void MeshPack::loadTextures(File& file)
{
	auto const& block = _blocks.find("INFO");
	if (block == _blocks.end())
		throw std::runtime_error("no INFO block in mesh pack");

	auto const& infoBlock = block->second;
	assert(infoBlock.blockSize == 0x1004);

	file.Seek(infoBlock.position, FileSeekMode::Begin);

	uint32_t totalTextures;
	file.Read<uint32_t>(&totalTextures, 1);

	// todo: CI / mods will change this
	assert(totalTextures == 110);

	// firstly read ids
	std::vector<std::pair<uint32_t, uint32_t>> textureTypeMap(totalTextures);
	file.Read<uint32_t>(reinterpret_cast<uint32_t*>(textureTypeMap.data()), totalTextures * 2);

	_textures.resize(totalTextures + 1);

	// textures start at 1 - 0 would be an error texture
	_textures[0] = std::make_unique<Graphics::Texture2D>();

	char sBlockID[4];
	for (auto const& tex : textureTypeMap)
	{
		itoa(tex.first, sBlockID, 16);

		auto const& textureBlock = _blocks.find(sBlockID);
		if (textureBlock == _blocks.end())
			throw std::runtime_error("no " + std::string(sBlockID) + " block in mesh pack");

		file.Seek(textureBlock->second.position, FileSeekMode::Begin);

		uint32_t size, id, type, ddsSize;
		file.Read<uint32_t>(&size, 1);
		file.Read<uint32_t>(&id, 1);
		file.Read<uint32_t>(&type, 1);
		file.Read<uint32_t>(&ddsSize, 1);

		uint8_t* ddsBuffer = new uint8_t[ddsSize];
		file.ReadBytes<uint8_t>(ddsBuffer, ddsSize);

		_textures[id] = std::make_unique<Graphics::Texture2D>();
		createCompressedDDS(_textures[id].get(), ddsBuffer);

		delete[] ddsBuffer;

	}
}

} // namespace OpenBlack

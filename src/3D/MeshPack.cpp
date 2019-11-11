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
#include "Common/IStream.h"
#include "Common/MemoryStream.h"
#include "Graphics/Texture2D.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <stdexcept>

namespace openblack
{

// todo: move this code to it's own image parser
void createCompressedDDS(graphics::Texture2D* texture, uint8_t* buffer)
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
		uint8_t* data;
		int32_t width;
		int32_t height;
		int32_t size;
	};

	auto* header = reinterpret_cast<dds_header*>(buffer);

	// some assumptions:
	// - no mipmaps
	// - no cubemap or volume textures
	// - always dxt1 or dxt3
	// - all are compressed

	graphics::Format internalFormat;
	int32_t width = header->dwWidth;
	int32_t height = header->dwHeight;
	int bytesPerBlock;

	switch (header->ddspf.dwFourCC)
	{
	case ('D' | ('X' << 8) | ('T' << 16) | ('1' << 24)): internalFormat = graphics::Format::BlockCompression1; break;
	case ('D' | ('X' << 8) | ('T' << 16) | ('3' << 24)): internalFormat = graphics::Format::BlockCompression2; break;
	default: throw std::runtime_error("Unsupported compressed texture format"); break;
	}

	// DXT1 = 8bpp or DXT3 = 16bpp
	int bpp = internalFormat == graphics::Format::BlockCompression2 ? 16 : 8;
	size_t size = std::max(1, ((int)width + 3) >> 2) * std::max(1, ((int)height + 3) >> 2) * bpp;

	texture->Create(width, height, 1, internalFormat, graphics::Wrapping::ClampEdge, buffer + header->dwSize, size);
}

void MeshPack::Load(IStream& stream)
{
	// check magic header
	constexpr char kLionheadMagic[] = "LiOnHeAd";

	char magic[8];
	stream.Read(magic, 8);
	if (!std::equal(kLionheadMagic, kLionheadMagic + 8, magic))
		throw std::runtime_error("invalid MeshPack file magic");

	std::size_t size = stream.Size();
	while (stream.Position() < size)
	{
		LHBlockHeader header;
		stream.Read(&header, 36);

		header.position = stream.Position();
		stream.Seek(header.blockSize, SeekMode::Current);

		_blocks[header.blockName] = header;
	}

	loadTextures(stream);
	loadMeshes(stream);
}

void MeshPack::loadTextures(IStream& stream)
{
	auto const& block = _blocks.find("INFO");
	if (block == _blocks.end())
		throw std::runtime_error("no INFO block in mesh pack");

	auto const& infoBlock = block->second;
	assert(infoBlock.blockSize == 0x1004);

	stream.Seek(infoBlock.position, SeekMode::Begin);

	uint32_t totalTextures = stream.ReadValue<uint32_t>();

	// todo: CI/mod support
	if (totalTextures != 110)
	{
		spdlog::warn("MeshPack contains {} textures, expected 110", totalTextures);
	}

	// firstly read ids
	std::vector<std::pair<uint32_t, uint32_t>> textureTypeMap(totalTextures);
	stream.Read(textureTypeMap.data(), totalTextures * 2 * sizeof(uint32_t));

	_textures.resize(static_cast<std::size_t>(totalTextures + 1));

	// textures start at 1 - 0 would be an error texture
	_textures[0] = std::make_unique<graphics::Texture2D>("Error Texture");

	char sBlockID[4];
	for (auto const& tex : textureTypeMap)
	{
		snprintf(sBlockID, sizeof(sBlockID), "%x", tex.first);

		auto const& textureBlock = _blocks.find(sBlockID);
		if (textureBlock == _blocks.end())
			throw std::runtime_error("no " + std::string(sBlockID) + " block in mesh pack");

		stream.Seek(textureBlock->second.position, SeekMode::Begin);

		uint32_t size = stream.ReadValue<uint32_t>();
		uint32_t id = stream.ReadValue<uint32_t>();
		uint32_t type = stream.ReadValue<uint32_t>();
		uint32_t ddsSize = stream.ReadValue<uint32_t>();

		uint8_t* ddsBuffer = new uint8_t[ddsSize];
		stream.Read(ddsBuffer, ddsSize);

		// TODO(bwrsandman): Give unique name to Texture for better debugging
		_textures[id] = std::make_unique<graphics::Texture2D>("");
		createCompressedDDS(_textures[id].get(), ddsBuffer);

		delete[] ddsBuffer;
	}

	spdlog::debug("MeshPack loaded {0} textures", totalTextures);
}

void MeshPack::loadMeshes(IStream& stream)
{
	auto const& block = _blocks.find("MESHES");
	if (block == _blocks.end())
		throw std::runtime_error("no MESHES block in mesh pack");

	// seek to the MESHES block
	stream.Seek(block->second.position, SeekMode::Begin);

	// Greetings Jean-Claude Cottier
	uint32_t magic = stream.ReadValue<uint32_t>();
	constexpr uint32_t kMKJC = ('C' << 24) | ('J' << 16) | ('K' << 8) | 'M';
	if (magic != kMKJC)
	{
		spdlog::error("MeshPack MESHES magic mismatch, expected MKJC");
		return;
	}

	uint32_t meshCount = stream.ReadValue<uint32_t>();
	std::vector<uint32_t> meshOffsets(meshCount);
	stream.Read(meshOffsets.data(), meshCount * sizeof(uint32_t));

	_meshes.resize(meshCount);
	for (uint32_t i = 0; i < meshCount; i++)
	{
		stream.Seek(block->second.position + meshOffsets[i], SeekMode::Begin);

		// slightly hacky, but lets read the header, get the size, and return a
		// MemoryStream
		struct
		{
			uint32_t magic;
			uint32_t flags;
			uint32_t size;
		} header;
		stream.Read(&header, 12);
		stream.Seek(-12, SeekMode::Current);

		std::vector<std::byte> data(header.size);
		stream.Read(data.data(), data.size());

		MemoryStream modelStream(data.data(), data.size());

		// spdlog::debug("L3DMesh {}", i);
		std::unique_ptr<L3DMesh> mesh = std::make_unique<L3DMesh>(MeshNames[i].data());
		mesh->Load(modelStream);

		_meshes[i] = std::move(mesh);
	}

	spdlog::debug("MeshPack loaded {0} meshes", meshCount);
}

} // namespace openblack

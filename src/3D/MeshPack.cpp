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
#include <algorithm>
#include <stdexcept>
#include <stdint.h>

constexpr size_t StrLen(const char* s) noexcept
{
	return *s ? 1 + StrLen(s + 1) : 0;
}

struct G3DMeshes
{
	char magic[4];
	uint32_t meshCount;
};

// Just enough for us to parse
struct L3DSMiniHeader
{
	char magic[4];
	uint32_t unknown;
	uint32_t l3dSize;
};

struct G3DHiResTexture
{
	uint32_t size;
	uint32_t id;
	uint32_t type;

	uint32_t dxSize;
	// surface desc shit.
};

struct DDSurfaceDesc
{
	uint32_t size;
	uint32_t flags;
	uint32_t height;
	uint32_t width;
};

void createCompressedDDS(char* buffer)
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

	GLenum internalFormat = 0;
	GLsizei width         = header->dwWidth;
	GLsizei height        = header->dwHeight;
	int bytesPerBlock;

	switch (header->ddspf.dwFourCC)
	{
	case ('D' | ('X' << 8) | ('T' << 16) | ('1' << 24)):
		internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		bytesPerBlock  = 8;
		break;
	case ('D' | ('X' << 8) | ('T' << 16) | ('3' << 24)):
		internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		bytesPerBlock  = 16;
		break;
	default:
		throw std::runtime_error("Unsupported compressed texture format");
		break;
	}

	GLsizei size = std::max(1, (width + 3) >> 2) * std::max(1, (height + 3) >> 2) * bytesPerBlock;

	glCompressedTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, size,
	                       reinterpret_cast<void*>(buffer + header->dwSize));
}

namespace OpenBlack
{

char kLionheadMagic[] = "LiOnHeAd";

void MeshPack::LoadFromFile(File& file)
{
	// check magic header
	char magic[8];
	file.ReadBytes<char>(magic, 8);
	if (!std::equal(kLionheadMagic, kLionheadMagic + 8, magic))
		throw std::runtime_error("invalid MeshPack file magic");

	struct LHBlockHeader
	{
		char blockName[32];
		uint32_t blockSize;
		uint32_t position;
	};

	std::unordered_map<std::string, LHBlockHeader> blocks;

	std::size_t size = file.Size();
	while (file.Position() < size)
	{
		LHBlockHeader header;
		file.ReadBytes<LHBlockHeader>(&header, 36);

		header.position = file.Position();
		file.Seek(header.blockSize, FileSeekMode::Current);

		blocks[header.blockName] = header;
	}

	if (blocks.find("MESHES") == blocks.end())
		throw std::runtime_error("no MESHES block in mesh pack");

	if (blocks.find("INFO") == blocks.end())
		throw std::runtime_error("no INFO block in mesh pack");

	file.Seek(blocks.find("INFO")->second.position, FileSeekMode::Begin);
	uint32_t totalTextures;
	file.Read<uint32_t>(&totalTextures, 1);

	// INFO block should be 0x1004 long
	// assert(totalTextures == 110);

	for (uint32_t i = 0; i < totalTextures; i++)
	{
		uint32_t textureID, textureType;
		file.Read<uint32_t>(&textureID, 1);
		file.Read<uint32_t>(&textureType, 1);

		// 1/2 = DXT1/3

		printf("texture %d: type=%d\n", textureID, textureType);
	}

	//const int totalTextures = 110;

	/*std::size_t fileSize = file.Size();
	std::vector<char> fileData(fileSize);
	
	// We now access the data using the vector's underlying array
	const auto rawFileData = fileData.data();
	file.Read(rawFileData, fileSize);
	file.Close();

	const int totalTextures            = 110;
	const auto modelMagic              = std::string("MKJC");
	constexpr auto lionheadMagicLength = StrLen("LiOnHeAd");
	auto* blockHeaderOffset            = rawFileData + lionheadMagicLength;
	// The end of the contiguous array of file data
	const auto rawFileDataEndOffset = &rawFileData[fileSize - 1];

	for (int i = 0; i < totalTextures; i++)
	{
		// skip block header
		file->Seek(36, LH_SEEK_MODE::Current);

		G3DHiResTexture* hiresTexture = new G3DHiResTexture;
		file->Read(hiresTexture, sizeof(G3DHiResTexture));

		void* surfaceDesc = malloc(hiresTexture->dxSize - 4);
		file->Read(surfaceDesc, hiresTexture->dxSize - 4);

		DDSurfaceDesc* desc = (DDSurfaceDesc*)surfaceDesc;

		assert(id == texture->id);

		glBindTexture(GL_TEXTURE_2D, textures[texture->id - 1]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// texture->type = 1 = DXT1, texture->type = 2 = DXT3 (lets read the DXT header anyway just in case)
		createCompressedDDS(reinterpret_cast<char*>(&texture->dxSurfDesc));

		blockHeaderOffset += 36 + blockHeader->blockSize;
	}

	// each high res texture is actually a lionhead block...
	// char textureid[32];
	// uint32_t size;
	// AWKWARD...............

	LHSegment meshesSegment;
	file->GetSegment("MESHES", &meshesSegment, true);

	printf("Meshes Segment %s of %d bytes\n", meshesSegment.Name, meshesSegment.SegmentSize);

	uint8_t* data         = (uint8_t*)meshesSegment.SegmentData;
	uint32_t* meshCount   = (uint32_t*)(data + 4);
	uint32_t* meshOffsets = (uint32_t*)(data + 8);

	m_meshCount = *meshCount;

			for (uint32_t i = 0; i < meshes->meshCount; i++)
			{
				const auto header = reinterpret_cast<L3DSMiniHeader*>(dataAfterHeader + meshOffsets[i]);
				auto model        = std::make_shared<SkinnedModel>();
				auto* modelData   = dataAfterHeader + meshOffsets[i];
				model->LoadFromL3D(modelData, header->l3dSize);
				models.push_back(model);
			}
		}
		else if (blockName == "INFO")
		{
			// Skip
		}
		else if (blockName == "LOW")
		{
			// Low resolution textures; skipping
		}

		blockHeaderOffset += 36l + blockHeader->blockSize;
	}*/
}

} // namespace OpenBlack

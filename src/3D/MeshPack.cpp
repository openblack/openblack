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

#include "MeshPack.h"

#include <algorithm>
#include <stdexcept>
#include <stdint.h>

using namespace OpenBlack;

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
		bytesPerBlock           = 8;
		break;
	case ('D' | ('X' << 8) | ('T' << 16) | ('3' << 24)):
		internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		bytesPerBlock           = 16;
		break;
	default:
		throw std::runtime_error("Unsupported compressed texture format");
		break;
	}

	GLsizei size = std::max(1, (width + 3) >> 2) * std::max(1, (height + 3) >> 2) * bytesPerBlock;

	glCompressedTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, size,
		reinterpret_cast<void*>(buffer + header->dwSize));
}

MeshPack::MeshPack(OSFile* file):
    m_meshCount(0)
{
	// LiOnHeAd and a block header, but we already know it's high res textures
	file->Seek(8, LH_SEEK_MODE::Set);

	int totalTextures = 110;

	Textures = new GLuint[totalTextures];
	glGenTextures(totalTextures, Textures);

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

	Models = new L3DModel*[*meshCount];

	for (uint32_t i = 0; i < *meshCount; i++)
	{
		L3DSMiniHeader* header = (L3DSMiniHeader*)(data + meshOffsets[i]);

		L3DModel* model = new L3DModel();
		model->LoadFromL3D(data + meshOffsets[i], header->l3dSize, true);

		Models[i] = model;
	}
}

uint32_t MeshPack::GetMeshCount()
{
	return m_meshCount;
}

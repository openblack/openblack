#define GLEW_STATIC
#include <GL/glew.h>
#include <GL/glu.h>

#include <stdio.h>
#include <stdexcept>
#include <vector>

#include <string>

#include <3D/oldLH3DIsland.h>
#include <Common/OSFile.h>

using namespace OpenBlack;

/*
Porting this to LandIsland.cpp
*/

bool LH3DIsland::g_b_created = false;
bool LH3DIsland::g_b_need_rebuild_all = true;
uint32_t LH3DIsland::g_block_count = 0;
uint8_t LH3DIsland::g_index_block[1024];
LH3DLandBlock** LH3DIsland::g_blocks = nullptr;
const float LH3DIsland::g_height_unit = 0.67;

uint32_t LH3DIsland::g_lowrestexture_count = 0;
uint32_t LH3DIsland::g_country_count = 0;
uint32_t LH3DIsland::g_material_count = 0;

Texture2DArray* LH3DIsland::g_materialarray = nullptr;
Texture2D* LH3DIsland::g_materials = nullptr;
LNDCountry* LH3DIsland::g_countries = nullptr;
uint32_t* LH3DIsland::g_lowrestextures = nullptr;

void LH3DIsland::CreateCommonPart()
{
	if (LH3DIsland::g_b_created)
		LH3DIsland::Release();

	LH3DIsland::g_b_created = 1;
	LH3DIsland::g_b_need_rebuild_all = 1;

	// LH3DLandscape::Create();

	//memset(LH3DIsland::TC_g_index_block, 0, 0x400u);
	memset(LH3DIsland::g_index_block, 0, 0x400u);
	LH3DIsland::g_block_count = 1;
}

/*void LH3DIsland::LoadFromDisk(const char* filename)
{
// LH3DIsland::CreateCommonPart();

// LH3DIsland::g_block_count = *lndFileBuffer;
// nMaterials = lndFileBuffer[257];
// nCountries = lndFileBuffer[258];

//qmemcpy(LH3DIsland::g_index_block, lndFileBuffer + 1, 1024u);// blocksArray
//bufPos = LowResTexture::LoadThemAll(&LH3DIsland::g_index_block[1024], (lndFileBuffer + 257), lndFileBuffer + 262);

// load blocks

// load countries

// load materials

// copy noise bitmap
// copy bump
}*/

static const int DDS_HEADER_SIZE_BYTES = 124;
#define FOURCC_DXT1 0x31545844
#define FOURCC_DXT3 0x33545844
#define FOURCC_DXT5 0x35545844

// todo: move and clean this up
bool loadDDS(uint8_t* buffer)
{
	uint32_t height = *(uint32_t*) &(buffer[8]);
	uint32_t width = *(uint32_t*) &(buffer[12]);
	uint32_t linearSize = *(uint32_t*) &(buffer[16]);
	uint32_t mipMapCount = *(uint32_t*) &(buffer[24]);
	uint32_t fourCC = *(uint32_t*) &(buffer[80]);

	//uint32_t bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
	//uint8_t* tex = new uint8_t[bufsize];

	printf("Loading %dx%d DDS w/ %d mimaps\n", width, height, mipMapCount);

	uint32_t format;
	switch (fourCC) {
	case FOURCC_DXT1:
		format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		break;
	case FOURCC_DXT3:
		format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		break;
	case FOURCC_DXT5:
		format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		break;
	default:
		delete[] buffer;
		return false;
	}

	unsigned int blockSize = (format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;
	unsigned int offset = 124;

	/* load the mipmaps */
	for (unsigned int level = 0; level < mipMapCount && (width || height); ++level) {
		unsigned int size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
		glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height,
			0, size, buffer + offset);

		offset += size;
		width /= 2;
		height /= 2;
	}
}

void LH3DIsland::LoadFromDisk(const char * filename)
{
	if (!OSFile::Exists(filename))
		throw std::runtime_error("Land file does not exist.");

	OSFile* file = new OSFile();
	file->Open(filename, LH_FILE_MODE::Read);
	size_t fileSize = file->Size();

	file->Read(&LH3DIsland::g_block_count, 4);
	file->Read(&LH3DIsland::g_index_block, 1024);

	file->Read(&LH3DIsland::g_material_count, 4);
	file->Read(&LH3DIsland::g_country_count, 4);
	file->Seek(12, LH_SEEK_MODE::Current); // block,mat,country struct sizes
	file->Read(&LH3DIsland::g_lowrestexture_count, 4);

	LH3DIsland::g_lowrestextures = new GLuint[LH3DIsland::g_lowrestexture_count];
	glGenTextures(LH3DIsland::g_lowrestexture_count, LH3DIsland::g_lowrestextures);
	for (int i = 0; i < LH3DIsland::g_lowrestexture_count; i++)
	{
		uint32_t textureSize;

		file->Seek(16, LH_SEEK_MODE::Current);
		file->Read(&textureSize, 4);

		uint8_t* texData = (uint8_t*)malloc(textureSize-4);
		file->Read(texData, textureSize - 4);

		glBindTexture(GL_TEXTURE_2D, LH3DIsland::g_lowrestextures[i]);
		loadDDS(texData);

		free(texData);
	}

	LH3DIsland::g_blocks = (LH3DLandBlock**)malloc((LH3DIsland::g_block_count - 1) * 4);

	if (LH3DIsland::g_block_count > 1)
	{
		auto blocks = LH3DIsland::g_blocks;

		for (int i = 0; i < LH3DIsland::g_block_count - 1; i++)
		{
			LH3DLandBlock* block = (LH3DLandBlock*)malloc(sizeof(LH3DLandBlock));
			file->Read(block, sizeof(LH3DLandBlock));

			memset(&block->clipped, 0, 0x28);
			block->smallTextUpdated = 0;

			uint8_t highestAlt = 0;
			for (int j = 0; j < 289; j++) {
				uint8_t cellAlt = block->cells[j].altitude;

				if (highestAlt < cellAlt)
					highestAlt = cellAlt;
			}
			block->highestAltitude = highestAlt;

			*blocks = block;
			blocks++;
		}
	}

	// Countries
	LH3DIsland::g_countries = new LNDCountry[LH3DIsland::g_country_count];
	file->Read(LH3DIsland::g_countries, sizeof(LNDCountry) * LH3DIsland::g_country_count);
	for (int i = 0; i < LH3DIsland::g_country_count; i++)
	{
		LNDCountry country = LH3DIsland::g_countries[i];
		printf("Country %d: Mixes materials %d and %d w/ a coef of %d\n", i, country.MapMaterial[190].FirstMaterialIndex, country.MapMaterial[190].SecondMaterialIndex, country.MapMaterial[190].Coeficient);
	}

	// Materials
	LH3DIsland::g_materials = new Texture2D[LH3DIsland::g_material_count];
	LH3DIsland::g_materialarray = new Texture2DArray();
	//glGenTextures(LH3DIsland::g_material_count, LH3DIsland::g_materials);
	for (int i = 0; i < LH3DIsland::g_material_count; i++)
	{
		uint16_t terrainType;
		auto oldtex = new uint16_t[256 * 256];

		file->Read(&terrainType, 2);
		file->Read((void*)oldtex, 131072);

		TextureDef2D tex;
		tex.width = 256;
		tex.height = 256;
		tex.format = GL_RGBA;
		tex.internalFormat = GL_RGBA;
		tex.type = GL_UNSIGNED_INT_8_8_8_8;
		tex.data = new uint8_t[256 * 256 * 4];

		for (int i = 0; i < 256 * 256; i++)
		{
			uint16_t col = oldtex[i];

			uint8_t r = (col & 0x7C00) >> 10;
			uint8_t g = (col & 0x3E0) >> 5;
			uint8_t b = (col & 0x1F);

			((uint8_t*)tex.data)[i * 4 + 3] = r << 3; // 5
			((uint8_t*)tex.data)[i * 4 + 2] = g << 3; // 5
			((uint8_t*)tex.data)[i * 4 + 1] = b << 3; // 5
			((uint8_t*)tex.data)[i * 4 + 0] = 255;
		}

		//memset((void*)tex.data, 100, 256 * 256 * 4);


		//LH3DIsland::g_materials[i].Create(tex);
		LH3DIsland::g_materialarray->AddTexture(tex);
	}

	LH3DIsland::g_materialarray->Create();

	// Noisemap
	file->Seek(65536, LH_SEEK_MODE::Current);

	// Bumpmap
	file->Seek(65536, LH_SEEK_MODE::Current);

	file->Close();
}

void LH3DIsland::Release()
{
	if (!LH3DIsland::g_b_created)
		return; // 0

	LH3DIsland::g_b_created = false;
	LH3DIsland::g_b_need_rebuild_all = true;
	//LH3DIsland::ReleaseSmallBump();
	//LH3DLandscape::Release();

	for (int i = 0; i < LH3DIsland::g_block_count; i++)
		free(LH3DIsland::g_blocks[i]);

	glDeleteTextures(LH3DIsland::g_lowrestexture_count, LH3DIsland::g_lowrestextures);

    delete LH3DIsland::g_materialarray;

	/*int* block = LH3DIsland::TC_g_index_block;
	do
	{
		if (*block)
		{
			LandBlock::Release((_DWORD *)*block);
			*block = 0;
		}
		++block;
	} while (block < (int *)LH3DIsland::g_index_block);
	LH3DJustFootPrint::ReleaseAll();*/
	//memset(LH3DIsland::TC_g_index_block, 0, 0x400u);
	memset(LH3DIsland::g_index_block, 0, 0x400u);
	//dword_E9CD6C = 0;
	LH3DIsland::g_block_count = 0;
	//LH3DIsland::g_b_view_has_changed = 1;
	return; // true
}

std::vector<LH3DVertex> LH3DIsland::GetVerticies()
{
	std::vector<LH3DVertex> verts((LH3DIsland::g_block_count - 1 ) * 289);
	int vertexOffset = 0;

	auto blocks = LH3DIsland::g_blocks;
	for (int b = 0; b < LH3DIsland::g_block_count - 1; b++)
	{
		LH3DLandBlock* block = *blocks; // shrug
		
		//printf("processing block [%d,%d] @ [%.3f,%.3f]\n", block->blockX, block->blockY, block->mapX, block->mapY);

		uint8_t maxAlt = 0;

		for (int cx = 0; cx < 17; cx++)
			for (int cy = 0; cy < 17; cy++)
			{
				LH3DLandCell* cell = &block->cells[cy*17+cx];

				if (maxAlt < cell->altitude)
					maxAlt = cell->altitude;

				LH3DVertex* vert = &verts[vertexOffset + cy * 17 + cx];
				vert->position[0] = block->mapX + (cx * 10.0f);
				vert->position[1] = (float)cell->altitude * g_height_unit;
				vert->position[2] = block->mapY + (cy * 10.0f);
				vert->color[0] = cell->r;
				vert->color[1] = cell->g;
				vert->color[2] = cell->b;
				vert->uv[0] = (float)cx / (float)17;
				vert->uv[1] = (float)cy / (float)17;
				vert->country = cell->flags1 & 0x0F;
				vert->altitude = cell->altitude;
			}

		vertexOffset += 17 * 17;
		blocks++;
	}

	return verts;
}

std::vector<uint32_t> LH3DIsland::GetIndices()
{
	// per block: 512 tris, 1536 indices, 289 verts

	std::vector<uint32_t> indices((LH3DIsland::g_block_count - 1) * 1536);
	int vertexOffset = 0;
	int indexOffset = 0;

	auto blocks = LH3DIsland::g_blocks;
	for (int b = 0; b < LH3DIsland::g_block_count - 1; b++)
	{
		LH3DLandBlock* block = *blocks;

		// loop through the 16*16 cells and work from the center

		for (int cx = 0; cx < 16; cx++)
		{
			for (int cy = 0; cy < 16; cy++)
			{
				indices[indexOffset++] = vertexOffset + cy * 17 + cx; // top left
				indices[indexOffset++] = vertexOffset + cy * 17 + cx + 1; // top right
				indices[indexOffset++] = vertexOffset + (cy+1)*17+cx; // bottom left

				indices[indexOffset++] = vertexOffset + cy * 17 + cx + 1; // top right
				indices[indexOffset++] = vertexOffset + (cy + 1) * 17 + cx + 1; // bottom right
				indices[indexOffset++] = vertexOffset + (cy + 1) * 17 + cx; // bottom left
			}
		}

		vertexOffset += 17 * 17;
		blocks++;
	}

	return indices;
}

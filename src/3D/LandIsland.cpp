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

#include "LandIsland.h"

#include <Common/FileSystem.h>
#include <Common/OSFile.h>
#include <Game.h>
#include <inttypes.h>
#include <stdexcept>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <Common/stb_image_write.h>

using namespace OpenBlack;

const float LandIsland::HeightUnit = 0.67f;
const float LandIsland::CellSize   = 10.0f;

LandIsland::LandIsland():
    _lowresCount(0), _materialCount(0), _blockIndexLookup { 0 }
{
	auto file = Game::instance()->GetFileSystem().Open("Data/Textures/smallbumpa.raw", FileMode::Read);
	uint8_t* smallbumpa = new uint8_t[file->Size()];
	file->Read(smallbumpa, file->Size());
	file->Close();

	_textureSmallBump = std::make_unique<Texture2D>(256, 256, GL_R8, GL_RED, GL_UNSIGNED_BYTE, smallbumpa);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	delete[] smallbumpa;
}

LandIsland::~LandIsland()
{
}

/*
Loads from the original Black & White .lnd file format and
converts the data into our own
*/
void LandIsland::LoadFromFile(File& file)
{
	uint32_t blockCount, countryCount, blockSize, matSize, countrySize;

	file.ReadBytes(&blockCount, 4);
	file.ReadBytes(_blockIndexLookup.data(), 1024);
	file.ReadBytes(&_materialCount, 4);
	file.ReadBytes(&countryCount, 4);

	// todo: lets assert these against sizeof(LandBlock) etc..
	file.ReadBytes(&blockSize, 4);
	file.ReadBytes(&matSize, 4);
	file.ReadBytes(&countrySize, 4);
	file.ReadBytes(&_lowresCount, 4);

	// skip over low resolution textures
	// for future reference these are formatted GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
	for (unsigned int i = 0; i < _lowresCount; i++)
	{
		uint32_t textureSize;
		file.Seek(16, FileSeekMode::Current);
		file.ReadBytes(&textureSize, 4);
		file.Seek(textureSize - 4, FileSeekMode::Current);
	}

	blockCount--; // take away a block from the count, because it's not in the file?
	_landBlocks.reserve(blockCount);
	for (uint32_t i = 0; i < blockCount; i++)
	{
		uint8_t* blockData = new uint8_t[blockSize];
		file.ReadBytes(blockData, blockSize);

		_landBlocks.push_back(LandBlock());
		_landBlocks[i].Load(blockData, blockSize);

		delete[] blockData;
	}

	_countries.reserve(countryCount);
	for (uint32_t i = 0; i < countryCount; i++)
	{
		Country country;
		file.ReadBytes(&country, sizeof(Country));
		_countries.push_back(country);
	}

	_materialArray = std::make_shared<Texture2DArray>(256, 256, _materialCount, GL_RGBA8);
	for (uint32_t i = 0; i < _materialCount; i++)
	{
		uint16_t* rgba5TextureData = new uint16_t[256 * 256];
		uint32_t* rgba8TextureData = new uint32_t[256 * 256];

		uint16_t terrainType;
		file.ReadBytes(&terrainType, 2);
		file.ReadBytes(rgba5TextureData, 256 * 256 * sizeof(uint16_t));

		convertRGB5ToRGB8(rgba5TextureData, rgba8TextureData, 256 * 256);
		_materialArray->SetTexture(i, 256, 256, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, rgba8TextureData);

		delete[] rgba5TextureData;
		delete[] rgba8TextureData;
	}

	// read noise map into Texture2D
	uint8_t* noiseMapTextureData = new uint8_t[256 * 256];
	file.ReadBytes(noiseMapTextureData, 256 * 256);
	_textureNoiseMap = std::make_shared<Texture2D>(256, 256, GL_RED, GL_RED, GL_UNSIGNED_BYTE, noiseMapTextureData);
	delete[] noiseMapTextureData;

	// read bump map into Texture2D
	uint8_t* bumpMapTextureData = new uint8_t[256 * 256];
	file.ReadBytes(bumpMapTextureData, 256 * 256);
	_textureBumpMap = std::make_shared<Texture2D>(256, 256, GL_RED, GL_RED, GL_UNSIGNED_BYTE, bumpMapTextureData);
	delete[] bumpMapTextureData;

	file.Close();

	// build the meshes (we could move this elsewhere)
	for (auto& block : _landBlocks)
		block.BuildMesh(*this);
}

/*const uint8_t LandIsland::GetAltitudeAt(glm::ivec2 vec) const
{
	return uint8_t();
}
*/

const float LandIsland::GetHeightAt(glm::vec2 vec) const
{
	return GetCell(vec.x * 0.1f, vec.y * 0.1f).Altitude() * LandIsland::HeightUnit;
}

const LandBlock* LandIsland::GetBlock(int8_t x, int8_t y) const
{
	// our blocks can only be between [0-31, 0-31]
	if (x < 0 || x > 32 || y < 0 || y > 32)
		return nullptr;

	const uint8_t blockIndex = _blockIndexLookup[y * 32 + x];
	if (blockIndex == 0)
		return nullptr;

	return &_landBlocks[blockIndex - 1];
}

const LandCell& LandIsland::GetCell(int x, int y) const
{
	if (x < 0 || x > 511 || y < 0 || y > 511)
		return LandCell(); // return empty water cell

	const uint8_t blockIndex = _blockIndexLookup[32 * (y >> 4) + (x >> 4)];

	if (blockIndex == 0)
		return LandCell(); // return empty water cell

	return _landBlocks[blockIndex - 1].GetCells()[(x & 0xF) + 17 * (y & 0xF)];
}

void LandIsland::Draw(ShaderProgram& program)
{
	for (auto& block : _landBlocks)
		block.Draw(program);
}

void LandIsland::convertRGB5ToRGB8(uint16_t* rgba5, uint32_t* rgba8, size_t pixels)
{
	for (size_t i = 0; i < pixels; i++)
	{
		uint16_t col = rgba5[i];

		uint8_t r = (col & 0x7C00) >> 10;
		uint8_t g = (col & 0x3E0) >> 5;
		uint8_t b = (col & 0x1F);

		((uint8_t*)rgba8)[i * 4 + 3] = r << 3; // 5
		((uint8_t*)rgba8)[i * 4 + 2] = g << 3; // 5
		((uint8_t*)rgba8)[i * 4 + 1] = b << 3; // 5
		((uint8_t*)rgba8)[i * 4 + 0] = 255;
	}
}

/*
	Dumps Textures from VRAM using a FBO (Works on OpenGL 3.2+)
	A better way would be glGetTexSubImage from OpenGL 4.5.
*/
void LandIsland::DumpTextures()
{
	GLuint textureID = _materialArray->GetHandle();

	GLuint fboID = 0;
	glGenFramebuffers(1, &fboID);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboID);

	for (unsigned int i = 0; i < _materialCount; i++)
	{
		uint8_t* pixels = new uint8_t[256 * 256 * 4];

		glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureID, 0, i);
		glReadPixels(0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

		auto filename = "dump/landtex" + std::to_string(i) + ".png";

		printf("Writing texture %d to %s\n", i, filename.c_str());
		stbi_write_png(filename.c_str(), 256, 256, 4, pixels, 256 * 4);

		delete[] pixels;
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fboID);
}

void LandIsland::DumpMaps()
{
	const int cellsize = 17;

	// 32x32 block grid with 16x16 cells
	// 512 x 512 pixels
	// lets go with 3 channels for a laugh
	uint8_t* data = new uint8_t[32 * 32 * cellsize * cellsize];

	memset(data, 0x00, 32 * 32 * cellsize * cellsize);

	/*for (unsigned int b = 0; b < _blockCount; b++)
	{
		LandBlock* block = &_landBlocks[b];
		int mapx         = block->GetBlockPosition()->x;
		int mapz         = block->GetBlockPosition()->y;
		int lineStride   = 32 * cellsize;

		for (int x = 0; x < cellsize; x++)
		{
			for (int y = 0; y < cellsize; y++)
			{
				LandCell cell = block->GetCells()[y * 17 + x];

				int cellX = (mapx * cellsize) + x;
				int cellY = (mapz * cellsize) + y;

				uint8_t col = 0x00;
				if (cell.Coastline())
					col = 100;
				else if (cell.HasWater())
					col = 200;
				else if (cell.FullWater())
					col = 255;

				data[(cellY * lineStride) + cellX] = cell.Altitude();
			}
		}
	}*/

	FILE* fptr = fopen("dump.raw", "wb");
	fwrite(data, 32 * 32 * cellsize * cellsize, 1, fptr);
	fclose(fptr);

	delete[] data;
}

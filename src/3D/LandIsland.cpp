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

#include "LandIsland.h"

#include <stdexcept>

#include <spdlog/spdlog.h>

#include <Graphics/OpenGL.h>
#include <Common/FileSystem.h>
#include <Common/IStream.h>
#include <Game.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <Common/stb_image_write.h>

using namespace openblack;

const float LandIsland::HeightUnit = 0.67f;
const float LandIsland::CellSize   = 10.0f;

LandIsland::LandIsland()
	: _materialCount(0)
	, _lowresCount(0)
	, _blockIndexLookup{ 0 }
{
	auto file           = Game::instance()->GetFileSystem().Open("Data/Textures/smallbumpa.raw", FileMode::Read);
	uint8_t* smallbumpa = new uint8_t[file->Size()];
	file->Read(smallbumpa, file->Size());

	_textureSmallBump = std::make_unique<Texture2D>();
	_textureSmallBump->Create(256, 256, 1, Format::R8, Wrapping::Repeat, smallbumpa, file->Size());
	delete[] smallbumpa;
}

LandIsland::~LandIsland() = default;

/*
Loads from the original Black & White .lnd file format and
converts the data into our own
*/
void LandIsland::LoadFromFile(IStream& stream)
{
	uint32_t blockCount, countryCount, blockSize, matSize, countrySize;

	stream.Read<uint32_t>(&blockCount);
	stream.Read(_blockIndexLookup.data(), 1024);
	stream.Read<uint32_t>(&_materialCount);
	stream.Read<uint32_t>(&countryCount);

	// todo: lets assert these against sizeof(LandBlock) etc..
	stream.Read<uint32_t>(&blockSize);
	stream.Read<uint32_t>(&matSize);
	stream.Read<uint32_t>(&countrySize);
	stream.Read<uint32_t>(&_lowresCount);

	// skip over low resolution textures
	// for future reference these are formatted GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
	for (unsigned int i = 0; i < _lowresCount; i++)
	{
		stream.Seek(16, SeekMode::Current);
		uint32_t textureSize = stream.ReadValue<uint32_t>();
		stream.Seek(textureSize - 4, SeekMode::Current);
	}

	blockCount--; // take away a block from the count, because it's not in the file?

	spdlog::debug("[LandIsland] loading {} blocks", blockCount);
	_landBlocks.reserve(blockCount);
	for (uint32_t i = 0; i < blockCount; i++)
	{
		uint8_t* blockData = new uint8_t[blockSize];
		stream.Read(blockData, blockSize);

		_landBlocks.push_back(LandBlock());
		_landBlocks[i].Load(blockData, blockSize);

		delete[] blockData;
	}

	spdlog::debug("[LandIsland] loading {} countries", countryCount);
	_countries.reserve(countryCount);
	for (uint32_t i = 0; i < countryCount; i++)
	{
		Country country;
		stream.Read(&country);
		_countries.push_back(country);
	}

	spdlog::debug("[LandIsland] loading {} textures", _materialCount);
	std::vector<uint32_t> rgba8TextureData;
	rgba8TextureData.resize(256 * 256 * _materialCount);
	for (uint32_t i = 0; i < _materialCount; i++)
	{
		std::array<uint16_t, 256 * 256> rgba5TextureData;

		uint16_t terrainType = stream.ReadValue<uint16_t>();
		stream.Read(rgba5TextureData.data(), static_cast<std::size_t>(256 * 256 * sizeof(uint16_t)));

		convertRGB5ToRGB8(rgba5TextureData.data(), rgba8TextureData.data() + i * rgba5TextureData.size(), 256 * 256);
	}
	_materialArray = std::make_unique<Texture2D>();
	_materialArray->Create(256, 256, _materialCount, Format::RGBA8, Wrapping::ClampEdge, rgba8TextureData.data(), rgba8TextureData.size() * sizeof(rgba8TextureData[0]));

	// read noise map into Texture2D
	stream.Read(_noiseMap.data(), _noiseMap.size() * sizeof(_noiseMap[0]));
	_textureNoiseMap = std::make_unique<Texture2D>();
	_textureNoiseMap->Create(256, 256, 1, Format::R8, Wrapping::ClampEdge, _noiseMap.data(), _noiseMap.size() * sizeof(_noiseMap[0]));

	// read bump map into Texture2D
	std::array<uint8_t, 256* 256> bumpMapTextureData;
	stream.Read(bumpMapTextureData.data(), bumpMapTextureData.size() * sizeof(bumpMapTextureData[0]));
	_textureBumpMap = std::make_unique<Texture2D>();
	_textureBumpMap->Create(256, 256, 1, Format::R8, Wrapping::ClampEdge, bumpMapTextureData.data(), bumpMapTextureData.size() * sizeof(bumpMapTextureData[0]));

	// build the meshes (we could move this elsewhere)
	for (auto& block : _landBlocks)
		block.BuildMesh(*this);
}

/*const uint8_t LandIsland::GetAltitudeAt(glm::ivec2 vec) const
{
	return uint8_t();
}
*/

float LandIsland::GetHeightAt(glm::vec2 vec) const
{
	return GetCell(vec.x * 0.1f, vec.y * 0.1f).Altitude() * LandIsland::HeightUnit;
}

uint8_t LandIsland::GetNoise(int x, int y)
{
	return _noiseMap[(y & 0xFF) + 256 * (x & 0xFF)];
}

const LandBlock* LandIsland::GetBlock(int8_t x, int8_t z) const
{
	// our blocks can only be between [0-31, 0-31]
	if (x < 0 || x > 32 || z < 0 || z > 32)
		return nullptr;

	const uint8_t blockIndex = _blockIndexLookup[x * 32 + z];
	if (blockIndex == 0)
		return nullptr;

	return &_landBlocks[blockIndex - 1];
}

const LandCell EmptyCell;

const LandCell& LandIsland::GetCell(int x, int z) const
{
	if (x < 0 || x > 511 || z < 0 || z > 511)
		return EmptyCell; // return empty water cell

	const uint8_t blockIndex = _blockIndexLookup[32 * (x >> 4) + (z >> 4)];

	if (blockIndex == 0)
		return EmptyCell; // return empty water cell

	return _landBlocks[blockIndex - 1].GetCells()[(z & 0xF) + 17 * (x & 0xF)];
}

void LandIsland::Draw(ShaderProgram& program)
{
	//program.SetTextureSampler("s_materials", 0, *_materialArray);
	//program.SetTextureSampler("s_bump", 1, *_textureBumpMap);
	//program.SetTextureSampler("s_smallBump", 2, *_textureSmallBump);

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

		((uint8_t*)rgba8)[i * 4 + 0] = r << 3; // 5
		((uint8_t*)rgba8)[i * 4 + 1] = g << 3; // 5
		((uint8_t*)rgba8)[i * 4 + 2] = b << 3; // 5
		((uint8_t*)rgba8)[i * 4 + 3] = 255;
	}
}

/*
	Dumps Textures from VRAM using a FBO (Works on OpenGL 3.2+)
	A better way would be glGetTexSubImage from OpenGL 4.5.
*/
void LandIsland::DumpTextures()
{
	GLuint textureID = _materialArray->GetNativeHandle();

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
	const int cellsize = 16;

	// 32x32 block grid with 16x16 cells
	// 512 x 512 pixels
	// lets go with 3 channels for a laugh
	uint8_t* data = new uint8_t[32 * 32 * cellsize * cellsize];

	memset(data, 0x00, 32 * 32 * cellsize * cellsize);

	for (unsigned int b = 0; b < _landBlocks.size(); b++)
	{
		LandBlock& block = _landBlocks[b];

		int mapx         = block.GetBlockPosition().x;
		int mapz         = block.GetBlockPosition().y;
		int lineStride   = 32 * cellsize;

		for (int x = 0; x < cellsize; x++)
		{
			for (int y = 0; y < cellsize; y++)
			{
				LandCell cell = block.GetCells()[y * 17 + x];

				int cellX = (mapx * cellsize) + x;
				int cellY = (mapz * cellsize) + y;

				data[(cellY * lineStride) + cellX] = cell.Altitude();
			}
		}
	}

	FILE* fptr = fopen("dump.raw", "wb");
	fwrite(data, 32 * 32 * cellsize * cellsize, 1, fptr);
	fclose(fptr);

	delete[] data;
}

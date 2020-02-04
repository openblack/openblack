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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Common/FileSystem.h"
#include "Common/IStream.h"
#include "Common/stb_image_write.h"
#include "Game.h"

#include <spdlog/spdlog.h>

#include <stdexcept>
#include <lnd_file.h>

using namespace openblack;
using namespace openblack::graphics;

const float LandIsland::HeightUnit = 0.67f;
const float LandIsland::CellSize = 10.0f;

LandIsland::LandIsland(): _blockIndexLookup {0}
{
	auto file = Game::instance()->GetFileSystem().Open("Data/Textures/smallbumpa.raw", File::Mode::Read);
	auto* smallbumpa = new uint8_t[file->GetSize()];
	file->Read(smallbumpa, file->GetSize());

	_textureSmallBump = std::make_unique<Texture2D>("LandIslandSmallBump");
	_textureSmallBump->Create(256, 256, 1, Format::R8, Wrapping::Repeat, smallbumpa, file->GetSize());
	delete[] smallbumpa;
}

LandIsland::~LandIsland() = default;

void LandIsland::LoadFromFile(const std::string& filename)
{
	spdlog::debug("Loading Land from file: {}", filename);
	lnd::LNDFile lnd;

	try
	{
		lnd.Open(filename);
	}
	catch (std::runtime_error& err)
	{
		spdlog::error("Failed to open lnd file from filesystem {}: {}", filename, err.what());
		return;
	}

	std::memcpy(_blockIndexLookup.data(), lnd.GetHeader().lookUpTable, _blockIndexLookup.size() * sizeof(_blockIndexLookup[0]));

	auto& lndBlocks = lnd.GetBlocks();
	spdlog::debug("[LandIsland] loading {} blocks", lndBlocks.size());
	_landBlocks.resize(lndBlocks.size());
	for (size_t i = 0; i < _landBlocks.size(); i++)
	{
		_landBlocks[i]._block = std::make_unique<lnd::LNDBlock>(lndBlocks[i]);
	}

	spdlog::debug("[LandIsland] loading {} countries", lnd.GetCountries().size());
	_countries = lnd.GetCountries();

	auto materialCount = lnd.GetMaterials().size();
	spdlog::debug("[LandIsland] loading {} textures", materialCount);
	std::vector<uint16_t> rgba5TextureData;
	rgba5TextureData.resize(lnd::LNDMaterial::width * lnd::LNDMaterial::height * lnd.GetMaterials().size());
	for (size_t i = 0; i < lnd.GetMaterials().size(); i++)
	{
		std::memcpy(&rgba5TextureData[lnd::LNDMaterial::width * lnd::LNDMaterial::height * i],
			lnd.GetMaterials()[i].texels,
			sizeof(lnd.GetMaterials()[i].texels));
	}
	_materialArray = std::make_unique<Texture2D>("LandIslandMaterialArray");
	_materialArray->Create(lnd::LNDMaterial::width, lnd::LNDMaterial::height, materialCount,
		Format::RGB5A1, Wrapping::ClampEdge, rgba5TextureData.data(),
		rgba5TextureData.size() * sizeof(rgba5TextureData[0]));

	// read noise map into Texture2D
	std::memcpy(_noiseMap.data(), lnd.GetExtra().noise.texels, _noiseMap.size() * sizeof(_noiseMap[0]));
	_textureNoiseMap = std::make_unique<Texture2D>("LandIslandNoiseMap");
	_textureNoiseMap->Create(lnd::LNDBumpMap::width, lnd::LNDBumpMap::height, 1,
		Format::R8, Wrapping::ClampEdge, _noiseMap.data(),
		_noiseMap.size() * sizeof(_noiseMap[0]));

	// read bump map into Texture2D
	_textureBumpMap = std::make_unique<Texture2D>("LandIslandBumpMap");
	_textureBumpMap->Create(lnd::LNDBumpMap::width, lnd::LNDBumpMap::height, 1,
		Format::R8, Wrapping::ClampEdge, lnd.GetExtra().bump.texels,
		sizeof(lnd.GetExtra().bump.texels));

	// build the meshes (we could move this elsewhere)
	for (auto& block : _landBlocks) block.BuildMesh(*this);
	bgfx::frame();
}

void openblack::LandIsland::Update(float timeOfDay, float bumpMapStrength, float smallBumpMapStrength)
{
	_timeOfDay = timeOfDay;
	_bumpMapStrength = bumpMapStrength;
	_smallBumpMapStrength = smallBumpMapStrength;
}

float LandIsland::GetHeightAt(glm::vec2 vec) const
{
	return GetCell(vec * 0.1f).altitude * LandIsland::HeightUnit;
}

uint8_t LandIsland::GetNoise(int x, int y)
{
	return _noiseMap[(y & 0xFF) + 256 * (x & 0xFF)];
}

const LandBlock* LandIsland::GetBlock(const glm::u8vec2& coordinates) const
{
	// our blocks can only be between [0-31, 0-31]
	if (coordinates.x > 32 || coordinates.y > 32)
		return nullptr;

	const uint8_t blockIndex = _blockIndexLookup[coordinates.x * 32 + coordinates.y];
	if (blockIndex == 0)
		return nullptr;

	return &_landBlocks[blockIndex - 1];
}

constexpr lnd::LNDCell EmptyCell() noexcept
{
	lnd::LNDCell cell{};
	cell.properties.fullWater = true;
	return cell;
}

constexpr lnd::LNDCell s_EmptyCell = EmptyCell();

const lnd::LNDCell& LandIsland::GetCell(const glm::u16vec2& coordinates) const
{
	if (coordinates.x > 511 || coordinates.y > 511)
	{
		return s_EmptyCell;
	}

	const uint16_t lookupIndex = ((coordinates.x & ~0xFU) << 1U) | (coordinates.y >> 4U);
	const uint16_t cellIndex = (coordinates.x & 0xFU) * 0x11u + (coordinates.y & 0xFU);

	const uint8_t blockIndex = _blockIndexLookup[lookupIndex];

	if (blockIndex == 0)
	{
		return s_EmptyCell;
	}
	assert(_landBlocks.size() >= blockIndex);
	return _landBlocks[blockIndex - 1].GetCells()[cellIndex];
}

void LandIsland::Draw(graphics::RenderPass viewId, const ShaderProgram& program, bool cullBack) const
{
	for (auto& block : _landBlocks)
	{
		program.SetTextureSampler("s_materials", 0, *_materialArray);
		program.SetTextureSampler("s_bump", 1, *_textureBumpMap);
		program.SetTextureSampler("s_smallBump", 2, *_textureSmallBump);
		program.SetUniformValue("u_timeOfDay", &_timeOfDay);
		program.SetUniformValue("u_bumpmapStrength", &_bumpMapStrength);
		program.SetUniformValue("u_smallBumpmapStrength", &_smallBumpMapStrength);
		block.Draw(viewId, program, cullBack);
	}
}

void LandIsland::DumpTextures()
{
	_materialArray->DumpTexture();
}

void LandIsland::DumpMaps()
{
	const int cellsize = 16;

	// 32x32 block grid with 16x16 cells
	// 512 x 512 pixels
	// lets go with 3 channels for a laugh
	auto* data = new uint8_t[32 * 32 * cellsize * cellsize];

	memset(data, 0x00, 32 * 32 * cellsize * cellsize);

	for (unsigned int b = 0; b < _landBlocks.size(); b++)
	{
		LandBlock& block = _landBlocks[b];

		int mapx = block.GetBlockPosition().x;
		int mapz = block.GetBlockPosition().y;
		int lineStride = 32 * cellsize;

		for (int x = 0; x < cellsize; x++)
		{
			for (int y = 0; y < cellsize; y++)
			{
				auto cell = block.GetCells()[y * 17 + x];

				int cellX = (mapx * cellsize) + x;
				int cellY = (mapz * cellsize) + y;

				data[(cellY * lineStride) + cellX] = cell.altitude;
			}
		}
	}

	FILE* fptr = fopen("dump.raw", "wb");
	fwrite(data, 32 * 32 * cellsize * cellsize, 1, fptr);
	fclose(fptr);

	delete[] data;
}

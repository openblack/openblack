/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "LandIsland.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stdexcept>

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <LNDFile.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/spdlog.h>

#include "Common/IStream.h"
#include "Common/stb_image_write.h"
#include "Dynamics/LandBlockBulletMeshInterface.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/Texture2D.h"

using namespace openblack;
using namespace openblack::graphics;

const uint8_t LandIsland::k_GridCount = 32;
const uint8_t LandIsland::k_CellCount = 16;
const float LandIsland::k_HeightUnit = 0.67f;
const float LandIsland::k_CellSize = 10.0f;

LandIsland::LandIsland() = default;

LandIsland::~LandIsland() = default;

void LandIsland::LoadFromFile(const std::filesystem::path& path)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading Land from file: {}", path.string());
	lnd::LNDFile lnd;

	try
	{
		lnd.Open(path);
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open lnd file from filesystem {}: {}", path.string(), err.what());
		return;
	}

	_blockIndexLookup = lnd.GetHeader().lookUpTable;

	const auto& lndBlocks = lnd.GetBlocks();
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "[LandIsland] loading {} blocks", lndBlocks.size());
	_landBlocks.resize(lndBlocks.size());
	for (size_t i = 0; i < _landBlocks.size(); i++)
	{
		_landBlocks[i]._block = std::make_unique<lnd::LNDBlock>(lndBlocks[i]);
	}

	uint32_t minX = std::numeric_limits<uint32_t>::max();
	uint32_t minZ = std::numeric_limits<uint32_t>::max();
	uint32_t maxX = 0;
	uint32_t maxZ = 0;
	for (auto& b : _landBlocks)
	{
		if (minX > b._block->blockX)
		{
			minX = b._block->blockX;
			_extentMin.x = b.GetMapPosition().x;
		}
		if (maxX < b._block->blockX)
		{
			maxX = b._block->blockX;
			_extentMax.x = b.GetMapPosition().x;
		}
		if (minZ > b._block->blockZ)
		{
			minZ = b._block->blockZ;
			_extentMin.y = b.GetMapPosition().y;
		}
		if (maxZ < b._block->blockZ)
		{
			maxZ = b._block->blockZ;
			_extentMax.y = b.GetMapPosition().y;
		}
	}
	const auto res =
	    glm::u16vec2(maxX - minX, maxZ - minZ) * glm::u16vec2(lnd::LNDMaterial::k_Width, lnd::LNDMaterial::k_Height);
	_footprintFrameBuffer = std::make_unique<FrameBuffer>("Footprints", res.x, res.y, graphics::Format::RGBA8);

	_proj = glm::ortho(_extentMin.x, _extentMax.x, _extentMin.y, _extentMax.y);
	_view = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "[LandIsland] loading {} countries", lnd.GetCountries().size());
	_countries = lnd.GetCountries();

	auto materialCount = static_cast<uint16_t>(lnd.GetMaterials().size());
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "[LandIsland] loading {} textures", materialCount);
	std::vector<uint16_t> rgba5TextureData;
	rgba5TextureData.resize(lnd::LNDMaterial::k_Width * lnd::LNDMaterial::k_Height * lnd.GetMaterials().size());
	for (size_t i = 0; i < lnd.GetMaterials().size(); i++)
	{
		std::memcpy(&rgba5TextureData[lnd::LNDMaterial::k_Width * lnd::LNDMaterial::k_Height * i],
		            lnd.GetMaterials()[i].texels.data(),
		            sizeof(lnd.GetMaterials()[i].texels[0]) * lnd.GetMaterials()[i].texels.size());
	}
	_materialArray = std::make_unique<Texture2D>("LandIslandMaterialArray");
	_materialArray->Create(lnd::LNDMaterial::k_Width, lnd::LNDMaterial::k_Height, materialCount, Format::RGB5A1,
	                       Wrapping::ClampEdge, Filter::Linear, rgba5TextureData.data(),
	                       static_cast<uint32_t>(rgba5TextureData.size() * sizeof(rgba5TextureData[0])));

	// read noise map into Texture2D
	_noiseMap = lnd.GetExtra().noise.texels;
	_textureNoiseMap = std::make_unique<Texture2D>("LandIslandNoiseMap");
	_textureNoiseMap->Create(lnd::LNDBumpMap::k_Width, lnd::LNDBumpMap::k_Height, 1, Format::R8, Wrapping::ClampEdge,
	                         Filter::Linear, _noiseMap.data(), static_cast<uint32_t>(_noiseMap.size() * sizeof(_noiseMap[0])));

	// read bump map into Texture2D
	_textureBumpMap = std::make_unique<Texture2D>("LandIslandBumpMap");
	_textureBumpMap->Create(lnd::LNDBumpMap::k_Width, lnd::LNDBumpMap::k_Height, 1, Format::R8, Wrapping::Repeat,
	                        Filter::Linear, lnd.GetExtra().bump.texels.data(),
	                        static_cast<uint32_t>(sizeof(lnd.GetExtra().bump.texels[0]) * lnd.GetExtra().bump.texels.size()));

	// build the meshes (we could move this elsewhere)
	for (auto& block : _landBlocks)
	{
		block.BuildMesh(*this);
	}
	bgfx::frame();
}

float LandIsland::GetHeightAt(glm::vec2 vec) const
{
	return GetCell(vec * 0.1f).altitude * LandIsland::k_HeightUnit;
}

uint8_t LandIsland::GetNoise(glm::u8vec2 pos)
{
	return _noiseMap.at(pos.x * 256 + pos.y);
}

const LandBlock* LandIsland::GetBlock(const glm::u8vec2& coordinates) const
{
	// our blocks can only be between [0-31, 0-31]
	if (coordinates.x > 32 || coordinates.y > 32)
	{
		return nullptr;
	}

	const uint8_t blockIndex = _blockIndexLookup.at(coordinates.x * 32 + coordinates.y);
	if (blockIndex == 0)
	{
		return nullptr;
	}

	return &_landBlocks[blockIndex - 1];
}

constexpr lnd::LNDCell EmptyCell() noexcept
{
	lnd::LNDCell cell {};
	cell.properties.fullWater = true;
	return cell;
}

constexpr lnd::LNDCell k_EmptyCell = EmptyCell();

const lnd::LNDCell& LandIsland::GetCell(const glm::u16vec2& coordinates) const
{
	if (coordinates.x > 511 || coordinates.y > 511)
	{
		return k_EmptyCell;
	}

	const uint16_t lookupIndex = ((coordinates.x & ~0xFU) << 1U) | (coordinates.y >> 4U);
	const uint16_t cellIndex = (coordinates.x & 0xFU) * 0x11u + (coordinates.y & 0xFU);

	const uint8_t blockIndex = _blockIndexLookup.at(lookupIndex);

	if (blockIndex == 0)
	{
		return k_EmptyCell;
	}
	assert(_landBlocks.size() >= blockIndex);
	return _landBlocks[blockIndex - 1].GetCells()[cellIndex];
}

void LandIsland::DumpTextures() const
{
	_materialArray->DumpTexture();
}

std::vector<uint8_t> LandIsland::CreateHeightMap() const
{
	// 32x32 block grid with 16x16 cells
	// 512 x 512 pixels
	std::vector<uint8_t> data;
	data.resize(k_GridCount * k_GridCount * k_CellCount * k_CellCount, 0);

	for (const auto& block : _landBlocks)
	{
		const int mapx = block.GetBlockPosition().x;
		const int mapz = block.GetBlockPosition().y;
		const int lineStride = k_GridCount * k_CellCount;

		for (int y = 0; y < k_CellCount; y++)
		{
			for (int x = 0; x < k_CellCount; x++)
			{
				const auto& cell = block.GetCells()[x * (k_CellCount + 1) + y];

				const int cellX = (mapx * k_CellCount) + x;
				const int cellY = (mapz * k_CellCount) + y;

				data[(cellY * lineStride) + cellX] = cell.altitude;
			}
		}
	}
	return data;
}

void LandIsland::DumpMaps() const
{
	auto data = CreateHeightMap();
	FILE* fptr = fopen("dump.raw", "wb");
	fwrite(data.data(), data.size() * sizeof(data[0]), 1, fptr);
	fclose(fptr);
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "LandIsland.h"

#include <cmath>

#include <stdexcept>

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <LNDFile.h>
#include <bgfx/bgfx.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/spdlog.h>
#include <stb_image_write.h>

#include "3D/LandBlock.h"
#include "3D/MapCoords.h"
#include "Dynamics/LandBlockBulletMeshInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::graphics;

const uint8_t LandIslandInterface::k_CellCount = 16;
const float LandIslandInterface::k_HeightUnit = 0.67f;
const float LandIslandInterface::k_CellSize = 10.0f;

namespace
{
constexpr int32_t k_MapSize = 0x200;
// The 1/256 of LH3DIsland::GetAltitude's integer interpolation
constexpr double k_AltitudeFraction = 1.0 / 256.0;
// Land at or below this altitude is at sea level. LH3DIsland draws it at height 0, and GetAltitude treats it the same
// in cells no higher than k_SeaLevelClampAltitude. The game only turns the latter off while it creates a fish farm.
constexpr uint8_t k_SeaLevelAltitude = 3;
constexpr uint8_t k_SeaLevelClampAltitude = 4;
} // namespace

int32_t LandIslandInterface::ToMapCoords(float unit)
{
	return map_coords::ToFixed(unit);
}

float LandIslandInterface::GetDrawnAltitude(uint8_t altitude)
{
	return altitude <= k_SeaLevelAltitude ? 0.0f : static_cast<float>(altitude) * k_HeightUnit;
}

double LandIslandInterface::GetAltitude(int32_t mapX, int32_t mapZ) const
{
	const auto cellX = static_cast<int16_t>(static_cast<uint32_t>(mapX) >> 16);
	const auto cellZ = static_cast<int16_t>(static_cast<uint32_t>(mapZ) >> 16);
	if (cellX < 0 || cellX >= k_MapSize || cellZ < 0 || cellZ >= k_MapSize)
	{
		return 0.0;
	}
	const auto* cell = FindCell({static_cast<uint16_t>(cellX), static_cast<uint16_t>(cellZ)});
	if (cell == nullptr)
	{
		return 0.0;
	}

	// Neighbours within the block's 17x17 cell array: +1 is z + 1, +17 is x + 1
	const auto clamp = cell[0].altitude <= k_SeaLevelClampAltitude;
	const auto altitude = [clamp](const lnd::LNDCell& c) -> int32_t {
		return clamp && c.altitude <= k_SeaLevelAltitude ? 0 : c.altitude;
	};
	const auto a00 = altitude(cell[0]);
	const auto a01 = altitude(cell[1]);
	const auto a10 = altitude(cell[17]);
	const auto a11 = altitude(cell[18]);

	const auto fractionX = static_cast<uint32_t>(mapX) & 0xFFFF;
	const auto fractionZ = static_cast<uint32_t>(mapZ) & 0xFFFF;

	// Complete the plane of the triangle the point is in. Cells split from x + 1 to z + 1 rather than corner to corner.
	int32_t v00 = a00;
	int32_t v01 = a01;
	int32_t v10 = a10;
	int32_t v11 = a11;
	if (cell[0].properties.split != 0)
	{
		if (fractionZ > 0xFFFF - fractionX)
		{
			v00 = a10 - a11 + a01;
		}
		else
		{
			v11 = a10 - a00 + a01;
		}
	}
	else if (fractionX > fractionZ)
	{
		v01 = a00 - a10 + a11;
	}
	else
	{
		v10 = a00 - a01 + a11;
	}

	const auto z8 = static_cast<int32_t>(fractionZ >> 8);
	const auto x8 = static_cast<int32_t>(fractionX >> 8);
	const auto edgeX0 = ((v01 - v00) * z8) + (v00 << 8);
	const auto edgeX1 = ((v11 - v10) * z8) + (v10 << 8);
	const auto height = (((edgeX1 - edgeX0) * x8) >> 8) + edgeX0;
	return static_cast<double>(height) * static_cast<double>(k_HeightUnit) * k_AltitudeFraction;
}

LandIsland::LandIsland(const std::filesystem::path& path)
{
	LoadFromFile(path);
}

LandIsland::~LandIsland() noexcept = default;

void LandIsland::LoadFromFile(const std::filesystem::path& path)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading Land from file: {}", path.string());
	lnd::LNDFile lnd;

	const auto result = lnd.ReadFile(*Locator::filesystem::value().GetData(path));
	if (result != lnd::LNDResult::Success)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to open lnd file from filesystem {}: {}", path.string(),
		                    lnd::ResultToStr(result));
		throw lnd::ResultToStr(result);
	}

	_blockIndexLookup = lnd.GetHeader().lookUpTable;

	const auto& lndBlocks = lnd.GetBlocks();
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "[LandIsland] loading {} blocks", lndBlocks.size());
	_landBlocks.resize(lndBlocks.size());
	for (size_t i = 0; i < _landBlocks.size(); i++)
	{
		_landBlocks[i].SetLndBlock(lndBlocks[i]);
	}

	_extentIndexMin.x = std::numeric_limits<uint16_t>::max();
	_extentIndexMin.y = std::numeric_limits<uint16_t>::max();
	_extentIndexMax.x = 0;
	_extentIndexMax.y = 0;
	for (auto& b : _landBlocks)
	{
		if (_extentIndexMin.x > b.GetLndBlock()->blockX)
		{
			_extentIndexMin.x = static_cast<uint16_t>(b.GetLndBlock()->blockX);
			_extentMin.x = b.GetMapPosition().x;
		}
		if (_extentIndexMax.x < b.GetLndBlock()->blockX)
		{
			_extentIndexMax.x = static_cast<uint16_t>(b.GetLndBlock()->blockX);
			_extentMax.x = b.GetMapPosition().x;
		}
		if (_extentIndexMin.y > b.GetLndBlock()->blockZ)
		{
			_extentIndexMin.y = static_cast<uint16_t>(b.GetLndBlock()->blockZ);
			_extentMin.y = b.GetMapPosition().y;
		}
		if (_extentIndexMax.y < b.GetLndBlock()->blockZ)
		{
			_extentIndexMax.y = static_cast<uint16_t>(b.GetLndBlock()->blockZ);
			_extentMax.y = b.GetMapPosition().y;
		}
	}
	_extentMax += k_CellSize * k_CellCount;

	const auto indexSize = _extentIndexMax - _extentIndexMin + glm::u16vec2(1, 1);

	_heightMap = std::make_unique<Texture2D>("Height Map");
	const auto heightMapData = CreateHeightMap();
	_heightMap->Create(indexSize.x * k_CellCount + 1, indexSize.y * k_CellCount + 1, 1, graphics::TextureFormat::R8,
	                   Wrapping::ClampEdge, Filter::Linear,
	                   bgfx::makeRef(heightMapData.data(), static_cast<uint32_t>(heightMapData.size())));

	const auto res = indexSize * glm::u16vec2(lnd::LNDMaterial::k_Width, lnd::LNDMaterial::k_Height);
	_footprintFrameBuffer = std::make_unique<FrameBuffer>("Footprints", res.x, res.y, graphics::TextureFormat::RGBA8);

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
	_materialArray->Create(
	    lnd::LNDMaterial::k_Width, lnd::LNDMaterial::k_Height, materialCount, TextureFormat::BGR5A1, Wrapping::ClampEdge,
	    Filter::Linear,
	    bgfx::makeRef(rgba5TextureData.data(), static_cast<uint32_t>(rgba5TextureData.size() * sizeof(rgba5TextureData[0]))));

	// read noise map into Texture2D
	_noiseMap = lnd.GetExtra().noise.texels;
	_textureNoiseMap = std::make_unique<Texture2D>("LandIslandNoiseMap");
	_textureNoiseMap->Create(lnd::LNDBumpMap::k_Width, lnd::LNDBumpMap::k_Height, 1, TextureFormat::R8, Wrapping::ClampEdge,
	                         Filter::Linear,
	                         bgfx::makeRef(_noiseMap.data(), static_cast<uint32_t>(_noiseMap.size() * sizeof(_noiseMap[0]))));

	// read bump map into Texture2D
	_textureBumpMap = std::make_unique<Texture2D>("LandIslandBumpMap");
	_textureBumpMap->Create(
	    lnd::LNDBumpMap::k_Width, lnd::LNDBumpMap::k_Height, 1, TextureFormat::R8, Wrapping::Repeat, Filter::Linear,
	    bgfx::makeRef(lnd.GetExtra().bump.texels.data(),
	                  static_cast<uint32_t>(sizeof(lnd.GetExtra().bump.texels[0]) * lnd.GetExtra().bump.texels.size())));

	// build the meshes (we could move this elsewhere)
	for (auto& block : _landBlocks)
	{
		block.BuildMesh(*this);
	}
	bgfx::frame();
}

float LandIsland::GetHeightAt(glm::vec2 vec) const
{
	return static_cast<float>(GetAltitude(ToMapCoords(vec.x), ToMapCoords(vec.y)));
}

glm::vec3 LandIsland::GetNormalAt(glm::vec2 vec) const
{
	const auto delta = 0.1f;
	const auto posLeft = vec - glm::vec2(delta, 0.0f);
	const auto posRight = vec + glm::vec2(delta, 0.0f);
	const auto posBack = vec - glm::vec2(0.0f, delta);
	const auto posForward = vec + glm::vec2(0.0f, delta);

	const auto heightLeft = GetHeightAt(posLeft);
	const auto heightRight = GetHeightAt(posRight);
	const auto heightBack = GetHeightAt(posBack);
	const auto heightForward = GetHeightAt(posForward);

	const auto slopeLeftRight = glm::vec3(2.0f * delta, heightRight - heightLeft, 0.0f);
	const auto slopeBackForward = glm::vec3(0.0f, heightForward - heightBack, 2.0f * delta);

	auto normal = glm::cross(slopeBackForward, slopeLeftRight);
	normal = glm::normalize(normal);

	if (normal.y < 0)
	{
		normal = -normal;
	}

	return normal;
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
	const auto* cell = FindCell(coordinates);
	return cell != nullptr ? *cell : k_EmptyCell;
}

const lnd::LNDCell* LandIsland::FindCell(const glm::u16vec2& coordinates) const
{
	if (coordinates.x > 511 || coordinates.y > 511)
	{
		return nullptr;
	}

	const auto mapCoordinates = coordinates >> static_cast<uint16_t>(0x4);
	const auto cellCoordinates = static_cast<glm::u8vec2>(coordinates) & static_cast<uint8_t>(0xF);
	const auto lookupIndex = mapCoordinates.x << 5u | mapCoordinates.y;
	const auto cellIndex = cellCoordinates.x * 0x11u + cellCoordinates.y;

	const uint8_t blockIndex = _blockIndexLookup.at(lookupIndex);

	if (blockIndex == 0)
	{
		return nullptr;
	}
	assert(_landBlocks.size() >= blockIndex);
	return &_landBlocks[blockIndex - 1].GetCells()[cellIndex];
}

void LandIsland::DumpTextures() const
{
	_materialArray->DumpTexture();
}

std::vector<uint8_t> LandIsland::CreateHeightMap() const
{
	// 16x16 cells but the last is shared
	// max of 32x32 block grid
	// max of 512 x 512 pixels
	// extra pixel at the end of the map
	std::vector<uint8_t> data;
	const auto extentSize = _extentIndexMax - _extentIndexMin + glm::u16vec2(1, 1);
	const auto resolution = extentSize * static_cast<uint16_t>(k_CellCount) + static_cast<uint16_t>(1);
	data.resize(resolution.x * resolution.y, 0);

	for (const auto& block : _landBlocks)
	{
		const auto blockOffset = static_cast<glm::u16vec2>(block.GetBlockPosition() * 16);
		const auto mapPos = block.GetBlockPosition() - static_cast<glm::ivec2>(_extentIndexMin);
		for (int y = 0; y < k_CellCount; y++)
		{
			for (int x = 0; x < k_CellCount; x++)
			{
				const auto offset = glm::u16vec2(x, y);
				const auto cellPos = mapPos * static_cast<int>(k_CellCount) + static_cast<glm::ivec2>(offset);
				const auto& cell = GetCell(blockOffset + offset);
				if ((cellPos.y * resolution.x) + cellPos.x < static_cast<int>(data.size()))
				{
					// Flat at sea level, as the land is drawn
					data.at((cellPos.y * resolution.x) + cellPos.x) =
					    static_cast<uint8_t>(std::lround(GetDrawnAltitude(cell.altitude) / k_HeightUnit));
				}
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

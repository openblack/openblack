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

#include <algorithm>
#include <iterator>
#include <span>
#include <stdexcept>

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <LNDFile.h>
#include <bgfx/bgfx.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/spdlog.h>
#include <stb_image_write.h>

#include "3D/BlockTexture.h"
#include "3D/LandBlock.h"
#include "3D/LandData.h"
#include "3D/LandNormal.h"
#include "3D/MapCoords.h"
#include "Dynamics/LandBlockBulletMeshInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexBuffer.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::graphics;

const uint8_t LandIslandInterface::k_CellCount = 16;
const float LandIslandInterface::k_HeightUnit = 0.67f;
const float LandIslandInterface::k_CellSize = 10.0f;

namespace
{
constexpr int32_t k_MapSize = 0x200;
// The 1/256 of the game's integer altitude interpolation
constexpr double k_AltitudeFraction = 1.0 / 256.0;
// Land at or below this altitude is at sea level. The game draws it at height 0, and GetAltitude treats it the same
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

LandIsland::LandIsland(const LandData& data)
{
	Build(data);
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
	Build(LandData::FromLnd(lnd));
}

void LandIsland::Build(const LandData& data)
{
	_blockIndexLookup = data.blockIndexLookup;

	const auto& lndBlocks = data.blocks;
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
	_heightMap->Create(indexSize.x * k_CellCount + 1, indexSize.y * k_CellCount + 1, 1, graphics::TextureFormat::RG8,
	                   Wrapping::ClampEdge, Filter::Nearest,
	                   bgfx::copy(heightMapData.data(), static_cast<uint32_t>(heightMapData.size())));

	_luminosityMap = std::make_unique<Texture2D>("Luminosity Map");
	const auto luminosityMapData = CreateLuminosityMap();
	_luminosityMap->Create(indexSize.x * k_CellCount + 1, indexSize.y * k_CellCount + 1, 1, graphics::TextureFormat::R8,
	                       Wrapping::ClampEdge, Filter::Nearest,
	                       bgfx::copy(luminosityMapData.data(), static_cast<uint32_t>(luminosityMapData.size())));

	_cellColourMap = std::make_unique<Texture2D>("Cell Colour Map");
	const auto cellColourMapData = CreateCellColourMap();
	_cellColourMap->Create(indexSize.x * k_CellCount + 1, indexSize.y * k_CellCount + 1, 1, graphics::TextureFormat::RGBA8,
	                       Wrapping::ClampEdge, Filter::Nearest,
	                       bgfx::copy(cellColourMapData.data(), static_cast<uint32_t>(cellColourMapData.size())));

	const auto res = indexSize * glm::u16vec2(lnd::LNDMaterial::k_Width, lnd::LNDMaterial::k_Height);
	_footprintFrameBuffer = std::make_unique<FrameBuffer>("Footprints", res.x, res.y, graphics::TextureFormat::RGBA8);
	_landAlphaFrameBuffer = std::make_unique<FrameBuffer>("LandAlpha", res.x, res.y, graphics::TextureFormat::R8);

	_proj = glm::ortho(_extentMin.x, _extentMax.x, _extentMin.y, _extentMax.y);
	_view = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "[LandIsland] loading {} countries", data.countries.size());
	_countries = data.countries;
	_materialTypes.clear();
	std::ranges::transform(data.materials, std::back_inserter(_materialTypes),
	                       [](const lnd::LNDMaterial& material) { return material.type; });

	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "[LandIsland] loading {} textures", data.materials.size());
	std::vector<uint16_t> rgba5TextureData;
	rgba5TextureData.resize(lnd::LNDMaterial::k_Width * lnd::LNDMaterial::k_Height * data.materials.size());
	for (size_t i = 0; i < data.materials.size(); i++)
	{
		std::memcpy(&rgba5TextureData[lnd::LNDMaterial::k_Width * lnd::LNDMaterial::k_Height * i],
		            data.materials[i].texels.data(), sizeof(data.materials[i].texels[0]) * data.materials[i].texels.size());
	}
	std::ranges::copy(data.noise, _noiseMap.begin());

	// Paint each block's texture from the countries, the materials, the noise and the bump map
	const block_texture::Sources sources {
	    .countries = _countries,
	    .materials = rgba5TextureData,
	    .noise = _noiseMap,
	    .bump = data.bump,
	};
	const auto* blockTexels = bgfx::alloc(static_cast<uint32_t>(_landBlocks.size() * block_texture::k_BlockBytes));
	const auto blockTexelSpan = std::span(blockTexels->data, blockTexels->size);
	for (size_t i = 0; i < _landBlocks.size(); ++i)
	{
		block_texture::BuildBlock(_landBlocks[i].GetLndBlock()->cells, sources,
		                          blockTexelSpan.subspan(i * block_texture::k_BlockBytes, block_texture::k_BlockBytes));
	}
	_blockTextures = std::make_unique<Texture2D>("LandIslandBlockTextures");
	_blockTextures->Create(block_texture::k_Side, block_texture::k_Side, static_cast<uint16_t>(_landBlocks.size()),
	                       TextureFormat::RGBA8, Wrapping::ClampEdge, Filter::Linear, blockTexels);

	// The blocks' vertices, one after another in one buffer
	const auto vertexCount = _landBlocks.size() * LandBlock::k_VertexCount;
	const auto* vertexMemory = bgfx::alloc(static_cast<uint32_t>(vertexCount * sizeof(LandVertex)));
	const auto vertices = std::span(reinterpret_cast<LandVertex*>(vertexMemory->data), vertexCount);
	for (size_t i = 0; i < _landBlocks.size(); ++i)
	{
		_landBlocks[i].BuildMesh(*this, vertices.subspan(i * LandBlock::k_VertexCount, LandBlock::k_VertexCount));
	}
	VertexDecl decl;
	decl.emplace_back(VertexAttrib::Attribute::Position, static_cast<uint8_t>(3), VertexAttrib::Type::Float);
	_blockVertices = std::make_unique<VertexBuffer>("LandBlocks", vertexMemory, decl);
	for (size_t i = 0; i < _landBlocks.size(); ++i)
	{
		_landBlocks[i].SetVertices(*_blockVertices, static_cast<uint32_t>(i * LandBlock::k_VertexCount));
	}
	bgfx::frame();
}

float LandIsland::GetHeightAt(glm::vec2 vec) const
{
	return static_cast<float>(GetAltitude(ToMapCoords(vec.x), ToMapCoords(vec.y)));
}

glm::vec3 LandIsland::GetNormalAt(glm::vec2 vec) const
{
	// The flat normal of the cell triangle under the point, straight up off the map
	const auto mapX = ToMapCoords(vec.x);
	const auto mapZ = ToMapCoords(vec.y);
	const auto cellX = static_cast<int16_t>(static_cast<uint32_t>(mapX) >> 16);
	const auto cellZ = static_cast<int16_t>(static_cast<uint32_t>(mapZ) >> 16);
	if (cellX < 0 || cellX >= k_MapSize || cellZ < 0 || cellZ >= k_MapSize)
	{
		return {0.0f, 1.0f, 0.0f};
	}
	const auto* cell = FindCell({static_cast<uint16_t>(cellX), static_cast<uint16_t>(cellZ)});
	if (cell == nullptr)
	{
		return {0.0f, 1.0f, 0.0f};
	}
	// Neighbours within the block's 17x17 cell array: +1 is z + 1, +17 is x + 1
	return land_normal::OfCell(static_cast<uint32_t>(mapX) & 0xFFFF, static_cast<uint32_t>(mapZ) & 0xFFFF,
	                           cell[0].properties.split != 0, cell[0].altitude, cell[1].altitude, cell[17].altitude,
	                           cell[18].altitude);
}

uint8_t LandIsland::GetNoise(glm::u8vec2 pos)
{
	return _noiseMap.at(pos.x * 256 + pos.y);
}

const LandBlock* LandIsland::GetBlock(const glm::u8vec2& coordinates) const
{
	// our blocks can only be between [0-31, 0-31]
	if (coordinates.x > 31 || coordinates.y > 31)
	{
		return nullptr;
	}

	const auto blockIndex = _blockIndexLookup.at(coordinates.x * 32 + coordinates.y);
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

	const auto blockIndex = _blockIndexLookup.at(lookupIndex);

	if (blockIndex == 0)
	{
		return nullptr;
	}
	assert(_landBlocks.size() >= blockIndex);
	return &_landBlocks[blockIndex - 1].GetCells()[cellIndex];
}

void LandIsland::DumpTextures() const
{
	_blockTextures->DumpTexture();
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
	// Two bytes a corner: its altitude, and whether its cell is split the other way
	data.resize(static_cast<size_t>(resolution.x) * resolution.y * 2, 0);

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
				const auto texel = static_cast<size_t>((cellPos.y * resolution.x) + cellPos.x) * 2;
				if (texel + 1 < data.size())
				{
					data.at(texel) = cell.altitude;
					data.at(texel + 1) = cell.properties.split != 0 ? 255 : 0;
				}
			}
		}
	}
	return data;
}

std::vector<uint8_t> LandIsland::CreateLuminosityMap() const
{
	// As the height map: a texel for each cell's corner, with the far edge's from the next block's cells
	const auto extentSize = _extentIndexMax - _extentIndexMin + glm::u16vec2(1, 1);
	const auto resolution = extentSize * static_cast<uint16_t>(k_CellCount) + static_cast<uint16_t>(1);
	std::vector<uint8_t> data(static_cast<size_t>(resolution.x) * resolution.y, 0xFF);
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
				data.at(static_cast<size_t>(cellPos.y * resolution.x + cellPos.x)) = GetCell(blockOffset + offset).luminosity;
			}
		}
	}
	return data;
}

std::vector<uint8_t> LandIsland::CreateCellColourMap() const
{
	// As the luminosity map, four bytes a texel
	const auto extentSize = _extentIndexMax - _extentIndexMin + glm::u16vec2(1, 1);
	const auto resolution = extentSize * static_cast<uint16_t>(k_CellCount) + static_cast<uint16_t>(1);
	std::vector<uint8_t> data(static_cast<size_t>(resolution.x) * resolution.y * 4, 0);
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
				const auto at = static_cast<size_t>(cellPos.y * resolution.x + cellPos.x) * 4;
				// Red and blue swapped, as the game reads them
				data.at(at) = cell.b;
				data.at(at + 1) = cell.g;
				data.at(at + 2) = cell.r;
				data.at(at + 3) = 0xFF;
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

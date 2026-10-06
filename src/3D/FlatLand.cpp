/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FlatLand.h"

#include <algorithm>

using namespace openblack;

namespace
{
constexpr int k_CellsPerBlock = 16;
constexpr float k_BlockSize = 160.0f;
constexpr int k_TexelsPerCell = lnd::LNDMaterial::k_Width / k_CellsPerBlock;

lnd::LNDCell FlatCell()
{
	lnd::LNDCell cell {};
	cell.luminosity = flat_land::k_Luminosity;
	cell.altitude = flat_land::k_Altitude;
	cell.flags = flat_land::k_SoundFlags;
	return cell;
}

/// The pool's cells in a block: down at the sea and, inside its edges, open sea, where no land is drawn
void LetInPool(lnd::LNDBlock& block, int blockX, int blockZ)
{
	constexpr uint8_t k_OpenSea = 0x02;
	constexpr int k_CellsPerSide = k_CellsPerBlock + 1;
	for (int x = 0; x < k_CellsPerSide; ++x)
	{
		for (int z = 0; z < k_CellsPerSide; ++z)
		{
			const auto mapX = (blockX * k_CellsPerBlock) + x;
			const auto mapZ = (blockZ * k_CellsPerBlock) + z;
			if (mapX < flat_land::k_PoolMinX || mapX > flat_land::k_PoolMaxX || mapZ < flat_land::k_PoolMinZ ||
			    mapZ > flat_land::k_PoolMaxZ)
			{
				continue;
			}
			auto& cell = block.cells.at(static_cast<size_t>((x * k_CellsPerSide) + z));
			cell.altitude = 0;
			if (mapX < flat_land::k_PoolMaxX && mapZ < flat_land::k_PoolMaxZ)
			{
				cell.flags = static_cast<uint8_t>(cell.flags | k_OpenSea);
			}
		}
	}
}
} // namespace

flat_land::Colour flat_land::MaterialColour(int x, int z)
{
	if (x == 0 || z == 0)
	{
		return k_BlockEdge;
	}
	const auto odd = ((x / k_TexelsPerCell) + (z / k_TexelsPerCell)) % 2 != 0;
	return odd ? k_DarkSquare : k_LightSquare;
}

LandData flat_land::Build(bool pool)
{
	LandData data;

	// Every block of the map, in order along z then x
	auto cell = FlatCell();
	if (pool)
	{
		cell.altitude = k_LowAltitude;
	}
	data.blocks.reserve(data.blockIndexLookup.size());
	for (int x = 0; x < LandData::k_BlocksPerSide; ++x)
	{
		for (int z = 0; z < LandData::k_BlocksPerSide; ++z)
		{
			auto& block = data.blocks.emplace_back();
			std::ranges::fill(block.cells, cell);
			block.index = static_cast<uint32_t>(data.blocks.size());
			block.mapX = static_cast<float>(x) * k_BlockSize;
			block.mapZ = static_cast<float>(z) * k_BlockSize;
			block.blockX = static_cast<uint32_t>(x);
			block.blockZ = static_cast<uint32_t>(z);
			if (pool)
			{
				LetInPool(block, x, z);
			}
			data.blockIndexLookup.at(static_cast<size_t>(x * LandData::k_BlocksPerSide + z)) =
			    static_cast<uint16_t>(block.index);
		}
	}

	// One country, the one material at every height
	auto& country = data.countries.emplace_back();
	std::ranges::fill(country.materials, lnd::LNDMapMaterial {.indices = {0, 0}, .coefficient = 0});

	auto& material = data.materials.emplace_back();
	// Grass, so that what walks on it sounds as on land
	material.type = k_GrassMaterial;
	for (int x = 0; x < lnd::LNDMaterial::k_Width; ++x)
	{
		for (int z = 0; z < lnd::LNDMaterial::k_Height; ++z)
		{
			const auto colour = MaterialColour(x, z);
			auto& texel = material.texels.at(static_cast<size_t>(x * lnd::LNDMaterial::k_Height + z));
			texel.b = colour.b;
			texel.g = colour.g;
			texel.r = colour.r;
			texel.a = 1;
		}
	}

	data.noise.assign(static_cast<size_t>(lnd::LNDBumpMap::k_Width) * lnd::LNDBumpMap::k_Height, 0);
	data.bump.assign(static_cast<size_t>(lnd::LNDBumpMap::k_Width) * lnd::LNDBumpMap::k_Height, k_FlatBump);
	return data;
}

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

/// A block holds the corners along both its edges, 17 a side
constexpr int k_BlockCornersPerSide = k_CellsPerBlock + 1;

/// How many cells out from the lake's open water the map's corner (x, z) lies, 0 on or inside its edge
int CellsFromOpenWater(int x, int z)
{
	const auto outside = [](int v, int min, int max) { return std::max({min - v, v - max, 0}); };
	return std::max(outside(x, flat_land::k_LakeMinX, flat_land::k_LakeMaxX),
	                outside(z, flat_land::k_LakeMinZ, flat_land::k_LakeMaxZ));
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

uint8_t flat_land::Altitude(int x, int z)
{
	const auto out = CellsFromOpenWater(x, z);
	return out < static_cast<int>(k_ShoreAltitudes.size()) ? k_ShoreAltitudes.at(static_cast<size_t>(out)) : k_Altitude;
}

flat_land::CellKind flat_land::KindOf(int x, int z)
{
	const std::array corners {Altitude(x, z), Altitude(x + 1, z), Altitude(x, z + 1), Altitude(x + 1, z + 1)};
	const auto [low, high] = std::ranges::minmax(corners);
	if (high == 0)
	{
		return CellKind::OpenWater;
	}
	if (high <= k_SeaLevelAltitude)
	{
		return CellKind::Shallows;
	}
	return low <= k_SeaLevelAltitude ? CellKind::Shore : CellKind::Land;
}

lnd::LNDCell flat_land::CellAt(int x, int z)
{
	lnd::LNDCell cell {};
	cell.luminosity = k_Luminosity;
	cell.altitude = Altitude(x, z);
	switch (KindOf(x, z))
	{
	case CellKind::OpenWater:
		cell.properties.hasWater = 1;
		cell.flags = k_FreshWaterSoundFlags | k_OpenWaterFlag;
		break;
	case CellKind::Shallows:
		cell.properties.hasWater = 1;
		cell.flags = k_FreshWaterSoundFlags;
		break;
	case CellKind::Shore:
		cell.flags = k_CoastalSoundFlags;
		break;
	case CellKind::Land:
		cell.flags = k_SoundFlags;
		break;
	}
	for (const auto& patch : {k_SandPatch, k_SnowPatch})
	{
		if (patch.Contains(x, z))
		{
			cell.properties.country = patch.country;
		}
	}
	return cell;
}

LandData flat_land::Build()
{
	LandData data;

	// Every block of the map, in order along z then x; a block's cells run [x * 17 + z], its last row and column the
	// first of the next block's
	data.blocks.reserve(data.blockIndexLookup.size());
	for (int x = 0; x < LandData::k_BlocksPerSide; ++x)
	{
		for (int z = 0; z < LandData::k_BlocksPerSide; ++z)
		{
			auto& block = data.blocks.emplace_back();
			for (int cx = 0; cx < k_BlockCornersPerSide; ++cx)
			{
				for (int cz = 0; cz < k_BlockCornersPerSide; ++cz)
				{
					const auto mapX = (x * k_CellsPerBlock) + cx;
					const auto mapZ = (z * k_CellsPerBlock) + cz;
					block.cells.at(static_cast<size_t>((cx * k_BlockCornersPerSide) + cz)) = CellAt(mapX, mapZ);
				}
			}
			block.index = static_cast<uint32_t>(data.blocks.size());
			block.mapX = static_cast<float>(x) * k_BlockSize;
			block.mapZ = static_cast<float>(z) * k_BlockSize;
			block.blockX = static_cast<uint32_t>(x);
			block.blockZ = static_cast<uint32_t>(z);
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

	// The patches' countries, each its own material at every height, painted plain
	for (const auto& patch : {k_SandPatch, k_SnowPatch})
	{
		auto& patchCountry = data.countries.emplace_back();
		const auto index = static_cast<uint8_t>(data.materials.size());
		std::ranges::fill(patchCountry.materials, lnd::LNDMapMaterial {.indices = {index, index}, .coefficient = 0});
		auto& patchMaterial = data.materials.emplace_back();
		patchMaterial.type = patch.materialType;
		for (auto& texel : patchMaterial.texels)
		{
			texel.b = patch.colour.b;
			texel.g = patch.colour.g;
			texel.r = patch.colour.r;
			texel.a = 1;
		}
	}

	data.noise.assign(static_cast<size_t>(lnd::LNDBumpMap::k_Width) * lnd::LNDBumpMap::k_Height, 0);
	data.bump.assign(static_cast<size_t>(lnd::LNDBumpMap::k_Width) * lnd::LNDBumpMap::k_Height, k_FlatBump);
	return data;
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <algorithm>

#include <gtest/gtest.h>

#include "3D/FlatLand.h"

using namespace openblack;

namespace
{
bool SameColour(flat_land::Colour a, flat_land::Colour b)
{
	return a.r == b.r && a.g == b.g && a.b == b.b;
}
} // namespace

TEST(FlatLand, CoversEveryBlockOfTheMap)
{
	const auto land = flat_land::Build();
	ASSERT_EQ(land.blocks.size(), 32u * 32u);
	for (int x = 0; x < 32; ++x)
	{
		for (int z = 0; z < 32; ++z)
		{
			const auto index = land.blockIndexLookup.at(static_cast<size_t>(x * 32 + z));
			ASSERT_GT(index, 0);
			const auto& block = land.blocks.at(index - 1u);
			EXPECT_EQ(block.index, index);
			EXPECT_EQ(block.blockX, static_cast<uint32_t>(x));
			EXPECT_EQ(block.blockZ, static_cast<uint32_t>(z));
			EXPECT_FLOAT_EQ(block.mapX, static_cast<float>(x) * 160.0f);
			EXPECT_FLOAT_EQ(block.mapZ, static_cast<float>(z) * 160.0f);
		}
	}
}

TEST(FlatLand, EveryCellIsFlatDryLand)
{
	const auto land = flat_land::Build();
	for (const auto& block : land.blocks)
	{
		EXPECT_TRUE(std::ranges::all_of(block.cells, [](const lnd::LNDCell& cell) {
			return cell.altitude == flat_land::k_Altitude && cell.properties.country == 0 && cell.properties.hasWater == 0 &&
			       cell.properties.fullWater == 0 && cell.properties.split == 0 && cell.flags == flat_land::k_SoundFlags;
		}));
	}
}

TEST(FlatLand, PaintsOneMaterialAtEveryHeight)
{
	const auto land = flat_land::Build();
	ASSERT_EQ(land.countries.size(), 1u);
	ASSERT_EQ(land.materials.size(), 1u);
	EXPECT_TRUE(std::ranges::all_of(land.countries.front().materials, [](const lnd::LNDMapMaterial& material) {
		return material.indices[0] == 0 && material.indices[1] == 0;
	}));
	// No noise to shift the height, and a bump that leaves the colours be
	EXPECT_TRUE(std::ranges::all_of(land.noise, [](uint8_t n) { return n == 0; }));
	EXPECT_TRUE(std::ranges::all_of(land.bump, [](uint8_t b) { return b == flat_land::k_FlatBump; }));
	EXPECT_EQ(land.noise.size(), 256u * 256u);
	EXPECT_EQ(land.bump.size(), 256u * 256u);
}

TEST(FlatLand, MaterialIsACheckerboardOfCellsEdgedAtTheBlock)
{
	// 16 texels a cell
	EXPECT_TRUE(SameColour(flat_land::MaterialColour(0, 40), flat_land::k_BlockEdge));
	EXPECT_TRUE(SameColour(flat_land::MaterialColour(40, 0), flat_land::k_BlockEdge));
	EXPECT_TRUE(SameColour(flat_land::MaterialColour(1, 1), flat_land::k_LightSquare));
	EXPECT_TRUE(SameColour(flat_land::MaterialColour(15, 15), flat_land::k_LightSquare));
	EXPECT_TRUE(SameColour(flat_land::MaterialColour(16, 1), flat_land::k_DarkSquare));
	EXPECT_TRUE(SameColour(flat_land::MaterialColour(1, 16), flat_land::k_DarkSquare));
	EXPECT_TRUE(SameColour(flat_land::MaterialColour(16, 16), flat_land::k_LightSquare));

	const auto land = flat_land::Build();
	const auto& texel = land.materials.front().texels.at(16 * 256 + 1);
	EXPECT_EQ(texel.r, flat_land::k_DarkSquare.r);
	EXPECT_EQ(texel.g, flat_land::k_DarkSquare.g);
	EXPECT_EQ(texel.b, flat_land::k_DarkSquare.b);
}

TEST(LandData, CopiesALandscapeFilesBlocksAndMaps)
{
	lnd::LNDFile lnd;
	lnd::LNDBlock block {};
	block.blockX = 3;
	block.blockZ = 5;
	lnd.AddBlock(block);
	const auto land = LandData::FromLnd(lnd);
	ASSERT_EQ(land.blocks.size(), 1u);
	EXPECT_EQ(land.blocks.front().blockX, 3u);
	EXPECT_EQ(land.noise.size(), 256u * 256u);
	EXPECT_EQ(land.bump.size(), 256u * 256u);
}

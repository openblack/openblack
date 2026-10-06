/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <algorithm>
#include <array>
#include <optional>

#include <gtest/gtest.h>

#include "3D/FlatLand.h"
#include "Creature/CreatureRoute.h"

using namespace openblack;

namespace
{
/// Within the lake or its shore, with a cell to spare
bool NearLake(int x, int z)
{
	const auto reach = flat_land::k_ShoreCells + 1;
	return x >= flat_land::k_LakeMinX - reach && x <= flat_land::k_LakeMaxX + reach && z >= flat_land::k_LakeMinZ - reach &&
	       z <= flat_land::k_LakeMaxZ + reach;
}

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

TEST(FlatLand, AwayFromTheLakeEveryCellIsFlatDryLand)
{
	const auto land = flat_land::Build();
	for (const auto& block : land.blocks)
	{
		for (int x = 0; x < 17; ++x)
		{
			for (int z = 0; z < 17; ++z)
			{
				const auto mapX = static_cast<int>(block.blockX) * 16 + x;
				const auto mapZ = static_cast<int>(block.blockZ) * 16 + z;
				if (NearLake(mapX, mapZ))
				{
					continue;
				}
				const auto& cell = block.cells.at(static_cast<size_t>(x * 17 + z));
				ASSERT_TRUE(cell.altitude == flat_land::k_Altitude && cell.properties.country == 0 &&
				            cell.properties.hasWater == 0 && cell.properties.fullWater == 0 && cell.properties.split == 0 &&
				            cell.flags == flat_land::k_SoundFlags)
				    << mapX << ", " << mapZ;
			}
		}
	}
}

TEST(FlatLand, TheLakeIsAHundredUnitsOfOpenWaterNorthOfTheMiddle)
{
	EXPECT_EQ(flat_land::k_LakeMaxX - flat_land::k_LakeMinX, 10);
	EXPECT_EQ(flat_land::k_LakeMaxZ - flat_land::k_LakeMinZ, 10);
	EXPECT_FLOAT_EQ(flat_land::k_LakeHalfExtent.x * 2.0f, 100.0f);
	EXPECT_FLOAT_EQ(flat_land::k_LakeHalfExtent.y * 2.0f, 100.0f);
	EXPECT_FLOAT_EQ(flat_land::k_MapMiddle.x, 2560.0f);
	EXPECT_FLOAT_EQ(flat_land::k_LakeCentre.x, 2560.0f);
	// Beyond the middle, its bank clear of the scenarios and spawn area within 120 units of the middle
	const auto bankStart = (static_cast<float>(flat_land::k_LakeMinZ - flat_land::k_ShoreCells) * flat_land::k_CellSize) -
	                       flat_land::k_MapMiddle.y;
	EXPECT_GE(bankStart, 120.0f);

	int open = 0;
	for (int x = 0; x < flat_land::k_CellsPerSide; ++x)
	{
		for (int z = 0; z < flat_land::k_CellsPerSide; ++z)
		{
			const bool inLake = x >= flat_land::k_LakeMinX && x < flat_land::k_LakeMaxX && z >= flat_land::k_LakeMinZ &&
			                    z < flat_land::k_LakeMaxZ;
			const bool isOpen = flat_land::KindOf(x, z) == flat_land::CellKind::OpenWater;
			ASSERT_EQ(isOpen, inLake) << x << ", " << z;
			open += isOpen ? 1 : 0;
		}
	}
	EXPECT_EQ(open, 100);
}

TEST(FlatLand, OpenWaterIsAtTheBottomShowingTheSea)
{
	const auto cell = flat_land::CellAt(flat_land::k_LakeMinX + 4, flat_land::k_LakeMinZ + 4);
	EXPECT_EQ(cell.altitude, 0);
	EXPECT_EQ(cell.properties.hasWater, 1);
	EXPECT_NE(cell.flags & flat_land::k_OpenWaterFlag, 0);
	EXPECT_EQ(cell.flags >> 2, 2) << "still fresh water";
	// The corners on the open water's edge are at the bottom too
	EXPECT_EQ(flat_land::Altitude(flat_land::k_LakeMaxX, flat_land::k_LakeMaxZ), 0);
}

TEST(FlatLand, ShallowsRingTheOpenWaterAtSeaLevel)
{
	// Two cells out on every side, corners included, the cells are wadeable water at sea level, not open
	for (int out = 1; out <= 2; ++out)
	{
		for (const auto [x, z] :
		     std::array<std::array<int, 2>, 4> {{{flat_land::k_LakeMinX - out, flat_land::k_LakeMinZ + 5},
		                                         {flat_land::k_LakeMaxX - 1 + out, flat_land::k_LakeMinZ + 5},
		                                         {flat_land::k_LakeMinX + 5, flat_land::k_LakeMaxZ - 1 + out},
		                                         {flat_land::k_LakeMinX - out, flat_land::k_LakeMinZ - out}}})
		{
			ASSERT_EQ(flat_land::KindOf(x, z), flat_land::CellKind::Shallows) << x << ", " << z;
			const auto cell = flat_land::CellAt(x, z);
			EXPECT_EQ(cell.properties.hasWater, 1);
			EXPECT_EQ(cell.flags & flat_land::k_OpenWaterFlag, 0);
			EXPECT_EQ(cell.flags >> 2, 2);
			EXPECT_LE(cell.altitude, flat_land::k_SeaLevelAltitude);
		}
	}
	// Then the bank: dry, coastal, then the plain
	const auto shore = flat_land::CellAt(flat_land::k_LakeMinX - 3, flat_land::k_LakeMinZ + 5);
	EXPECT_EQ(flat_land::KindOf(flat_land::k_LakeMinX - 3, flat_land::k_LakeMinZ + 5), flat_land::CellKind::Shore);
	EXPECT_EQ(shore.properties.hasWater, 0);
	EXPECT_EQ(shore.flags, flat_land::k_CoastalSoundFlags);
	EXPECT_EQ(flat_land::KindOf(flat_land::k_LakeMinX - 4, flat_land::k_LakeMinZ + 5), flat_land::CellKind::Land);
	EXPECT_EQ(flat_land::Altitude(flat_land::k_LakeMinX - flat_land::k_ShoreCells, flat_land::k_LakeMinZ),
	          flat_land::k_Altitude);
}

TEST(FlatLand, TheBankIsNoSteeperThanACreatureCanWalk)
{
	// Rising no more than 10 units across a cell, at 0.67 units an altitude step
	for (size_t i = 1; i < flat_land::k_ShoreAltitudes.size(); ++i)
	{
		const auto rise = flat_land::k_ShoreAltitudes.at(i) - flat_land::k_ShoreAltitudes.at(i - 1);
		EXPECT_GE(rise, 0);
		EXPECT_LE(static_cast<float>(rise) * 0.67f, 10.0f);
	}
	EXPECT_EQ(flat_land::k_ShoreAltitudes.back(), flat_land::k_Altitude);
}

TEST(FlatLand, RoutesWadeTheShallowsButNotTheOpenWater)
{
	const auto land = creature_route::WalkableLand::Build(
	    [](int32_t x, int32_t z) { return static_cast<float>(flat_land::Altitude(x, z)) * 0.67f; },
	    [](int32_t x, int32_t z) -> std::optional<bool> { return flat_land::CellAt(x, z).properties.hasWater != 0; });
	using creature_route::Ground;
	EXPECT_EQ(land.At(flat_land::k_LakeMinX + 5, flat_land::k_LakeMinZ + 5), Ground::Blocked);
	EXPECT_EQ(land.At(flat_land::k_LakeMinX - 1, flat_land::k_LakeMinZ + 5), Ground::Water);
	EXPECT_EQ(land.At(flat_land::k_LakeMinX - 2, flat_land::k_LakeMinZ + 5), Ground::Water);
	EXPECT_EQ(land.At(flat_land::k_LakeMinX - 3, flat_land::k_LakeMinZ + 5), Ground::Open);
	EXPECT_EQ(land.At(256, 256), Ground::Open);
	// The middle of the near shallows is somewhere to stand
	const glm::vec2 shallows {flat_land::k_LakeCentre.x, flat_land::k_LakeCentre.y - flat_land::k_LakeHalfExtent.y - 10.0f};
	EXPECT_TRUE(land.IsValid(shallows, creature_route::k_DestinationClearance));
}

TEST(FlatLand, BlocksCarryTheLakeAcrossTheirEdges)
{
	const auto land = flat_land::Build();
	for (const auto& block : land.blocks)
	{
		for (int x = 0; x < 17; ++x)
		{
			for (int z = 0; z < 17; ++z)
			{
				const auto mapX = static_cast<int>(block.blockX) * 16 + x;
				const auto mapZ = static_cast<int>(block.blockZ) * 16 + z;
				if (!NearLake(mapX, mapZ))
				{
					continue;
				}
				const auto& cell = block.cells.at(static_cast<size_t>(x * 17 + z));
				const auto expected = flat_land::CellAt(mapX, mapZ);
				ASSERT_EQ(cell.altitude, expected.altitude) << mapX << ", " << mapZ;
				ASSERT_EQ(cell.flags, expected.flags) << mapX << ", " << mapZ;
				ASSERT_EQ(cell.properties.hasWater, expected.properties.hasWater) << mapX << ", " << mapZ;
			}
		}
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

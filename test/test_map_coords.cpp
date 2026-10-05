/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <vector>

#include <3D/MapCoords.h>
#include <gtest/gtest.h>

namespace map_coords = openblack::map_coords;
using map_coords::JustMapXZ;
using map_coords::MapCoords;

TEST(MapCoords, MetresBecomeFixedWithAFloatProduct)
{
	EXPECT_EQ(map_coords::ToFixed(5.0f), 0x8000);
	EXPECT_EQ(map_coords::ToFixed(-5.0f), -0x8000);
	EXPECT_EQ(map_coords::ToFixed(123.456f), 809081);
	// The product rounds to a float before it is truncated, as with the game's FPU at 24 bits; in double it is 17173883
	EXPECT_EQ(map_coords::ToFixed(2620.5266f), 17173884);
}

TEST(MapCoords, ARoundTripCanLoseAUnit)
{
	EXPECT_EQ(map_coords::ToMetres(0x10000), 10.0f);
	EXPECT_EQ(map_coords::ToFixed(map_coords::ToMetres(8090858)), 8090857);
}

TEST(MapCoords, CellsAreTheHighWords)
{
	const MapCoords coords {0x00050000 + 0x1234, 0x01FF0000, 0.0f};
	EXPECT_EQ(map_coords::Cell(coords), glm::ivec2(5, 511));
	EXPECT_EQ(map_coords::CellIndex(coords), 5 * 512 + 511);
	EXPECT_EQ(map_coords::CellIndex(coords, 256), -1);
	// a negative position is cell 0xFFFF, off the map
	EXPECT_FALSE(map_coords::InBounds(MapCoords {-1, 0, 0.0f}));
	EXPECT_EQ(map_coords::SignedCellOf(-1), -1);
}

TEST(MapCoords, AddingCellsKeepsTheFraction)
{
	MapCoords coords {0x00050123, 0x00000456, 0.0f};
	map_coords::AddCells(coords, {-1, 2});
	EXPECT_EQ(coords.x, 0x00040123);
	EXPECT_EQ(coords.z, 0x00020456);
}

TEST(MapCoords, TheSpiralGrowsASquare)
{
	map_coords::Spiral spiral;
	std::vector<JustMapXZ> steps;
	for (int i = 0; i < 9; ++i)
	{
		steps.push_back(spiral.Next());
	}
	const std::vector<JustMapXZ> expected {{-1, 0}, {0, -1}, {1, 0}, {1, 0}, {0, 1}, {0, 1}, {-1, 0}, {-1, 0}, {-1, 0}};
	EXPECT_EQ(steps, expected);
}

TEST(MapCoords, SpiralSizes)
{
	EXPECT_EQ(map_coords::CellSpiralSize(0.0f), 1);
	EXPECT_EQ(map_coords::CellSpiralSize(15.0f), 9);
	EXPECT_EQ(map_coords::IncrementSpiralSize(10.0f, 5.0f), 25);
}

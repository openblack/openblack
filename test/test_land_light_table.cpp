/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdint>

#include <vector>

#include <gtest/gtest.h>

#include "3D/LandLightTable.h"

using namespace openblack;

namespace
{
/// A palette whose land rows are all one colour through the day, whatever the alignment
LandLightPalette FakePalette(uint8_t r, uint8_t g, uint8_t b)
{
	std::vector<uint8_t> bytes(LandLightPalette::k_Side * LandLightPalette::k_Side * 4, 0);
	for (size_t row = 0; row < 3; ++row)
	{
		for (size_t column = 0; column < LandLightPalette::k_Side; ++column)
		{
			const auto at = (row * LandLightPalette::k_Side + column) * 4;
			bytes[at] = r;
			bytes[at + 1] = g;
			bytes[at + 2] = b;
			bytes[at + 3] = 0xFF;
		}
	}
	return LandLightPalette(bytes);
}
} // namespace

TEST(LandLightTable, OvercastDarkensTheLand)
{
	const auto palette = FakePalette(200, 220, 240);
	EXPECT_EQ(LandLightTable::GetLandColour(palette, 2.0f, 1.0f, 0.0f), 0xC8DCF0u);
	// No channel brighter than 255 - 96 times the overcast
	EXPECT_EQ(LandLightTable::GetLandColour(palette, 2.0f, 1.0f, 0.5f), 0xC8CFCFu);
	EXPECT_EQ(LandLightTable::GetLandColour(palette, 2.0f, 1.0f, 1.0f), 0x9F9F9Fu);

	LandLightTable table;
	table.Build(palette, 2.0f, 1.0f, 0.5f);
	EXPECT_EQ(table.GetLandColour(), 0xC8CFCFu);
}

TEST(LandLightTable, OvercastClosesTheHazeIn)
{
	const auto palette = FakePalette(200, 220, 240);
	LandLightTable table;
	table.Build(palette, 2.0f, 1.0f, 0.0f);
	EXPECT_EQ(table.GetHaze().colour, glm::vec3(66.0f, 73.0f, 80.0f));
	EXPECT_FLOAT_EQ(table.GetHaze().k, 233.0f);
	EXPECT_NEAR(table.GetHaze().nearDistance, 400.0f, 0.01f);
	EXPECT_NEAR(table.GetHaze().farDistance, 900.0f, 0.01f);

	// Halfway to a storm's haze, from the darkened land's colour
	table.Build(palette, 2.0f, 1.0f, 0.5f);
	EXPECT_FLOAT_EQ(table.GetHaze().colour.r, 61.5f);
	EXPECT_FLOAT_EQ(table.GetHaze().k, 131.0f);

	// A full overcast, or more, is a storm's, though more darkens the land further: 139, an eighth and 32
	table.Build(palette, 2.0f, 1.0f, 1.2f);
	EXPECT_EQ(table.GetHaze().colour, glm::vec3(49.0f));
	EXPECT_FLOAT_EQ(table.GetHaze().k, 48.0f);
	EXPECT_NEAR(table.GetHaze().nearDistance, 15.0f, 0.01f);
	EXPECT_NEAR(table.GetHaze().farDistance, 350.0f, 0.01f);
}

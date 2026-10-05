/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Graphics/HandWaterGlow.h"

namespace hand_water_glow = openblack::graphics::hand_water_glow;

TEST(HandWaterGlow, WarmsThePaletteColourTowardsOrange)
{
	// A quarter of the way from black to (255, 128, 64), rounding down; full strength is 190 of alpha
	EXPECT_EQ(hand_water_glow::Colour(0x000000, 1.0f), 0xBE3F2010u);
	EXPECT_EQ(hand_water_glow::Colour(0xFFFFFF, 0.5f), 0x5FFFDFCFu);
	EXPECT_EQ(hand_water_glow::Colour(0x123456, 0.0f), 0x004D4750u);
}

TEST(HandWaterGlow, ShowsOnlyNearWater)
{
	// High land everywhere
	const auto high = [](int /*x*/, int /*z*/) { return std::optional<uint8_t>(40); };
	EXPECT_FALSE(hand_water_glow::NearLowLand({2560.0f, 2560.0f}, high));

	// One low cell within reach, 70 units from the hand
	const auto lowCell = [](int x, int z) { return std::optional<uint8_t>(x == 263 && z == 256 ? 4 : 40); };
	EXPECT_TRUE(hand_water_glow::NearLowLand({2560.0f, 2560.0f}, lowCell));
	EXPECT_FALSE(hand_water_glow::NearLowLand({2400.0f, 2560.0f}, lowCell));

	// Where there is no land at all
	const auto noLand = [](int x, int /*z*/) { return x < 300 ? std::optional<uint8_t>(40) : std::nullopt; };
	EXPECT_TRUE(hand_water_glow::NearLowLand({2950.0f, 2560.0f}, noLand));
}

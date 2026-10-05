/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <vector>

#include <gtest/gtest.h>

#include "3D/LandLightTable.h"
#include "3D/Lightning.h"

using namespace openblack;

TEST(Lightning, FlashFlickersAsItFades)
{
	auto flash = lightning::Strike(glm::vec3(0.0f), 500.0f, lightning::k_ThunderStrength);
	EXPECT_FLOAT_EQ(lightning::Brightness(flash), 1.0f);
	// A game turn at a time, as the storms update
	std::vector<float> brightnesses;
	for (int i = 0; i < 9; ++i)
	{
		flash = lightning::Advance(flash, 0.1f);
		brightnesses.push_back(lightning::Brightness(flash));
	}
	EXPECT_NEAR(brightnesses[0], 0.9f, 1e-6f);
	// Down to a tenth for a while, then back up and fading out
	EXPECT_NEAR(brightnesses[1], 0.1f, 1e-6f);
	EXPECT_NEAR(brightnesses[3], 0.1f, 1e-6f);
	EXPECT_NEAR(brightnesses[4], 0.5f, 1e-6f);
	EXPECT_NEAR(brightnesses[6], 0.3f, 1e-6f);
	// Out once older than 0.8 s
	EXPECT_FALSE(flash.active);
	EXPECT_FLOAT_EQ(brightnesses[8], 0.0f);
}

TEST(Lightning, BoltsFlashAtHalfStrength)
{
	const auto flash = lightning::Strike(glm::vec3(0.0f), 500.0f, lightning::k_BoltStrength);
	EXPECT_FLOAT_EQ(lightning::Brightness(flash), 0.5f);
	EXPECT_EQ(lightning::LandLightFlash(lightning::Brightness(flash)), 127);
	EXPECT_EQ(lightning::LandLightFlash(1.0f), 255);
	EXPECT_EQ(lightning::LandLightFlash(-1.0f), 0);
}

TEST(Lightning, FlashLightsTheLandWhite)
{
	std::vector<uint8_t> bytes(LandLightPalette::k_Side * LandLightPalette::k_Side * 4, 0);
	const LandLightPalette palette(bytes);
	LandLightTable table;
	table.Build(palette, 2.0f, 1.0f, 0.0f, 0);
	const auto dark = table.GetTexels().back();
	table.Build(palette, 2.0f, 1.0f, 0.0f, 128);
	// Half way to white from black, in whole steps
	EXPECT_EQ(dark & 0xFFFFFFu, 0u);
	EXPECT_EQ(table.GetTexels().back() & 0xFFFFFFu, 0x7F7F7Fu);
	// The haze of black land, a third of nothing plus 8 for k, half way to white
	EXPECT_FLOAT_EQ(table.GetHaze().colour.r, 127.5f);
	EXPECT_FLOAT_EQ(table.GetHaze().k, 8.0f + 123.0f);
}

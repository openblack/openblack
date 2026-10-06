/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "3D/LandColourStamps.h"
#include "3D/Lightning.h"

using namespace openblack;

TEST(LandColourStamps, PlacedWithRoundedWeights)
{
	// Halfway into cell 12: 127.5 rounds to the even 128
	const auto placement = land_colour_stamps::Place({125.0f, 0.0f});
	EXPECT_EQ(placement.cell, glm::ivec2(12, 0));
	EXPECT_EQ(placement.weight, glm::ivec2(128, 255));
	// A little before cell 0 along x, in cell -1 most of the way to 0: 76.5 rounds to the even 76
	const auto before = land_colour_stamps::Place({-3.0f, 104.0f});
	EXPECT_EQ(before.cell, glm::ivec2(-1, 10));
	EXPECT_EQ(before.weight, glm::ivec2(76, 153));
}

TEST(LandColourStamps, CentredOnAPoint)
{
	EXPECT_EQ(land_colour_stamps::CentredCorner(glm::vec3(1000.0f, 50.0f, 2000.0f), 64), glm::vec2(685.0f, 1685.0f));
}

TEST(LandColourStamps, StrengthIsAByte)
{
	EXPECT_EQ(land_colour_stamps::Strength(1.0f), 255);
	EXPECT_EQ(land_colour_stamps::Strength(0.5f), 127);
	EXPECT_EQ(land_colour_stamps::Strength(2.0f), 255);
	EXPECT_EQ(land_colour_stamps::Strength(-1.0f), 0);
}

TEST(LandColourStamps, LightningGlowsGreyFromItsCentre)
{
	const auto image = land_colour_stamps::LightningImage();
	ASSERT_EQ(image.size(), 64u * 64u * 3u);
	const auto at = [&image](int row, int column) { return image.at((static_cast<size_t>(row) * 64 + column) * 3); };
	// At the centre, 32 * 9 is past white
	EXPECT_EQ(at(32, 32), 255);
	// 30 texels out, (32 - 30) * 9
	EXPECT_EQ(at(32, 2), 18);
	// None in the corners, past 32 texels
	EXPECT_EQ(at(0, 0), 0);
	EXPECT_EQ(image.at(((32 * 64) + 2) * 3 + 1), 18);
}

TEST(LandColourStamps, LightningGlowFadesCubed)
{
	auto flash = lightning::Strike(glm::vec3(0.0f), 500.0f, lightning::k_ThunderStrength);
	EXPECT_FLOAT_EQ(lightning::GlowStrength(flash), 1.0f);
	flash = lightning::Advance(flash, 0.1f);
	EXPECT_NEAR(lightning::GlowStrength(flash), 0.9f * 0.9f * 0.9f, 1e-6f);
	flash = lightning::Advance(flash, 0.1f);
	EXPECT_NEAR(lightning::GlowStrength(flash), 0.1f, 1e-6f);
	const auto bolt = lightning::Strike(glm::vec3(0.0f), 500.0f, lightning::k_BoltStrength);
	EXPECT_FLOAT_EQ(lightning::GlowStrength(bolt), 0.5f);
}

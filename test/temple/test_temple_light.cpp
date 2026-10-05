/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "3D/TempleLight.h"

using namespace openblack;

TEST(TempleLight, NeutralIsAsItIs)
{
	const auto light = TempleLight::At(0.0f, 12345);
	EXPECT_EQ(light.multiply, glm::vec3(1.0f));
	EXPECT_EQ(light.add, glm::vec3(0.0f));
}

TEST(TempleLight, WhollyEvilIsRed)
{
	const auto light = TempleLight::At(-1.0f, 0);
	EXPECT_FLOAT_EQ(light.multiply.r, 1.0f);
	EXPECT_FLOAT_EQ(light.multiply.g, 175.0f / 255.0f);
	EXPECT_FLOAT_EQ(light.multiply.b, 160.0f / 255.0f);
	EXPECT_EQ(light.add.g, 0.0f);
	EXPECT_EQ(light.add.b, 0.0f);
	// Near the height of the red pulse, just short of 22, it is 21 by 255 of 256
	EXPECT_FLOAT_EQ(TempleLight::At(-1.0f, 300 * 8).add.r, 20.0f / 255.0f);
}

TEST(TempleLight, HalfEvilIsHalfAsRed)
{
	// 127 of 255: green is 255 less 80 by 127 over 256, rounded down
	const auto light = TempleLight::At(-0.5f, 0);
	EXPECT_FLOAT_EQ(light.multiply.g, 215.0f / 255.0f);
	EXPECT_FLOAT_EQ(light.multiply.b, 207.0f / 255.0f);
}

TEST(TempleLight, GoodPulsesThroughTheColours)
{
	// At the start: red 22, green 22 + 11 sin(2 pi / 3) and blue 22 + 11 sin(2 pi), each by 255 of 256
	const auto light = TempleLight::At(1.0f, 0);
	EXPECT_EQ(light.multiply, glm::vec3(1.0f));
	EXPECT_FLOAT_EQ(light.add.r, 21.0f / 255.0f);
	EXPECT_FLOAT_EQ(light.add.g, 30.0f / 255.0f);
	EXPECT_FLOAT_EQ(light.add.b, 21.0f / 255.0f);
	// An eighth of the way round, red is 22 + 11 sin(pi / 4)
	EXPECT_FLOAT_EQ(TempleLight::At(1.0f, 64 * 8).add.r, 28.0f / 255.0f);
}

TEST(TempleLight, ColoursAreMultipliedByTheLight)
{
	const auto light = TempleLight::At(-1.0f, 0);
	const auto colour = light.Colour(glm::vec3(0x80 / 255.0f));
	EXPECT_FLOAT_EQ(colour.r, 127.0f / 255.0f);
	EXPECT_FLOAT_EQ(colour.g, 87.0f / 255.0f);
	EXPECT_FLOAT_EQ(colour.b, 80.0f / 255.0f);
}

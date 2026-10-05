/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Graphics/HandLight.h"

using openblack::graphics::HandLight;

namespace
{
constexpr auto k_Hand = glm::vec3(1000.0f, 80.0f, 2000.0f);
} // namespace

TEST(HandLight, MapIsCentredOnTheHand)
{
	// 12 vertices 10 units apart span 110 units, half of them each side of the hand
	EXPECT_EQ(HandLight::GetOrigin(k_Hand), glm::vec2(945.0f, 1945.0f));
}

TEST(HandLight, ComesUpAsTheLandDarkens)
{
	EXPECT_FLOAT_EQ(HandLight::GetStrength(0xFFFFFF), 0.0f);
	EXPECT_FLOAT_EQ(HandLight::GetStrength(0x787878), 0.0f);
	// A mean of 112, eight fifteenths of the way down
	EXPECT_FLOAT_EQ(HandLight::GetStrength(0x707070), 8.0f / 15.0f);
	// The mean is rounded down: (110 + 111 + 111) / 3 is 110
	EXPECT_FLOAT_EQ(HandLight::GetStrength(0x6E6F6F), 10.0f / 15.0f);
	EXPECT_FLOAT_EQ(HandLight::GetStrength(0x696969), 1.0f);
	EXPECT_FLOAT_EQ(HandLight::GetStrength(0x000000), 1.0f);
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Graphics/ModelLight.h"

using namespace openblack;

namespace
{
constexpr glm::vec3 k_Hand {100.0f, 50.0f, 100.0f};
constexpr glm::vec3 k_Camera {100.0f, 50.0f, 200.0f};
constexpr float k_Night = 0.0f;
constexpr float k_Day = 2.0f;
} // namespace

TEST(ModelLight, TheSunByDay)
{
	EXPECT_EQ(model_light::FrameLight(k_Hand, 0.0f, k_Camera, k_Day, false), model_light::k_Sun);
}

TEST(ModelLight, TheHandCarriesTheLightAtNight)
{
	const auto light = model_light::FrameLight(k_Hand, 0.0f, k_Camera, k_Night, false);
	// A little before the hand, towards the camera
	EXPECT_NEAR(glm::distance(light, k_Hand), 3.0f, 1e-4f);
	EXPECT_GT(light.z, k_Hand.z);
	// Never lower than 10 over the ground under the hand
	const auto low = model_light::FrameLight({100.0f, 0.0f, 100.0f}, 5.0f, k_Camera, k_Night, false);
	EXPECT_GE(low.y, 15.0f - 3.0f);
}

TEST(ModelLight, NoNightLightOnTheHandInsideTheTemple)
{
	// Inside the temple the hand carries no light, so it doesn't light itself there
	EXPECT_EQ(model_light::FrameLight(k_Hand, 0.0f, k_Camera, k_Night, true), model_light::k_Sun);
	EXPECT_EQ(model_light::FrameLight(k_Hand, 0.0f, k_Camera, k_Day, true), model_light::k_Sun);
}

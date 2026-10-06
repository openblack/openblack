/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Graphics/CreatureShadow.h"

using namespace openblack::graphics;

TEST(CreatureShadow, FadesBetween50And80Radii)
{
	EXPECT_EQ(CreatureShadow::Alpha(10.0f, 1.0f), 255);
	EXPECT_EQ(CreatureShadow::Alpha(49.9f, 1.0f), 255);
	EXPECT_EQ(CreatureShadow::Alpha(65.0f, 1.0f), 127);
	EXPECT_EQ(CreatureShadow::Alpha(80.0f, 1.0f), 0);
	// In radii of the creature
	EXPECT_EQ(CreatureShadow::Alpha(650.0f, 10.0f), 127);
	EXPECT_EQ(CreatureShadow::Alpha(10.0f, 0.0f), 0);
}

TEST(CreatureShadow, TheLightIsKeptAtLeast45DegreesUp)
{
	const glm::vec3 centre {100.0f, 10.0f, 100.0f};
	// A low sun far away, 10 degrees up
	const auto low = CreatureShadow::LightPoint(centre + glm::vec3(1000.0f, 176.3f, 0.0f), centre, 2.0f);
	const auto towards = low - centre;
	EXPECT_NEAR(towards.y, 1000.0f, 1e-2f);
	EXPECT_NEAR(towards.x, 1000.0f, 1e-2f);
	// A high one is left as it is
	const auto high = CreatureShadow::LightPoint(centre + glm::vec3(10.0f, 500.0f, 0.0f), centre, 2.0f);
	EXPECT_NEAR(high.y - centre.y, 500.0f, 1e-3f);
}

TEST(CreatureShadow, ANearLightIsPushedOutToThreeRadii)
{
	const glm::vec3 centre {0.0f, 0.0f, 0.0f};
	const auto pushed = CreatureShadow::LightPoint(glm::vec3(1.0f, 0.5f, 0.0f), centre, 2.0f);
	const auto across = std::sqrt((pushed.x * pushed.x) + (pushed.z * pushed.z));
	// Out to 6 units, then raised to 45 degrees
	EXPECT_GT(across, 5.0f);
	EXPECT_NEAR(pushed.y, across, 1e-4f);
	// Straight overhead, it is nudged to a side first
	const auto overhead = CreatureShadow::LightPoint(glm::vec3(0.0f, 1.0f, 0.0f), centre, 2.0f);
	EXPECT_GT(overhead.x, 0.0f);
	EXPECT_GT(overhead.z, 0.0f);
}

TEST(CreatureShadow, TheCreaturesCentreFallsInTheMiddleOfItsCell)
{
	const glm::vec3 centre {50.0f, 20.0f, 60.0f};
	const auto shadow = CreatureShadow::Compute(centre, 5.0f, 10.0f, centre + glm::vec3(-100.0f, 300.0f, -100.0f),
	                                            centre + glm::vec3(0.0f, 30.0f, -50.0f), 3, 8, true, false);
	ASSERT_TRUE(shadow.has_value());
	EXPECT_FLOAT_EQ(shadow->strength, 1.0f);
	const auto coordinate = shadow->receiverMatrix * glm::vec4(centre, 1.0f);
	EXPECT_NEAR(coordinate.x, 3.5f / 8.0f, 1e-4f);
	EXPECT_NEAR(coordinate.y, 0.5f, 1e-4f);
	EXPECT_NEAR(coordinate.z, 0.0f, 1e-3f);
	// The ground under it is further along the light, so is shaded
	const auto ground = shadow->receiverMatrix * glm::vec4(centre.x, 10.0f, centre.z, 1.0f);
	EXPECT_GT(ground.z, shadow->startDepth);
}

TEST(CreatureShadow, FarCamerasSeeNoShadow)
{
	const glm::vec3 centre {0.0f, 5.0f, 0.0f};
	EXPECT_FALSE(CreatureShadow::Compute(centre, 1.0f, 0.0f, glm::vec3(0.0f, 100.0f, 10.0f), glm::vec3(0.0f, 100.0f, 0.0f), 0,
	                                     8, true, false)
	                 .has_value());
}

/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Graphics/ObjectShadows.h"

using openblack::graphics::ObjectShadows;

namespace
{
constexpr auto k_Origin = glm::vec3(1000.0f, 50.0f, 2000.0f);
} // namespace

TEST(ObjectShadows, BaseStaysWhereItIs)
{
	const auto point = k_Origin + glm::vec3(3.0f, 0.0f, -4.0f);
	const auto cast = ObjectShadows::Project(point, k_Origin);
	EXPECT_NEAR(cast.x, point.x, 1e-3f);
	EXPECT_NEAR(cast.y, k_Origin.y, 1e-3f);
	EXPECT_NEAR(cast.z, point.z, 1e-3f);
}

TEST(ObjectShadows, FallsAsFarAlongXAndZAsItIsHigh)
{
	// DEFAULT_SUN sits up and back along -x and -z, so a point's shadow falls its height along +x and +z
	const auto top = ObjectShadows::Project(k_Origin + glm::vec3(0.0f, 20.0f, 0.0f), k_Origin);
	EXPECT_NEAR(top.x - k_Origin.x, 20.0f, 0.01f);
	EXPECT_NEAR(top.z - k_Origin.z, 20.0f, 0.01f);
	EXPECT_FLOAT_EQ(top.y, k_Origin.y);
}

TEST(ObjectShadows, FallsOntoThePlaneThroughTheOrigin)
{
	// Wherever the land beneath is, the shadow is cast onto the height of the object's origin
	const auto cast = ObjectShadows::Project(glm::vec3(10.0f, 90.0f, 10.0f), glm::vec3(0.0f, 80.0f, 0.0f));
	EXPECT_FLOAT_EQ(cast.y, 80.0f);
	EXPECT_NEAR(cast.x, 20.0f, 0.01f);
	EXPECT_NEAR(cast.z, 20.0f, 0.01f);
}

TEST(ObjectShadows, BelowTheOriginFallsStraightDown)
{
	const auto point = k_Origin + glm::vec3(2.0f, -5.0f, 1.0f);
	const auto cast = ObjectShadows::Project(point, k_Origin);
	EXPECT_NEAR(cast.x, point.x, 1e-3f);
	EXPECT_NEAR(cast.z, point.z, 1e-3f);
}

TEST(ObjectShadows, HalvesTheLandUnderAFullShadow)
{
	EXPECT_FLOAT_EQ(1.0f - ObjectShadows::k_MaxDarkness, 128.0f / 256.0f);
}

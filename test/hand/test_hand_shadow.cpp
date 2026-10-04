/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Graphics/HandShadow.h"

using openblack::SkyInterface;
using openblack::graphics::HandShadow;

namespace
{
constexpr SkyInterface::DayNightTimes k_Times {.nightFull = 4.0f, .duskStart = 5.0f, .duskEnd = 6.0f, .dayFull = 7.0f};

std::vector<glm::mat4> HandAt(glm::vec3 position)
{
	// A hand of bones spread two units around a point
	std::vector<glm::mat4> bones;
	for (const auto offset :
	     {glm::vec3(-2.0f, 0.0f, 0.0f), glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, 2.0f)})
	{
		bones.push_back(glm::translate(glm::mat4(1.0f), position + offset));
	}
	return bones;
}
} // namespace

TEST(HandShadow, SunCrossesTheSky)
{
	const auto midday = HandShadow::SunlightDirection(12.0f, k_Times);
	EXPECT_LT(midday.y, -0.8f);
	// The midday sun comes from the side of Black & White's light at (-500000, 500000, -500000)
	EXPECT_GT(midday.x, 0.0f);
	EXPECT_GT(midday.z, 0.0f);
	EXPECT_NEAR(midday.x, midday.z, 0.001f);

	const auto morning = HandShadow::SunlightDirection(7.0f, k_Times);
	const auto evening = HandShadow::SunlightDirection(17.0f, k_Times);
	EXPECT_GT(morning.y, midday.y);
	EXPECT_NEAR(morning.y, evening.y, 0.001f);
	// Morning and evening shadows point opposite ways
	EXPECT_LT(glm::dot(glm::vec2(morning.x, morning.z), glm::vec2(evening.x, evening.z)), 0.0f);
}

TEST(HandShadow, NoShadowAtNight)
{
	EXPECT_FLOAT_EQ(HandShadow::Daylight(2.0f, k_Times), 0.0f);
	EXPECT_FLOAT_EQ(HandShadow::Daylight(23.0f, k_Times), 0.0f);
	EXPECT_FLOAT_EQ(HandShadow::Daylight(5.5f, k_Times), 0.25f);
	EXPECT_FLOAT_EQ(HandShadow::Daylight(12.0f, k_Times), 1.0f);
	EXPECT_FALSE(HandShadow::Compute(HandAt({100.0f, 10.0f, 100.0f}), {100.0f, 30.0f, 80.0f}, 0.0f, 1.0f, k_Times, true, true));
}

TEST(HandShadow, FadesWithCameraDistance)
{
	EXPECT_FLOAT_EQ(HandShadow::DistanceFade(49.0f, 1.0f), 1.0f);
	EXPECT_FLOAT_EQ(HandShadow::DistanceFade(65.0f, 1.0f), 0.5f);
	EXPECT_FLOAT_EQ(HandShadow::DistanceFade(81.0f, 1.0f), 0.0f);
}

TEST(HandShadow, ProjectsAlongTheSunlight)
{
	for (const bool originBottomLeft : {false, true})
	{
		const auto hand = glm::vec3(100.0f, 10.0f, 100.0f);
		const auto shadow =
		    HandShadow::Compute(HandAt(hand), {100.0f, 40.0f, 60.0f}, 0.0f, 9.0f, k_Times, originBottomLeft, true);
		if (!shadow)
		{
			ADD_FAILURE() << "no shadow";
			continue;
		}
		EXPECT_FLOAT_EQ(shadow->strength, 1.0f);

		// The hand's centre is in the middle of the texture, so is the ground where the sunlight carries it
		const auto centre = shadow->receiverMatrix * glm::vec4(hand, 1.0f);
		EXPECT_NEAR(centre.x, 0.5f, 0.001f);
		EXPECT_NEAR(centre.y, 0.5f, 0.001f);
		EXPECT_NEAR(centre.z, 0.0f, 0.001f);
		const auto distance = hand.y / -shadow->lightDirection.y;
		const auto ground = shadow->receiverMatrix * glm::vec4(hand + shadow->lightDirection * distance, 1.0f);
		EXPECT_NEAR(ground.x, 0.5f, 0.001f);
		EXPECT_NEAR(ground.y, 0.5f, 0.001f);
		EXPECT_NEAR(ground.z, distance, 0.01f);

		// The texture coordinates are those of the silhouette pass's clip space
		const auto side = hand + glm::vec3(1.5f, 0.0f, 0.0f);
		const auto clip = shadow->projection * shadow->view * glm::vec4(side, 1.0f);
		const auto uv = shadow->receiverMatrix * glm::vec4(side, 1.0f);
		EXPECT_NEAR(uv.x, (clip.x * 0.5f) + 0.5f, 0.001f);
		EXPECT_NEAR(uv.y, (clip.y * (originBottomLeft ? 0.5f : -0.5f)) + 0.5f, 0.001f);
	}
}

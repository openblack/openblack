/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Creature/CreatureCatch.h"
#include "Creature/CreatureLayers.h"

using namespace openblack;
using namespace openblack::creature_catch;

namespace
{
Approach Coming(glm::vec3 velocity)
{
	// A creature of size 1 plays at 1.175 times; a step of 1175 ms is then 1 s, closing at 1175 ms is 1 s in
	return {.thing = {0.0f, 10.0f, 20.0f},
	        .velocity = velocity,
	        .creature = {0.0f, 0.0f, 0.0f},
	        .size = 1.0f,
	        .modelScale = 1.0f,
	        .catchMs = 1175.0f,
	        .stepMs = 1175.0f,
	        .stepTravel = -10.0f};
}
} // namespace

TEST(CreatureCatch, ItReachesWhatPassesNearInTime)
{
	ASSERT_FLOAT_EQ(creature_layers::PlaybackRate(1.0f), 1.175f);
	// Passing straight through it in 4 s, with 3 s to spare: it reaches (0.5 + 3) × 10 = 35 m
	EXPECT_TRUE(Reaches(Coming({0.0f, 0.0f, -5.0f})));
	// Too slow across the land
	EXPECT_FALSE(Reaches(Coming({0.0f, -20.0f, -0.9f})));
	// Passing closest too late
	EXPECT_FALSE(Reaches(Coming({0.0f, 0.0f, -3.0f})));
	// Flying away: it passed closest in the past
	EXPECT_FALSE(Reaches(Coming({0.0f, 0.0f, 5.0f})));
}

TEST(CreatureCatch, ItMissesWhatPassesBeyondItsReach)
{
	// Passing 40 m to the side
	auto approach = Coming({0.0f, 0.0f, -5.0f});
	approach.thing.x = 40.0f;
	EXPECT_FALSE(Reaches(approach));
	approach.thing.x = 30.0f;
	EXPECT_TRUE(Reaches(approach));
}

TEST(CreatureCatch, TheCatchingAnimationsBlendByHeightAndSide)
{
	// The hand closes low and to one side, low and to the other, high and to one side, high and to the other
	const std::array<glm::vec3, 4> hands {glm::vec3(2.0f, 1.0f, 0.0f), glm::vec3(-2.0f, 1.0f, 0.0f),
	                                      glm::vec3(2.0f, 5.0f, 0.0f), glm::vec3(-2.0f, 5.0f, 0.0f)};
	const auto low = Weigh({2.0f, 1.0f, -3.0f}, hands, 1.0f, false);
	EXPECT_FALSE(low.clamped);
	EXPECT_FLOAT_EQ(low.weights[0], 1.0f);
	const auto high = Weigh({-2.0f, 5.0f, -3.0f}, hands, 1.0f, false);
	EXPECT_FLOAT_EQ(high.weights[3], 1.0f);
	const auto middle = Weigh({0.0f, 3.0f, -3.0f}, hands, 1.0f, false);
	for (const float weight : middle.weights)
	{
		EXPECT_FLOAT_EQ(weight, 0.25f);
	}
	// Far above the highest it leans past its limit and misses
	EXPECT_TRUE(Weigh({0.0f, 20.0f, -3.0f}, hands, 1.0f, false).clamped);
}

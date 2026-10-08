/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <vector>

#include <gtest/gtest.h>

#include "ECS/TownPlaythings.h"

using namespace openblack;
using namespace openblack::ecs::town_playthings;

TEST(TownPlaythings, ATownKeepsOneFootballWhileItIsAbout)
{
	std::vector<entt::entity> playthings;
	const auto first = static_cast<entt::entity>(1);
	const auto second = static_cast<entt::entity>(2);
	bool firstAbout = true;
	const auto about = [&](entt::entity thing) { return thing == first && firstAbout; };
	EXPECT_TRUE(Add(playthings, first, about));
	EXPECT_FALSE(Add(playthings, second, about));
	firstAbout = false;
	EXPECT_TRUE(Add(playthings, second, about));
	// The newest comes first
	EXPECT_EQ(playthings, (std::vector {second, first}));
}

TEST(TownPlaythings, TheNearestTownTakesItThePlayersBeforeTheNeutral)
{
	const std::array towns {
	    Candidate {
	        .town = static_cast<entt::entity>(10), .owner = PlayerNames::NEUTRAL, .id = 0, .position = {100.0f, 0.0f, 0.0f}},
	    Candidate {
	        .town = static_cast<entt::entity>(11), .owner = PlayerNames::PLAYER_TWO, .id = 5, .position = {0.0f, 0.0f, 100.0f}},
	    Candidate {
	        .town = static_cast<entt::entity>(12), .owner = PlayerNames::PLAYER_ONE, .id = 9, .position = {400.0f, 0.0f, 0.0f}},
	};
	// Equally far, a player's town is looked at first and kept
	EXPECT_EQ(Nearest(towns, {0.0f, 0.0f, 0.0f}), static_cast<entt::entity>(11));
	EXPECT_EQ(Nearest(towns, {390.0f, 0.0f, 0.0f}), static_cast<entt::entity>(12));
	EXPECT_FALSE(Nearest({}, {0.0f, 0.0f, 0.0f}).has_value());
}

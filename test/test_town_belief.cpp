/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Magic/TownBelief.h"

using namespace openblack;
using namespace openblack::magic::town_belief;

TEST(TownBelief, PendingBeliefIsBelievedAtTheTownsTurnByItsScale)
{
	Belief town;
	town.scale = 2.0f;
	Add(town, PlayerNames::PLAYER_ONE, 0.25f);
	Add(town, PlayerNames::PLAYER_ONE, 0.25f);
	EXPECT_FLOAT_EQ(town.belief.at(0), 0.0f);
	const auto gained = Turn(town, 0.997f);
	ASSERT_EQ(gained.size(), 1u);
	EXPECT_EQ(gained[0].player, PlayerNames::PLAYER_ONE);
	EXPECT_FLOAT_EQ(gained[0].amount, 1.0f);
	EXPECT_FLOAT_EQ(town.belief.at(0), 1.0f);
	EXPECT_FLOAT_EQ(town.pending.at(0), 0.0f);
	EXPECT_FLOAT_EQ(town.recent.at(0), 0.5f * 0.997f);
	EXPECT_TRUE(Turn(town, 0.997f).empty());
}

TEST(TownBelief, CappedAboveOnly)
{
	Belief town;
	Add(town, PlayerNames::PLAYER_TWO, 15.0f);
	(void)Turn(town, 1.0f);
	EXPECT_FLOAT_EQ(town.belief.at(1), k_DefaultCap);
	Add(town, PlayerNames::PLAYER_TWO, -20.0f);
	(void)Turn(town, 1.0f);
	EXPECT_FLOAT_EQ(town.belief.at(1), -10.0f);
}

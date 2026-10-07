/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Creature/CreatureMiracleReactions.h"

using namespace openblack::creature_mind;

TEST(CreatureMiracleReactions, ItRunsFromANastyMiracleSometimesStartingInFright)
{
	const auto startled = RunAwayFromMiracle({10.0f, 0.0f}, 15.0f, [](uint32_t) { return 0u; });
	ASSERT_EQ(startled.size(), 2u);
	EXPECT_EQ(startled[0].kind, Step::Kind::Action);
	EXPECT_EQ(startled[0].animation, k_FrightenedAnimation);
	EXPECT_EQ(startled[1].movement.kind, Movement::Kind::FleeFrom);
	EXPECT_TRUE(startled[1].movement.run);
	EXPECT_EQ(RunAwayFromMiracle({10.0f, 0.0f}, 15.0f, [](uint32_t) { return 1u; }).size(), 1u);
}

TEST(CreatureMiracleReactions, ItGoesToLookAndPointsOrIsPuzzled)
{
	const auto pointing = ExamineMiracle({10.0f, 0.0f}, 15.0f, [](uint32_t) { return 1u; });
	ASSERT_EQ(pointing.size(), 3u);
	EXPECT_EQ(pointing[0].movement.kind, Movement::Kind::ToPoint);
	EXPECT_FLOAT_EQ(pointing[0].movement.maxDistance, 5.0f * 15.0f);
	EXPECT_EQ(pointing[1].movement.kind, Movement::Kind::TurnToFace);
	EXPECT_EQ(pointing[2].order.kind, ObjectOrder::Kind::PointAt);
	const auto puzzled = ExamineMiracle({10.0f, 0.0f}, 15.0f, [](uint32_t) { return 0u; });
	ASSERT_EQ(puzzled.size(), 5u);
	EXPECT_EQ(puzzled[1].order.kind, ObjectOrder::Kind::PointAt);
	EXPECT_EQ(puzzled.back().kind, Step::Kind::Wait);
	EXPECT_FLOAT_EQ(puzzled.back().seconds, k_PuzzledSeconds);
}

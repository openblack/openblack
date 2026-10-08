/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>

#include <gtest/gtest.h>

#include "ECS/LandForests.h"

using namespace openblack::ecs::land_forests;

TEST(LandForests, AnyForestFurtherThanAFewCentimetresScoresTheSameAsALair)
{
	// Its distance in whole units over 1000, below -1 for anything past about 15 cm, sits in the sigmoid's 19th step
	EXPECT_NEAR(LairScore(1000), 0.114437282f, 1e-7f);
	EXPECT_NEAR(LairScore(6553600), 0.114437282f, 1e-7f);
	EXPECT_GT(LairScore(0), LairScore(1000));
}

TEST(LandForests, TheSecondForestMetIsTheLairWhenAllScoreAlike)
{
	const std::array<float, 4> alike {0.11f, 0.11f, 0.11f, 0.11f};
	EXPECT_EQ(LairForest(alike), 1u);
	const std::array<float, 1> one {0.11f};
	EXPECT_EQ(LairForest(one), 0u);
	EXPECT_FALSE(LairForest({}).has_value());
	// The first is taken without a score; a later one must beat the best score so far
	const std::array<float, 3> rising {0.9f, 0.2f, 0.5f};
	EXPECT_EQ(LairForest(rising), 2u);
}

TEST(LandForests, TheNearestKeepsTheFirstOfEquals)
{
	const std::array<int32_t, 4> distances {30, 10, 10, 20};
	EXPECT_EQ(Nearest(distances), 1u);
	EXPECT_FALSE(Nearest({}).has_value());
}

TEST(LandForests, ATownTakesForestsStrictlyWithinReachThatHoldWood)
{
	EXPECT_TRUE(NearTown(49.0f, 50.0f, 10.0f));
	EXPECT_FALSE(NearTown(50.0f, 50.0f, 10.0f));
	EXPECT_FALSE(NearTown(10.0f, 50.0f, 0.0f));
}

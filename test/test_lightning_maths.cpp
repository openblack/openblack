/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <numbers>
#include <vector>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Particles/LightningMaths.h"

using namespace openblack::particles::maths;

TEST(LightningMaths, SearchesASquareOfCellsTwiceItsRadiusAcross)
{
	EXPECT_EQ(StrikeSearchCells(60.0f), 144u);
	EXPECT_EQ(StrikeSearchCells(100.0f), 400u);
	EXPECT_EQ(StrikeSearchCells(140.0f), 784u);
	EXPECT_EQ(StrikeSearchCells(5.0f), 4u);
}

TEST(LightningMaths, ABoltFromACloudSearchesASquareItsRadiusAcross)
{
	EXPECT_EQ(CloudSearchCells(60.0f), 36u);
	EXPECT_EQ(CloudSearchCells(25.0f), 9u);
	EXPECT_EQ(CloudSearchCells(5.0f), 1u);
}

TEST(LightningMaths, KeepsTwoForksForEachTargetStruckAtOnceAndTwoMore)
{
	EXPECT_EQ(ForkCount(4, 6), 10u);
	EXPECT_EQ(ForkCount(10, 3), 8u);
	EXPECT_EQ(ForkCount(20, 28), 42u);
}

TEST(LightningMaths, ACreatureTakesEveryFork)
{
	const std::vector<bool> none {false, false, false};
	EXPECT_EQ(PreferCreatures(none), (std::vector<size_t> {0, 1, 2}));
	const std::vector<bool> one {false, true, false, false};
	EXPECT_EQ(PreferCreatures(one), (std::vector<size_t> {1, 1, 1, 1}));
	// Two creatures share the forks in turn
	const std::vector<bool> two {true, false, false, true, false};
	EXPECT_EQ(PreferCreatures(two), (std::vector<size_t> {0, 3, 0, 3, 0}));
}

TEST(LightningMaths, BoltsPointingTheSameWayClash)
{
	const float north = std::numbers::pi_v<float> / 2.0f;
	const BoltPose older {.origin = {-15.0f, 10.0f, 0.0f}, .centroid = {0.0f, 2.0f, 40.0f}, .heading = north};
	const BoltPose newer {.origin = {15.0f, 10.0f, 0.0f}, .centroid = {0.0f, 2.0f, 40.0f}, .heading = north};
	const auto meeting = ClashPoint(newer, older, 60.0f);
	ASSERT_TRUE(meeting.has_value());
	// Between the older bolt's middle and the hands, in front of the newer one
	EXPECT_LT(meeting->z, 40.0f);
	EXPECT_GT(meeting->z, 0.0f);
	// Out of reach, or pointing apart, they don't
	EXPECT_FALSE(ClashPoint(newer, older, 5.0f).has_value());
	BoltPose away = newer;
	away.heading = -north;
	EXPECT_FALSE(ClashPoint(away, older, 60.0f).has_value());
}

TEST(LightningMaths, ArcsRunFromEndToEnd)
{
	const ArcEnds ends {.from = {0.0f, 0.0f, 0.0f},
	                    .to = {4.0f, 0.0f, 0.0f},
	                    .fromNormal = {0.0f, 1.0f, 0.0f},
	                    .toNormal = {0.0f, -1.0f, 0.0f}};
	const auto fromTangent = ArcTangent(ends.fromNormal, glm::vec3(0.0f), 0.2f, 4.0f, 3.0f);
	EXPECT_EQ(fromTangent, glm::vec3(0.0f, 12.0f, 0.0f));
	const std::vector<glm::vec3> still(8, glm::vec3(0.0f));
	const auto joints = ArcJoints(ends, fromTangent, ArcTangent(ends.toNormal, glm::vec3(0.0f), 0.2f, 4.0f, 3.0f), still, 0.1f);
	ASSERT_EQ(joints.size(), 8u);
	EXPECT_NEAR(glm::distance(joints.front(), ends.from), 0.0f, 1e-5f);
	EXPECT_NEAR(glm::distance(joints.back(), ends.to), 0.0f, 1e-4f);
	// It leaves upwards along the first normal
	EXPECT_GT(joints[1].y, 0.0f);
	// The jitter moves every joint by its share of the arc's length
	const std::vector<glm::vec3> jitters(8, glm::vec3(1.0f, 0.0f, 0.0f));
	const auto jittered = ArcJoints(ends, fromTangent, glm::vec3(0.0f), jitters, 0.1f);
	EXPECT_NEAR(jittered.front().x, 0.4f, 1e-5f);
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <algorithm>
#include <numbers>
#include <optional>
#include <vector>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Creature/CreatureLocomotion.h"
#include "Creature/CreatureRoute.h"

using namespace openblack;
using namespace openblack::creature_route;

namespace
{
constexpr float k_Tolerance = 1e-3f;
constexpr float k_Pi = std::numbers::pi_v<float>;

/// Flat land 5 units up, without water
float Flat(int32_t /*x*/, int32_t /*z*/)
{
	return 5.0f;
}
std::optional<bool> Dry(int32_t /*x*/, int32_t /*z*/)
{
	return false;
}

WalkableLand FlatLand()
{
	return WalkableLand::Build(Flat, Dry);
}

/// Flat land with a steep ridge across x = 2000 to 2020, from z = 1790 to 2210
WalkableLand LandWithCliff()
{
	return WalkableLand::Build([](int32_t x, int32_t z) { return (x == 201 && z >= 180 && z <= 220) ? 40.0f : 5.0f; }, Dry);
}

/// Plans with as many turns as it takes
Planner::Status PlanAll(Planner& planner, const WalkableLand& land)
{
	for (int turn = 0; turn < 1000 && planner.GetStatus() == Planner::Status::Planning; ++turn)
	{
		planner.Step(land, 4000);
	}
	return planner.GetStatus();
}

bool ClearOf(const std::vector<glm::vec2>& route, const Circle& circle)
{
	for (size_t i = 1; i < route.size(); ++i)
	{
		if (DistanceToSegment(circle.centre, route[i - 1], route[i]) < circle.radius - 1.0f)
		{
			return false;
		}
	}
	return true;
}
} // namespace

TEST(CreatureRoute, SortsTheLand)
{
	const auto land = WalkableLand::Build(
	    [](int32_t x, int32_t /*z*/) {
		    // Deep sea west of x = 100, a steep wall along x = 300, an island cut off beyond x = 400
		    if (x < 100 || (x >= 300 && x <= 301) || x >= 400)
		    {
			    return x < 100 || x >= 420 ? 0.0f : (x == 301 ? 60.0f : 5.0f);
		    }
		    return 5.0f;
	    },
	    [](int32_t x, int32_t z) -> std::optional<bool> {
		    if (z == 0)
		    {
			    return std::nullopt;
		    }
		    return x >= 150 && x < 160;
	    });
	EXPECT_EQ(land.At(50, 50), Ground::Blocked);
	EXPECT_EQ(land.At(200, 50), Ground::Open);
	EXPECT_EQ(land.At(155, 50), Ground::Water);
	// No cell at all counts as water
	EXPECT_EQ(land.At(200, 0), Ground::Water);
	EXPECT_EQ(land.At(300, 50), Ground::Blocked);
	EXPECT_EQ(land.At(350, 50), Ground::CutOff);
	EXPECT_EQ(land.At(-1, 50), Ground::Blocked);
	EXPECT_EQ(land.AtPoint({2005.0f, 505.0f}), Ground::Open);
}

TEST(CreatureRoute, CreaturesStandClearOfBlockedCells)
{
	const auto land = LandWithCliff();
	EXPECT_TRUE(land.IsValid({1000.0f, 1000.0f}, k_Clearance));
	// The ridge's cells are 200 and 201, either side of its high corners; standing in them is not allowed
	EXPECT_FALSE(land.IsValid({2005.0f, 2000.0f}, k_Clearance));
	// Within 7.05 of a blocked cell's centre is not allowed either
	EXPECT_FALSE(land.IsValid({1999.0f, 2005.0f}, k_Clearance));
	EXPECT_TRUE(land.IsValid({1990.0f, 2005.0f}, k_Clearance));
	const auto nearest = land.NearestValid({2005.0f, 2005.0f}, k_Clearance, 100.0f);
	ASSERT_TRUE(nearest.has_value());
	EXPECT_TRUE(land.IsValid(*nearest, k_Clearance));
	EXPECT_LT(glm::distance(*nearest, glm::vec2(2005.0f, 2005.0f)), 20.0f);
}

TEST(CreatureRoute, BigCreaturesWalkThroughSmallTrees)
{
	EXPECT_FALSE(MustAvoid(Obstacle::Tree, 20.0f, 15.0f, false));
	EXPECT_TRUE(MustAvoid(Obstacle::Tree, 22.5f, 15.0f, false));
	EXPECT_TRUE(MustAvoid(Obstacle::Tree, 2.0f, 15.0f, true));
	EXPECT_TRUE(MustAvoid(Obstacle::Building, 1.5f, 15.0f, false));
	EXPECT_FALSE(MustAvoid(Obstacle::Object, 1.4f, 15.0f, false));
	EXPECT_FALSE(MustAvoid(Obstacle::Living, 100.0f, 15.0f, false));
	EXPECT_TRUE(MustAvoid(Obstacle::StandingCreature, 0.0f, 15.0f, false));
	EXPECT_FALSE(MustAvoid(Obstacle::MovingCreature, 0.0f, 15.0f, false));
}

TEST(CreatureRoute, OpenGroundIsAStraightLine)
{
	const auto land = FlatLand();
	Planner planner({.start = {1000.0f, 1000.0f},
	                 .destination = {1000.0f, 1100.0f},
	                 .minDistance = 0.0f,
	                 .maxDistance = 1.0f,
	                 .obstacles = {},
	                 .cornerRadius = 6.0f});
	EXPECT_EQ(planner.Step(land, 10), Planner::Status::Found);
	const auto& route = planner.GetRoute();
	ASSERT_EQ(route.size(), 2u);
	// It stops at the near edge of the arrival ring
	EXPECT_NEAR(route.back().y, 1099.0f, k_Tolerance);
	EXPECT_EQ(planner.GetSearched(), 0u);
}

TEST(CreatureRoute, WalksRoundSomethingInTheWay)
{
	const auto land = FlatLand();
	const Circle cow {.centre = {1000.0f, 1050.0f}, .radius = 12.0f};
	Planner planner({.start = {1000.0f, 1000.0f},
	                 .destination = {1000.0f, 1100.0f},
	                 .minDistance = 0.0f,
	                 .maxDistance = 1.0f,
	                 .obstacles = {cow},
	                 .cornerRadius = 6.0f});
	ASSERT_EQ(PlanAll(planner, land), Planner::Status::Found);
	const auto& route = planner.GetRoute();
	EXPECT_GT(route.size(), 2u);
	EXPECT_TRUE(ClearOf(route, cow));
	EXPECT_LE(glm::distance(route.back(), glm::vec2(1000.0f, 1100.0f)), 1.0f + k_Tolerance);
	// Its corners are rounded: no two segments turn more than the corner angle
	for (size_t i = 2; i < route.size(); ++i)
	{
		const auto before = glm::normalize(route[i - 1] - route[i - 2]);
		const auto after = glm::normalize(route[i] - route[i - 1]);
		EXPECT_LT(std::acos(std::clamp(glm::dot(before, after), -1.0f, 1.0f)), creature_locomotion::k_CornerAngle);
	}
}

TEST(CreatureRoute, GoesRoundACliff)
{
	const auto land = LandWithCliff();
	Planner planner({.start = {1900.0f, 2000.0f},
	                 .destination = {2200.0f, 2000.0f},
	                 .minDistance = 0.0f,
	                 .maxDistance = 1.0f,
	                 .obstacles = {},
	                 .cornerRadius = 6.0f});
	ASSERT_EQ(PlanAll(planner, land), Planner::Status::Found);
	const auto& route = planner.GetRoute();
	for (size_t i = 1; i < route.size(); ++i)
	{
		for (float t = 0.0f; t <= 1.0f; t += 0.1f)
		{
			EXPECT_NE(land.AtPoint(route[i - 1] + ((route[i] - route[i - 1]) * t)), Ground::Blocked);
		}
	}
	// Round one end of the ridge or the other
	const auto furthest = std::ranges::max(route, {}, [](glm::vec2 p) { return std::abs(p.y - 2000.0f); });
	EXPECT_GT(std::abs(furthest.y - 2000.0f), 200.0f);
}

TEST(CreatureRoute, GivesUpWhenThereIsNoWay)
{
	const auto land = FlatLand();
	// Walled in by a ring of creatures
	std::vector<Circle> ring;
	for (int i = 0; i < 24; ++i)
	{
		const auto angle = 2.0f * k_Pi * static_cast<float>(i) / 24.0f;
		ring.push_back(
		    {.centre = glm::vec2(1000.0f, 1000.0f) + (40.0f * glm::vec2(std::cos(angle), std::sin(angle))), .radius = 12.0f});
	}
	Planner planner({.start = {1000.0f, 1000.0f},
	                 .destination = {1200.0f, 1000.0f},
	                 .minDistance = 0.0f,
	                 .maxDistance = 1.0f,
	                 .obstacles = ring,
	                 .cornerRadius = 6.0f});
	EXPECT_EQ(PlanAll(planner, land), Planner::Status::Failed);
}

TEST(CreatureRoute, ArrivesAnywhereOnTheRing)
{
	const auto land = FlatLand();
	Planner planner({.start = {1000.0f, 1000.0f},
	                 .destination = {1000.0f, 1100.0f},
	                 .minDistance = 10.0f,
	                 .maxDistance = 15.0f,
	                 .obstacles = {{.centre = {1000.0f, 1100.0f}, .radius = 9.0f}},
	                 .cornerRadius = 6.0f});
	ASSERT_EQ(PlanAll(planner, land), Planner::Status::Found);
	EXPECT_NEAR(glm::distance(planner.GetRoute().back(), glm::vec2(1000.0f, 1100.0f)), 15.0f, k_Tolerance);
	// Already within the ring, it doesn't go anywhere
	Planner there({.start = {1000.0f, 1088.0f},
	               .destination = {1000.0f, 1100.0f},
	               .minDistance = 10.0f,
	               .maxDistance = 15.0f,
	               .obstacles = {},
	               .cornerRadius = 6.0f});
	ASSERT_EQ(there.Step(land, 10), Planner::Status::Found);
	EXPECT_NEAR(glm::distance(there.GetRoute().front(), there.GetRoute().back()), 0.0f, k_Tolerance);
}

TEST(CreatureRoute, StraightensWhereClear)
{
	const std::vector<glm::vec2> points {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {3, 1}, {3, 2}};
	// Only the corner at (3, 0) blocks the view
	const auto clear = [](glm::vec2 a, glm::vec2 b) { return a.y == b.y || a.x == b.x; };
	const auto straight = Straighten(points, clear);
	ASSERT_EQ(straight.size(), 3u);
	EXPECT_EQ(straight[1], glm::vec2(3, 0));
}

TEST(CreatureRoute, RoundsCornersInSmallSteps)
{
	const std::vector<glm::vec2> points {{0, 0}, {0, 50}, {50, 50}};
	const auto rounded = RoundCorners(points, 10.0f, k_Pi / 18.0f);
	EXPECT_GT(rounded.size(), 9u);
	EXPECT_EQ(rounded.front(), points.front());
	EXPECT_EQ(rounded.back(), points.back());
	// The arc starts and ends 10 units either side of the corner
	EXPECT_NEAR(rounded[1].y, 40.0f, k_Tolerance);
	EXPECT_NEAR(rounded[rounded.size() - 2].x, 10.0f, k_Tolerance);
}

TEST(CreatureRoute, FollowsTheRoute)
{
	Route route {.points = {{0, 0}, {0, 10}, {10, 10}}, .segment = 0, .travelled = 0.0f};
	EXPECT_NEAR(route.RemainingTotal(), 20.0f, k_Tolerance);
	EXPECT_NEAR(std::abs(route.TurnAtEnd()), k_Pi / 2.0f, k_Tolerance);
	auto advanced = Advance(route, 4.0f);
	EXPECT_FALSE(advanced.segmentChanged);
	EXPECT_NEAR(advanced.position.y, 4.0f, k_Tolerance);
	// Heading along +z
	EXPECT_NEAR(std::abs(advanced.heading), k_Pi, k_Tolerance);
	advanced = Advance(route, 8.0f);
	EXPECT_TRUE(advanced.segmentChanged);
	EXPECT_NEAR(advanced.position.x, 2.0f, k_Tolerance);
	EXPECT_TRUE(route.OnLastSegment());
	advanced = Advance(route, 100.0f);
	EXPECT_TRUE(advanced.finished);
	EXPECT_EQ(advanced.position, glm::vec2(10, 10));
}

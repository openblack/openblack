/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <algorithm>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Creature/LeashRope.h"

using namespace openblack;
using namespace openblack::leash_rope;

namespace
{
constexpr float k_Tolerance = 1e-4f;
// Flat land at height 0, well below the ropes
float Flat(glm::vec2 /*point*/)
{
	return 0.0f;
}
// A rope 32.5 long at rest and 77 at full length, as held in the hand for a creature of size 1
constexpr float k_Slack = 32.5f;
constexpr float k_Max = 77.0f;
const glm::vec3 k_Start {100.0f, 50.0f, 100.0f};
} // namespace

TEST(LeashRope, StartsStraightAndEvenlySpread)
{
	const auto end = k_Start + glm::vec3(41.0f, 0.0f, 0.0f);
	const auto rope = Create(k_Start, end, k_Slack, k_Max, {});
	for (size_t i = 0; i < k_NodeCount; ++i)
	{
		EXPECT_NEAR(rope.nodes.at(i).position.x, k_Start.x + static_cast<float>(i + 1), k_Tolerance);
		EXPECT_EQ(rope.nodes.at(i).velocity, glm::vec3(0.0f));
	}
	EXPECT_NEAR(RestLength(41.0f), 1.0f, k_Tolerance);
}

TEST(LeashRope, ASlackRopeHasNoTension)
{
	// The ends closer together than the rope's rest length
	const auto rope = Create(k_Start, k_Start + glm::vec3(20.0f, 0.0f, 0.0f), k_Slack, k_Max, {});
	EXPECT_FLOAT_EQ(rope.tension, 0.0f);
}

TEST(LeashRope, StretchedToItsFullLengthIsTaut)
{
	const auto atMax = Create(k_Start, k_Start + glm::vec3(k_Max, 0.0f, 0.0f), k_Slack, k_Max, {});
	EXPECT_NEAR(atMax.tension, 1.0f, k_Tolerance);
	const auto beyond = Create(k_Start, k_Start + glm::vec3(2.0f * k_Max, 0.0f, 0.0f), k_Slack, k_Max, {});
	EXPECT_FLOAT_EQ(beyond.tension, 1.0f);
	// Halfway between the rest length and the full length, half taut
	const auto half = Create(k_Start, k_Start + glm::vec3((k_Slack + k_Max) * 0.5f, 0.0f, 0.0f), k_Slack, k_Max, {});
	EXPECT_NEAR(half.tension, 0.5f, 1e-3f);
}

TEST(LeashRope, SagsUnderItsWeight)
{
	auto rope = Create(k_Start, k_Start + glm::vec3(25.0f, 0.0f, 0.0f), k_Slack, k_Max, {});
	for (int frame = 0; frame < 120; ++frame)
	{
		Step(rope, rope.start, rope.end, 1.0f / 30.0f, Flat);
	}
	// The middle hangs below the ends, the ends exactly where they were put
	EXPECT_LT(rope.nodes.at(k_NodeCount / 2).position.y, k_Start.y - 1.0f);
	EXPECT_EQ(rope.start, k_Start);
	EXPECT_FLOAT_EQ(rope.tension, 0.0f);
}

TEST(LeashRope, PullingTheEndAwayTightensIt)
{
	auto rope = Create(k_Start, k_Start + glm::vec3(25.0f, 0.0f, 0.0f), k_Slack, k_Max, {});
	auto end = rope.end;
	for (int frame = 0; frame < 60; ++frame)
	{
		end.x += 2.0f;
		Step(rope, rope.start, end, 1.0f / 30.0f, Flat);
	}
	// The ends are 145 apart, far beyond the rope's full length
	EXPECT_GT(rope.tension, 0.8f);
}

TEST(LeashRope, StaysOffTheGround)
{
	const auto high = [](glm::vec2 /*point*/) { return 45.0f; };
	auto rope = Create(k_Start, k_Start + glm::vec3(10.0f, 0.0f, 0.0f), k_Slack, k_Max, {});
	for (int frame = 0; frame < 120; ++frame)
	{
		Step(rope, rope.start, rope.end, 1.0f / 30.0f, high);
	}
	const auto floor = 45.0f + Look {}.halfWidth + k_GroundClearance;
	EXPECT_TRUE(std::ranges::all_of(rope.nodes, [floor](const Node& node) { return node.position.y >= floor - 1e-3f; }));
}

TEST(LeashRope, NothingFasterThanItsSpeedCap)
{
	auto rope = Create(k_Start, k_Start + glm::vec3(10.0f, 0.0f, 0.0f), k_Slack, k_Max, {});
	// Yanked far away in one frame
	Step(rope, rope.start, k_Start + glm::vec3(3000.0f, 0.0f, 0.0f), 0.05f, Flat);
	EXPECT_TRUE(
	    std::ranges::all_of(rope.nodes, [](const Node& node) { return glm::length(node.velocity) <= k_MaxSpeed + 1e-2f; }));
}

TEST(LeashRope, NoTimeMeansOnlyTheEndsMove)
{
	auto rope = Create(k_Start, k_Start + glm::vec3(10.0f, 0.0f, 0.0f), k_Slack, k_Max, {});
	const auto before = rope.nodes;
	Step(rope, k_Start + glm::vec3(0.0f, 5.0f, 0.0f), rope.end, 0.0f, Flat);
	EXPECT_EQ(rope.start, k_Start + glm::vec3(0.0f, 5.0f, 0.0f));
	for (size_t i = 0; i < k_NodeCount; ++i)
	{
		EXPECT_EQ(rope.nodes.at(i).position, before.at(i).position);
	}
}

TEST(LeashRope, EndsAreKeptWithinTheWorld)
{
	EXPECT_EQ(ClampEnd({-5000.0f, -3.0f, 9000.0f}), glm::vec3(k_MinAcross, k_MinHeight, k_MaxAcross));
}

TEST(LeashRope, RibbonFacesTheEyeAndShadowLiesOnTheLand)
{
	const auto rope = Create(k_Start, k_Start + glm::vec3(41.0f, 0.0f, 0.0f), k_Slack, k_Max, {});
	// Looking straight down on a rope running along x: the ribbon spreads along z, half its width each side
	const auto ribbon = BuildRibbon(rope, k_Start + glm::vec3(20.0f, 100.0f, 0.0f), Flat);
	const auto& a = ribbon.rope.at(2);
	const auto& b = ribbon.rope.at(3);
	EXPECT_NEAR(glm::distance(a.position, b.position), 2.0f * Look {}.halfWidth, k_Tolerance);
	EXPECT_NEAR(a.position.x, b.position.x, k_Tolerance);
	EXPECT_FLOAT_EQ(a.uv.y, Look {}.v1);
	EXPECT_FLOAT_EQ(b.uv.y, Look {}.v0);
	// The texture runs along the rope's length: one unit is 2.5 x 0.05 of it
	EXPECT_NEAR(a.uv.x, 0.125f, k_Tolerance);
	EXPECT_NEAR(ribbon.rope.back().uv.x, 41.0f * 0.125f, 1e-3f);
	// The shadow sits just over the land, faint, and fades out at the ends
	for (const auto& corner : ribbon.shadow)
	{
		EXPECT_NEAR(corner.position.y, k_ShadowLift, k_Tolerance);
	}
	EXPECT_FLOAT_EQ(ribbon.shadow.front().alpha, 0.0f);
	EXPECT_FLOAT_EQ(ribbon.shadow.back().alpha, 0.0f);
	EXPECT_NEAR(ribbon.shadow.at(10).alpha, 65.0f / 255.0f, k_Tolerance);
}

TEST(LeashRope, RibbonTrianglesCoverEverySegment)
{
	const auto indices = RibbonIndices();
	EXPECT_EQ(indices.size(), (k_PointCount - 1) * 6);
	EXPECT_EQ(*std::ranges::max_element(indices), k_RibbonVertexCount - 1);
}

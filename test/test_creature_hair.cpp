/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>
#include <cstdint>

#include <array>
#include <numbers>
#include <vector>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Creature/CreatureHair.h"

using namespace openblack;
using namespace openblack::creature_hair;

namespace
{
constexpr float k_Tolerance = 1e-4f;
constexpr auto k_Pi = std::numbers::pi_v<float>;

const Variants k_Looks {{
    {.colour = {155, 31, 0}, .length = 0.16f, .damping = 0.0825f, .stiffness = 0.14f, .thickness = 0.047f},
    {.colour = {117, 23, 54}, .length = 0.37f, .damping = 0.1025f, .stiffness = 0.09f, .thickness = 0.11f},
    {.colour = {210, 142, 52}, .length = 0.14f, .damping = 0.0425f, .stiffness = 0.21f, .thickness = 0.05f},
}};

/// A strand hanging from a root on a flat ceiling, its surface facing down
Root CeilingRoot(const glm::vec3& position)
{
	return {.position = position, .inwardNormal = {0.0f, 1.0f, 0.0f}, .direction = {0.0f, -1.0f, 0.0f}};
}

/// A strand growing sideways out of a wall facing +x
Root WallRoot(const glm::vec3& position)
{
	return {.position = position, .inwardNormal = {-1.0f, 0.0f, 0.0f}, .direction = {1.0f, 0.0f, 0.0f}};
}

void ExpectNear(const glm::vec3& actual, const glm::vec3& expected, float tolerance = k_Tolerance)
{
	EXPECT_NEAR(actual.x, expected.x, tolerance);
	EXPECT_NEAR(actual.y, expected.y, tolerance);
	EXPECT_NEAR(actual.z, expected.z, tolerance);
}
} // namespace

TEST(CreatureHair, ValuesMoveFromNeutralTowardsEvilOrGood)
{
	constexpr std::array k_Values {1.0f, 3.0f, -1.0f};
	EXPECT_FLOAT_EQ(ByAlignment(k_Values, 0.0f), 1.0f);
	EXPECT_FLOAT_EQ(ByAlignment(k_Values, -1.0f), 3.0f);
	EXPECT_FLOAT_EQ(ByAlignment(k_Values, 1.0f), -1.0f);
	EXPECT_FLOAT_EQ(ByAlignment(k_Values, -0.5f), 2.0f);
	EXPECT_FLOAT_EQ(ByAlignment(k_Values, 0.25f), 0.5f);
}

TEST(CreatureHair, WholeNumbersAreTruncated)
{
	constexpr std::array<int32_t, 3> k_Values {155, 117, 210};
	// 155 * 0.5 + 117 * 0.5 = 136
	EXPECT_EQ(ByAlignment(k_Values, -0.5f), 136);
	// 155 * 0.9 + 210 * 0.1 = 160.5
	EXPECT_EQ(ByAlignment(k_Values, 0.1f), 160);
}

TEST(CreatureHair, LookFollowsAlignment)
{
	const auto neutral = LookFor(k_Looks, 0.0f);
	EXPECT_EQ(neutral.colour, glm::ivec3(155, 31, 0));
	EXPECT_FLOAT_EQ(neutral.length, 0.16f);

	const auto evil = LookFor(k_Looks, -1.0f);
	EXPECT_EQ(evil.colour, glm::ivec3(117, 23, 54));
	EXPECT_FLOAT_EQ(evil.length, 0.37f);
	EXPECT_FLOAT_EQ(evil.thickness, 0.11f);

	const auto good = LookFor(k_Looks, 1.0f);
	EXPECT_EQ(good.colour, glm::ivec3(210, 142, 52));
	EXPECT_FLOAT_EQ(good.stiffness, 0.21f);
	EXPECT_FLOAT_EQ(good.damping, 0.0425f);
}

TEST(CreatureHair, ScaleGrowsWithSizeButHeadsShrink)
{
	EXPECT_FLOAT_EQ(HairScale(1.0f), 1.3f * 15.0f);
	EXPECT_FLOAT_EQ(HairScale(0.5f), 1.55f * 0.5f * 15.0f);
	// Past size 2 the head no longer shrinks for its size
	EXPECT_FLOAT_EQ(HairScale(4.0f), 0.8f * 4.0f * 15.0f);
	EXPECT_FLOAT_EQ(HairScale(0.0f), 0.0f);
}

TEST(CreatureHair, PhysicsAtAScale)
{
	const auto look = LookFor(k_Looks, 0.0f);
	const auto physics = PhysicsFor(look, 20.0f, 4);
	EXPECT_FLOAT_EQ(physics.segmentLength, 0.16f * 20.0f / 4.0f);
	EXPECT_FLOAT_EQ(physics.stiffness, 0.14f * 1000.0f / 20.0f / 0.16f);
	EXPECT_FLOAT_EQ(physics.damping, (0.0825f * 0.16f) * (0.0825f * 0.16f));
	EXPECT_FLOAT_EQ(physics.rootDepth, 0.047f * 20.0f * 0.5f);
	EXPECT_FLOAT_EQ(physics.halfWidth, physics.rootDepth);
}

TEST(CreatureHair, StraightStrandStartsFromItsSunkenRoot)
{
	const Physics physics {.segmentLength = 2.0f, .stiffness = 0.0f, .damping = 1.0f, .rootDepth = 0.5f, .halfWidth = 0.5f};
	const auto strand = Straight(WallRoot({0.0f, 10.0f, 0.0f}), physics, 4);
	ASSERT_EQ(strand.positions.size(), 4u);
	ExpectNear(strand.positions[0], {-0.5f, 10.0f, 0.0f});
	ExpectNear(strand.positions[3], {5.5f, 10.0f, 0.0f});
	for (const auto& velocity : strand.velocities)
	{
		ExpectNear(velocity, glm::vec3(0.0f));
	}
}

TEST(CreatureHair, RootFollowsTheBody)
{
	const Physics physics {.segmentLength = 1.0f, .stiffness = 50.0f, .damping = 0.5f, .rootDepth = 0.25f, .halfWidth = 0.25f};
	auto strand = Straight(CeilingRoot({0.0f, 0.0f, 0.0f}), physics, 3);
	Step(strand, CeilingRoot({3.0f, 1.0f, -2.0f}), physics, 1.0f / 30.0f);
	ExpectNear(strand.positions[0], {3.0f, 1.25f, -2.0f});
}

TEST(CreatureHair, StrandNeverStretches)
{
	const Physics physics {.segmentLength = 1.5f, .stiffness = 40.0f, .damping = 0.2f, .rootDepth = 0.0f, .halfWidth = 0.1f};
	auto strand = Straight(WallRoot({0.0f, 0.0f, 0.0f}), physics, 6);
	for (int frame = 0; frame < 60; ++frame)
	{
		// The body swings the root about
		const auto t = static_cast<float>(frame) / 10.0f;
		Step(strand, WallRoot({std::sin(t) * 3.0f, std::cos(t), 0.0f}), physics, 1.0f / 30.0f);
		for (size_t i = 1; i < strand.positions.size(); ++i)
		{
			EXPECT_NEAR(glm::distance(strand.positions[i], strand.positions[i - 1]), 1.5f, 1e-3f);
		}
	}
}

TEST(CreatureHair, LimpStrandSagsUnderGravity)
{
	const Physics physics {.segmentLength = 1.0f, .stiffness = 0.0f, .damping = 1.0f, .rootDepth = 0.0f, .halfWidth = 0.1f};
	auto strand = Straight(WallRoot({0.0f, 0.0f, 0.0f}), physics, 4);
	for (int frame = 0; frame < 30; ++frame)
	{
		Step(strand, WallRoot({0.0f, 0.0f, 0.0f}), physics, 1.0f / 30.0f);
	}
	EXPECT_LT(strand.positions.back().y, -0.5f);
	// It falls straight down, staying in its plane
	EXPECT_NEAR(strand.positions.back().z, 0.0f, k_Tolerance);
}

TEST(CreatureHair, StiffStrandHoldsItsShape)
{
	const Physics stiff {.segmentLength = 1.0f, .stiffness = 400.0f, .damping = 0.01f, .rootDepth = 0.0f, .halfWidth = 0.1f};
	const Physics limp {.segmentLength = 1.0f, .stiffness = 0.0f, .damping = 0.01f, .rootDepth = 0.0f, .halfWidth = 0.1f};
	auto stiffStrand = Straight(WallRoot({0.0f, 0.0f, 0.0f}), stiff, 4);
	auto limpStrand = Straight(WallRoot({0.0f, 0.0f, 0.0f}), limp, 4);
	for (int frame = 0; frame < 90; ++frame)
	{
		Step(stiffStrand, WallRoot({0.0f, 0.0f, 0.0f}), stiff, 1.0f / 30.0f);
		Step(limpStrand, WallRoot({0.0f, 0.0f, 0.0f}), limp, 1.0f / 30.0f);
	}
	EXPECT_GT(stiffStrand.positions.back().y, limpStrand.positions.back().y);
	EXPECT_GT(stiffStrand.positions.back().x, 2.0f);
}

TEST(CreatureHair, SpeedsAreHowFarThePointsMoved)
{
	const Physics physics {.segmentLength = 1.0f, .stiffness = 10.0f, .damping = 0.5f, .rootDepth = 0.0f, .halfWidth = 0.1f};
	auto strand = Straight(WallRoot({0.0f, 0.0f, 0.0f}), physics, 3);
	const auto before = strand.positions;
	constexpr float k_Seconds = 0.05f;
	Step(strand, WallRoot({0.0f, 0.0f, 0.0f}), physics, k_Seconds);
	for (size_t i = 1; i < strand.positions.size(); ++i)
	{
		ExpectNear(strand.velocities[i], (strand.positions[i] - before[i]) / k_Seconds);
	}
}

TEST(CreatureHair, NoTimeLeavesTheSpeeds)
{
	const Physics physics {.segmentLength = 1.0f, .stiffness = 10.0f, .damping = 1.0f, .rootDepth = 0.0f, .halfWidth = 0.1f};
	auto strand = Straight(WallRoot({0.0f, 0.0f, 0.0f}), physics, 3);
	strand.velocities[2] = {0.0f, 0.0f, 4.0f};
	Step(strand, WallRoot({0.0f, 0.0f, 0.0f}), physics, 0.0f);
	ExpectNear(strand.velocities[2], {0.0f, 0.0f, 4.0f});
}

TEST(CreatureHair, SpeedIsCapped)
{
	const Physics physics {.segmentLength = 1.0f, .stiffness = 0.0f, .damping = 1.0f, .rootDepth = 0.0f, .halfWidth = 0.1f};
	auto strand = Straight(WallRoot({0.0f, 0.0f, 0.0f}), physics, 2);
	strand.velocities[1] = {0.0f, 0.0f, 1000.0f};
	// The point moves by at most the capped speed before it is put back a segment from the root
	Step(strand, WallRoot({0.0f, 0.0f, 0.0f}), physics, 0.001f);
	const auto moved = strand.positions[1] - glm::vec3(1.0f, 0.0f, 0.0f);
	EXPECT_LE(glm::length(moved), (k_MaxSpeed * 0.001f) + 1e-4f);
}

TEST(CreatureHair, StrandsGrowStraightOutOfTheSurface)
{
	ExpectNear(GrowthDirection({0.0f, 0.0f, 1.0f}), {0.0f, 0.0f, -1.0f});
	ExpectNear(GrowthDirection({0.0f, 0.0f, 1.0f}, glm::mat3(1.0f), glm::vec3(0.0f)), {0.0f, 0.0f, -1.0f});
}

TEST(CreatureHair, TurnedStrandsTurnInTheirBonesFrame)
{
	// Out along -z, turned about y by a quarter turn: the game's y rotation takes row vector (0, 0, -1) to (1, 0, 0)
	const auto turned = GrowthDirection({0.0f, 0.0f, 1.0f}, glm::mat3(1.0f), {0.0f, k_Pi / 2.0f, 0.0f});
	ExpectNear(turned, {1.0f, 0.0f, 0.0f});
	// The same turn in a frame rotated a quarter turn about x, its y axis along world z
	const glm::mat3 frame {{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};
	const auto inFrame = GrowthDirection({0.0f, -1.0f, 0.0f}, frame, {0.0f, k_Pi / 2.0f, 0.0f});
	EXPECT_NEAR(glm::length(inFrame), 1.0f, k_Tolerance);
	EXPECT_NEAR(glm::dot(inFrame, glm::vec3(0.0f, 1.0f, 0.0f)), 0.0f, k_Tolerance);
}

TEST(CreatureHair, BoneFrameAddsUnitAxesAndKeepsTheSkew)
{
	// Two bones, one scaled by 3 and one turned a quarter about z: each axis is made unit length before they are added,
	// so the scale counts for nothing, and the sum's axes are made unit length again but not set square
	const glm::mat3 scaled(3.0f);
	const glm::mat3 turned {{0.0f, 1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
	const std::array bones {scaled, turned};
	const auto frame = BoneFrame(bones);
	const auto half = std::sqrt(0.5f);
	ExpectNear(frame[0], {half, half, 0.0f});
	ExpectNear(frame[1], {-half, half, 0.0f});
	ExpectNear(frame[2], {0.0f, 0.0f, 1.0f});

	// A skewed pair: the frame's x and y axes come out unit length but not at right angles
	const glm::mat3 sheared {{1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
	const std::array skewed {glm::mat3(1.0f), sheared};
	const auto skewedFrame = BoneFrame(skewed);
	EXPECT_NEAR(glm::length(skewedFrame[0]), 1.0f, k_Tolerance);
	EXPECT_NEAR(glm::length(skewedFrame[1]), 1.0f, k_Tolerance);
	EXPECT_GT(glm::dot(skewedFrame[0], skewedFrame[1]), 0.1f);
}

TEST(CreatureHair, SkewedFramesTurnByTheirInverse)
{
	// In a skewed frame the direction is taken in by the frame's inverse, turned and taken back out: with no turn it
	// comes back as it was, which the transpose would not give
	const glm::mat3 skewed {{1.0f, 0.0f, 0.0f}, glm::normalize(glm::vec3(1.0f, 1.0f, 0.0f)), {0.0f, 0.0f, 1.0f}};
	const glm::vec3 inward = glm::normalize(glm::vec3(0.3f, -1.0f, 0.2f));
	ExpectNear(GrowthDirection(inward, skewed, glm::vec3(0.0f)), -inward);
	// A half turn about the frame's z axis: up is (-1, 1.414) in the frame's axes, turned to (1, -1.414), which is down
	ExpectNear(GrowthDirection({0.0f, -1.0f, 0.0f}, skewed, {0.0f, 0.0f, k_Pi}), {0.0f, -1.0f, 0.0f});
}

TEST(CreatureHair, UnnormalisedNormalSinksTheRootDeeperOnBiggerTriangles)
{
	const Physics physics {.segmentLength = 1.0f, .stiffness = 0.0f, .damping = 1.0f, .rootDepth = 0.5f, .halfWidth = 0.5f};
	// A normal twice as long, from a triangle twice the area, sinks the root twice as deep
	const Root root {
	    .position = {0.0f, 0.0f, 0.0f}, .inwardNormal = {0.0f, 2.0f, 0.0f}, .direction = GrowthDirection({0.0f, 2.0f, 0.0f})};
	const auto strand = Straight(root, physics, 2);
	ExpectNear(strand.positions[0], {0.0f, 1.0f, 0.0f});
	ExpectNear(root.direction, {0.0f, -1.0f, 0.0f});
}

TEST(CreatureHair, StrandsTakeTheLandsLightAndColour)
{
	// The colour scaled by the light, in whole numbers, then the land's colour and the haze added
	EXPECT_EQ(StrandColour({200, 100, 50}, {255, 255, 255}, {0, 0, 0}), glm::ivec3(200, 100, 50));
	EXPECT_EQ(StrandColour({200, 100, 50}, {128, 64, 255}, {0, 0, 0}), glm::ivec3(100, 25, 50));
	EXPECT_EQ(StrandColour({200, 100, 50}, {128, 64, 255}, {10, 20, 30}), glm::ivec3(110, 45, 80));
	// Night: a dark blue light leaves little of a ginger strand
	EXPECT_EQ(StrandColour({155, 31, 0}, {40, 50, 90}, {0, 0, 0}), glm::ivec3(24, 6, 0));
	// At most white
	EXPECT_EQ(StrandColour({250, 250, 250}, {255, 255, 255}, {40, 0, 255}), glm::ivec3(255, 250, 255));
}

TEST(CreatureHair, RibbonFacesTheEye)
{
	const std::vector<glm::vec3> points {{0.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, -2.0f, 0.0f}};
	std::vector<RibbonVertex> ribbon(points.size() * 2);
	const glm::vec3 eye {0.0f, -1.0f, 10.0f};
	BuildRibbon(points, eye, 0.5f, ribbon);
	for (size_t i = 0; i < points.size(); ++i)
	{
		const auto across = ribbon[i * 2].position - ribbon[(i * 2) + 1].position;
		EXPECT_NEAR(glm::length(across), 1.0f, k_Tolerance);
		// Across the strand and square to the eye
		EXPECT_NEAR(across.y, 0.0f, k_Tolerance);
		EXPECT_NEAR(across.z, 0.0f, k_Tolerance);
		ExpectNear((ribbon[i * 2].position + ribbon[(i * 2) + 1].position) * 0.5f, points[i]);
		EXPECT_FLOAT_EQ(ribbon[i * 2].uv.x, static_cast<float>(i) / 2.0f);
		EXPECT_FLOAT_EQ(ribbon[i * 2].uv.y, 0.0f);
		EXPECT_FLOAT_EQ(ribbon[(i * 2) + 1].uv.y, k_RibbonTextureHeight);
	}
}

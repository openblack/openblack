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

#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <gtest/gtest.h>

#include "Magic/HandHoldPose.h"

using namespace openblack;
using namespace openblack::magic::hand_hold;
using Cycle = HandAnimation::Cycle;

namespace
{
constexpr float k_Epsilon = 1e-4f;
constexpr uint32_t k_HoldSideMs = 533;
constexpr uint32_t k_HoldAboveMs = 533;
constexpr uint32_t k_WiggleMs = 266;

void ExpectNear(glm::vec3 a, glm::vec3 b)
{
	EXPECT_NEAR(a.x, b.x, k_Epsilon);
	EXPECT_NEAR(a.y, b.y, k_Epsilon);
	EXPECT_NEAR(a.z, b.z, k_Epsilon);
}
} // namespace

TEST(HandHoldPose, ASeedIsHeldAsAMiracleNotYetReadyUntilItIsReady)
{
	EXPECT_EQ(HoldTypeOf(false, HoldType::Side), HoldType::Magic);
	EXPECT_EQ(HoldTypeOf(true, HoldType::Side), HoldType::Side);
	EXPECT_EQ(HoldTypeOf(true, HoldType::Above), HoldType::Above);
}

TEST(HandHoldPose, EachHoldTakesAStillFrameOfItsAnimation)
{
	EXPECT_EQ(HoldCycle(HoldType::Above), Cycle::HoldAbove);
	EXPECT_EQ(HoldCycle(HoldType::Magic), Cycle::Wiggle);
	EXPECT_EQ(HoldCycle(HoldType::Side), Cycle::HoldSide);
	EXPECT_EQ(HoldCycle(HoldType::Tree), Cycle::HoldSide);
	EXPECT_EQ(HoldCycle(HoldType::Villager), Cycle::HoldSide);
	EXPECT_EQ(HoldCycle(HoldType::Grain), Cycle::Horn);
	EXPECT_FALSE(HoldCycle(HoldType::Fingers).has_value());

	// At the standard size: food (scale 0.8, radius 1), wood (0.65) and the physical shield (1.0) at the side
	EXPECT_EQ(HoldTimeMs(HoldType::Side, k_HoldSideMs, 0.8f, 1.0f), 66u);
	EXPECT_EQ(HoldTimeMs(HoldType::Side, k_HoldSideMs, 0.65f, 1.0f), 54u);
	EXPECT_EQ(HoldTimeMs(HoldType::Side, k_HoldSideMs, 1.0f, 1.0f), 83u);
	// The forest (1.0) and the flocks (2.6 and 1.6 times a radius of 0.4) above
	EXPECT_EQ(HoldTimeMs(HoldType::Above, k_HoldAboveMs, 1.0f, 1.0f), 197u);
	EXPECT_EQ(HoldTimeMs(HoldType::Above, k_HoldAboveMs, 2.6f * 0.4f, 1.0f), 194u);
	EXPECT_EQ(HoldTimeMs(HoldType::Above, k_HoldAboveMs, 1.6f * 0.4f, 1.0f), 222u);
	// Every miracle not yet ready, and every one without a model, takes the middle of the rest pose
	EXPECT_EQ(HoldTimeMs(HoldType::Magic, k_WiggleMs, 0.8f, 1.0f), 133u);
	// A smaller hand, closer to the camera, opens wider round the same seed
	EXPECT_EQ(HoldTimeMs(HoldType::Side, k_HoldSideMs, 0.8f, 0.5f), 133u);
	// Never past the middle of the animation
	EXPECT_EQ(HoldTimeMs(HoldType::Side, k_HoldSideMs, 100.0f, 1.0f), 266u);
	EXPECT_EQ(HoldTimeMs(HoldType::Above, k_HoldAboveMs, 100.0f, 1.0f), 0u);
}

TEST(HandHoldPose, TheHandRisesByHowItHolds)
{
	// Food's horn hangs 0.7 of its height below the hand point
	EXPECT_FLOAT_EQ(SeedHang(HoldType::Side, 0.7f, 4.0f), 2.8f);
	EXPECT_FLOAT_EQ(SeedHang(HoldType::Magic, 0.5f, 4.0f), 0.0f);
	// The forest sits a tenth of its height above
	EXPECT_FLOAT_EQ(SeedHang(HoldType::Above, -0.1f, 5.0f), -0.5f);

	EXPECT_FLOAT_EQ(HoldLift(HoldType::Above, -0.5f, 1.0f), 0.2f);
	EXPECT_FLOAT_EQ(HoldLift(HoldType::Magic, 0.0f, 0.5f), 1.6f);
	EXPECT_FLOAT_EQ(HoldLift(HoldType::Side, 2.8f, 1.0f), 2.8f);
	EXPECT_FLOAT_EQ(HoldLift(HoldType::Side, 0.4f, 1.0f), 1.9f);
}

TEST(HandHoldPose, TheCursorRunningAheadSwaysTheHandUpToThreeTenthsOfARadian)
{
	const auto sway = CursorSway({40.0f, -80.0f});
	EXPECT_NEAR(sway.x, 0.15f, k_Epsilon);
	EXPECT_NEAR(sway.y, -0.3f, k_Epsilon);
	EXPECT_NEAR(CursorSway({500.0f, 0.0f}).x, 0.3f, k_Epsilon);
}

TEST(HandHoldPose, TheHandRollsBackAboutTheLineToTheCamera)
{
	// The camera due south of the hand, level with it: rolling turns up about the north-south line
	const glm::vec3 camera {0.0f, 0.0f, -10.0f};
	const glm::vec3 hand {0.0f, 0.0f, 0.0f};
	ExpectNear(HeldUp(camera, hand, 0.0f, 0.0f), {0.0f, 1.0f, 0.0f});
	const float angle = 0.5f;
	// Up turned by minus the roll about the line from the hand to the camera (towards -z): towards -x
	ExpectNear(HeldUp(camera, hand, angle, 0.0f), {-std::sin(angle), std::cos(angle), 0.0f});
	// The pitch turns it back about the level line across that, (1, 0, 0) here
	ExpectNear(HeldUp(camera, hand, 0.0f, angle), {0.0f, std::cos(angle), -std::sin(angle)});
	// The camera on the hand: only shrinks
	ExpectNear(HeldUp(hand, hand, angle, 0.0f), {0.0f, std::cos(angle), 0.0f});
}

TEST(HandHoldPose, UprightTheSeedTakesTheHandsTurn)
{
	const auto turn = glm::mat3(glm::eulerAngleY(0.7f));
	const auto basis = HeldBasis(turn, {0.0f, 1.0f, 0.0f});
	for (glm::length_t c = 0; c < 3; ++c)
	{
		ExpectNear(basis[c], turn[c]);
	}
	// Tipped, its up is the up given and its axes stay square
	const auto up = glm::normalize(glm::vec3(0.3f, 1.0f, 0.1f));
	const auto tipped = HeldBasis(turn, up);
	ExpectNear(tipped[1], up);
	EXPECT_NEAR(glm::dot(tipped[0], tipped[1]), 0.0f, k_Epsilon);
	EXPECT_NEAR(glm::dot(tipped[0], tipped[2]), 0.0f, k_Epsilon);
	EXPECT_NEAR(glm::length(tipped[0]), 1.0f, k_Epsilon);
	// Its side lies across the level way the hand faces
	EXPECT_NEAR(glm::dot(tipped[0], glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), turn[0])), 0.0f, k_Epsilon);
}

TEST(HandHoldPose, TheSeedTurnsHalfRoundForARightHandAndByItsOwnTurn)
{
	const glm::mat3 basis {1.0f};
	const auto left = SeedTurn(basis, 0.0f, false);
	ExpectNear(left[0], {1.0f, 0.0f, 0.0f});
	const auto right = SeedTurn(basis, 0.0f, true);
	ExpectNear(right[0], {-1.0f, 0.0f, 0.0f});
	ExpectNear(right[2], {0.0f, 0.0f, -1.0f});
	ExpectNear(right[1], {0.0f, 1.0f, 0.0f});
	// The ground flock turned a quarter round
	const auto quarter = SeedTurn(basis, std::numbers::pi_v<float> * 0.5f, false);
	ExpectNear(quarter[0], {0.0f, 0.0f, 1.0f});
	ExpectNear(quarter[2], {-1.0f, 0.0f, 0.0f});
}

TEST(HandHoldPose, TakingASeedFadesTheDrawnHandOverThirteenHundredths)
{
	HandFade fade;
	fade.Start({0.0f, 0.0f, 0.0f}, glm::mat3(1.0f));
	glm::vec3 position {13.0f, 0.0f, 0.0f};
	glm::mat3 rotation {2.0f};
	fade.Step(0.065f, position, rotation);
	EXPECT_NEAR(position.x, 6.5f, 1e-3f);
	EXPECT_NEAR(rotation[0][0], 1.5f, 1e-3f);
	EXPECT_TRUE(fade.Fading());
	glm::vec3 later {13.0f, 0.0f, 0.0f};
	glm::mat3 laterRotation {2.0f};
	fade.Step(0.07f, later, laterRotation);
	EXPECT_FLOAT_EQ(later.x, 13.0f);
	EXPECT_FALSE(fade.Fading());
}

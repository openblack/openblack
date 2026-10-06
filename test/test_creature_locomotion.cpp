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
#include <gtest/gtest.h>

#include "Creature/CreatureLocomotion.h"

using namespace openblack;
using namespace openblack::creature_locomotion;

namespace
{
constexpr float k_Tolerance = 1e-4f;
constexpr float k_Pi = std::numbers::pi_v<float>;

float Degrees(float degrees)
{
	return degrees * k_Pi / 180.0f;
}

// A made-up species: a stand of 2 s, a walk of 2.4 s striding 70 mesh units, a run of 1 s striding 130
constexpr Cycle k_Stand {.durationMs = 2000.0f, .stride = 0.0f};
constexpr Cycle k_Walk {.durationMs = 2400.0f, .stride = 70.0f};
constexpr Cycle k_Run {.durationMs = 1000.0f, .stride = 130.0f};
} // namespace

TEST(CreatureLocomotion, SpeedsGrowWithSize)
{
	const auto one = SpeedsFor(1.0f);
	EXPECT_NEAR(one.walk, 8.0f * 1.125f, k_Tolerance);
	EXPECT_NEAR(one.run, 20.0f * 1.125f, k_Tolerance);
	const auto two = SpeedsFor(2.0f);
	EXPECT_NEAR(two.walk, 8.0f * 2.0f, k_Tolerance);
	// Sizes are kept within 0.05 and 4
	EXPECT_NEAR(SpeedsFor(10.0f).run, SpeedsFor(4.0f).run, k_Tolerance);
	EXPECT_NEAR(SpeedsFor(0.0f).walk, 8.0f * ((0.875f * 0.05f) + 0.25f), k_Tolerance);
}

TEST(CreatureLocomotion, TargetSpeedIsAFractionOfALittleOverRunning)
{
	EXPECT_NEAR(TargetSpeed(1.0f, 20.0f), 22.0f, k_Tolerance);
	EXPECT_NEAR(TargetSpeed(0.5f, 20.0f), 11.0f, k_Tolerance);
	EXPECT_NEAR(TargetSpeed(2.0f, 20.0f), 22.0f, k_Tolerance);
	EXPECT_NEAR(TargetSpeed(-1.0f, 20.0f), 0.0f, k_Tolerance);
}

TEST(CreatureLocomotion, ExhaustedCreaturesGoSlowly)
{
	EXPECT_NEAR(RequiredFraction(0.9f, 0.5f, 0.79f), 0.9f, k_Tolerance);
	EXPECT_NEAR(RequiredFraction(0.9f, 0.5f, 0.8f), 0.5f, k_Tolerance);
}

TEST(CreatureLocomotion, PullingTheLeashSpeedsItUp)
{
	EXPECT_NEAR(LeashFraction(0.7f, 0.9f, 0.0f), 0.7f, k_Tolerance);
	EXPECT_NEAR(LeashFraction(0.7f, 0.9f, 1.0f), 1.8f, k_Tolerance);
}

TEST(CreatureLocomotion, AcceleratesAtTwelveUnitsASecondEachSecond)
{
	EXPECT_NEAR(Accelerate(0.0f, 10.0f, 0.1f), 1.2f, k_Tolerance);
	EXPECT_NEAR(Accelerate(9.5f, 10.0f, 0.1f), 10.0f, k_Tolerance);
	// It never slows down towards a lower target by itself
	EXPECT_NEAR(Accelerate(15.0f, 10.0f, 0.1f), 15.0f, k_Tolerance);
	// From standing it takes nine turns to reach 10
	float speed = 0.0f;
	int turns = 0;
	while (speed < 10.0f)
	{
		speed = Accelerate(speed, 10.0f, 0.1f);
		++turns;
	}
	EXPECT_EQ(turns, 9);
}

TEST(CreatureLocomotion, BrakesToStopInTheDistanceLeft)
{
	EXPECT_NEAR(StopCap(6.0f), 12.0f, k_Tolerance);
	EXPECT_NEAR(StopCap(0.0f), k_MinSpeed, k_Tolerance);
	EXPECT_NEAR(CornerCap(1.0f, 2.0f), std::sqrt(24.0f + 24.0f), k_Tolerance);
	// Braking each turn to the cap stops it right at the end
	float remaining = 30.0f;
	float speed = 20.0f;
	while (remaining > 0.0f && speed > k_MinSpeed)
	{
		speed = std::min(speed, StopCap(remaining));
		remaining -= speed * 0.1f;
	}
	EXPECT_GT(remaining, -1.5f);
}

TEST(CreatureLocomotion, UphillIsSlowerDownhillALittleFaster)
{
	EXPECT_NEAR(SlopeFactor(10.0f, 10.0f, 5.0f), 1.0f, k_Tolerance);
	EXPECT_NEAR(SlopeFactor(12.5f, 10.0f, 5.0f), 0.7f, k_Tolerance);
	EXPECT_NEAR(SlopeFactor(100.0f, 10.0f, 5.0f), k_MinSlopeFactor, k_Tolerance);
	EXPECT_NEAR(SlopeFactor(0.0f, 10.0f, 5.0f), k_MaxSlopeFactor, k_Tolerance);
}

TEST(CreatureLocomotion, HeadingsFaceTheWayTheyPoint)
{
	for (const float heading : {0.0f, 1.0f, -2.0f, 3.0f})
	{
		EXPECT_NEAR(HeadingOf(DirectionOf(heading)), heading, k_Tolerance);
	}
	// Heading 0 looks along -z, as the meshes do
	EXPECT_NEAR(DirectionOf(0.0f).y, -1.0f, k_Tolerance);
	EXPECT_NEAR(WrapAngle(Degrees(270.0f)), Degrees(-90.0f), k_Tolerance);
	EXPECT_NEAR(WrapAngle(Degrees(-190.0f)), Degrees(170.0f), k_Tolerance);
	// Halfway round the short way, through 180 degrees
	EXPECT_NEAR(std::abs(LerpHeading(Degrees(170.0f), Degrees(-170.0f), 500.0f, 1000.0f)), k_Pi, k_Tolerance);
	EXPECT_NEAR(LerpHeading(0.0f, 1.0f, 2000.0f, 1000.0f), 1.0f, k_Tolerance);
}

TEST(CreatureLocomotion, SlowerThanWalkingBlendsStandAndWalk)
{
	const Speeds speeds {.walk = 9.0f, .run = 22.5f};
	const auto gait = BlendGait(4.5f, speeds, 0.45f, 0.2f, k_Stand, k_Walk, k_Run, 0.0f, 0.25f);
	EXPECT_EQ(gait.slots[0].animation, animations::k_Stand);
	EXPECT_EQ(gait.slots[1].animation, animations::k_Walk);
	EXPECT_NEAR(gait.slots[0].weight, 0.5f, k_Tolerance);
	EXPECT_NEAR(gait.slots[1].weight, 0.5f, k_Tolerance);
	// The stand follows the breath
	EXPECT_NEAR(gait.slots[0].timeMs, 500.0f, k_Tolerance);
	// The blend's stride is half the walk's: 0.5 * 70 * 0.2 = 7 units, so 0.45 units is 0.45 / 7 of the walk
	EXPECT_NEAR(gait.walkAdvanceMs, 2400.0f * 0.45f / 7.0f, 1e-2f);
	EXPECT_NEAR(gait.slots[1].timeMs, gait.walkAdvanceMs, 1e-2f);
}

TEST(CreatureLocomotion, FasterThanWalkingBlendsWalkAndRunInStep)
{
	const Speeds speeds {.walk = 10.0f, .run = 20.0f};
	const auto gait = BlendGait(15.0f, speeds, 1.5f, 0.1f, k_Stand, k_Walk, k_Run, 2000.0f, 0.0f);
	EXPECT_EQ(gait.slots[0].animation, animations::k_Run);
	EXPECT_EQ(gait.slots[1].animation, animations::k_Walk);
	EXPECT_NEAR(gait.slots[0].weight, 0.5f, k_Tolerance);
	EXPECT_NEAR(gait.slots[1].weight, 0.5f, k_Tolerance);
	// Stride (0.5 * 70 + 0.5 * 130) * 0.1 = 10 units, so 1.5 units is 15% of the walk, which loops
	EXPECT_NEAR(gait.walkAdvanceMs, 360.0f, 1e-2f);
	EXPECT_NEAR(gait.walkTimeMs, 2360.0f, 1e-2f);
	// The run keeps the walk's phase
	EXPECT_NEAR(gait.slots[0].timeMs / k_Run.durationMs, gait.slots[1].timeMs / k_Walk.durationMs, k_Tolerance);
	// Past running speed the weights stay within 0 and 1
	const auto flatOut = BlendGait(22.0f, speeds, 2.2f, 0.1f, k_Stand, k_Walk, k_Run, 0.0f, 0.0f);
	EXPECT_NEAR(flatOut.slots[0].weight, 1.0f, k_Tolerance);
	EXPECT_NEAR(flatOut.slots[1].weight, 0.0f, k_Tolerance);
}

TEST(CreatureLocomotion, StandingStillMovesNoFeet)
{
	const auto gait = BlendGait(0.0f, {.walk = 9.0f, .run = 22.0f}, 0.0f, 0.2f, k_Stand, k_Walk, k_Run, 300.0f, 0.5f);
	EXPECT_NEAR(gait.slots[0].weight, 1.0f, k_Tolerance);
	EXPECT_NEAR(gait.walkTimeMs, 300.0f, k_Tolerance);
	EXPECT_NEAR(gait.walkAdvanceMs, 0.0f, k_Tolerance);
}

TEST(CreatureLocomotion, AnglesPickTheirPairOfAnimations)
{
	const auto ahead = PairFor(animations::k_RightSpin, 0.0f);
	EXPECT_EQ(ahead.from, animations::k_RightSpin);
	EXPECT_NEAR(ahead.weight, 0.0f, k_Tolerance);
	const auto side = PairFor(animations::k_RightSpin, Degrees(45.0f));
	EXPECT_EQ(side.from, animations::k_RightSpin);
	EXPECT_EQ(side.to, animations::k_RightSpin + 1);
	EXPECT_NEAR(side.weight, 0.5f, k_Tolerance);
	const auto behind = PairFor(animations::k_RightSpin, Degrees(-135.0f));
	EXPECT_EQ(behind.from, animations::k_RightSpin + 1);
	EXPECT_EQ(behind.to, animations::k_RightSpin + 2);
	EXPECT_NEAR(behind.weight, 0.5f, k_Tolerance);
}

TEST(CreatureLocomotion, StartsByStepWalkOrTurn)
{
	const StartOptions steps {.stepStrides = std::array {46.0f, 42.0f, 40.0f}, .hasSpins = true, .moving = true};
	// Far enough for every step: it steps off, right for positive angles
	const auto step = ChooseStart(Degrees(100.0f), 50.0f, steps);
	EXPECT_EQ(step.kind, Start::Kind::Step);
	EXPECT_EQ(step.side, Side::Right);
	ASSERT_TRUE(step.animations.has_value());
	EXPECT_EQ(step.animations->from, animations::k_RightStep + 1);
	const auto left = ChooseStart(Degrees(-30.0f), 50.0f, steps);
	EXPECT_EQ(left.side, Side::Left);
	EXPECT_EQ(left.animations->from, animations::k_LeftStep);
	// Too close for a step: nearly facing it walks straight on, otherwise turns on the spot
	EXPECT_EQ(ChooseStart(Degrees(10.0f), 45.0f, steps).kind, Start::Kind::Walk);
	const auto turn = ChooseStart(Degrees(-60.0f), 45.0f, steps);
	EXPECT_EQ(turn.kind, Start::Kind::Turn);
	EXPECT_EQ(turn.animations->from, animations::k_LeftSpin);
	// Without spins the turn has no animations
	EXPECT_FALSE(ChooseStart(Degrees(60.0f), 10.0f, {.stepStrides = std::nullopt, .hasSpins = false, .moving = true})
	                 .animations.has_value());
	// Only turning to face, it never steps
	EXPECT_EQ(ChooseStart(Degrees(100.0f), 500.0f, {.stepStrides = steps.stepStrides, .hasSpins = true, .moving = false}).kind,
	          Start::Kind::Turn);
}

TEST(CreatureLocomotion, ArrivalRings)
{
	const auto ring = MoveRing(50.0f, 0.0f, 1.0f);
	EXPECT_NEAR(ring.min, 0.0f, k_Tolerance);
	EXPECT_NEAR(ring.max, 1.0f, k_Tolerance);
	// Never asked to stop beyond where it already is, and never thinner than a hair
	const auto close = MoveRing(3.0f, 10.0f, 2.0f);
	EXPECT_NEAR(close.min, 2.95f, k_Tolerance);
	EXPECT_NEAR(close.max, 2.951f, k_Tolerance);
	const auto object = ObjectRing(5.0f, 2.0f, 3.0f);
	EXPECT_NEAR(object.min, 7.0f, k_Tolerance);
	EXPECT_NEAR(object.max, 10.0f, k_Tolerance);
	EXPECT_TRUE(Arrived(9.0f, object));
	EXPECT_FALSE(Arrived(10.5f, object));
	EXPECT_NEAR(TimeLimitMs(100.0f), 5200.0f, k_Tolerance);
}

TEST(CreatureLocomotion, RunsAwayFromTheThreat)
{
	const auto point = RunAwayPoint({10.0f, 0.0f}, {0.0f, 0.0f}, 50.0f);
	EXPECT_NEAR(point.x, 60.0f, k_Tolerance);
	EXPECT_NEAR(point.y, 0.0f, k_Tolerance);
	EXPECT_NEAR(glm::length(RunAwayPoint({3.0f, 3.0f}, {3.0f, 3.0f}, 20.0f) - glm::vec2(3.0f, 3.0f)), 20.0f, k_Tolerance);
}

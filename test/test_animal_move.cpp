/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "3D/MapCoords.h"
#include "Animals/AnimalMove.h"
#include "Animals/Zoomer.h"

using namespace openblack;
using namespace openblack::animals;

namespace
{
constexpr float k_Epsilon = 1e-3f;
/// A speed state of 8 metres a second, about a metre a turn, and a turn angle of 17 game angles (3 degrees)
constexpr uint16_t k_Speed = 5243;
constexpr uint16_t k_TurnAngle = 17;
} // namespace

TEST(AnimalMove, ItTurnsByAtMostItsTurnAngle)
{
	// Facing +x, wanting +z far off: one turn angle the short way round
	auto turn = TurnTowards(0, 0x200, k_TurnAngle, k_Speed, 500.0f);
	EXPECT_EQ(turn.angle, k_TurnAngle);
	EXPECT_EQ(turn.step, k_TurnAngle);
	EXPECT_EQ(turn.direction, 1);
	turn = TurnTowards(0, 0x600, k_TurnAngle, k_Speed, 500.0f);
	EXPECT_EQ(turn.angle, 0x800 - k_TurnAngle);
	EXPECT_EQ(turn.step, -k_TurnAngle);
	EXPECT_EQ(turn.direction, -1);
	// Within its turn angle it faces the target
	turn = TurnTowards(0, 10, k_TurnAngle, k_Speed, 500.0f);
	EXPECT_EQ(turn.angle, 10);
	// Facing it already: no turn
	turn = TurnTowards(100, 100, k_TurnAngle, k_Speed, 500.0f);
	EXPECT_EQ(turn.direction, 0);
	EXPECT_EQ(turn.step, 0);
}

TEST(AnimalMove, CloseToItsGoalItTurnsHarder)
{
	// Its reach: twice a step (0.8 m) over 17 game angles (0.0522 rad), about 30.7 m; at half of it, it turns half of
	// the way from the whole difference down to the turn angle
	const auto far = TurnTowards(0, 0x200, k_TurnAngle, k_Speed, 1000.0f);
	const auto near = TurnTowards(0, 0x200, k_TurnAngle, k_Speed, 1.0f);
	EXPECT_EQ(far.step, k_TurnAngle);
	EXPECT_GT(near.step, 0x1C0);
	EXPECT_LE(near.step, 0x200);
}

TEST(AnimalMove, ItStepsItsSpeedAndArrivesTheTurnAfterComingWithinAStep)
{
	Move move {.position = {0, 0}, .speed = k_Speed};
	const glm::ivec2 goal {map_coords::ToFixed(3.0f), 0};
	(void)SetUpMove(move, goal, k_TurnAngle);
	EXPECT_EQ(move.stage, MoveStage::StepThrough);
	auto result = StepMove(move, k_TurnAngle);
	EXPECT_FALSE(result.arrived);
	// Its speed along +x, to a sixteenth of the speed
	EXPECT_EQ(move.position.x, (k_Speed >> 4) * 16);
	EXPECT_EQ(move.position.y, 0);
	int turns = 1;
	while (!StepMove(move, k_TurnAngle).arrived && turns < 10)
	{
		++turns;
	}
	EXPECT_EQ(move.position, goal);
	EXPECT_EQ(turns, 3);
}

TEST(AnimalMove, BirdsClimbSlowlyAndKeepOffTheLand)
{
	// 10 over flat land wanting 45: up by 0.2 a turn
	EXPECT_NEAR(FlyingHeight(0.0f, 10.0f, 0.0f, 0.0f, 45.0f, 0.2f), 10.2f, k_Epsilon);
	// Its height in the air holds over a hill, so it is lower above the hill
	EXPECT_NEAR(FlyingHeight(0.0f, 45.0f, 20.0f, 0.0f, 45.0f, 0.2f), 25.0f, k_Epsilon);
	// Within a change of its goal it holds its height rather than snapping onto it
	EXPECT_NEAR(FlyingHeight(0.0f, 45.1f, 0.0f, 0.0f, 45.0f, 0.2f), 45.1f, k_Epsilon);
	// Never below 2 above the land
	EXPECT_NEAR(FlyingHeight(0.0f, 10.0f, 30.0f, 0.0f, 10.0f, 0.2f), 2.0f, k_Epsilon);
	// A goal on the land: its height above the land itself steps down
	EXPECT_NEAR(FlyingHeight(0.0f, 1.0f, 5.0f, 0.0f, 0.0f, 0.2f), 0.8f, k_Epsilon);
}

TEST(AnimalMove, AHunterMakesForTheNearSideOfItsPrey)
{
	// Prey 1 m wide at the origin, a hunter 0.5 m wide 10 m along +x: it makes for 1.5 m along +x
	const auto goal = WorkingPosition({0.0f, 0.0f}, 1.0f, {10.0f, 0.0f}, 0.5f);
	EXPECT_NEAR(goal.x, 1.5f, 0.01f);
	EXPECT_NEAR(goal.y, 0.0f, 0.01f);
}

TEST(AnimalMove, WhatItCanReachWithoutCircling)
{
	// Turning in a circle of about 30.7 m either side: a point just beside it is inside one, a point ahead is not
	EXPECT_FALSE(OutsideTurningCircles({0.0f, 0.0f}, 0, k_Speed, k_TurnAngle, {0.0f, 20.0f}));
	EXPECT_FALSE(OutsideTurningCircles({0.0f, 0.0f}, 0, k_Speed, k_TurnAngle, {0.0f, -20.0f}));
	EXPECT_TRUE(OutsideTurningCircles({0.0f, 0.0f}, 0, k_Speed, k_TurnAngle, {40.0f, 0.0f}));
}

TEST(Zoomer, ItGlidesToItsTargetArrivingAtRest)
{
	// Twenty turns of a tenth of a second, from full and at rest: slow to start, fastest in the middle, easing in
	Zoomer fade(255.0f);
	fade.SetTarget(0.0f, 0.0f, 2.0f);
	EXPECT_FLOAT_EQ(fade.Value(), 255.0f);
	EXPECT_NEAR(fade.Step(0.1f), 251.425f, 0.01f);
	for (int turn = 2; turn <= 10; ++turn)
	{
		fade.Step(0.1f);
	}
	EXPECT_NEAR(fade.Value(), 79.6875f, 0.01f);
	for (int turn = 11; turn < 20; ++turn)
	{
		fade.Step(0.1f);
	}
	EXPECT_NEAR(fade.Value(), 0.1227f, 0.01f);
	EXPECT_FALSE(fade.Done());
	// The twentieth turn ends it: exactly the target
	EXPECT_EQ(fade.Step(0.1f), 0.0f);
	EXPECT_TRUE(fade.Done());
	EXPECT_EQ(fade.Speed(), 0.0f);
}

TEST(Zoomer, ItSetsOffAtTheSpeedItHas)
{
	// Banking: half a radian over half a second, then back while still moving
	Zoomer bank;
	bank.SetTarget(0.5f, 0.0f, 0.5f);
	bank.Step(0.25f);
	const float speed = bank.Speed();
	EXPECT_GT(speed, 0.0f);
	bank.SetTarget(0.0f, 0.0f, 0.5f);
	// It keeps going a little before turning back
	EXPECT_GT(bank.Step(0.01f), 0.25f);
	bank.Step(1.0f);
	EXPECT_EQ(bank.Value(), 0.0f);
	// Too short a glide is there at once
	Zoomer instant(255.0f);
	instant.SetTarget(0.0f, 0.0f, 0.0005f);
	EXPECT_EQ(instant.Value(), 0.0f);
}

TEST(AnimalMove, ABirdKeepsOverTheLand)
{
	const auto everywhere = [](glm::ivec2) { return true; };
	EXPECT_TRUE(OverLandAllTheWay({1000.0f, 1000.0f}, {1100.0f, 1000.0f}, everywhere));
	// A gap in the land between them, on a cell its 16 m steps meet
	const auto gap = [](glm::ivec2 cell) { return cell.x != 106; };
	EXPECT_FALSE(OverLandAllTheWay({1000.0f, 1000.0f}, {1100.0f, 1000.0f}, gap));
	// The gap off to the side of the line doesn't matter
	EXPECT_TRUE(OverLandAllTheWay({1000.0f, 1300.0f}, {1100.0f, 1300.0f}, [](glm::ivec2 cell) { return cell.y != 100; }));
	// Off the map
	EXPECT_FALSE(OverLandAllTheWay({20.0f, 20.0f}, {-200.0f, 20.0f}, everywhere));
}

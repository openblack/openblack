/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <array>
#include <numbers>
#include <vector>

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Animals/AnimalRules.h"
#include "Magic/FlockMiracleRules.h"

using namespace openblack;
using namespace openblack::magic;

namespace
{
constexpr float k_Epsilon = 1e-4f;
constexpr float k_Pi = std::numbers::pi_v<float>;

bool OnSquareMap(glm::vec2 point)
{
	return point.x >= 0.0f && point.y >= 0.0f && point.x < 1000.0f && point.y < 1000.0f;
}
} // namespace

TEST(FlockMiracleRules, NumberToCreateRoundsTheTribalPower)
{
	EXPECT_EQ(flock::NumberToCreate(12, 1.0f), 12);
	EXPECT_EQ(flock::NumberToCreate(14, 1.5f), 21);
	EXPECT_EQ(flock::NumberToCreate(12, 1.04f), 12);
	EXPECT_EQ(flock::NumberToCreate(12, 1.05f), 13);
	// A half goes to the even number
	EXPECT_EQ(flock::NumberToCreate(14, 0.75f), 10);
	EXPECT_EQ(flock::NumberToCreate(12, 1.375f), 16);
	EXPECT_EQ(flock::NumberToCreate(12, 1.125f), 14);
}

TEST(FlockMiracleRules, BatsOnlyBelowTheSwitch)
{
	EXPECT_FALSE(flock::IsEvil(-0.2f, -0.2f));
	EXPECT_TRUE(flock::IsEvil(-0.21f, -0.2f));
	EXPECT_FALSE(flock::IsEvil(0.5f, -0.2f));
}

TEST(FlockMiracleRules, TwelveASecondAlongTheSweep)
{
	float emitted = 0.0f;
	for (int turn = 0; turn < 10; ++turn)
	{
		emitted = flock::EmitAfterTurn(emitted, 0.1f);
	}
	EXPECT_NEAR(emitted, 12.0f, k_Epsilon);
	// The second animal of a turn from 1.2 to 2.4 appears 2/3 of the way along
	EXPECT_NEAR(flock::SpawnFraction(2, 1.2f, 2.4f), 0.6667f, 1e-3f);
	const auto point = flock::SpawnPoint({0.0f, 0.0f}, {30.0f, 0.0f}, 0.5f);
	EXPECT_NEAR(point.x, 15.0f, k_Epsilon);
	const auto jittered = flock::Jitter({10.0f, 10.0f}, 0.0f, 0.2f);
	EXPECT_NEAR(jittered.x, 9.9f, k_Epsilon);
	EXPECT_NEAR(jittered.y, 10.1f, k_Epsilon);
}

TEST(FlockMiracleRules, DirectionOfAHumanIsTheCamerasAndEastForNone)
{
	const auto human = flock::Direction(true, {0.0f, -0.5f, 2.0f}, {}, {});
	EXPECT_NEAR(human.x, 0.0f, k_Epsilon);
	EXPECT_NEAR(human.y, 2.0f, k_Epsilon);
	const auto none = flock::Direction(true, {0.0f, -1.0f, 0.0f}, {}, {});
	EXPECT_EQ(none, glm::vec2(1.0f, 0.0f));
	const auto script = flock::Direction(false, {}, {10.0f, 0.0f, 0.0f}, {0.0f, 5.0f, 0.0f});
	EXPECT_EQ(script, glm::vec2(10.0f, 0.0f));
}

TEST(FlockMiracleRules, SideBySweepOrAlternating)
{
	// Looking north (+z), released to the east of the cast point
	const glm::vec2 north {0.0f, 1.0f};
	EXPECT_EQ(flock::Side(true, north, {10.0f, 0.0f}, {0.0f, 0.0f}, 1), -1.0f);
	EXPECT_EQ(flock::Side(true, north, {-10.0f, 0.0f}, {0.0f, 0.0f}, 1), 1.0f);
	// Straight ahead: odd ones one way, even the other
	EXPECT_EQ(flock::Side(true, north, {0.0f, 10.0f}, {0.0f, 0.0f}, 1), -1.0f);
	EXPECT_EQ(flock::Side(true, north, {0.0f, 10.0f}, {0.0f, 0.0f}, 2), 1.0f);
	EXPECT_EQ(flock::Side(false, north, {10.0f, 0.0f}, {0.0f, 0.0f}, 2), 1.0f);
}

TEST(FlockMiracleRules, TheFanWidensWithEachAnimal)
{
	EXPECT_NEAR(flock::FanAngle(12, 1.0f, 12), 2.0f, k_Epsilon);
	EXPECT_NEAR(flock::FanAngle(3, -1.0f, 12), -0.5f, k_Epsilon);
	const auto turned = flock::Rotate({1.0f, 0.0f}, k_Pi * 0.5f);
	EXPECT_NEAR(turned.x, 0.0f, k_Epsilon);
	EXPECT_NEAR(turned.y, 1.0f, k_Epsilon);
}

TEST(FlockMiracleRules, DestinationHalvesBackOntoTheMap)
{
	const auto inside = flock::Destination({100.0f, 100.0f}, {1.0f, 0.0f}, 800.0f, OnSquareMap);
	ASSERT_TRUE(inside.has_value());
	EXPECT_NEAR(inside->x, 900.0f, k_Epsilon);
	// 800 then 400 then 200 then 100 east of 900
	const auto halved = flock::Destination({900.0f, 100.0f}, {2.0f, 0.0f}, 800.0f, OnSquareMap);
	ASSERT_TRUE(halved.has_value());
	EXPECT_NEAR(halved->x, 950.0f, k_Epsilon);
	EXPECT_FALSE(flock::Destination({999.0f, 100.0f}, {1.0f, 0.0f}, 800.0f, OnSquareMap).has_value());
	// By whole cells, keeping the place in the cell: 50 m east of 905.3 is cell 95, 955.3
	const auto cells = flock::Destination({905.3f, 100.0f}, {1.0f, 0.0f}, 800.0f, OnSquareMap);
	ASSERT_TRUE(cells.has_value());
	EXPECT_NEAR(cells->x, 955.3f, 1e-3f);
	// 12.5 m is the last distance tried: from 992 nothing within it is on the map, and 6.25 m is never tried
	EXPECT_FALSE(flock::Destination({992.0f, 100.0f}, {1.0f, 0.0f}, 800.0f, OnSquareMap).has_value());
}

TEST(FlockMiracleRules, StepByWholeCells)
{
	// 17 m east of 3.5 is cell 1 (cut from 1.7), 13.5; 4 m west of 12 is cell 0 (cut from 0.8), 2
	const auto moved = flock::StepByCells({3.5f, 12.0f}, {17.0f, -4.0f});
	EXPECT_NEAR(moved.x, 13.5f, k_Epsilon);
	EXPECT_NEAR(moved.y, 2.0f, k_Epsilon);
}

TEST(FlockMiracleRules, ScaleAndFade)
{
	// The low end of its kind's range plus a random number up to the range, whatever its size as it was born
	EXPECT_NEAR(flock::SpawnScale(false, 0.1f), 2.9f, k_Epsilon);
	EXPECT_NEAR(flock::SpawnScale(false, 0.2f), 3.0f, k_Epsilon);
	EXPECT_NEAR(flock::SpawnScale(true, 0.25f), 1.75f, k_Epsilon);
}

TEST(AnimalRules, BirthScale)
{
	const std::array<float, 20> table {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f,
	                                   1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
	std::vector<float> asked;
	const auto random = [&asked](float max) {
		asked.push_back(max);
		return max * 0.5f;
	};
	// Young, at 5: the table's size kept a place before its age's, half way along three quarters of the way to 7's
	EXPECT_NEAR(animals::BirthScale(5, 13, table, random), 0.5f + (0.7f - 0.5f) * 0.75f * 0.5f, k_Epsilon);
	ASSERT_EQ(asked.size(), 1u);
	// Grown: two draws up to a tenth, the second giving 1.05 less half a tenth
	asked.clear();
	EXPECT_NEAR(animals::BirthScale(14, 13, table, random), 1.0f, k_Epsilon);
	EXPECT_EQ(asked, (std::vector<float> {0.1f, 0.1f}));
}

TEST(FlockMiracleRules, CorridorAlongTheRun)
{
	const auto corridor = flock::MakeCorridor({0.0f, 0.0f}, {0.0f, 800.0f}, 45.0f);
	EXPECT_TRUE(flock::IsOnCorridor(corridor, {44.0f, 100.0f}, {0.0f, 50.0f}));
	EXPECT_FALSE(flock::IsOnCorridor(corridor, {46.0f, 100.0f}, {0.0f, 50.0f}));
	// Behind the wolf, by no more than the half width
	EXPECT_TRUE(flock::IsOnCorridor(corridor, {0.0f, 10.0f}, {0.0f, 50.0f}));
	EXPECT_FALSE(flock::IsOnCorridor(corridor, {0.0f, 0.0f}, {0.0f, 50.0f}));
	// Behind is measured from the corner of the wolf's cell: a wolf at 59 m counts from 50 m
	EXPECT_TRUE(flock::IsOnCorridor(corridor, {0.0f, 6.0f}, {3.0f, 59.0f}));
	EXPECT_FALSE(flock::IsOnCorridor(corridor, {0.0f, 4.0f}, {3.0f, 59.0f}));
	// A run from a point to itself runs north
	const auto still = flock::MakeCorridor({10.0f, 10.0f}, {10.0f, 10.0f}, 45.0f);
	EXPECT_NEAR(still.normal.x, 1.0f, k_Epsilon);
	EXPECT_NEAR(still.along.y, 1.0f, k_Epsilon);
	EXPECT_TRUE(flock::WolfArrived({0.0f, 771.0f}, {0.0f, 800.0f}));
	EXPECT_FALSE(flock::WolfArrived({0.0f, 769.0f}, {0.0f, 800.0f}));
}

TEST(FlockMiracleRules, RememberedPreyGate)
{
	// The wolf's cell number (100, 100) against a remembered point in map units: away from the map's corner that way
	// is huge, so any prey is near enough
	const glm::ivec2 wolf(100 * 65536 + 5, 100 * 65536 + 5);
	EXPECT_TRUE(flock::CloserThanRemembered(wolf, wolf + glm::ivec2(1000, 0), {101 * 65536, 100 * 65536}));
	// Remembered at the map's corner, a prey within half of the way from the cell number is taken
	EXPECT_TRUE(flock::CloserThanRemembered(wolf, wolf + glm::ivec2(40, 0), {0, 0}));
	EXPECT_FALSE(flock::CloserThanRemembered(wolf, wolf + glm::ivec2(50, 0), {0, 0}));
}

TEST(FlockMiracleRules, WhatAWolfHunts)
{
	EXPECT_TRUE(flock::IsPrey({.isVillager = true}));
	EXPECT_TRUE(flock::IsPrey({.isOtherAnimal = true}));
	EXPECT_FALSE(flock::IsPrey({}));
	EXPECT_FALSE(flock::IsPrey({.isVillager = true, .heightAboveLand = 2.5f}));
	EXPECT_FALSE(flock::IsPrey({.isVillager = true, .helpless = true}));
	EXPECT_FALSE(flock::IsPrey({.isVillager = true, .onCorridor = false}));
	EXPECT_FALSE(flock::IsPrey({.isVillager = true, .hunterFading = true}));
	EXPECT_FALSE(flock::IsPrey({.isOtherAnimal = true, .hasMeat = false}));
}

TEST(AnimalRules, TurnAngles)
{
	EXPECT_NEAR(animals::TurnAngleRadians(0x200), k_Pi * 0.5f, k_Epsilon);
}

TEST(AnimalRules, FormationSlots)
{
	// The leader, then the birds behind it two by two
	const auto first = animals::FormationSlotOf(1);
	EXPECT_EQ(first.row, -3);
	EXPECT_EQ(first.column, 2);
	const auto second = animals::FormationSlotOf(2);
	EXPECT_EQ(second.row, -3);
	EXPECT_EQ(second.column, -1);
	const auto fifth = animals::FormationSlotOf(5);
	EXPECT_EQ(fifth.row, -2);
	EXPECT_EQ(fifth.column, 2);
	// A slot about the leader is as far as its row and column say
	const auto goal = animals::FormationGoal({0.0f, 0.0f}, {-10.0f, 0.0f}, {.row = -3, .column = 0});
	EXPECT_NEAR(glm::length(goal), 30.0f, k_Epsilon);
}

TEST(AnimalRules, AKilledAnimalFallsAndLiesInItsKindsClips)
{
	EXPECT_EQ(animals::DyingClip(AnimalInfo::Lion), AnimId::ALionDie);
	EXPECT_EQ(animals::DeadClip(AnimalInfo::Lion), AnimId::ALionSleep);
	// The farm animals killed lie on their left
	EXPECT_EQ(animals::DyingClip(AnimalInfo::Cow), AnimId::ACowDie);
	EXPECT_EQ(animals::DeadClip(AnimalInfo::Cow), AnimId::ACowDeadOnLhs);
	EXPECT_EQ(animals::DeadClip(AnimalInfo::Horse), AnimId::AHorseDeadlhs);
	// A kind with clips of its own for neither keeps the clip it had
	EXPECT_FALSE(animals::DyingClip(AnimalInfo::Goat).has_value());
	EXPECT_EQ(animals::k_TurnsToDieOver, 600);
}

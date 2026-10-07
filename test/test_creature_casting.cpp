/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Creature/CreatureCastAgenda.h"
#include "Creature/CreatureCastMoves.h"
#include "Creature/CreatureIdleMind.h"

using namespace openblack;
using namespace openblack::creature_mind;
namespace cast_moves = openblack::creature_cast_moves;

namespace
{
Random Always(uint32_t value)
{
	return [value](uint32_t range) { return std::min(value, range - 1); };
}
} // namespace

TEST(CreatureCastAgenda, LightningFromFiftyBackingOffToTenMoreThanItsHeight)
{
	const auto agenda = CastAt(CastStyle::Lightning, 4, 0, 7, 15.0f, Always(1));
	ASSERT_EQ(agenda.size(), 4u);
	EXPECT_EQ(agenda[0].movement.kind, Movement::Kind::GoNearObject);
	EXPECT_FLOAT_EQ(agenda[0].movement.maxDistance, 50.0f);
	EXPECT_EQ(agenda[0].face, creature_face::Cue::Anger);
	EXPECT_EQ(agenda[1].movement.kind, Movement::Kind::GetAwayFromObject);
	EXPECT_FLOAT_EQ(agenda[1].movement.maxDistance, 25.0f);
	EXPECT_EQ(agenda[2].movement.kind, Movement::Kind::TurnToFaceObject);
	EXPECT_FLOAT_EQ(agenda[2].seconds, 0.1f);
	EXPECT_EQ(agenda[3].kind, Step::Kind::Cast);
	EXPECT_EQ(agenda[3].cast.magicType, 4u);
	EXPECT_EQ(*agenda[3].cast.object, 7u);
	EXPECT_EQ(agenda[3].sequence, (std::array<size_t, 3> {40, 41, 42}));
	EXPECT_FLOAT_EQ(agenda[3].seconds, 3.0f);
}

TEST(CreatureCastAgenda, OneTimeInFiveItShowsHowItFeelsFirst)
{
	const auto playful = CastAt(CastStyle::Playful, 26, 0, 7, 10.0f, Always(0));
	ASSERT_EQ(playful.size(), 5u);
	EXPECT_EQ(playful[0].kind, Step::Kind::Action);
	EXPECT_EQ(playful[0].animation, 67u);
	EXPECT_FLOAT_EQ(playful[1].movement.maxDistance, 50.0f);
	EXPECT_FLOAT_EQ(playful[2].movement.maxDistance, 20.0f);
	EXPECT_EQ(playful[3].face, creature_face::Cue::Compassion);
	const auto helpful = CastAt(CastStyle::Helpful, 10, 0, 7, 10.0f, Always(0));
	EXPECT_EQ(helpful[0].animation, 64u);
	EXPECT_FLOAT_EQ(helpful[1].movement.maxDistance, 20.0f);
	// A power-up draws its gesture before casting
	const auto powered = CastAt(CastStyle::Helpful, 11, 1, 7, 10.0f, Always(1));
	ASSERT_EQ(powered.size(), 5u);
	EXPECT_EQ(powered[3].kind, Step::Kind::Gesture);
	EXPECT_EQ(powered[3].animation, 1u);
}

TEST(CreatureCastAgenda, TheMiracleIsCastAsThePoseLoopsAndHeldThirtyTurns)
{
	IdleMind mind;
	Plan(mind, Activity::Planned, {CastAt(CastStyle::Lightning, 4, 0, 7, 15.0f, Always(1)).back()});
	Senses senses {.seconds = 0.1f};
	auto commands = Think(mind, senses, Always(0));
	ASSERT_TRUE(commands.startSequence.has_value());
	EXPECT_FALSE(commands.cast.has_value());
	senses.bodyBusy = true;
	commands = Think(mind, senses, Always(0));
	EXPECT_FALSE(commands.cast.has_value());
	senses.bodyLooping = true;
	commands = Think(mind, senses, Always(0));
	ASSERT_TRUE(commands.cast.has_value());
	EXPECT_EQ(commands.cast->magicType, 4u);
	for (int turn = 0; turn < 30; ++turn)
	{
		commands = Think(mind, senses, Always(0));
		EXPECT_FALSE(commands.releaseCast) << turn;
	}
	commands = Think(mind, senses, Always(0));
	EXPECT_TRUE(commands.releaseCast);
	EXPECT_TRUE(commands.endSit);
	senses.bodyBusy = false;
	senses.bodyLooping = false;
	commands = Think(mind, senses, Always(0));
	EXPECT_FALSE(commands.releaseCast);
	EXPECT_EQ(mind.step, 1u);
	EXPECT_FALSE(mind.gaveUp);
}

TEST(CreatureCastAgenda, AMoveTheBodySaysFailedGivesUpTheRest)
{
	IdleMind mind;
	Plan(mind, Activity::Planned, CastAt(CastStyle::Playful, 30, 0, 7, 15.0f, Always(1)));
	Senses senses {.seconds = 0.1f};
	auto commands = Think(mind, senses, Always(0));
	ASSERT_TRUE(commands.move.has_value());
	EXPECT_EQ(commands.move->kind, Movement::Kind::GoNearObject);
	senses.subMove = SubMove::Running;
	(void)Think(mind, senses, Always(0));
	EXPECT_EQ(mind.step, 0u);
	senses.subMove = SubMove::Done;
	(void)Think(mind, senses, Always(0));
	EXPECT_EQ(mind.step, 1u);
	senses.subMove = SubMove::Running;
	(void)Think(mind, senses, Always(0));
	senses.subMove = SubMove::Failed;
	(void)Think(mind, senses, Always(0));
	EXPECT_TRUE(mind.gaveUp);
	EXPECT_EQ(mind.step, mind.agenda.size());
}

TEST(CreatureCastMoves, RoutePlanRadiusAndArrival)
{
	// A thing as low as nothing: seven tenths of the creature's radius off its own
	EXPECT_FLOAT_EQ(cast_moves::RoutePlanRadius(10.0f, 0.0f, false, 15.0f, 5.0f), 10.0f - 0.7f * 5.0f);
	// Half of four fifths of the creature's height
	EXPECT_FLOAT_EQ(cast_moves::RoutePlanRadius(10.0f, 6.0f, false, 15.0f, 5.0f), 10.0f - (0.7f - 0.3f * 0.5f) * 5.0f);
	EXPECT_FLOAT_EQ(cast_moves::RoutePlanRadius(10.0f, 20.0f, false, 15.0f, 5.0f), 10.0f - 0.4f * 5.0f);
	EXPECT_FLOAT_EQ(cast_moves::RoutePlanRadius(10.0f, 20.0f, true, 15.0f, 5.0f), 0.25f);
	EXPECT_FLOAT_EQ(cast_moves::RoutePlanRadius(1.0f, 20.0f, true, 15.0f, 5.0f), 0.1f);
	EXPECT_TRUE(cast_moves::Arrived(10.9f, 2.0f, 3.0f, 5.0f));
	EXPECT_FALSE(cast_moves::Arrived(11.0f, 2.0f, 3.0f, 5.0f));
}

TEST(CreatureCastMoves, GettingAway)
{
	EXPECT_FLOAT_EQ(cast_moves::GetAwayDistance(20.0f, 5.0f), 26.0f);
	EXPECT_FLOAT_EQ(cast_moves::GetAwayDistance(20.0f, std::nullopt), 20.0f);
	const auto away = cast_moves::GetAwayPoint({10.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 26.0f);
	EXPECT_FLOAT_EQ(away.x, 36.0f);
	const auto onTop = cast_moves::GetAwayPoint({10.0f, 0.0f, 10.0f}, {10.0f, 0.0f, 10.0f}, 5.0f);
	EXPECT_FLOAT_EQ(onTop.x, 15.0f);
}

TEST(CreatureCastMoves, TheNearestClearAreaIsTheMiddleOfAClearSquare)
{
	// All clear: a square of two cells about the point, its middle at a cell corner
	const auto open = cast_moves::FindClearArea({125.0f, 125.0f}, 15.0f, [](int32_t, int32_t) { return true; });
	ASSERT_TRUE(open.has_value());
	EXPECT_FLOAT_EQ(open->x, 127.5f);
	EXPECT_FLOAT_EQ(open->y, 127.5f);
	// Nothing clear west of x = 200
	const auto east = cast_moves::FindClearArea({125.0f, 125.0f}, 15.0f, [](int32_t x, int32_t) { return x >= 20; });
	ASSERT_TRUE(east.has_value());
	EXPECT_FLOAT_EQ(east->x, 207.5f);
	EXPECT_FALSE(cast_moves::FindClearArea({125.0f, 125.0f}, 15.0f, [](int32_t, int32_t) { return false; }).has_value());
}

TEST(CreatureCastMoves, AThingKeepsCirclesClear)
{
	const glm::mat2 still(1.0f);
	// Squarish: one circle as wide as its longer side
	const auto square = cast_moves::CollideCircles({3.0f, 2.5f}, {10.0f, 10.0f}, still);
	ASSERT_EQ(square.size(), 1u);
	EXPECT_FLOAT_EQ(square[0].radius, 3.0f);
	// Long: a row of three as wide as its shorter side, along its length
	const auto wall = cast_moves::CollideCircles({6.0f, 2.5f}, {10.0f, 10.0f}, still);
	ASSERT_EQ(wall.size(), 3u);
	EXPECT_FLOAT_EQ(wall[0].radius, 2.5f);
	EXPECT_FLOAT_EQ(wall[0].centre.x, 10.0f - 4.0f);
	EXPECT_FLOAT_EQ(wall[2].centre.x, 10.0f + 4.0f);
	// Sides under 1 count as 1
	EXPECT_FLOAT_EQ(cast_moves::CollideCircles({0.2f, 0.2f}, {0.0f, 0.0f}, still)[0].radius, 1.0f);
	// A cell is blocked by a circle coming within 5 of its middle
	EXPECT_TRUE(cast_moves::BlocksCell({.centre = {19.5f, 5.0f}, .radius = 1.0f}, 1, 0));
	EXPECT_FALSE(cast_moves::BlocksCell({.centre = {5.0f, 5.0f}, .radius = 0.3f}, 1, 0));
}

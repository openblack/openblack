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

#include "Common/GUtilsDistance.h"
#include "Magic/TeleportRules.h"

using namespace openblack::magic::teleport;

TEST(TeleportRules, FastDistanceIsTheLongerSidePlusHalfTheShorter)
{
	// 10 m across and 4 m up the land: 10 + 2 in fixed point
	const auto d = FastDistance({0.0f, 0.0f, 0.0f}, {10.0f, 50.0f, 4.0f});
	EXPECT_EQ(d, 65536 + (26214 >> 1));
	EXPECT_EQ(FastDistance({0.0f, 0.0f, 0.0f}, {-4.0f, 0.0f, 10.0f}), d);
	EXPECT_EQ(FastDistance({1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}), 0);
}

TEST(TeleportRules, DetourMustBeAFifthShorterThanTheWalk)
{
	const glm::vec3 traveller {0.0f, 0.0f, 0.0f};
	const glm::vec3 destination {100.0f, 0.0f, 0.0f};
	// 10 to the stone and 10 from the other: 24 with the fifth more, well under 100
	EXPECT_TRUE(IsWorthTheDetour(traveller, destination, {10.0f, 0.0f, 0.0f}, {90.0f, 0.0f, 0.0f}));
	// 40 and 45: 102 with the fifth more, over 100
	EXPECT_FALSE(IsWorthTheDetour(traveller, destination, {40.0f, 0.0f, 0.0f}, {55.0f, 0.0f, 0.0f}));
}

TEST(TeleportRules, ReactsOnlyThroughAnotherStone)
{
	const std::array stones {glm::vec3 {5.0f, 0.0f, 0.0f}, glm::vec3 {95.0f, 0.0f, 0.0f}};
	EXPECT_TRUE(ShouldReact({0.0f, 0.0f, 0.0f}, {100.0f, 0.0f, 0.0f}, stones, 0));
	// Alone, a stone leads nowhere
	EXPECT_FALSE(ShouldReact({0.0f, 0.0f, 0.0f}, {100.0f, 0.0f, 0.0f}, std::span(stones).first(1), 0));
	// Going the other way the stones don't help
	EXPECT_FALSE(ShouldReact({0.0f, 0.0f, 0.0f}, {-100.0f, 0.0f, 0.0f}, stones, 0));
}

TEST(TeleportRules, ChoosesTheStoneNearestTheDestination)
{
	const std::array stones {glm::vec3 {0.0f, 0.0f, 0.0f}, glm::vec3 {60.0f, 0.0f, 0.0f}, glm::vec3 {90.0f, 0.0f, 0.0f}};
	const auto jump = ChooseTarget({0.0f, 0.0f, 0.0f}, {100.0f, 0.0f, 0.0f}, stones, 0, false);
	ASSERT_TRUE(jump.has_value());
	EXPECT_EQ(jump->stone, 2u);
	// Measured as the game measures it, a little long
	const auto metres = [](int32_t units) {
		return openblack::gutils::ConvertWholeDistanceToMeters(openblack::gutils::Hypotenuse(units, 0));
	};
	EXPECT_FLOAT_EQ(jump->saving, metres(655360) - metres(65536));
	EXPECT_NEAR(jump->saving, 90.0f, 0.1f);
}

TEST(TeleportRules, OnlyAForcedJumpGoesBackwards)
{
	const std::array stones {glm::vec3 {0.0f, 0.0f, 0.0f}, glm::vec3 {-50.0f, 0.0f, 0.0f}};
	EXPECT_FALSE(ChooseTarget({0.0f, 0.0f, 0.0f}, {100.0f, 0.0f, 0.0f}, stones, 0, false).has_value());
	const auto forced = ChooseTarget({0.0f, 0.0f, 0.0f}, {100.0f, 0.0f, 0.0f}, stones, 0, true);
	ASSERT_TRUE(forced.has_value());
	EXPECT_EQ(forced->stone, 1u);
	EXPECT_NEAR(forced->saving, -50.0f, 0.1f);
}

TEST(TeleportRules, EqualStonesKeepTheFirst)
{
	// Stones come newest first: of two as good, the newer
	const std::array stones {glm::vec3 {0.0f, 0.0f, 0.0f}, glm::vec3 {100.0f, 0.0f, 10.0f}, glm::vec3 {100.0f, 0.0f, -10.0f}};
	const auto jump = ChooseTarget({0.0f, 0.0f, 0.0f}, {100.0f, 0.0f, 0.0f}, stones, 0, false);
	ASSERT_TRUE(jump.has_value());
	EXPECT_EQ(jump->stone, 1u);
}

TEST(TeleportRules, UsefulJumpsGiveBackPrayerPower)
{
	EXPECT_FLOAT_EQ(JumpCost(100.0f, 200.0f), -20.0f);
	EXPECT_FLOAT_EQ(JumpCost(-50.0f, 200.0f), 10.0f);
}

TEST(TeleportRules, NoStoneNextToSomethingFixed)
{
	const std::array fixed {glm::vec3 {10.0f, 0.0f, 0.0f}};
	EXPECT_FALSE(CanPlaceStone({5.0f, 0.0f, 0.0f}, fixed));
	EXPECT_TRUE(CanPlaceStone({3.9f, 0.0f, 0.0f}, fixed));
	EXPECT_TRUE(CanPlaceStone({5.0f, 0.0f, 0.0f}, {}));
}

TEST(TeleportRules, WorshippersGoThroughTheNearestStone)
{
	const std::array stones {glm::vec3 {200.0f, 0.0f, 0.0f}, glm::vec3 {10.0f, 0.0f, 0.0f}, glm::vec3 {490.0f, 0.0f, 0.0f}};
	const auto route = FindRouteStone({0.0f, 0.0f, 0.0f}, {500.0f, 0.0f, 0.0f}, stones, 100.0f);
	ASSERT_TRUE(route.has_value());
	EXPECT_EQ(*route, 1u);
	// Too far either way
	EXPECT_FALSE(FindRouteStone({0.0f, 0.0f, 0.0f}, {500.0f, 0.0f, 0.0f}, stones, 15.0f).has_value());
}

TEST(TeleportRules, DroppedVillagersJumpOnlyFromTheirOwnPlayersNetwork)
{
	EXPECT_TRUE(CanDropOnStone(true, 2));
	EXPECT_FALSE(CanDropOnStone(true, 1));
	EXPECT_FALSE(CanDropOnStone(false, 3));
}

TEST(TeleportRules, AVillagerIsThereWithinAStep)
{
	// Within a step of half a metre, measured in the land's units
	EXPECT_TRUE(WithinAStep({100.0f, 0.0f, 100.0f}, {100.3f, 5.0f, 100.3f}, 0.5f));
	EXPECT_FALSE(WithinAStep({100.0f, 0.0f, 100.0f}, {100.4f, 0.0f, 100.4f}, 0.5f));
	// Standing still, only on the point itself
	EXPECT_TRUE(WithinAStep({100.0f, 0.0f, 100.0f}, {100.0f, 0.0f, 100.0f}, 0.0f));
}

TEST(TeleportRules, ACreatureFadesOutAndInOverTwoSeconds)
{
	EXPECT_EQ(CreatureFadeTurns(100.0f), 20);
	// Fading out it never quite vanishes before it comes out; fading in it starts from there
	EXPECT_FLOAT_EQ(CreatureFadeOut(19, 20), 0.05f);
	EXPECT_FLOAT_EQ(CreatureFadeOut(1, 20), 0.95f);
	EXPECT_FLOAT_EQ(CreatureFadeIn(19, 20), 0.95f);
	EXPECT_FLOAT_EQ(CreatureFadeIn(0, 20), 0.0f);
}

TEST(TeleportRules, AVillagerComesBackToWhatItWasDoing)
{
	// Through the stone it comes back to the state it was to take, unless its table says that state keeps the one kept
	// before
	EXPECT_EQ(PreviousToKeep(openblack::VillagerStates::ArrivesAtWorshipSiteForWorship, openblack::VillagerStates::InvalidState,
	                         false),
	          openblack::VillagerStates::ArrivesAtWorshipSiteForWorship);
	EXPECT_EQ(PreviousToKeep(openblack::VillagerStates::ArrivesAtWorshipSiteForWorship,
	                         openblack::VillagerStates::DecideWhatToDo, true),
	          openblack::VillagerStates::DecideWhatToDo);
	// With nothing to come back to it decides afresh
	EXPECT_EQ(StateAfterReacting(openblack::VillagerStates::GotoWorshipSiteForWorship),
	          openblack::VillagerStates::GotoWorshipSiteForWorship);
	EXPECT_EQ(StateAfterReacting(openblack::VillagerStates::InvalidState), openblack::VillagerStates::DecideWhatToDo);
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Creature/LeashRules.h"

using namespace openblack;
using namespace openblack::creature_leash;
using creature_desires::Desire;
using creature_mind::Activity;

namespace
{
constexpr float k_Tolerance = 1e-4f;
}

TEST(LeashRules, HandLengthsGrowWithTheCreature)
{
	const auto one = InHand(1.0f);
	EXPECT_NEAR(one.slack, 32.5f, k_Tolerance);
	EXPECT_NEAR(one.max, 77.0f, k_Tolerance);
	const auto two = InHand(2.0f);
	EXPECT_NEAR(two.slack, 43.0f, k_Tolerance);
	EXPECT_NEAR(two.max, 122.0f, k_Tolerance);
}

TEST(LeashRules, TiedLengths)
{
	EXPECT_NEAR(TiedToStatic(50.0f).max, 180.0f, k_Tolerance);
	EXPECT_NEAR(TiedToStatic(200.0f).max, 300.0f, k_Tolerance);
	EXPECT_NEAR(TiedToStatic(200.0f).slack, 150.0f, k_Tolerance);
	EXPECT_NEAR(TiedToStatic(1000.0f).max, 360.0f, k_Tolerance);
	EXPECT_NEAR(TiedToMobile(4.0f).max, 28.0f, k_Tolerance);
	EXPECT_NEAR(TiedToMobile(15.0f).max, 40.0f, k_Tolerance);
	EXPECT_NEAR(TiedToMobile(15.0f).slack, 20.0f, k_Tolerance);
}

TEST(LeashRules, EachLeashLooksItsOwn)
{
	EXPECT_FLOAT_EQ(LookFor(LeashType::Evil).v0, 0.375f);
	EXPECT_FLOAT_EQ(LookFor(LeashType::Evil).v1, 0.5f);
	EXPECT_FLOAT_EQ(LookFor(LeashType::Rope).v0, 0.125f);
	EXPECT_FLOAT_EQ(LookFor(LeashType::Good).v0, 0.25f);
	EXPECT_FLOAT_EQ(LookFor(LeashType::Good).halfWidth, 0.225f);
	EXPECT_FLOAT_EQ(LookFor(LeashType::Rope).halfWidth, 0.15f);
}

TEST(LeashRules, LeashesForceTheirFeelings)
{
	EXPECT_EQ(ForcedDesireFor(LeashType::Evil), Desire::Anger);
	EXPECT_EQ(ForcedDesireFor(LeashType::Good), Desire::Compassion);
	EXPECT_FALSE(ForcedDesireFor(LeashType::Rope).has_value());
	EXPECT_EQ(MiracleSightingWeight(true), 3u);
	EXPECT_EQ(MiracleSightingWeight(false), 1u);
}

TEST(LeashRules, PullsOnlyWhenTaut)
{
	EXPECT_FALSE(ShouldPull(0.8f));
	EXPECT_TRUE(ShouldPull(0.81f));
	EXPECT_FALSE(ShouldPull(0.0f));
}

TEST(LeashRules, SecondPullHoldsTheDesireBack)
{
	PullMemory memory;
	EXPECT_FALSE(RecordPull(memory, Desire::Hunger).has_value());
	// Another desire's pull doesn't count towards hunger
	EXPECT_FALSE(RecordPull(memory, Desire::Rest).has_value());
	const auto suppressed = RecordPull(memory, Desire::Hunger);
	ASSERT_TRUE(suppressed.has_value());
	EXPECT_FLOAT_EQ(*suppressed, 60.0f);
	// The count starts again
	EXPECT_FALSE(RecordPull(memory, Desire::Hunger).has_value());
}

TEST(LeashRules, DesireBehindActivities)
{
	EXPECT_EQ(DesireBehind(Activity::Eat, std::nullopt), Desire::Hunger);
	EXPECT_EQ(DesireBehind(Activity::Sleep, std::nullopt), Desire::Tiredness);
	EXPECT_EQ(DesireBehind(Activity::ShowDesire, Desire::Play), Desire::Play);
	EXPECT_FALSE(DesireBehind(Activity::None, std::nullopt).has_value());
}

TEST(LeashRules, LeadsToTheHand)
{
	const glm::vec3 hand {100.0f, 20.0f, 100.0f};
	// Close by, it stays
	EXPECT_EQ(DecideLead({95.0f, 15.0f, 100.0f}, hand, std::nullopt, false), Lead::Stay);
	// Far off and standing, it goes
	EXPECT_EQ(DecideLead({50.0f, 0.0f, 100.0f}, hand, std::nullopt, false), Lead::GoToHand);
	// Already walking to near the hand, it carries on
	EXPECT_EQ(DecideLead({50.0f, 0.0f, 100.0f}, hand, glm::vec2(100.5f, 100.0f), true), Lead::KeepGoing);
	// Walking somewhere far from the hand, it is sent to the hand
	EXPECT_EQ(DecideLead({50.0f, 0.0f, 100.0f}, hand, glm::vec2(0.0f, 100.0f), true), Lead::GoToHand);
	// Walking to somewhere much nearer the hand than it is, but more than a unit off, it carries on
	EXPECT_EQ(DecideLead({0.0f, 0.0f, 100.0f}, hand, glm::vec2(80.0f, 100.0f), true), Lead::KeepGoing);
}

TEST(LeashRules, PullFades)
{
	EXPECT_NEAR(FadePull(1.0f), 0.95f, k_Tolerance);
	EXPECT_FLOAT_EQ(FadePull(0.31f), 0.0f);
	EXPECT_FLOAT_EQ(FadePull(0.0f), 0.0f);
}

TEST(LeashRules, Confinement)
{
	EXPECT_TRUE(FreeOfHome(139.0f, true));
	EXPECT_FALSE(FreeOfHome(141.0f, true));
	EXPECT_FALSE(FreeOfHome(10.0f, false));
	EXPECT_TRUE(IsConfined(k_HomeConfinement, false, true));
	EXPECT_FALSE(IsConfined(0.0f, false, true));
	// On a leash that doesn't work, it isn't kept anywhere
	EXPECT_FALSE(IsConfined(12.0f, true, false));
	EXPECT_TRUE(IsConfined(12.0f, true, true));
	EXPECT_TRUE(OutsideArea({20.0f, 0.0f}, {0.0f, 0.0f}, 12.0f));
	EXPECT_FALSE(OutsideArea({10.0f, 0.0f}, {0.0f, 0.0f}, 12.0f));
	EXPECT_FALSE(OutsideArea({100.0f, 0.0f}, {0.0f, 0.0f}, 0.0f));
}

TEST(LeashRules, LeashedCreaturesWarmOrCoolToEachOther)
{
	EXPECT_FLOAT_EQ(AttitudeChange(LeashType::Evil, 600), -0.1f);
	EXPECT_FLOAT_EQ(AttitudeChange(LeashType::Good, 1200), 0.1f);
	EXPECT_FLOAT_EQ(AttitudeChange(LeashType::Good, 599), 0.0f);
	EXPECT_FLOAT_EQ(AttitudeChange(LeashType::Rope, 600), 0.0f);
	EXPECT_FLOAT_EQ(AttitudeChange(LeashType::Good, 0), 0.0f);
}

TEST(LeashRules, TypeIndices)
{
	EXPECT_EQ(IndexOf(LeashType::Evil), 0u);
	EXPECT_EQ(IndexOf(LeashType::Good), 2u);
	EXPECT_FALSE(IndexOf(LeashType::None).has_value());
}

TEST(LeashRules, LessonsOfWhatThePlayerShows)
{
	const auto evil = LessonsFor(LeashType::Evil, false);
	ASSERT_EQ(evil.size(), 2u);
	EXPECT_EQ(evil[0].desire, Desire::Anger);
	EXPECT_FLOAT_EQ(evil[0].change, 1.0f);
	EXPECT_EQ(evil[1].desire, Desire::Compassion);
	EXPECT_FLOAT_EQ(evil[1].change, -1.0f);
	const auto good = LessonsFor(LeashType::Good, true);
	ASSERT_EQ(good.size(), 2u);
	EXPECT_EQ(good[1].desire, Desire::BeFriends);
	EXPECT_FLOAT_EQ(good[1].change, 1.0f);
	EXPECT_TRUE(LessonsFor(LeashType::Rope, false).empty());
}

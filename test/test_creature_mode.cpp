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

#include <gtest/gtest.h>

#include "Camera/CreatureFollow.h"
#include "Creature/CreatureMode.h"

using namespace openblack;

namespace
{
constexpr float k_Pi = std::numbers::pi_v<float>;

creature_follow::View View(float yaw, float pitch, float distance)
{
	return {.yaw = yaw, .pitch = pitch, .distance = distance};
}

float Flat(glm::vec2 /*point*/)
{
	return 0.0f;
}
} // namespace

// The follow camera

TEST(CreatureFollow, LooksAtTheMiddleOfTheCreature)
{
	const auto focus = creature_follow::Focus({10.0f, 5.0f, 20.0f}, 15.0f);
	EXPECT_FLOAT_EQ(focus.y, 12.5f);
	EXPECT_FLOAT_EQ(creature_follow::ViewingDistance(15.0f), 120.0f);
}

TEST(CreatureFollow, StartsAtTheViewingDistanceFromAfar)
{
	// The camera south of the creature, far away and looking north at it
	auto view = creature_follow::Start({0.0f, 300.0f, -500.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 15.0f);
	EXPECT_FLOAT_EQ(view.distance, 120.0f);
	// It keeps the way the camera was turned: from the south, heading half a turn round
	EXPECT_NEAR(std::abs(view.yaw), k_Pi, 1e-5f);
	EXPECT_NEAR(view.pitch, std::atan2(300.0f, 500.0f), 1e-5f);
	// But looks down at least as steeply as the follow does
	view = creature_follow::Start({0.0f, 50.0f, -500.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 15.0f);
	EXPECT_FLOAT_EQ(view.pitch, creature_follow::k_MinPitch);
}

TEST(CreatureFollow, StaysAboutAsCloseWhenAlreadyClose)
{
	const glm::vec3 creature {0.0f, 0.0f, 0.0f};
	// 50 away, between twice the height (30) and the viewing distance (120)
	auto view = creature_follow::Start({0.0f, 30.0f, -40.0f}, creature, creature, 15.0f);
	EXPECT_NEAR(view.distance, 50.0f, 1e-4f);
	// Too close, no nearer than twice its height
	view = creature_follow::Start({0.0f, 5.0f, -5.0f}, creature, creature, 15.0f);
	EXPECT_FLOAT_EQ(view.distance, 30.0f);
}

TEST(CreatureFollow, KeepsWithinItsBounds)
{
	auto view = creature_follow::Clamped(View(0.0f, -1.0f, 0.5f));
	EXPECT_FLOAT_EQ(view.distance, creature_follow::k_MinDistance);
	EXPECT_FLOAT_EQ(view.pitch, creature_follow::k_MinPitch);
	view = creature_follow::Clamped(View(0.0f, 1.0f, 5000.0f));
	EXPECT_FLOAT_EQ(view.distance, creature_follow::k_MaxDistance);
	EXPECT_FLOAT_EQ(view.pitch, 1.0f);
}

TEST(CreatureFollow, EasesInTwoSecondsAtFirstAndOneLater)
{
	EXPECT_FLOAT_EQ(creature_follow::EaseSeconds(0.0f), 2.0f);
	EXPECT_FLOAT_EQ(creature_follow::EaseSeconds(1.0f), 1.5f);
	EXPECT_FLOAT_EQ(creature_follow::EaseSeconds(2.0f), 1.0f);
	EXPECT_FLOAT_EQ(creature_follow::EaseSeconds(30.0f), 1.0f);
}

TEST(CreatureFollow, OriginIsWhereHeadingAndPitchSay)
{
	const glm::vec3 focus {100.0f, 10.0f, 200.0f};
	const auto view = View(0.7f, 0.4f, 80.0f);
	const auto origin = creature_follow::Origin(focus, view);
	EXPECT_NEAR(glm::distance(origin, focus), 80.0f, 1e-3f);
	const auto turned = creature_follow::HeadingPitchOf(origin, focus);
	EXPECT_NEAR(turned.heading, 0.7f, 1e-5f);
	EXPECT_NEAR(turned.pitch, 0.4f, 1e-5f);
	// Straight overhead has no heading, and the game's pitch a little short of a right angle
	const auto overhead = creature_follow::HeadingPitchOf(focus + glm::vec3(0.0f, 10.0f, 0.0f), focus);
	EXPECT_FLOAT_EQ(overhead.heading, 0.0f);
	EXPECT_FLOAT_EQ(overhead.pitch, 1.5393804f);
}

TEST(CreatureFollow, ShiftAndCursorKeysTurnAndTilt)
{
	auto view = View(0.0f, 0.5f, 100.0f);
	// A second of the left key on a 1000 pixel wide screen: 400 pixels, which turns it 0.4 of half a turn
	auto keys = creature_follow::KeysFor(-1, 0, 1.0f);
	keys.shift = true;
	ASSERT_EQ(creature_follow::Apply(view, keys, 1000.0f), creature_follow::KeyOutcome::Stay);
	EXPECT_NEAR(view.yaw, 0.4f * k_Pi, 1e-5f);
	EXPECT_FLOAT_EQ(view.distance, 100.0f);
	// Up tilts it higher, a five-hundredth of a radian a pixel
	keys = creature_follow::KeysFor(0, -1, 0.25f);
	keys.shift = true;
	ASSERT_EQ(creature_follow::Apply(view, keys, 1000.0f), creature_follow::KeyOutcome::Stay);
	EXPECT_NEAR(view.pitch, 0.7f, 1e-5f);
	// But no higher than the most
	keys = creature_follow::KeysFor(0, -1, 10.0f);
	keys.shift = true;
	ASSERT_EQ(creature_follow::Apply(view, keys, 1000.0f), creature_follow::KeyOutcome::Stay);
	EXPECT_FLOAT_EQ(view.pitch, creature_follow::k_MaxKeyPitch);
}

TEST(CreatureFollow, CtrlAndCursorKeysTurnAndZoom)
{
	auto view = View(0.0f, 0.5f, 100.0f);
	// Up for a quarter second: 100 pixels, which brings it in to 0.8 of the way
	auto keys = creature_follow::KeysFor(0, -1, 0.25f);
	keys.ctrl = true;
	ASSERT_EQ(creature_follow::Apply(view, keys, 1000.0f), creature_follow::KeyOutcome::Stay);
	EXPECT_NEAR(view.distance, 80.0f, 1e-3f);
	EXPECT_FLOAT_EQ(view.pitch, 0.5f);
	// Ctrl wins when both are held
	keys = creature_follow::KeysFor(0, 1, 0.25f);
	keys.ctrl = true;
	keys.shift = true;
	ASSERT_EQ(creature_follow::Apply(view, keys, 1000.0f), creature_follow::KeyOutcome::Stay);
	EXPECT_NEAR(view.distance, 96.0f, 1e-3f);
	EXPECT_FLOAT_EQ(view.pitch, 0.5f);
}

TEST(CreatureFollow, CursorKeysAloneGiveTheCameraBack)
{
	auto view = View(0.0f, 0.5f, 100.0f);
	EXPECT_EQ(creature_follow::Apply(view, creature_follow::KeysFor(1, 0, 0.1f), 1000.0f), creature_follow::KeyOutcome::Leave);
	// Nothing held changes nothing
	view = View(0.0f, 0.5f, 100.0f);
	EXPECT_EQ(creature_follow::Apply(view, {}, 1000.0f), creature_follow::KeyOutcome::Stay);
	EXPECT_FLOAT_EQ(view.distance, 100.0f);
}

TEST(CreatureFollow, TheWheelZoomsTwice)
{
	auto view = View(0.0f, 0.5f, 100.0f);
	// A notch out scales the distance by 1.036, applied before and after the cursor keys as the game does
	ASSERT_EQ(creature_follow::Apply(view, {.wheel = -creature_follow::k_WheelNotch}, 1000.0f),
	          creature_follow::KeyOutcome::Stay);
	EXPECT_NEAR(view.distance, 100.0f * 1.036f * 1.036f, 1e-3f);
	// The keys bring it no further out than a thousand
	view = View(0.0f, 0.5f, 990.0f);
	ASSERT_EQ(creature_follow::Apply(view, {.wheel = -creature_follow::k_WheelNotch}, 1000.0f),
	          creature_follow::KeyOutcome::Stay);
	EXPECT_FLOAT_EQ(view.distance, creature_follow::k_MaxKeyDistance);
}

TEST(CreatureFollow, ClearingDistanceIsDrawnTowardsFiftyToAHundred)
{
	EXPECT_FLOAT_EQ(creature_follow::ClearingDistance(10.0f), 42.0f);
	EXPECT_FLOAT_EQ(creature_follow::ClearingDistance(75.0f), 75.0f);
	EXPECT_FLOAT_EQ(creature_follow::ClearingDistance(200.0f), 190.0f);
}

TEST(CreatureFollow, ClearViewKeepsTheHeadingOnLevelLand)
{
	const glm::vec3 focus {0.0f, 7.5f, 0.0f};
	EXPECT_NEAR(creature_follow::ClearHeading(0.3f, 100.0f, focus, Flat), 0.3f, 1e-5f);
}

TEST(CreatureFollow, ClearViewSwingsAwayFromAHill)
{
	// A hill rising to the north (+z) of the creature, the camera behind it to the north
	const auto hill = [](glm::vec2 point) { return std::max(point.y, 0.0f) * 2.0f; };
	const glm::vec3 focus {0.0f, 7.5f, 0.0f};
	const auto heading = creature_follow::ClearHeading(0.0f, 100.0f, focus, hill);
	// It looks for where the land falls away: round to the south, away from the hill
	EXPECT_GT(std::abs(heading), k_Pi / 2.0f - 1e-4f);
	const auto origin = creature_follow::Origin(focus, View(heading, 0.4f, 100.0f));
	EXPECT_LE(origin.z, 1e-3f);
}

TEST(CreatureFollow, ClearViewTiltsTowardsTwentyTwoDegrees)
{
	// Level land's normal is straight up, which the game takes as a little short of a right angle
	const auto slope = creature_follow::SlopePitch({0.0f, 1.0f, 0.0f});
	EXPECT_FLOAT_EQ(slope, 1.5393804f);
	EXPECT_NEAR(creature_follow::ClearPitch(0.5f, slope), (0.1f * 1.5393804f) + (0.2f * 0.5f) + 0.37699112f, 1e-5f);
	// Kept between 22.5 and 60 degrees
	EXPECT_FLOAT_EQ(creature_follow::ClearPitch(-5.0f, 0.0f), k_Pi / 8.0f);
	EXPECT_FLOAT_EQ(creature_follow::ClearPitch(5.0f, 1.5f), k_Pi / 3.0f);
	const auto view = creature_follow::ClearView(View(0.2f, 0.5f, 60.0f), {0.0f, 7.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, Flat);
	EXPECT_FLOAT_EQ(view.distance, 60.0f);
	EXPECT_NEAR(view.yaw, 0.2f, 1e-5f);
}

// The mode's rules

TEST(CreatureMode, CreatureKeyLocksOntoYourCreatureAndLetsGo)
{
	using creature_mode::KeyAction;
	const auto yours = static_cast<entt::entity>(4);
	const auto theirs = static_cast<entt::entity>(9);
	EXPECT_EQ(creature_mode::OnCreatureKey(std::nullopt, std::nullopt), KeyAction::None);
	EXPECT_EQ(creature_mode::OnCreatureKey(std::nullopt, yours), KeyAction::Enter);
	EXPECT_EQ(creature_mode::OnCreatureKey(yours, yours), KeyAction::Leave);
	// On another god's creature, C moves over to yours
	EXPECT_EQ(creature_mode::OnCreatureKey(theirs, yours), KeyAction::Enter);
	// Without a creature of your own, C does nothing even while on theirs
	EXPECT_EQ(creature_mode::OnCreatureKey(theirs, std::nullopt), KeyAction::None);
}

TEST(CreatureMode, DoubleClickIsTwoQuickPressesOnTheSameCreature)
{
	const auto creature = static_cast<entt::entity>(3);
	const auto other = static_cast<entt::entity>(5);
	creature_mode::DoubleClicks clicks;
	EXPECT_FALSE(clicks.OnPress({.milliseconds = 1000, .screen = {100.0f, 100.0f}, .creature = creature}).has_value());
	EXPECT_EQ(clicks.OnPress({.milliseconds = 1300, .screen = {101.0f, 99.0f}, .creature = creature}), creature);
	// A third quick press starts again
	EXPECT_FALSE(clicks.OnPress({.milliseconds = 1400, .screen = {101.0f, 99.0f}, .creature = creature}).has_value());

	// Too slow
	creature_mode::DoubleClicks slow;
	EXPECT_FALSE(slow.OnPress({.milliseconds = 0, .creature = creature}).has_value());
	EXPECT_FALSE(slow.OnPress({.milliseconds = 501, .creature = creature}).has_value());
	// The mouse moved off the first press
	creature_mode::DoubleClicks moved;
	EXPECT_FALSE(moved.OnPress({.milliseconds = 0, .screen = {0.0f, 0.0f}, .creature = creature}).has_value());
	EXPECT_FALSE(moved.OnPress({.milliseconds = 100, .screen = {5.0f, 0.0f}, .creature = creature}).has_value());
	// Another creature, or the land
	creature_mode::DoubleClicks others;
	EXPECT_FALSE(others.OnPress({.milliseconds = 0, .creature = creature}).has_value());
	EXPECT_FALSE(others.OnPress({.milliseconds = 100, .creature = other}).has_value());
	EXPECT_FALSE(others.OnPress({.milliseconds = 200}).has_value());
	EXPECT_FALSE(others.OnPress({.milliseconds = 300}).has_value());
}

TEST(CreatureMode, PassesOutWhenAStatusReachesAHundredPercent)
{
	using creature_physiology::Faint;
	EXPECT_FALSE(creature_mode::PassOutFrom({.damage = 0.99f, .hunger = 0.995f, .tiredness = 0.999f}).has_value());
	EXPECT_EQ(creature_mode::PassOutFrom({.damage = 1.0f}), Faint::OutOfLife);
	EXPECT_EQ(creature_mode::PassOutFrom({.hunger = 1.0f}), Faint::Starving);
	EXPECT_EQ(creature_mode::PassOutFrom({.tiredness = 1.0f}), Faint::Exhausted);
}

TEST(CreatureMode, PassingOutAgreesWithTheBodysFainting)
{
	// At every step of life, energy and exhaustion, the panel at 100% and the body's own fainting agree
	for (int life = 0; life <= 4; ++life)
	{
		for (int energy = -1; energy <= 4; ++energy)
		{
			for (int exhaustion = 0; exhaustion <= 4; ++exhaustion)
			{
				creature_physiology::Needs needs;
				needs.life = static_cast<float>(life) * 0.25f;
				needs.energy = static_cast<float>(energy) * 0.25f;
				needs.exhaustion = static_cast<float>(exhaustion) * 0.25f;
				const auto panel = creature_panel::FromNeeds(needs, std::nullopt);
				EXPECT_EQ(creature_mode::PassOutFrom(panel), creature_physiology::ShouldFaint(needs, 13, true))
				    << "life " << needs.life << " energy " << needs.energy << " exhaustion " << needs.exhaustion;
			}
		}
	}
}

TEST(CreatureMode, PenIsTheHomeThenTheTempleThenTheFallback)
{
	const glm::vec3 home {1.0f, 0.0f, 1.0f};
	const glm::vec3 temple {2.0f, 0.0f, 2.0f};
	const glm::vec3 none {3.0f, 0.0f, 3.0f};
	EXPECT_EQ(creature_mode::PenOf(home, temple, none), home);
	EXPECT_EQ(creature_mode::PenOf(std::nullopt, temple, none), temple);
	EXPECT_EQ(creature_mode::PenOf(std::nullopt, std::nullopt, none), none);
}

TEST(CreatureMode, CreatureHeightGrowsWithSize)
{
	EXPECT_FLOAT_EQ(creature_mode::CreatureHeight(1.0f), 15.0f);
	EXPECT_FLOAT_EQ(creature_mode::CreatureHeight(2.0f), 30.0f);
	EXPECT_GT(creature_mode::CreatureHeight(0.0f), 0.0f);
}

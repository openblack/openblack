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

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "InfoConstants.h"
#include "Magic/AreaEffect.h"
#include "Magic/CastInput.h"
#include "Magic/HandMotion.h"
#include "Magic/Impressiveness.h"
#include "Magic/PrayerRules.h"
#include "Magic/SpellBehaviours.h"
#include "Magic/SpellLifetime.h"

using namespace openblack;
using namespace openblack::magic;
using openblack::ecs::components::PrayerPower;

namespace
{
constexpr float k_Epsilon = 1e-4f;
} // namespace

// How long a miracle lives

TEST(SpellLifetime, AMiracleWithAnEffectLivesWhileItsEffectDoes)
{
	EXPECT_EQ(FateOf({.hasParticleType = true, .effectRunning = true}), SpellFate::Continue);
	// Closed down, its effect dies away first
	EXPECT_EQ(FateOf({.hasParticleType = true, .effectRunning = true, .closedDown = true}), SpellFate::Continue);
	EXPECT_EQ(FateOf({.hasParticleType = true, .effectRunning = false}), SpellFate::Delete);
}

TEST(SpellLifetime, AMiracleWithoutAnEffectLivesUntilItClosesDown)
{
	// The physical shield, teleport and the flocks have no particle effect: they aren't gone on their first turn
	EXPECT_EQ(FateOf({.hasParticleType = false, .effectRunning = false, .closedDown = false}), SpellFate::Continue);
	EXPECT_EQ(FateOf({.hasParticleType = false, .effectRunning = false, .closedDown = true}), SpellFate::Delete);
}

TEST(SpellLifetime, WhatItsKindMadeKeepsItLonger)
{
	EXPECT_EQ(FateOf({.hasParticleType = false, .closedDown = true, .keptByKind = true}), SpellFate::Continue);
	EXPECT_EQ(FateOf({.hasParticleType = true, .effectRunning = false, .closedDown = true, .keptByKind = true}),
	          SpellFate::Continue);
}

TEST(SpellLifetime, ACastIsRefusedWhenItsEffectCantStart)
{
	EXPECT_TRUE(CastRefusedWithoutEffect(true, false));
	EXPECT_FALSE(CastRefusedWithoutEffect(true, true));
	EXPECT_FALSE(CastRefusedWithoutEffect(false, false));
}

TEST(SpellLifetime, TheSeedStaysGoesOrFollowsByItsRecord)
{
	// Lightning, water, food and wood stay in the hand
	EXPECT_EQ(SeedAfterCastOf(true, false, true), SeedAfterCast::StaysInHand);
	// The fireball, the heal and the flocks go
	EXPECT_EQ(SeedAfterCastOf(false, true, false), SeedAfterCast::Deleted);
	// The creature spells' phials are marked both kept and deleted: they go
	EXPECT_EQ(SeedAfterCastOf(true, true, true), SeedAfterCast::Deleted);
	// The storms, the shields and the forest follow their miracle
	EXPECT_EQ(SeedAfterCastOf(false, false, true), SeedAfterCast::FollowsSpell);
	// Teleport neither follows nor is kept
	EXPECT_EQ(SeedAfterCastOf(false, true, false), SeedAfterCast::Deleted);
}

// The action button

namespace
{
constexpr CastProfile k_Gesture {.castType = SpellCastType::SpellCastHandGesture, .castOnObject = false};
constexpr CastProfile k_InHand {.castType = SpellCastType::SpellCastInHand, .castOnObject = false};
constexpr CastProfile k_Position {.castType = SpellCastType::SpellCastHandPosition, .castOnObject = false};
constexpr CastProfile k_OnCreature {.castType = SpellCastType::SpellCastHandPosition, .castOnObject = true};
constexpr PressContext k_OverLand {.seedReady = true, .onValidObject = false, .pointValid = true, .turn = 10};
} // namespace

TEST(CastInput, AGestureSeedIsArmedOnPressAndCastOnRelease)
{
	CastInput input;
	const auto press = Press(input, k_Gesture, k_OverLand);
	EXPECT_TRUE(press.startHoldLoop);
	EXPECT_FALSE(press.cast.has_value());
	EXPECT_EQ(input.state, CastInput::State::Armed);
	const auto release = Release(input);
	EXPECT_TRUE(release.stopHoldLoop);
	EXPECT_EQ(release.cast, CastTarget::Point);
	EXPECT_EQ(input.state, CastInput::State::Idle);
}

TEST(CastInput, AHeldSeedLocksOnCastsAtOnceAndAppliesOnceATurn)
{
	CastInput input;
	const auto press = Press(input, k_InHand, k_OverLand);
	EXPECT_EQ(press.cast, CastTarget::Point);
	EXPECT_EQ(input.state, CastInput::State::Locked);
	// Not again the same turn
	EXPECT_FALSE(Tick(input, 10, true).cast.has_value());
	EXPECT_EQ(Tick(input, 11, true).cast, CastTarget::Point);
	EXPECT_FALSE(Tick(input, 11, true).cast.has_value());
	// Over a place it can't be cast it isn't applied, and nothing is shown
	const auto invalid = Tick(input, 12, false);
	EXPECT_EQ(invalid, CastActions {});
	EXPECT_EQ(TurnsHeld(input, 15), 5u);
	const auto release = Release(input);
	EXPECT_TRUE(release.unlock);
	EXPECT_FALSE(release.cast.has_value());
}

TEST(CastInput, APlacedSeedIsCastOnPress)
{
	CastInput input;
	EXPECT_EQ(Press(input, k_Position, k_OverLand).cast, CastTarget::Point);
	EXPECT_EQ(input.state, CastInput::State::Idle);
	EXPECT_EQ(Release(input), CastActions {});
}

TEST(CastInput, PressingWhereItCantBeCastFails)
{
	CastInput input;
	auto outside = k_OverLand;
	outside.pointValid = false;
	EXPECT_TRUE(Press(input, k_Gesture, outside).fail);
	EXPECT_TRUE(Press(input, k_InHand, outside).fail);
	EXPECT_TRUE(Press(input, k_Position, outside).fail);
	EXPECT_EQ(input.state, CastInput::State::Idle);
}

TEST(CastInput, ACreatureSpellIsCastOnlyOnACreature)
{
	CastInput input;
	EXPECT_TRUE(Press(input, k_OnCreature, k_OverLand).fail);
	auto onCreature = k_OverLand;
	onCreature.onValidObject = true;
	EXPECT_EQ(Press(input, k_OnCreature, onCreature).cast, CastTarget::Object);
}

TEST(CastInput, AnObjectUnderTheHandComesFirst)
{
	CastInput input;
	auto onObject = k_OverLand;
	onObject.onValidObject = true;
	EXPECT_TRUE(Press(input, k_Gesture, onObject).startHoldLoop);
	EXPECT_EQ(Release(input).cast, CastTarget::Object);
	EXPECT_EQ(Press(input, k_InHand, onObject).cast, CastTarget::Object);
	EXPECT_TRUE(input.onObject);
}

TEST(CastInput, APressBeforeTheSeedIsReadyFails)
{
	CastInput input;
	auto early = k_OverLand;
	early.seedReady = false;
	const auto press = Press(input, k_Gesture, early);
	EXPECT_TRUE(press.notReady);
	EXPECT_TRUE(press.fail);
	EXPECT_FALSE(press.startHoldLoop || press.cast.has_value());
	EXPECT_EQ(input.state, CastInput::State::Idle);
	// Over an object it could be cast on too
	early.onValidObject = true;
	EXPECT_TRUE(Press(input, k_InHand, early).fail);
	EXPECT_EQ(input.state, CastInput::State::Idle);
}

TEST(CastInput, APressOutsideThePlayersInfluenceDoesNothingAtAll)
{
	CastInput input;
	auto outside = k_OverLand;
	outside.inInfluence = false;
	EXPECT_EQ(Press(input, k_Gesture, outside), CastActions {});
	EXPECT_EQ(Press(input, k_InHand, outside), CastActions {});
	outside.seedReady = false;
	outside.pointValid = false;
	EXPECT_EQ(Press(input, k_Position, outside), CastActions {});
	outside.onValidObject = true;
	EXPECT_EQ(Press(input, k_OnCreature, outside), CastActions {});
	EXPECT_EQ(input.state, CastInput::State::Idle);
}

TEST(CastInput, CancellingStopsTheHumOrStoresTheHeldMiracleBack)
{
	CastInput input;
	(void)Press(input, k_Gesture, k_OverLand);
	const auto armed = Cancel(input);
	EXPECT_TRUE(armed.stopHoldLoop);
	EXPECT_FALSE(armed.cast.has_value());
	(void)Press(input, k_InHand, k_OverLand);
	EXPECT_TRUE(Cancel(input).unlock);
	EXPECT_EQ(input.state, CastInput::State::Idle);
	EXPECT_EQ(Cancel(input), CastActions {});
}

TEST(CastInput, ASecondPressWhileArmedIsIgnored)
{
	CastInput input;
	(void)Press(input, k_Gesture, k_OverLand);
	EXPECT_EQ(Press(input, k_Position, k_OverLand), CastActions {});
	EXPECT_EQ(input.state, CastInput::State::Armed);
}

// How the hand moves

TEST(HandMotion, TheHandsMovementGetsFourFifthsOfTheWayInATenthOfASecond)
{
	const glm::vec3 raw {100.0f, 0.0f, 0.0f};
	EXPECT_NEAR(FilterHandVelocity(glm::vec3(0.0f), raw, 0.1f).x, 80.0f, 0.01f);
	// The same in many small steps
	glm::vec3 smoothed(0.0f);
	for (int i = 0; i < 10; ++i)
	{
		smoothed = FilterHandVelocity(smoothed, raw, 0.01f);
	}
	EXPECT_NEAR(smoothed.x, 80.0f, 0.01f);
	EXPECT_EQ(FilterHandVelocity(glm::vec3(3.0f), raw, 0.0f), glm::vec3(3.0f));
}

TEST(HandMotion, ANaturalSplinePassesThroughItsPointsWithoutBendingAtTheEnds)
{
	const CubicSpline curve(k_PourKeyPoints);
	for (const auto& point : k_PourKeyPoints)
	{
		EXPECT_NEAR(curve(point.x), point.y, k_Epsilon);
	}
	// Symmetric, and rising above the held points between them
	EXPECT_NEAR(curve(0.1f), curve(0.9f), k_Epsilon);
	EXPECT_NEAR(curve(0.5f), 1.6136f, 1e-3f);
	// A straight line stays one
	const std::array line {glm::vec2 {0.0f, 0.0f}, glm::vec2 {1.0f, 2.0f}, glm::vec2 {3.0f, 6.0f}};
	EXPECT_NEAR(CubicSpline(line)(2.0f), 4.0f, k_Epsilon);
}

TEST(HandMotion, ThePoursCurveIsFlatAtBothEndsAndSwellsToAlmostTwiceItsPoints)
{
	const CubicSpline curve(k_PourKeyPoints, 0.0f, 0.0f);
	for (const auto& point : k_PourKeyPoints)
	{
		EXPECT_NEAR(curve(point.x), point.y, k_Epsilon);
	}
	EXPECT_NEAR(curve(0.1f), 0.3393f, 1e-3f);
	EXPECT_NEAR(curve(0.9f), 0.3393f, 1e-3f);
	EXPECT_NEAR(curve(0.5f), 1.9643f, 1e-3f);
	// Flat at the ends: barely moving a hair in
	EXPECT_NEAR(curve(0.001f), 0.0f, 1e-4f);
}

TEST(HandMotion, FoodAndWoodLiftTheHandOverFourSecondsAndStartOver)
{
	PourState pour;
	StartPour(pour, k_FoodWoodPour);
	EXPECT_TRUE(pour.active);
	// Eight tenths of a second in, the curve's first key point: up ten, tipped 61 degrees
	for (int turn = 0; turn < 8; ++turn)
	{
		StepPour(pour, 0.1f);
	}
	EXPECT_NEAR(pour.current.raise, 10.0f, 1e-3f);
	EXPECT_NEAR(pour.current.tilt, 1.07257f, 1e-3f);
	// Halfway through, at the top of the curve: up 19.6 and tipped 121 degrees
	for (int turn = 8; turn < 20; ++turn)
	{
		StepPour(pour, 0.1f);
	}
	EXPECT_NEAR(pour.current.raise, 19.643f, 1e-2f);
	EXPECT_NEAR(pour.current.tilt, 2.1068f, 1e-3f);
	// Halfway between turns it is halfway between their poses
	const auto half = PourPoseAt(pour, 0.5f);
	EXPECT_NEAR(half.raise, (pour.previous.raise + pour.current.raise) * 0.5f, k_Epsilon);
	// At the end it is down again, and it starts over
	for (int turn = 20; turn < 40; ++turn)
	{
		StepPour(pour, 0.1f);
	}
	EXPECT_NEAR(pour.current.raise, 0.0f, 1e-2f);
	StepPour(pour, 0.1f);
	EXPECT_TRUE(pour.active);
	StepPour(pour, 0.1f);
	EXPECT_GT(pour.current.raise, 0.0f);
	// Stopped, it comes back to rest over the next turn from where it was drawn
	const float before = pour.current.raise;
	StopPour(pour);
	EXPECT_FLOAT_EQ(PourPoseAt(pour, 1.0f).raise, 0.0f);
	EXPECT_FLOAT_EQ(PourPoseAt(pour, 0.0f).raise, pour.previous.raise);
	EXPECT_NEAR(PourPoseAt(pour, 0.5f).raise, pour.previous.raise * 0.5f, k_Epsilon);
	EXPECT_GT(before, 0.0f);
	StepPour(pour, 0.1f);
	EXPECT_FLOAT_EQ(PourPoseAt(pour, 0.0f).raise, 0.0f);
}

TEST(HandMotion, APourThatDoesntLoopStops)
{
	PourState pour;
	StartPour(pour, {.totalTime = 1.0f, .heightToRaise = 4.0f, .angleToRaise = 1.0f, .loops = false});
	for (int turn = 0; turn < 12; ++turn)
	{
		StepPour(pour, 0.1f);
	}
	EXPECT_FALSE(pour.active);
}

// Prayer power

TEST(PrayerRules, ThePlayerPaysFromTheirStoreAsMuchAsTheyHave)
{
	PrayerPower store {.chants = 100.0f};
	EXPECT_FLOAT_EQ(DrawPrayer(store, 30.0f), 30.0f);
	EXPECT_FLOAT_EQ(store.chants, 70.0f);
	EXPECT_FLOAT_EQ(DrawPrayer(store, 100.0f), 70.0f);
	EXPECT_FLOAT_EQ(store.chants, 0.0f);
	EXPECT_FLOAT_EQ(DrawPrayer(store, -5.0f), 0.0f);
	PrayerPower infinite {.chants = 0.0f, .infinite = true};
	EXPECT_FLOAT_EQ(DrawPrayer(infinite, 5000.0f), 5000.0f);
	EXPECT_FLOAT_EQ(infinite.chants, 0.0f);
}

TEST(PrayerRules, AMiracleFromAGlobesSeedIsToppedUpByNobodyButTheNeutralPlayer)
{
	GlobeSpellCaster caster;
	caster.Bind(PlayerNames::PLAYER_ONE);
	EXPECT_FLOAT_EQ(caster.MaintainSpell(40.0f), 0.0f);
	caster.Bind(PlayerNames::NEUTRAL);
	EXPECT_FLOAT_EQ(caster.MaintainSpell(40.0f), 40.0f);
}

TEST(PrayerRules, TheNeutralPlayerGivesAllAndAPlayerWithoutAStoreNothing)
{
	EXPECT_FLOAT_EQ(PlayerMaintainSpell(PlayerNames::NEUTRAL, nullptr, 40.0f), 40.0f);
	EXPECT_FLOAT_EQ(PlayerMaintainSpell(PlayerNames::PLAYER_ONE, nullptr, 40.0f), 0.0f);
	PrayerPower store {.chants = 25.0f};
	EXPECT_FLOAT_EQ(PlayerMaintainSpell(PlayerNames::PLAYER_ONE, &store, 40.0f), 25.0f);
}

TEST(PrayerRules, ASummonedSeedIsChargedAndGivesBackWhatItHoldsWhenDropped)
{
	PrayerPower store {.chants = 10000.0f};
	const float charge = ChargeSeed(store, 7000.0f);
	EXPECT_FLOAT_EQ(charge, 7000.0f);
	EXPECT_FLOAT_EQ(store.chants, 3000.0f);
	// Never cast: all of it back
	ReturnPrayer(store, SeedRefund(true, charge, -1.0f, false));
	EXPECT_FLOAT_EQ(store.chants, 10000.0f);
	// A held miracle let go: what it had left
	EXPECT_FLOAT_EQ(SeedRefund(true, charge, 1200.0f, true), 1200.0f);
	// Cast and gone: nothing
	EXPECT_FLOAT_EQ(SeedRefund(true, charge, -1.0f, true), 0.0f);
	// A bubble's seed was never the player's
	EXPECT_FLOAT_EQ(SeedRefund(false, charge, -1.0f, false), 0.0f);
	EXPECT_TRUE(ReadyAtOnce(SeedOrigin::Bubble));
	EXPECT_FALSE(ReadyAtOnce(SeedOrigin::Worship));
}

// Effects over an area

namespace
{
/// The alignment table's burn, crush, hit, heal and fly-away rows, as the game's tables have them for a villager,
/// a nasty animal and a building
std::array<GAlignmentInfo, 7> AlignmentTable()
{
	std::array<GAlignmentInfo, 7> table {};
	table[0].villager = -1.0f;
	table[1].villager = -0.9f;
	table[2].villager = -0.8f;
	table[3].villager = 1.0f;
	table[4].villager = -0.25f;
	table[2].animalNasty = 0.6f;
	table[2].building = -0.1f;
	return table;
}

EffectReceiver Villager(entt::entity entity, glm::vec3 position, float life)
{
	return {.entity = entity,
	        .position = position,
	        .radius = 0.5f,
	        .height = 2.0f,
	        .life = life,
	        .defence = {},
	        .alignmentType = AlignmentType::Villager,
	        .crushable = true};
}
} // namespace

TEST(AreaEffect, TheCellsSearchedSpanTheSquareOfTheRadius)
{
	const auto cells = EffectCellsAround({55.0f, 0.0f, 55.0f}, 12.0f);
	EXPECT_EQ(cells.first, glm::ivec2(4, 4));
	EXPECT_EQ(cells.last, glm::ivec2(6, 6));
	EXPECT_EQ(cells.Count(), 9u);
	EXPECT_TRUE(cells.Contains({5, 6}));
	EXPECT_FALSE(cells.Contains({7, 5}));
	EXPECT_EQ(EffectCellOf({-0.5f, 0.0f, 19.9f}), glm::ivec2(-1, 1));
}

TEST(AreaEffect, EveryObjectInReachTakesItNotOnlyTheNearest)
{
	EffectValues values;
	values[EffectKind::Hit] = 0.2f;
	values.radius = 3.0f;
	const auto table = AlignmentTable();
	const std::array receivers {
	    Villager(static_cast<entt::entity>(1), {1.0f, 0.0f, 0.0f}, 1.0f),
	    Villager(static_cast<entt::entity>(2), {0.0f, 0.0f, 3.4f}, 1.0f),
	    // Too far across the land: 3 + 0.5 is the reach
	    Villager(static_cast<entt::entity>(3), {3.6f, 0.0f, 0.0f}, 1.0f),
	    // Too far above
	    Villager(static_cast<entt::entity>(4), {0.0f, 5.5f, 0.0f}, 1.0f),
	};
	const auto outcomes = ApplyEffectToAll(values, glm::vec3(0.0f), receivers, table, 0.75f, 0.0f);
	ASSERT_EQ(outcomes.size(), 2u);
	EXPECT_EQ(outcomes[0].entity, static_cast<entt::entity>(1));
	EXPECT_EQ(outcomes[1].entity, static_cast<entt::entity>(2));
	// No falloff with distance
	EXPECT_FLOAT_EQ(*outcomes[0].lifeAfter, 0.8f);
	EXPECT_FLOAT_EQ(*outcomes[1].lifeAfter, 0.8f);
	EXPECT_TRUE(outcomes[0].aggression);
}

TEST(AreaEffect, AnEventsValuesCarryTheTribalPowerTwice)
{
	EffectValues values;
	values[EffectKind::Burn] = 100.0f;
	values.radius = 2.0f;
	// Paid strength 1.5 already carries a tribal power of 1.5
	const auto scaled = EventEffectValues(values, 1.5f, 1.5f, 0.5f);
	EXPECT_FLOAT_EQ(scaled[EffectKind::Burn], 100.0f * 1.5f * 1.5f * 0.5f);
	EXPECT_FLOAT_EQ(scaled.radius, 2.0f);
}

TEST(AreaEffect, HurtingVillagersTurnsTheCasterEvilAndHealingGood)
{
	const auto table = AlignmentTable();
	EffectValues hit;
	hit[EffectKind::Hit] = 0.4f;
	const auto hurt = ApplyEffectTo(hit, Villager(static_cast<entt::entity>(1), {}, 1.0f), table, 0.75f, 0.0f);
	// 0.4 hit times the villager's -0.8 times (0.4 of life lost + 0.75)
	EXPECT_NEAR(hurt.alignmentChange, 0.4f * -0.8f * (0.4f + 0.75f), k_Epsilon);
	EffectValues heal;
	heal[EffectKind::Heal] = 0.3f;
	const auto healed = ApplyEffectTo(heal, Villager(static_cast<entt::entity>(1), {}, 0.5f), table, 0.75f, 0.0f);
	EXPECT_NEAR(healed.alignmentChange, 0.3f * 1.0f * (0.3f + 0.75f), k_Epsilon);
	EXPECT_FALSE(healed.aggression);
	// Nothing changed, nothing moves
	const auto full = ApplyEffectTo(heal, Villager(static_cast<entt::entity>(1), {}, 1.0f), table, 0.75f, 0.0f);
	EXPECT_FLOAT_EQ(full.alignmentChange, 0.0f);
}

TEST(AreaEffect, HeatCountsOnlyWithAChangeOfLifeAndInFull)
{
	const auto table = AlignmentTable();
	auto receiver = Villager(static_cast<entt::entity>(1), {}, 1.0f);
	receiver.defence.combustionTemperature = 100.0f;
	receiver.defence.multipliers.at(static_cast<size_t>(EffectKind::Burn)) = 1.0f;
	// Heat alone heats, taking no life, so the caster's alignment doesn't move
	EffectValues burn;
	burn[EffectKind::Burn] = 1100.0f;
	EXPECT_FLOAT_EQ(ApplyEffectTo(burn, receiver, table, 0.75f, 0.0f).alignmentChange, 0.0f);
	// With a hit it counts by the harm it would do, 1 here, though that is more than the life lost
	burn[EffectKind::Hit] = 0.1f;
	const auto outcome = ApplyEffectTo(burn, receiver, table, 0.75f, 0.0f);
	const float weight = 0.1f + 0.75f;
	EXPECT_NEAR(outcome.alignmentChange, (0.1f * -0.8f * weight) + (1.0f * -1.0f * weight), k_Epsilon);
}

TEST(AreaEffect, TheChangeIsDampedByHowFarTheCasterLeans)
{
	// An evil caster turns more evil less, and good more
	EXPECT_NEAR(DampAlignmentChange(-0.1f, -0.5f), -0.1f * 0.75f, k_Epsilon);
	EXPECT_NEAR(DampAlignmentChange(0.1f, -0.5f), 0.1f * 1.25f, k_Epsilon);
	EXPECT_NEAR(DampAlignmentChange(0.1f, 0.0f), 0.1f, k_Epsilon);
}

TEST(AreaEffect, AKillDestroysItAndACrushMakesPeopleReact)
{
	const auto table = AlignmentTable();
	EffectValues crush;
	crush[EffectKind::Crush] = 2.0f;
	const auto outcome = ApplyEffectTo(crush, Villager(static_cast<entt::entity>(1), {}, 0.5f), table, 0.75f, 0.0f);
	EXPECT_TRUE(outcome.destroyed);
	EXPECT_TRUE(outcome.crushed);
	EXPECT_FLOAT_EQ(*outcome.lifeAfter, 0.0f);
	// Something without life is reached but loses none, and moves no alignment
	EffectReceiver hut {.entity = static_cast<entt::entity>(9), .radius = 4.0f, .height = 5.0f};
	hut.alignmentType = AlignmentType::Building;
	const auto building = ApplyEffectTo(crush, hut, table, 0.75f, 0.0f);
	EXPECT_FALSE(building.lifeAfter.has_value());
	EXPECT_FALSE(building.destroyed || building.crushed);
	EXPECT_FLOAT_EQ(building.alignmentChange, 0.0f);
}

TEST(AreaEffect, ThePendingAlignmentScalesTheTurnsChangeAndIsThenGone)
{
	float pending = 0.5f;
	float alignment = 0.0f;
	alignment = StepPendingAlignment(alignment, pending, 0.0019444f);
	EXPECT_NEAR(alignment, 0.5f * 0.0019444f, 1e-7f);
	EXPECT_FLOAT_EQ(pending, 0.0f);
	// Nothing carries over to the next turn
	alignment = StepPendingAlignment(alignment, pending, 0.0019444f);
	EXPECT_NEAR(alignment, 0.5f * 0.0019444f, 1e-7f);
	// More than a whole change waiting counts as a whole one
	pending = -5.0f;
	alignment = StepPendingAlignment(0.0f, pending, 0.01f);
	EXPECT_FLOAT_EQ(alignment, -0.01f);
	pending = -5.0f;
	alignment = StepPendingAlignment(-0.999f, pending, 0.01f);
	EXPECT_FLOAT_EQ(alignment, -1.0f);
}

// Impressiveness

TEST(Impressiveness, CloseByAllOfItAtTheEdgeAboutATenth)
{
	EXPECT_FLOAT_EQ(DistanceChangeToBelief(0.0f, 60.0f), 1.0f);
	EXPECT_NEAR(DistanceChangeToBelief(60.0f, 60.0f), 0.11443728f, 1e-6f);
	EXPECT_NEAR(DistanceChangeToBelief(600.0f, 60.0f), 0.11443728f, 1e-6f);
	// Falling all the way between
	EXPECT_GT(DistanceChangeToBelief(20.0f, 60.0f), DistanceChangeToBelief(50.0f, 60.0f));
}

TEST(Impressiveness, ItMultipliesTheLandsBalanceTheMiracleTheReactionAndTheBoredom)
{
	const float value = ImpressiveValue({.landBalance = 2.0f,
	                                     .impressiveValue = 3.0f,
	                                     .reactionMultiplier = 0.5f,
	                                     .distance = 0.0f,
	                                     .maxDistance = 60.0f,
	                                     .power = 1.0f,
	                                     .boredom = 0.25f});
	EXPECT_FLOAT_EQ(value, 2.0f * 3.0f * 0.5f * 0.25f);
	// Each impression adds the belief table's boredom, -0.02, by the villager's share of its town; never below 0
	EXPECT_FLOAT_EQ(BoredomAfterImpression(1.0f, -0.02f * 0.5f), 0.99f);
	EXPECT_FLOAT_EQ(BoredomAfterImpression(0.005f, -0.02f), 0.0f);
}

TEST(Impressiveness, BoredomWearsOffAtEachTownTurnBelowOne)
{
	EXPECT_FLOAT_EQ(BoredomAtTownTurn(0.5f, 0.00056f, 1.0f), 0.5f + 0.00056f);
	EXPECT_FLOAT_EQ(BoredomAtTownTurn(0.5f, 0.00056f, 2.0f), 0.5f + 0.00112f);
	// It is kept as it was rather than reach 1
	EXPECT_FLOAT_EQ(BoredomAtTownTurn(0.9999f, 0.00056f, 1.0f), 0.9999f);
	EXPECT_FLOAT_EQ(BoredomAtTownTurn(1.0f, 0.00056f, 1.0f), 1.0f);
}

TEST(Impressiveness, AnImpressionMovesTheAlignmentByItsKindAndTheTownsDesire)
{
	EXPECT_FLOAT_EQ(ImpressionAlignment(-0.01f, std::nullopt), -0.01f);
	EXPECT_FLOAT_EQ(ImpressionAlignment(0.01f, 0.5f), 0.005f);
}

TEST(Impressiveness, CloserMeansMoreUrgentToFlee)
{
	EXPECT_FLOAT_EQ(FleeFromSpellPriority(150.0f, 0.0f), 250.0f);
	EXPECT_NEAR(FleeFromSpellPriority(150.0f, k_FleeUrgencyReach * 0.5f), 200.0f, k_Epsilon);
	EXPECT_FLOAT_EQ(FleeFromSpellPriority(150.0f, 200.0f), 150.0f);
}

// What a miracle looks to a creature like its player did

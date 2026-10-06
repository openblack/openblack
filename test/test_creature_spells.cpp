/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <algorithm>
#include <array>

#include <gtest/gtest.h>

#include "Creature/CreatureDesires.h"
#include "Creature/CreatureSpells.h"

using namespace openblack;
using namespace openblack::creature_spells;

namespace
{
constexpr float k_TurnsPerSecond = 10.0f;
constexpr float k_Epsilon = 1e-4f;

/// The game's times: every spell holds 25 seconds; freeze eases in and out over 2, the body spells 4, invisible 5
std::array<Timing, k_SpellCount> Timings()
{
	std::array<Timing, k_SpellCount> timings {};
	timings.fill({.startSeconds = 4.0f, .finishSeconds = 4.0f});
	timings.at(static_cast<size_t>(Spell::Freeze)) = {.startSeconds = 2.0f, .finishSeconds = 2.0f};
	timings.at(static_cast<size_t>(Spell::Invisible)) = {.startSeconds = 5.0f, .finishSeconds = 5.0f};
	timings.at(static_cast<size_t>(Spell::Nice)) = {.startSeconds = 3.0f, .finishSeconds = 3.0f};
	timings.at(static_cast<size_t>(Spell::Itchy)) = {.startSeconds = 1.0f, .finishSeconds = 1.0f};
	return timings;
}

const auto k_Miracle = static_cast<entt::entity>(42);
const auto k_Other = static_cast<entt::entity>(43);

/// Steps until a spell is off, counting the turns and keeping every event
struct Run
{
	int turns {0};
	std::vector<TurnEvent> events;
	std::vector<entt::entity> ended;
};
Run StepUntilOff(Spells& spells, Spell spell, int limit = 2000)
{
	const auto timings = Timings();
	Run run;
	while (spells.IsActive(spell) && run.turns < limit)
	{
		auto turn = Step(spells, timings, k_TurnsPerSecond);
		run.events.insert(run.events.end(), turn.events.begin(), turn.events.end());
		run.ended.insert(run.ended.end(), turn.ended.begin(), turn.ended.end());
		++run.turns;
	}
	return run;
}
} // namespace

TEST(CreatureSpells, TheMagicTypesOfTheCreatureSpellsAreTheirSpells)
{
	EXPECT_EQ(SpellOf(MagicType::CreatureSpellFreeze), Spell::Freeze);
	EXPECT_EQ(SpellOf(MagicType::CreatureSpellBig), Spell::Big);
	EXPECT_EQ(SpellOf(MagicType::CreatureSpellItchy), Spell::Itchy);
	EXPECT_FALSE(SpellOf(MagicType::Fireball).has_value());
	EXPECT_EQ(SpellOf(CreatureReceiveSpellType::CreatureReceiveSpellCompassionate), Spell::Nice);
}

TEST(CreatureSpells, KindsKeepOpposingSpellsApart)
{
	EXPECT_EQ(KindOf(Spell::Small), KindOf(Spell::Big));
	EXPECT_EQ(KindOf(Spell::Weak), KindOf(Spell::Strong));
	EXPECT_EQ(KindOf(Spell::Nice), KindOf(Spell::Nasty));
	EXPECT_EQ(KindOf(Spell::Nice), KindOf(Spell::Itchy));
	EXPECT_EQ(KindOf(Spell::Freeze), KindOf(Spell::Invisible));
	EXPECT_NE(KindOf(Spell::Big), KindOf(Spell::Strong));
}

TEST(CreatureSpells, TheMoodSpellsMakeTheirDesireDominant)
{
	using creature_desires::Desire;
	EXPECT_EQ(EffectOf(Spell::Nice).desire, static_cast<uint8_t>(Desire::Compassion));
	EXPECT_EQ(EffectOf(Spell::Nasty).desire, static_cast<uint8_t>(Desire::Anger));
	EXPECT_EQ(EffectOf(Spell::Itchy).desire, static_cast<uint8_t>(Desire::Scratch));
	EXPECT_FALSE(EffectOf(Spell::Big).desire.has_value());
	EXPECT_EQ(EffectOf(Spell::Freeze).soundAction, 0x76);
	EXPECT_EQ(EffectOf(Spell::Itchy).soundAction, 0x7E);
}

TEST(CreatureSpells, ASpellEasesInHoldsForItsTimeAndEasesOut)
{
	Spells spells;
	const auto result = Receive(spells, Spell::Big, TurnsOf(25.0f, k_TurnsPerSecond), k_Miracle);
	EXPECT_EQ(result.received, Received::Started);
	const auto run = StepUntilOff(spells, Spell::Big);
	// A turn to start, four seconds in, a turn to hold, 25 held, a turn to begin wearing off, four out and a turn to end
	EXPECT_EQ(run.turns, 1 + 40 + 1 + 250 + 1 + 40 + 1);
	ASSERT_FALSE(run.events.empty());
	EXPECT_EQ(run.events.front().event, Event::Start);
	EXPECT_EQ(run.events.back().event, Event::Finish);
	EXPECT_EQ(run.ended, std::vector<entt::entity> {k_Miracle});
	// The ratio rises to 1, holds, and falls back to 0
	float last = -1.0f;
	bool rising = true;
	for (const auto& event : run.events)
	{
		if (event.event != Event::Ease)
		{
			continue;
		}
		if (rising && event.ratio < last)
		{
			rising = false;
			EXPECT_NEAR(last, 1.0f, k_Epsilon);
		}
		EXPECT_TRUE(rising ? event.ratio >= last : event.ratio <= last + k_Epsilon);
		last = event.ratio;
	}
	EXPECT_FALSE(rising);
	EXPECT_NEAR(last, 0.0f, k_Epsilon);
}

TEST(CreatureSpells, CastingTheSameSpellAgainAddsItsTimeAndTakesTheNewMiracle)
{
	Spells spells;
	const auto timings = Timings();
	Receive(spells, Spell::Freeze, 100, k_Miracle);
	for (int turn = 0; turn < 30; ++turn)
	{
		(void)Step(spells, timings, k_TurnsPerSecond);
	}
	ASSERT_EQ(spells[Spell::Freeze].phase, Phase::Holding);
	const int left = spells[Spell::Freeze].turnsLeft;
	const auto result = Receive(spells, Spell::Freeze, 50, k_Other);
	EXPECT_EQ(result.received, Received::Extended);
	EXPECT_EQ(result.replaced, k_Miracle);
	EXPECT_EQ(spells[Spell::Freeze].turnsLeft, left + 50);
	EXPECT_EQ(spells[Spell::Freeze].miracle, k_Other);
}

TEST(CreatureSpells, TheOpposingSpellCutsTheFirstShortAndWaitsForIt)
{
	Spells spells;
	const auto timings = Timings();
	Receive(spells, Spell::Small, 250, k_Miracle);
	for (int turn = 0; turn < 100; ++turn)
	{
		(void)Step(spells, timings, k_TurnsPerSecond);
	}
	ASSERT_EQ(spells[Spell::Small].phase, Phase::Holding);
	const auto result = Receive(spells, Spell::Big, 250, k_Other);
	EXPECT_EQ(result.received, Received::Queued);
	EXPECT_FALSE(spells.IsActive(Spell::Big));
	// Small eases out at once, then big starts
	auto run = StepUntilOff(spells, Spell::Small);
	EXPECT_EQ(run.turns, 1 + 40 + 1);
	EXPECT_TRUE(spells.IsActive(Spell::Big));
	EXPECT_TRUE(spells.waiting.empty());
	EXPECT_EQ(spells[Spell::Big].miracle, k_Other);
}

TEST(CreatureSpells, SpellsOfDifferentKindsRunTogether)
{
	Spells spells;
	EXPECT_EQ(Receive(spells, Spell::Big, 250, k_Miracle).received, Received::Started);
	EXPECT_EQ(Receive(spells, Spell::Strong, 250, k_Other).received, Received::Started);
	EXPECT_TRUE(spells.IsKindActive(Kind::Size));
	EXPECT_TRUE(spells.IsKindActive(Kind::Strength));
	EXPECT_FALSE(spells.IsKindActive(Kind::Mood));
}

TEST(CreatureSpells, ASpellWithoutATimeHoldsUntilCutShort)
{
	Spells spells;
	const auto timings = Timings();
	Receive(spells, Spell::Weak, TurnsOf(0.0f, k_TurnsPerSecond), k_Miracle);
	for (int turn = 0; turn < 1000; ++turn)
	{
		(void)Step(spells, timings, k_TurnsPerSecond);
	}
	EXPECT_EQ(spells[Spell::Weak].phase, Phase::Holding);
	// Strong cuts it short
	Receive(spells, Spell::Strong, 10, k_Other);
	const auto run = StepUntilOff(spells, Spell::Weak);
	EXPECT_LT(run.turns, 100);
}

TEST(CreatureSpells, TheBodySpellsTargets)
{
	EXPECT_NEAR(SizeTarget(Spell::Big, 1.0f, 0.05f, 4.0f), 1.8f, k_Epsilon);
	EXPECT_NEAR(SizeTarget(Spell::Small, 1.0f, 0.05f, 4.0f), 0.5555556f, k_Epsilon);
	// No bigger than the largest, never smaller for big; no smaller than the smallest
	EXPECT_NEAR(SizeTarget(Spell::Big, 3.0f, 0.05f, 4.0f), 4.0f, k_Epsilon);
	EXPECT_NEAR(SizeTarget(Spell::Big, 5.0f, 0.05f, 4.0f), 5.0f, k_Epsilon);
	EXPECT_NEAR(SizeTarget(Spell::Small, 0.06f, 0.05f, 4.0f), 0.05f, k_Epsilon);
	EXPECT_EQ(Target(Spell::Weak), k_WeakStrength);
	EXPECT_EQ(Target(Spell::Strong), k_StrongStrength);
	EXPECT_EQ(Target(Spell::Fat), k_FatFatness);
	EXPECT_EQ(Target(Spell::Invisible), k_InvisibleFizz);
	EXPECT_EQ(Target(Spell::Nasty), k_NastyAlignment);
	EXPECT_FALSE(Target(Spell::Itchy).has_value());
	EXPECT_NEAR(Ease(0.5f, 1.0f, 0.5f), 0.75f, k_Epsilon);
}

TEST(CreatureSpells, TheFrozenLookTintsTowardsIcyBlue)
{
	EXPECT_EQ(FrozenTint(0.0f), 0xFFFFFFu);
	EXPECT_EQ(FrozenTint(1.0f), k_FrozenColour);
	const auto half = FrozenTint(0.5f);
	EXPECT_EQ((half >> 16) & 0xFFu, (255u + 0x8Cu + 1u) / 2u);
	EXPECT_EQ(half & 0xFFu, 0xFFu);
}

TEST(CreatureSpells, TimesAreWholeTurns)
{
	EXPECT_EQ(TurnsOf(25.0f, k_TurnsPerSecond), 250);
	EXPECT_EQ(TurnsOf(0.0f, k_TurnsPerSecond), -1);
	Slot slot {.phase = Phase::Starting, .turnsLeft = 20, .holdTurns = 0, .miracle = entt::null, .before = 0.0f};
	EXPECT_NEAR(Ratio(slot, {.startSeconds = 4.0f, .finishSeconds = 4.0f}, k_TurnsPerSecond), 0.5f, k_Epsilon);
	slot.phase = Phase::Finishing;
	EXPECT_NEAR(Ratio(slot, {.startSeconds = 4.0f, .finishSeconds = 4.0f}, k_TurnsPerSecond), 0.5f, k_Epsilon);
}

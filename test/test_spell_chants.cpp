/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <vector>

#include <gtest/gtest.h>

#include "InfoConstants.h"
#include "Magic/MagicTables.h"
#include "Magic/SpellChants.h"

using namespace openblack;
using namespace openblack::magic;

namespace
{
/// A caster that gives everything it is asked for (like the scripts' neutral player) or nothing (like a player casting
/// from the hand), and remembers what it gave and what was spent
class FakeCaster final: public SpellCasterInterface
{
public:
	explicit FakeCaster(bool refills)
	    : _refills(refills)
	{
	}

	float MaintainSpell(float amount) override
	{
		const float result = _refills ? amount : 0.0f;
		given += result;
		return result;
	}

	void OnChantsSpent(float chants, ChantCharge charge) override { spent.push_back({chants, charge}); }

	struct Spent
	{
		float chants;
		ChantCharge charge;
	};

	float given {0.0f};
	std::vector<Spent> spent;

private:
	bool _refills;
};

/// A lightning bolt: 5000 to start, 50 a turn, 2 an event, recharged
SpellChantRules LightningRules()
{
	return {.costPerEvent = 2.0f, .costToMaintain = 50.0f, .recharged = true};
}

/// A shield: maintained, 20 a turn at its normal size, cheaper with a strong tribal power
SpellChantRules ShieldRules()
{
	return {.costToMaintain = 20.0f, .maintained = true, .recharged = true, .divideCostsByTribalPower = true};
}

struct Turn
{
	float strength;
	bool closed;
};

/// One turn of a running miracle: it ages, ends when its time is up, otherwise reads its strength, pays its upkeep
/// and is recharged, and ends once it has no strength left
Turn RunTurn(SpellChants& spell, float& age, float duration, const SpellChantRules& rules, FakeCaster& caster)
{
	age += 0.1f;
	if (duration >= 0.0f && age > duration)
	{
		return {0.0f, true};
	}
	const float strength = GetSpellStrength(spell, rules, &caster);
	PayForOneTurn(spell, rules, &caster);
	Recharge(spell, rules, caster);
	return {strength, strength <= 0.0f};
}
} // namespace

TEST(SpellChants, rulesFromTables)
{
	GMagicInfo magic {};
	magic.isSpellRecharged = 1;
	GMagicEffectInfo effect {};
	effect.costPerEvent = 2.0f;
	effect.costPerGameTurn = 20.0f;
	effect.divideCostsByTribalPower = 1;
	const auto rules = ChantRulesFor(MagicType::Shield, magic, effect);
	EXPECT_FLOAT_EQ(rules.costPerEvent, 2.0f);
	EXPECT_FLOAT_EQ(rules.costToMaintain, 20.0f);
	EXPECT_TRUE(rules.maintained);
	EXPECT_TRUE(rules.recharged);
	EXPECT_TRUE(rules.divideCostsByTribalPower);
	EXPECT_FALSE(ChantRulesFor(MagicType::LightningBolt, magic, effect).maintained);
}

TEST(SpellChants, safetyLevelIsFiveSecondsOfUpkeep)
{
	SpellChants spell;
	SetChants(spell, 5000.0f);
	FakeCaster caster(false);
	// 50 a turn x 10 turns a second x 5 seconds
	EXPECT_FLOAT_EQ(GetChantSafetyLevel(spell, LightningRules()), 2500.0f);
	EXPECT_FLOAT_EQ(GetSpellStrength(spell, LightningRules(), &caster), 1.0f);
	// at most what it was filled with
	SetChants(spell, 1000.0f);
	EXPECT_FLOAT_EQ(GetChantSafetyLevel(spell, LightningRules()), 1000.0f);
	// at least one event's cost
	auto rules = LightningRules();
	rules.costToMaintain = 0.01f;
	rules.costPerEvent = 3.0f;
	EXPECT_FLOAT_EQ(GetChantSafetyLevel(spell, rules), 3.0f);
	// slower turns mean fewer turns a second
	rules = LightningRules();
	rules.turnDuration = std::chrono::milliseconds(300);
	SetChants(spell, 5000.0f);
	EXPECT_FLOAT_EQ(GetChantSafetyLevel(spell, rules), 50.0f * 3.0f * k_ChantSafetySeconds);
}

TEST(SpellChants, playerCastLivesOnItsStore)
{
	// Nothing refills a hand cast: full strength above 2500, weaker below, ended by its 6 second timer
	SpellChants spell;
	SetChants(spell, 5000.0f);
	FakeCaster caster(false);
	const auto rules = LightningRules();
	float age = 0.0f;
	int turns = 0;
	for (; turns < 100; ++turns)
	{
		const float before = spell.chants;
		const auto turn = RunTurn(spell, age, 6.0f, rules, caster);
		if (turn.closed)
		{
			break;
		}
		EXPECT_NEAR(turn.strength, before >= 2500.0f ? 1.0f : before / 2500.0f, 1e-5f) << "turn " << turns;
	}
	EXPECT_GE(turns, 59);
	EXPECT_LE(turns, 60);
	EXPECT_FLOAT_EQ(caster.given, 0.0f);
	EXPECT_NEAR(spell.chants, 5000.0f - static_cast<float>(turns) * 50.0f, 1e-2f);
	ASSERT_EQ(caster.spent.size(), static_cast<size_t>(turns));
	EXPECT_EQ(caster.spent.front().charge, ChantCharge::PerTurn);
}

TEST(SpellChants, refillingCasterHoldsTheSafetyLevel)
{
	SpellChants spell;
	SetChants(spell, 5000.0f);
	FakeCaster caster(true);
	const auto rules = LightningRules();
	float age = 0.0f;
	for (int turn = 0; turn < 200; ++turn)
	{
		const auto result = RunTurn(spell, age, k_NoTimeLimit, rules, caster);
		ASSERT_FALSE(result.closed);
		EXPECT_FLOAT_EQ(result.strength, 1.0f);
	}
	EXPECT_NEAR(spell.chants, 2500.0f, 1e-2f);
	EXPECT_GT(caster.given, 0.0f);
}

TEST(SpellChants, eventCost)
{
	SpellChants spell;
	SetChants(spell, 3000.0f);
	FakeCaster caster(false);
	EXPECT_FLOAT_EQ(PayForOneEvent(spell, LightningRules(), &caster), 1.0f);
	EXPECT_FLOAT_EQ(spell.chants, 2998.0f);
	ASSERT_EQ(caster.spent.size(), 1u);
	EXPECT_EQ(caster.spent[0].charge, ChantCharge::PerEvent);
	EXPECT_FLOAT_EQ(caster.spent[0].chants, 2.0f);
	// a free miracle pays nothing and has full strength
	spell.free = true;
	EXPECT_FLOAT_EQ(PayForOneEvent(spell, LightningRules(), &caster), 1.0f);
	EXPECT_FLOAT_EQ(spell.chants, 2998.0f);
}

TEST(SpellChants, refillUpToCostOrWhole)
{
	SpellChants spell;
	SetChants(spell, 5000.0f);
	spell.chants = 1000.0f; // 1500 under the safety level
	FakeCaster caster(true);
	PayFor(spell, LightningRules(), &caster, 100.0f);
	EXPECT_FLOAT_EQ(spell.chants, 1000.0f); // paid 100, refilled 100
	PayFor(spell, LightningRules(), &caster, 100.0f, Refill::Whole);
	EXPECT_FLOAT_EQ(spell.chants, 2500.0f); // refilled to the safety level
}

TEST(SpellChants, maintainedShield)
{
	// A maintained miracle's safety level is its whole store; a shield of twice its normal radius costs four times as
	// much a turn
	SpellChants spell;
	SetChants(spell, 5000.0f);
	FakeCaster caster(false);
	auto rules = ShieldRules();
	rules.costToMaintain = 20.0f * 2.0f * 2.0f;
	EXPECT_FLOAT_EQ(GetChantSafetyLevel(spell, rules), 5000.0f);
	EXPECT_FLOAT_EQ(PayForOneTurn(spell, rules, &caster), (5000.0f - 80.0f) / 5000.0f);
	EXPECT_FLOAT_EQ(spell.chants, 4920.0f);
	// no time limit: only running out of prayer power ends it
	float age = 0.0f;
	bool closed = false;
	for (int turn = 0; turn < 1000 && !closed; ++turn)
	{
		closed = RunTurn(spell, age, k_NoTimeLimit, rules, caster).closed;
	}
	EXPECT_TRUE(closed);
	EXPECT_LE(spell.chants, 0.0f);
}

TEST(SpellChants, tribalPowerDividesCost)
{
	SpellChants spell;
	SetChants(spell, 5000.0f);
	FakeCaster caster(false);
	auto rules = ShieldRules();
	rules.tribalPower = 2.0f;
	PayForOneTurn(spell, rules, &caster);
	EXPECT_FLOAT_EQ(spell.chants, 4990.0f);
	// a tribal power under 1 does not make it dearer
	rules.tribalPower = 0.5f;
	PayForOneTurn(spell, rules, &caster);
	EXPECT_FLOAT_EQ(spell.chants, 4970.0f);
}

TEST(SpellChants, strengthEdges)
{
	SpellChants spell;
	FakeCaster caster(false);
	SpellChantRules rules {};
	// no safety level: full strength with any prayer power, none without
	SetChants(spell, 10.0f);
	EXPECT_FLOAT_EQ(GetChantSafetyLevel(spell, rules), 0.0f);
	EXPECT_FLOAT_EQ(GetSpellStrength(spell, rules, &caster), 1.0f);
	spell.chants = 0.0f;
	EXPECT_FLOAT_EQ(GetSpellStrength(spell, rules, &caster), 0.0f);
	// no upkeep: a turn is free and full strength
	EXPECT_FLOAT_EQ(PayForOneTurn(spell, rules, &caster), 1.0f);
	EXPECT_TRUE(caster.spent.empty());
	// no caster: no strength and no payments
	spell.chants = 10.0f;
	EXPECT_FLOAT_EQ(GetSpellStrength(spell, rules, nullptr), 0.0f);
	EXPECT_FLOAT_EQ(PayFor(spell, rules, nullptr, 1.0f), 0.0f);
	EXPECT_FLOAT_EQ(PayForOneEvent(spell, rules, nullptr), 0.0f);
	EXPECT_FLOAT_EQ(spell.chants, 10.0f);
	// the multipliers
	rules.tribalPower = 1.5f;
	rules.seedPower = 0.5f;
	spell.strengthMultiplier = 2.0f;
	EXPECT_FLOAT_EQ(GetSpellStrength(spell, rules, &caster), 1.5f);
}

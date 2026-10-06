/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SpellChants.h"

#include <cstdint>

#include "InfoConstants.h"
#include "MagicTables.h"

using namespace openblack;
using namespace openblack::magic;

SpellChantRules magic::ChantRulesFor(MagicType type, const GMagicInfo& magic, const GMagicEffectInfo& effect)
{
	return {
	    .costPerEvent = effect.costPerEvent,
	    .costToMaintain = effect.costPerGameTurn,
	    .maintained = IsMaintainedSpell(type),
	    .recharged = magic.isSpellRecharged != 0,
	    .divideCostsByTribalPower = effect.divideCostsByTribalPower == 1,
	};
}

void magic::SetChants(SpellChants& spell, float chants)
{
	spell.chants = chants;
	spell.initialChants = chants;
}

float magic::GetChantSafetyLevel(const SpellChants& spell, const SpellChantRules& rules)
{
	if (rules.maintained)
	{
		return spell.initialChants;
	}
	// Whole turns per second, counted in integers as the original does
	const auto turnsPerSecond = static_cast<float>(static_cast<uint64_t>(std::chrono::milliseconds(1000) / rules.turnDuration));
	float level = turnsPerSecond * k_ChantSafetySeconds * rules.costToMaintain;
	if (!(level < spell.initialChants))
	{
		level = spell.initialChants;
	}
	if (!(level > rules.costPerEvent))
	{
		level = rules.costPerEvent;
	}
	return level;
}

float magic::GetSpellStrength(const SpellChants& spell, const SpellChantRules& rules, const SpellCasterInterface* caster)
{
	if (caster == nullptr)
	{
		return 0.0f;
	}
	float ratio = 0.0f;
	const float safety = GetChantSafetyLevel(spell, rules);
	if (safety > 0.0f)
	{
		ratio = spell.chants / safety;
		if (1.0f < ratio)
		{
			ratio = 1.0f;
		}
		else if (ratio <= 0.0f)
		{
			ratio = 0.0f;
		}
	}
	else
	{
		// With no safety level any prayer power at all is full strength
		ratio = spell.chants > 0.0f ? 1.0f : 0.0f;
	}
	return ratio * rules.tribalPower * rules.seedPower * spell.strengthMultiplier;
}

float magic::PayFor(SpellChants& spell, const SpellChantRules& rules, SpellCasterInterface* caster, float cost, Refill refill)
{
	if (caster == nullptr)
	{
		return 0.0f;
	}
	if (spell.free)
	{
		return 1.0f;
	}
	if (rules.divideCostsByTribalPower)
	{
		// A tribal power under 1 never makes it dearer
		cost /= rules.tribalPower > 1.0f ? rules.tribalPower : 1.0f;
	}
	spell.chants -= cost;
	const float shortfall = GetChantSafetyLevel(spell, rules) - spell.chants;
	if (shortfall > 0.0f && rules.recharged)
	{
		const float asked = refill == Refill::Whole ? shortfall : (cost < shortfall ? cost : shortfall);
		spell.chants += caster->MaintainSpell(asked);
	}
	return GetSpellStrength(spell, rules, caster);
}

float magic::PayForOneTurn(SpellChants& spell, const SpellChantRules& rules, SpellCasterInterface* caster)
{
	const float cost = rules.costToMaintain;
	if (cost == 0.0f)
	{
		return 1.0f;
	}
	if (caster != nullptr)
	{
		caster->OnChantsSpent(cost, ChantCharge::PerTurn);
	}
	return PayFor(spell, rules, caster, cost);
}

float magic::PayForOneEvent(SpellChants& spell, const SpellChantRules& rules, SpellCasterInterface* caster)
{
	if (caster == nullptr)
	{
		return 0.0f;
	}
	const float strength = PayFor(spell, rules, caster, rules.costPerEvent);
	caster->OnChantsSpent(rules.costPerEvent, ChantCharge::PerEvent);
	return strength;
}

float magic::Recharge(SpellChants& spell, const SpellChantRules& rules, SpellCasterInterface& caster)
{
	const float shortfall = GetChantSafetyLevel(spell, rules) - spell.chants;
	if (!(shortfall > 0.0f) || !rules.recharged)
	{
		return 0.0f;
	}
	const float given = caster.MaintainSpell(shortfall);
	spell.chants += given;
	return given;
}

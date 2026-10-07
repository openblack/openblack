/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureSpellMind.h"

#include <algorithm>

#include "CreatureLearning.h"

using namespace openblack;
using openblack::creature_desires::Desire;

bool creature_spell_mind::HeldDownByCheat(Desire other, bool all)
{
	switch (other)
	{
	case Desire::Hunger:
	case Desire::Poo:
	case Desire::Tiredness:
	case Desire::Water:
		return all;
	case Desire::IdleWithPlayer:
	case Desire::RestoreHealth:
	case Desire::BeFriends:
	case Desire::ManifestState:
	case Desire::Rest:
	case Desire::PlayWithPlayer:
	case Desire::HangAroundAtHome:
	case Desire::LookAround:
		return false;
	default:
		return true;
	}
}

creature_spell_mind::Cheat creature_spell_mind::SetCheatDominant(creature_desires::Desires& desires, Desire desire, bool all,
                                                                 float turnsPerSecond)
{
	desires[desire].activated = true;
	const auto turns = static_cast<uint32_t>(turnsPerSecond * k_CheatSeconds);
	for (size_t i = 0; i < creature_desires::k_DesireCount; ++i)
	{
		const auto other = static_cast<Desire>(i);
		if (other != desire && HeldDownByCheat(other, all))
		{
			auto& state = desires.desires.at(i);
			state.suppressedTurns = std::max(state.suppressedTurns, turns);
		}
	}
	// Compassion makes it want to make friends above all too
	if (desire == Desire::Compassion)
	{
		desires[Desire::BeFriends].suppressedTurns = 0;
		creature_learning::MakeFullyDominant(desires, Desire::BeFriends);
	}
	desires[desire].suppressedTurns = 0;
	creature_learning::MakeFullyDominant(desires, desire);
	return {.desire = desire, .turns = 0};
}

bool creature_spell_mind::StepCheat(creature_desires::Desires& desires, Cheat& cheat, float turnsPerSecond)
{
	desires[cheat.desire].suppressedTurns = 0;
	++cheat.turns;
	// The game counts whole seconds by its whole turns a second
	const auto perSecond = std::max(static_cast<uint32_t>(turnsPerSecond), 1u);
	if (static_cast<float>(cheat.turns / perSecond) > k_CheatSeconds)
	{
		ClearCheatDominance(desires);
		return false;
	}
	return true;
}

void creature_spell_mind::ClearCheatDominance(creature_desires::Desires& desires)
{
	for (auto& state : desires.desires)
	{
		state.suppressedTurns = 0;
	}
}

void creature_spell_mind::MakeLeastDominant(creature_desires::Desires& desires, Desire desire, float factor)
{
	creature_learning::MakeLeastDominant(desires, desire, factor);
	auto& state = desires[desire];
	state.value = std::min(state.value, state.max);
}

void creature_spell_mind::MakeFullyDominantOverOthers(creature_desires::Desires& desires, Desire desire, float floor)
{
	auto& state = desires[desire];
	state.suppressedTurns = 0;
	if (!state.activated)
	{
		return;
	}
	state.value = state.max;
	for (size_t i = 0; i < creature_desires::k_DesireCount; ++i)
	{
		if (i != static_cast<size_t>(desire))
		{
			desires.desires.at(i).value = floor;
		}
	}
}

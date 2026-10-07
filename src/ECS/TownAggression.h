/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>

#include "Enums.h"

// How a town remembers being attacked. Each harm done to it adds to the aggression it holds against whoever did it,
// the first harm by a player more; the more often it is attacked the less each attack adds. Each turn the aggression
// fades a little: what its own player did is its want of mercy, what anyone else did its want of protection, which
// its desires take up. The last attacker and the turn it attacked are kept, which the villagers sheltering under a
// shield look at. Pure rules on the town's record, tested without the game.

namespace openblack::ecs::town_aggression
{

/// What a town keeps of the attacks on it
struct Record
{
	/// The aggression held against each player, and the turn each last attacked
	std::array<float, static_cast<size_t>(PlayerNames::_COUNT)> aggression {};
	std::array<uint32_t, static_cast<size_t>(PlayerNames::_COUNT)> lastTurns {};
	/// The last attacker, and the turn it attacked; none before any attack
	PlayerNames lastAggressor {PlayerNames::NEUTRAL};
	uint32_t lastTurn {0};
	/// How much of an attack counts, by someone else and by the town's own player, worn down by each
	float protectionMultiplier {1.0f};
	float mercyMultiplier {1.0f};
	/// This turn's wants, the sums of the aggression
	float protection {0.0f};
	float mercy {0.0f};
};

/// The town is attacked by a player with an amount of harm: the first harm a player does adds a bonus; it counts by
/// the multiplier for the town's own player or for anyone else, which is then worn down by a tenth
void Attacked(Record& record, PlayerNames aggressor, bool aggressorIsOwner, float amount, float firstTimeAddition,
              uint32_t turn);

/// A turn: the aggression fades, and is summed into the wants of mercy (the owner's) and protection (anyone else's, or
/// everyone's in a neutral town); a multiplier recovers while its want is small
void ProcessTurn(Record& record, PlayerNames owner);

/// The harm an attack does a town's thing, as aggression: the harm times the thing's kind's aggressor value
[[nodiscard]] constexpr float AggressionFromDamage(float damage, float aggressorValue)
{
	return damage * aggressorValue;
}

} // namespace openblack::ecs::town_aggression

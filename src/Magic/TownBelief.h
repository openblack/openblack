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
#include <vector>

#include "Enums.h"

// A town's belief in each player. What its people are impressed by waits as pending belief until the town's next turn,
// when all of it, times the town's belief scale, is believed at once, no more than the town's cap for the player; a
// symbol of the belief gained then rises from the town centre in the player's colour. The town also keeps a tally of
// the belief each player has been given lately, which fades a little every turn. Pure, tested on its own.

namespace openblack::magic::town_belief
{

inline constexpr size_t k_PlayerCount = static_cast<size_t>(PlayerNames::_COUNT);
/// Belief in a player is capped at this until a script sets another cap
inline constexpr float k_DefaultCap = 10.0f;

struct Belief
{
	/// What the town believes in each player
	std::array<float, k_PlayerCount> belief {};
	/// What it has been given and will believe at its next turn
	std::array<float, k_PlayerCount> pending {};
	/// What it has been given lately, fading every turn
	std::array<float, k_PlayerCount> recent {};
	/// The most it believes in each player
	std::array<float, k_PlayerCount> cap = [] {
		std::array<float, k_PlayerCount> caps {};
		caps.fill(k_DefaultCap);
		return caps;
	}();
	/// What pending belief is multiplied by as it is believed
	float scale {1.0f};
};

/// The town is given belief in a player
void Add(Belief& town, PlayerNames player, float amount);

/// Belief gained in a player at a turn
struct Gained
{
	PlayerNames player;
	float amount;
};

/// The town's turn: its recent tallies fade by the decay, and its pending belief is believed. The belief gained in each
/// player that gained any, in player order.
[[nodiscard]] std::vector<Gained> Turn(Belief& town, float recentDecay);

} // namespace openblack::magic::town_belief

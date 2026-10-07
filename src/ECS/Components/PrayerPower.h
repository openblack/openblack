/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

namespace openblack::ecs::components
{

/// The prayer power a player has to cast and keep miracles with, on the player's entity. Until the worship sites are
/// simulated it stands for what the player's worship has stored: summoning a seed charges its miracle's cost from it,
/// the miracles cast from the hand draw their upkeep from it, and a seed dropped unused gives its charge back.
struct PrayerPower
{
	float chants {0.0f};
	/// Never runs out, as the testbed's player and the debug window's cheat give
	bool infinite {false};
};

} // namespace openblack::ecs::components

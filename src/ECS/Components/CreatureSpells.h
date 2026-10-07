/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include "Creature/CreatureSpellMind.h"
#include "Creature/CreatureSpells.h"

namespace openblack::ecs::components
{

/// The miracles cast on a creature and what they are doing to it
struct CreatureSpells
{
	creature_spells::Spells spells;
	/// How frozen it is, 0 to 1: its body slows to a stop and takes an icy look
	float freeze {0.0f};
	/// How far it has fizzed out of sight, 0 to the most invisible makes it
	float fizz {0.0f};
	/// Others don't see it while it is invisible
	bool invisible {false};
	/// The freeze paused its mind, which it starts again as it thaws
	bool pausedMind {false};
	/// The smallest and largest the size spells take it to, which scripts may change
	float smallestSize {creature_spells::k_SmallestSize};
	float largestSize {creature_spells::k_LargestSize};
	/// A desire a spell made dominant over the others, while it lasts
	std::optional<creature_spell_mind::Cheat> cheat;
};

} // namespace openblack::ecs::components

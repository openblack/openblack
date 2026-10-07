/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <unordered_map>

#include "Enums.h"
#include "Magic/TownBelief.h"

namespace openblack::ecs::components
{

/// What a town has made of the impressive things it has seen, on the town's entity: the belief its people have in each
/// player, believed at the town's turn, and how weary it has grown of each kind of reaction (1 when it has never seen
/// one, less the more it has)
struct TownImpression
{
	magic::town_belief::Belief belief;
	std::unordered_map<Reaction, float> boredom;
	/// How much belief the last impression gave, for the debug window
	float lastImpression {0.0f};
};

/// How impressed a creature is with miracles, on the creature's entity: by its own player's, and by other creatures'
struct CreatureImpression
{
	float byOwnPlayer {0.0f};
	float byOtherCreatures {0.0f};
};

} // namespace openblack::ecs::components

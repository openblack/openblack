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

namespace openblack::ecs::components
{

/// What a land's script sets for the whole land, back to these each time a land opens
struct MapScriptGlobals
{
	/// Which land of the story this is, 0 outside the story
	int32_t landNumber {0};
	/// How far the towns' and the players' citadels' influence reaches, as a part of what it would be
	float townInfluenceMultiplier {1.0f};
	float playerInfluenceMultiplier {1.0f};
	/// The land's balances, all 1 unless its script sets them: how fast villagers go (4), how impressive miracles are (2),
	/// how belief speeds villagers (7) and others
	std::array<float, 8> landBalance {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
	/// How much of a town's boredom with each kind of reaction wears off at each of its turns, 1 unless the land's script
	/// sets it
	float lostTownScale {1.0f};
};

} // namespace openblack::ecs::components

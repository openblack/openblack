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
};

} // namespace openblack::ecs::components

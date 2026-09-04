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

#include "Creature/CreatureMind.h"

namespace openblack::ecs::components
{

// Present only when the creature's mind was loaded at spawn.
struct CreatureState
{
	std::array<float, creature::k_DesireCount> desireIncreaseTimes {};
};

} // namespace openblack::ecs::components

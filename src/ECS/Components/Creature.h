/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <string>

#include <entt/fwd.hpp>

#include "Creature.h"
#include "Enums.h"

namespace openblack::ecs::components
{

struct Creature
{
	PlayerNames owner;
	CreatureType species;
	entt::id_type mind;
	/// What the creature has become, which its body shows: alignment from -1 (evil) to 1 (good), fatness and strength
	/// from 0 to 1
	float alignment {0.0f};
	float fatness {0.5f};
	float strength {0.5f};
};
} // namespace openblack::ecs::components

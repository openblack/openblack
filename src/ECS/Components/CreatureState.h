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

#include <entt/fwd.hpp>

#include "Creature/CreatureMind.h"

namespace openblack::ecs::components
{

struct CreatureState
{
	std::array<float, creature::k_DesireCount> desireWeights {};
	entt::id_type mind {};
	bool initialized = false;
};

} // namespace openblack::ecs::components

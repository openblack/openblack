/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/fwd.hpp>

#include "Enums.h"

namespace openblack::ecs::archetypes
{

class PlayerArchetype
{
public:
	static entt::entity Create(PlayerNames name);
	PlayerArchetype() = delete;
};
} // namespace openblack::ecs::archetypes

/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>

#include "GameInterface.h"

namespace openblack::ecs
{
struct MainGameContext
{
	glm::ivec2 mousePosition;
	entt::entity player;
};
} // namespace openblack::ecs

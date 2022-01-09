/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <entt/fwd.hpp>
#include <glm/fwd.hpp>

#include "Enums.h"

namespace openblack::ecs::archetypes
{
class TreeArchetype
{
public:
	static entt::entity Create(uint32_t forestId, const glm::vec3& position, TreeInfo type, bool isNonScenic,
	                           float yAngleRadians, float maxSize, float scale);
	TreeArchetype() = delete;
};
} // namespace openblack::ecs::archetypes

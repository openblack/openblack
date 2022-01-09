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
class FieldArchetype
{
public:
	static entt::entity Create(int townId, const glm::vec3& position, FieldTypeInfo type, float yAngleRadians);
	FieldArchetype() = delete;
};
} // namespace openblack::ecs::archetypes

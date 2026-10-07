/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/entity/entity.hpp>
#include <glm/fwd.hpp>

#include "Enums.h"

namespace openblack::ecs::archetypes
{
/// An animal of a kind of the tables: its model, standing or flying where it is put, facing a way across the land
class AnimalArchetype
{
public:
	static entt::entity Create(AnimalInfo type, const glm::vec3& position, float heading, float scale, PlayerNames owner);
	AnimalArchetype() = delete;
};
} // namespace openblack::ecs::archetypes

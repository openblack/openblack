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
#include <glm/fwd.hpp>

#include "ECS/Components/VillageLight.h"

namespace openblack::ecs::archetypes
{
class VillageLightArchetype
{
public:
	/// A village light standing at `position`, with its flames and glow above it
	static entt::entity Create(const glm::vec3& position, components::VillageLight::Kind kind);
	VillageLightArchetype() = delete;
};
} // namespace openblack::ecs::archetypes

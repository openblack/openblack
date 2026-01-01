/*******************************************************************************
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

namespace openblack::ecs::systems
{
class TownSystemInterface
{
public:
	[[nodiscard]] virtual entt::entity FindAbodeWithSpace(entt::entity townEntity) const = 0;
	[[nodiscard]] virtual entt::entity FindClosestTown(const glm::vec3& point) const = 0;
	virtual void AddHomelessVillagerToTown(entt::entity townEntity, entt::entity villagerEntity) = 0;
};
} // namespace openblack::ecs::systems

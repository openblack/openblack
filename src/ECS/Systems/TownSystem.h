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

namespace openblack::ecs::systems
{

class TownSystem
{
public:
	static TownSystem& instance();

	entt::entity FindAbodeWithSpace(entt::entity townEntity) const;
	entt::entity FindClosestTown(const glm::vec3& point) const;
	void AddHomelessVillagerToTown(entt::entity townEntity, entt::entity villagerEntity);

private:
	TownSystem();
};
} // namespace openblack::ecs::systems

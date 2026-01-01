/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/TownSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::ecs::systems
{

class TownSystem final: public TownSystemInterface
{
public:
	[[nodiscard]] entt::entity FindAbodeWithSpace(entt::entity townEntity) const override;
	[[nodiscard]] entt::entity FindClosestTown(const glm::vec3& point) const override;
	void AddHomelessVillagerToTown(entt::entity townEntity, entt::entity villagerEntity) override;
};
} // namespace openblack::ecs::systems

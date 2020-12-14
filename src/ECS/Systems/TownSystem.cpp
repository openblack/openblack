/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "TownSystem.h"

#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

TownSystem& TownSystem::instance()
{
	static auto instance = new TownSystem();
	return *instance;
}

TownSystem::TownSystem() = default;

entt::entity TownSystem::FindClosestTown(const glm::vec3& point) const
{
	const auto& registry = Game::instance()->GetEntityRegistry();

	entt::entity result = entt::null;
	auto closest = std::numeric_limits<float>::infinity();

	registry.Each<const Town, const Transform>([&point, &result, &closest](entt::entity entity, auto& town, auto& transform) {
		float distance_2 = glm::dot(point, transform.position);
		if (distance_2 < closest)
		{
			closest = distance_2;
			result = entity;
		}
	});

	return result;
}

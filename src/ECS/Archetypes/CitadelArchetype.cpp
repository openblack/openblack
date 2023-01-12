/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CitadelArchetype.h"

#include <entt/fwd.hpp>

#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity CitadelArchetype::Create(const glm::vec3& position, PlayerNames playerOwner, const glm::mat4& rotation,
                                      const glm::vec3& size)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	registry.Assign<Transform>(entity, position, rotation, size);
	const auto meshId = entt::hashed_string("temple/b_first_temple_l3d");
	registry.Assign<Mesh>(entity, meshId, static_cast<int8_t>(0), static_cast<int8_t>(0));
	return entity;
}

entt::entity CitadelArchetype::CreatePlan(int32_t /*townId*/, const glm::vec3& position, PlayerNames playerOwner,
                                          const glm::mat4& rotation, const glm::vec3& size)
{
	return Create(position, playerOwner, rotation, size);
}

/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "BigForestArchetype.h"

#include <glm/gtx/euler_angles.hpp>

#include "ECS/Components/Fixed.h"
#include "ECS/Components/Forest.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Resources/MeshId.h"
#include "Utils.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity BigForestArchetype::Create(const glm::vec3& position, BigForestInfo type, [[maybe_unused]] uint32_t unknown,
                                        float yAngleRadians, float scale)
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	const auto& info = Game::Instance()->GetInfoConstants().bigForest.at(static_cast<size_t>(type));

	const auto& transform = registry.Assign<Transform>(entity, position, glm::eulerAngleY(-yAngleRadians), glm::vec3(scale));
	const auto [point, radius] = GetFixedObstacleBoundingCircle(info.meshId, transform);
	registry.Assign<Fixed>(entity, point, radius);
	registry.Assign<Forest>(entity);
	registry.Assign<BigForest>(entity);
	const auto resourceId = resources::MeshIdToResourceId(info.meshId);
	registry.Assign<Mesh>(entity, resourceId, static_cast<int8_t>(0), static_cast<int8_t>(1));

	return entity;
}

/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TreeArchetype.h"

#include <glm/gtx/euler_angles.hpp>

#include "ECS/Components/Fixed.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Locator.h"
#include "Resources/MeshId.h"
#include "Utils.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity TreeArchetype::Create([[maybe_unused]] uint32_t forestId, const glm::vec3& position, TreeInfo type,
                                   [[maybe_unused]] bool isNonScenic, float yAngleRadians, float maxSize, float scale)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();

	const auto& info = Game::Instance()->GetInfoConstants().tree.at(static_cast<size_t>(type));

	const auto& transform = registry.Assign<Transform>(entity, position, glm::eulerAngleY(-yAngleRadians), glm::vec3(scale));
	const auto [point, radius] = GetFixedObstacleBoundingCircle(info.normal, transform);
	registry.Assign<Fixed>(entity, point, radius);
	registry.Assign<Tree>(entity, type, maxSize);
	const auto resourceId = resources::MeshIdToResourceId(info.normal);
	registry.Assign<Mesh>(entity, resourceId, static_cast<int8_t>(0), static_cast<int8_t>(-1));

	return entity;
}

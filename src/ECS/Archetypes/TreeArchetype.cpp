/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "TreeArchetype.h"

#include <glm/gtx/euler_angles.hpp>

#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity TreeArchetype::Create([[maybe_unused]] uint32_t forestId, const glm::vec3& position, TreeInfo type,
                                   [[maybe_unused]] bool isNonScenic, float yAngleRadians, float maxSize, float scale)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	const auto& info = Game::instance()->GetInfoConstants().tree[static_cast<size_t>(type)];

	registry.Assign<Transform>(entity, position, glm::eulerAngleY(-yAngleRadians), glm::vec3(scale));
	registry.Assign<Tree>(entity, type, maxSize);
	registry.Assign<Mesh>(entity, info.normal, static_cast<int8_t>(0), static_cast<int8_t>(-1));

	return entity;
}

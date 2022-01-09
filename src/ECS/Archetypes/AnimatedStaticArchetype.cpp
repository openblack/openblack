/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "AnimatedStaticArchetype.h"

#include <ECS/Components/Feature.h>
#include <glm/gtx/euler_angles.hpp>

#include "ECS/Components/AnimatedStatic.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity AnimatedStaticArchetype::Create(const glm::vec3& position, AnimatedStaticInfo type, float yAngleRadians,
                                             float scale)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	const auto& info = Game::instance()->GetInfoConstants().animatedStatic[static_cast<size_t>(type)];

	// The exact same as Feature but info is different and type is a different enum
	registry.Assign<Transform>(entity, position, glm::eulerAngleY(-yAngleRadians), glm::vec3(scale));
	// const auto& feature = registry.Assign<Feature>(entity, type);
	registry.Assign<Mesh>(entity, info.meshId, static_cast<int8_t>(0), static_cast<int8_t>(1));

	registry.Assign<AnimatedStatic>(entity, type);

	return entity;
}

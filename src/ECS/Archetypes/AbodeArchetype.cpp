/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "AbodeArchetype.h"

#include <glm/gtx/euler_angles.hpp>

#include "ECS/Components/Abode.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity AbodeArchetype::Create(uint32_t townId, const glm::vec3& position, AbodeInfo type, float yAngleRadians,
                                    float scale, uint32_t foodAmount, uint32_t woodAmount)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	const auto& info = Game::instance()->GetInfoConstants().abode[static_cast<size_t>(type)];

	registry.Assign<Transform>(entity, position, glm::mat3(glm::eulerAngleY(-yAngleRadians)), glm::vec3(scale));
	registry.Assign<Abode>(entity, info.abodeNumber, townId, foodAmount, woodAmount);
	registry.Assign<Mesh>(entity, info.meshId, static_cast<int8_t>(0), static_cast<int8_t>(0));

	return entity;
}

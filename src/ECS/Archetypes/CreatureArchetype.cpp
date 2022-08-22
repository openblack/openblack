/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "CreatureArchetype.h"

#include <glm/gtx/euler_angles.hpp>

#include "ECS/Components/Creature.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Enums.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity CreatureArchetype::Create(const std::string& playerName, CreatureType creatureType,
                                       const std::string& creatureMind, const glm::vec3& position)
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	const auto entity = registry.Create();
	auto& creature = registry.Assign<Creature>(entity, playerName, creatureType, creatureMind);
	auto meshId = creature::GetIdFromType(creatureType);
	registry.Assign<ecs::components::Mesh>(entity, meshId);
	registry.Assign<Transform>(entity, position, glm::eulerAngleY(glm::radians(180.0f)), glm::vec3(.3f));
	return entity;
}

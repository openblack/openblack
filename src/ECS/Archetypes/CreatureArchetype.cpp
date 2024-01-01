/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureArchetype.h"

#include <glm/gtx/euler_angles.hpp>

#include "3D/CreatureBody.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Enums.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;
using namespace openblack::creature;

entt::entity CreatureArchetype::Create(const glm::vec3& position, PlayerNames playerName, CreatureType creatureType,
                                       entt::id_type creatureMindId, float yAngleRadians, float scale)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	auto meshId = creature::GetIdFromType(creatureType, CreatureBody::Appearance::Base);
	registry.Assign<Creature>(entity, playerName, creatureType, creatureMindId);
	registry.Assign<Mesh>(entity, meshId);
	registry.Assign<Transform>(entity, position, glm::eulerAngleY(yAngleRadians), glm::vec3(scale));
	return entity;
}

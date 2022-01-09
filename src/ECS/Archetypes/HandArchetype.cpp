/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "HandArchetype.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/vec3.hpp>

#include "ECS/Components/Hand.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity HandArchetype::Create(const glm::vec3& position, float xAngleRadians, float yAngleRadians, float zAngleRadians,
                                   float scale, bool rightHanded)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	const auto rotation = glm::mat3(glm::eulerAngleXYZ(xAngleRadians, yAngleRadians, zAngleRadians));
	registry.Assign<Hand>(entity, rightHanded);
	registry.Assign<Transform>(entity, position, rotation, glm::vec3(scale));
	registry.Assign<Mesh>(entity, static_cast<MeshId>(Hand::meshId), static_cast<int8_t>(0), static_cast<int8_t>(0));

	// Data/CreatureMesh/Hand_Boned_Base2.l3d
	// Data/CreatureMesh/Hand_Boned_Good2.l3d
	// Data/CreatureMesh/Hand_Boned_Evil2.l3d

	// load data\\hand.raw into a memory

	return entity;
}

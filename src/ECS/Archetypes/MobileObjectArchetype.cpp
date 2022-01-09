/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "MobileObjectArchetype.h"

#include <glm/gtx/euler_angles.hpp>

#include "AbodeArchetype.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "InfoConstants.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity MobileObjectArchetype::Create(const glm::vec3& position, MobileObjectInfo type, float yAngleRadians, float scale)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	const auto& info = Game::instance()->GetInfoConstants().mobileObject[static_cast<size_t>(type)];

	registry.Assign<Transform>(entity, position, glm::eulerAngleY(-yAngleRadians), glm::vec3(scale));
	registry.Assign<MobileObject>(entity, type);
	registry.Assign<Mesh>(entity, info.meshId, static_cast<int8_t>(0), static_cast<int8_t>(1));

	return entity;
}

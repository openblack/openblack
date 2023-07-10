/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "StreetLanternArchetype.h"

#include <glm/gtx/euler_angles.hpp>

#include "3D/AllMeshes.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Locator.h"
#include "Resources/MeshId.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity StreetLanternArchetype::Create(const glm::vec3& position)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	registry.Assign<Transform>(entity, position, glm::eulerAngleY(glm::radians(180.0f)), glm::vec3(1.0f));
	const auto resourceId = resources::MeshIdToResourceId(MeshId::ObjectTownLight);
	registry.Assign<Mesh>(entity, resourceId, static_cast<int8_t>(0), static_cast<int8_t>(1));
	return entity;
}

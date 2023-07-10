/*******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "PotArchetype.h"

#include <glm/gtx/euler_angles.hpp>

#include "ECS/Components/Mesh.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Locator.h"
#include "Resources/MeshId.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity PotArchetype::Create(const glm::vec3& position, float yAngleRadians, PotInfo type, int32_t amount)
{
	if (static_cast<int32_t>(type) < 0 || static_cast<int32_t>(type) >= static_cast<int32_t>(PotInfo::_COUNT))
	{
		return entt::null;
	}
	if (amount <= 0)
	{
		return entt::null;
	}

	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();

	const auto& info = Game::Instance()->GetInfoConstants().pot.at(static_cast<size_t>(type));

	registry.Assign<Transform>(entity, position, glm::mat3(glm::eulerAngleY(-yAngleRadians)), glm::vec3(1.0f));
	registry.Assign<Pot>(entity, static_cast<uint16_t>(amount), static_cast<uint16_t>(info.maxAmountInPot));
	const auto resourceId = resources::MeshIdToResourceId(info.meshId);
	registry.Assign<Mesh>(entity, resourceId, static_cast<int8_t>(0), static_cast<int8_t>(1));
	if (info.potType == PotType::PileFood)
	{
		registry.Assign<MorphWithTerrain>(entity);
	}

	return entity;
}

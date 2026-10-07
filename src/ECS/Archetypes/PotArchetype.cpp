/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
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
#include "ECS/Components/ResourcePile.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourceManager.h"

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
	const auto entity = CreateEmpty(position, yAngleRadians, type);
	Locator::entitiesRegistry::value().Get<Pot>(entity).amount = static_cast<uint32_t>(amount);
	return entity;
}

entt::entity PotArchetype::CreateEmpty(const glm::vec3& position, float yAngleRadians, PotInfo type)
{
	if (static_cast<int32_t>(type) < 0 || static_cast<int32_t>(type) >= static_cast<int32_t>(PotInfo::_COUNT))
	{
		return entt::null;
	}

	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();

	const auto& info = Locator::infoConstants::value().pot.at(static_cast<size_t>(type));

	registry.Assign<Transform>(entity, position, glm::mat3(glm::eulerAngleY(-yAngleRadians)), glm::vec3(1.0f));
	registry.Assign<Pot>(entity, 0u, info.maxAmountInPot, type);
	const auto resourceId = resources::HashIdentifier(info.meshId);
	registry.Assign<Mesh>(entity, resourceId, static_cast<int8_t>(0), static_cast<int8_t>(1));
	if (info.potType == PotType::PileFood)
	{
		registry.Assign<MorphWithTerrain>(entity);
	}
	// A pile rises out of the ground for what it holds
	if (info.potType == PotType::PileFood || info.potType == PotType::PileWood)
	{
		registry.Assign<ResourcePile>(entity);
	}

	return entity;
}

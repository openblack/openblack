/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SpellSeedArchetype.h"

#include <glm/gtx/euler_angles.hpp>

#include "ECS/Components/Fixed.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/SpellSeed.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Translucent.h"
#include "ECS/Registry.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/DispenserRules.h"
#include "Magic/MagicTables.h"
#include "Resources/ResourceManager.h"
#include "Utils.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

namespace
{
bool IsSeed(SpellSeedType seedType)
{
	const auto index = static_cast<int>(seedType);
	return index >= 0 && index < static_cast<int>(magic::k_SpellSeedCount);
}
} // namespace

entt::entity SpellSeedArchetype::Create(const glm::vec3& position, SpellSeedType seedType, PlayerNames player, int powerUp,
                                        float multiplier)
{
	if (!IsSeed(seedType))
	{
		return entt::null;
	}
	auto& registry = Locator::entitiesRegistry::value();
	const auto& info = magic::GetSpellSeedInfo(Locator::infoConstants::value(), seedType);
	const auto entity = registry.Create();
	registry.Assign<Transform>(entity, position, glm::mat3(1.0f), glm::vec3(info.scale));
	registry.Assign<SpellSeed>(
	    entity, SpellSeed {.seedType = seedType, .powerUp = powerUp, .player = player, .castMultiplier = multiplier});
	return entity;
}

entt::entity OneOffSpellSeedArchetype::Create(const glm::vec3& position, SpellSeedType seedType, int powerUp, float multiplier,
                                              entt::id_type bubbleMesh)
{
	if (!IsSeed(seedType))
	{
		return entt::null;
	}
	auto& registry = Locator::entitiesRegistry::value();
	const auto& info = magic::GetSpellSeedInfo(Locator::infoConstants::value(), seedType);
	// The seed spinning inside, at a share of the bubble's size
	const auto seedGraphic = registry.Create();
	registry.Assign<Transform>(seedGraphic, position, glm::mat3(1.0f), glm::vec3(info.scale * magic::k_OrbSeedScale));
	registry.Assign<Mesh>(seedGraphic, resources::HashIdentifier(info.mesh), static_cast<int8_t>(0), static_cast<int8_t>(0));

	const auto entity = registry.Create();
	registry.Assign<Transform>(entity, position, glm::mat3(1.0f), glm::vec3(1.0f));
	registry.Assign<Mesh>(entity, bubbleMesh, static_cast<int8_t>(0), static_cast<int8_t>(0));
	registry.Assign<Translucent>(entity, Translucent {.share = magic::k_OrbShare});
	registry.Assign<OneOffSpellSeed>(entity, OneOffSpellSeed {.seedType = seedType,
	                                                          .position = position,
	                                                          .powerUp = powerUp,
	                                                          .multiplier = multiplier,
	                                                          .seedGraphic = seedGraphic});
	return entity;
}

entt::entity SpellDispenserArchetype::Create(const glm::vec3& position, MagicType magicType, AbodeInfo building,
                                             float yAngleRadians, float scale)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto& info = Locator::infoConstants::value().abode.at(static_cast<size_t>(building));
	const auto entity = registry.Create();
	const auto& transform =
	    registry.Assign<Transform>(entity, position, glm::mat3(glm::eulerAngleY(-yAngleRadians)), glm::vec3(scale));
	registry.Assign<Mesh>(entity, resources::HashIdentifier(info.meshId), static_cast<int8_t>(0), static_cast<int8_t>(0));
	const auto [point, radius] = GetFixedObstacleBoundingCircle(info.meshId, transform);
	registry.Assign<Fixed>(entity, point, radius);
	const auto period = static_cast<uint32_t>(info.timeEachMobileObjectTakesToProduce);
	registry.Assign<SpellDispenser>(
	    entity, SpellDispenser {.magicType = magicType, .timer = {.tick = 0, .period = period, .active = period != 0}});
	return entity;
}

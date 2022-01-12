/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "VillagerArchetype.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/vec3.hpp>

#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/TownSystem.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

entt::entity VillagerArchetype::Create([[maybe_unused]] const glm::vec3& abodePosition, const glm::vec3& position,
                                       VillagerInfo type, uint32_t age)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	const auto& info = Game::instance()->GetInfoConstants().villager[static_cast<size_t>(type)];

	registry.Assign<Transform>(entity, position, glm::eulerAngleY(glm::radians(180.0f)), glm::vec3(1.0));
	const uint32_t health = 100;
	const uint32_t hunger = 100;

	const auto lifeStage = age < 18 ? Villager::LifeStage::Child : Villager::LifeStage::Adult;
	const auto sex = info.villagerNumber == VillagerNumber::Housewife ? Villager::Sex::FEMALE : Villager::Sex::MALE;
	const auto task = Villager::Task::IDLE;

	// TODO(bwrsandman): Might be better to make a FindClosestAbode
	const entt::entity town = TownSystem::instance().FindClosestTown(abodePosition);
	entt::entity abode = entt::null;
	if (town != entt::null)
	{
		abode = TownSystem::instance().FindAbodeWithSpace(town);
	}

	registry.Assign<Villager>(entity, health, static_cast<uint32_t>(age), hunger, lifeStage, sex, info.tribeType,
	                          info.villagerNumber, task, town, abode);
	registry.Assign<Mesh>(entity, info.highDetail, static_cast<int8_t>(0), static_cast<int8_t>(0));

	return entity;
}

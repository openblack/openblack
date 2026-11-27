/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureArchetype.h"

#include <optional>
#include <stdexcept>

#include <glm/gtx/euler_angles.hpp>

#include "3D/CreatureBody.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureState.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Enums.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;
using namespace openblack::creature;

entt::entity CreatureArchetype::Create(const glm::vec3& position, PlayerNames playerName, CreatureType creatureType,
                                       entt::id_type creatureMindId, float yAngleRadians, float scale)
{
	if (creatureType <= CreatureType::Unknown || creatureType >= CreatureType::_COUNT)
	{
		throw std::runtime_error("Invalid creature species");
	}
	const auto& minds = Locator::resources::value().GetCreatureMinds();
	std::optional<CreatureState> state;
	if (minds.Contains(creatureMindId))
	{
		if (!Locator::infoConstants::has_value())
		{
			throw std::runtime_error("Cannot initialise creature desires without InfoConstants");
		}
		const auto& info = Locator::infoConstants::value();
		const auto record = creatureType == CreatureType::GiantApe ? static_cast<size_t>(v120::CreatureType::Ape)
		                                                           : static_cast<size_t>(creatureType);
		const auto column = static_cast<size_t>(info.creature.at(record).creatureType);
		auto& times = state.emplace().desireIncreaseTimes;
		for (size_t i = 0; i < times.size(); ++i)
		{
			times.at(i) = info.creatureDesireForType.at(i).byCreatureType.at(column);
		}
	}
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	auto meshId = creature::GetIdFromType(creatureType, CreatureBody::Appearance::Base);
	registry.Assign<Creature>(entity, playerName, creatureType, creatureMindId);
	registry.Assign<Mesh>(entity, meshId);
	registry.Assign<Transform>(entity, position, glm::eulerAngleY(yAngleRadians), glm::vec3(scale));
	if (state)
	{
		registry.Assign<CreatureState>(entity, *state);
	}
	return entity;
}

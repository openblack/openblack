/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "TownArchetype.h"

#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity TownArchetype::Create(int id, const glm::vec3& position, [[maybe_unused]] PlayerNames playerOwner, Tribe tribe)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	// const auto& info = Game::instance()->GetInfoConstants().town;

	registry.Assign<Town>(entity, static_cast<uint32_t>(id));
	registry.Assign<Tribe>(entity, tribe);
	registry.Assign<Transform>(entity, position, glm::mat3(1.0f), glm::vec3(1.0f));
	auto& registryContext = registry.Context();
	registryContext.towns.insert({id, entity});

	return entity;
}

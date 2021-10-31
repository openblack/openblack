/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "TownArchetype.h"

#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity TownArchetype::Create(int id, const glm::vec3& position, PlayerNames playerOwner, Tribe tribe)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	const auto& info = Game::instance()->GetInfoConstants().town;

	registry.Assign<Town>(entity, id);
	auto& registryContext = registry.Context();
	registryContext.towns.insert({id, entity});

	return entity;
}

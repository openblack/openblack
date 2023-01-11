/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "PlayerSystem.h"

#include "ECS/Components/Player.h"
#include "ECS/Registry.h"
#include "Locator.h"

using namespace openblack::ecs::systems;

void PlayerSystem::AddPlayer(entt::entity playerEntity)
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto& player = registry.Get<components::Player>(playerEntity);
	_players.emplace(player.name, playerEntity);
}

entt::entity PlayerSystem::GetPlayer(PlayerNames playerName) const
{
	return _players.at(playerName);
}

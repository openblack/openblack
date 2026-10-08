/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// Food and wood the miracles pour goes to the stores and piles about the point it is poured at, as any poured food or
// wood does (ResourceStoreSystem).

#define LOCATOR_IMPLEMENTATIONS

#include "ECS/Systems/ResourceStoreSystemInterface.h"
#include "Locator.h"
#include "MagicSystem.h"

using namespace openblack;
using namespace openblack::ecs::systems;

bool GameMagicWorld::AddResource(ResourceType type, glm::vec3 point, uint32_t amount, bool speedUp, PlayerNames player)
{
	return Locator::resourceStoreSystem::has_value() &&
	       Locator::resourceStoreSystem::value().PourAt(type, point, amount, speedUp, player);
}

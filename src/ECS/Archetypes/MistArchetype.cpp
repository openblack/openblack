/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "MistArchetype.h"

#include <glm/vec3.hpp>

#include "3D/LandIslandInterface.h"
#include "3D/Mists.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Mist.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity MistArchetype::Create(const glm::vec3& position, float altitude, uint32_t colour, float size, float edgeShrink)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();

	glm::vec3 world {position.x, altitude, position.z};
	if (Locator::terrainSystem::has_value())
	{
		world.y += Locator::terrainSystem::value().GetHeightAt({position.x, position.z});
	}
	registry.Assign<Transform>(entity, world, glm::mat3(1.0f), glm::vec3(size));

	// It shrinks edge on when the script gives it an edge shrink other than 1; otherwise it keeps the default of 3
	const bool shrinksEdgeOn = edgeShrink != 1.0f;
	// Its animation starts at random, from the C runtime's numbers rather than the game's
	const auto counter = mists::StartCounter(Locator::gameRandom::value().CrtRandom(0.0f, 16.0f));
	registry.Assign<Mist>(entity, Mist {
	                                  .size = size,
	                                  .colour = colour,
	                                  .shrinksEdgeOn = shrinksEdgeOn,
	                                  .edgeShrink = shrinksEdgeOn ? edgeShrink : 3.0f,
	                                  .counter = counter,
	                                  .counterRemainder = 0.0f,
	                              });
	return entity;
}

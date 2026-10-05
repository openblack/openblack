/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CloudArchetype.h"

#include "3D/Clouds.h"
#include "3D/Mists.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Cloud.h"
#include "ECS/Components/Mist.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity CloudArchetype::Create(const clouds::Layout& layout)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();

	registry.Assign<Transform>(entity, clouds::WorldPosition(layout.track), glm::mat3(1.0f), glm::vec3(layout.size));
	registry.Assign<Cloud>(entity, layout.track, layout.pinned);
	// A puff of mist that shrinks edge on, its alpha how far it has faded at the track's ends; its animation starts at
	// random, from the C runtime's numbers as every mist's does
	const auto alpha = static_cast<uint32_t>(clouds::EdgeAlpha(layout.track, layout.pinned));
	registry.Assign<Mist>(entity, Mist {
	                                  .size = layout.size,
	                                  .colour = alpha << 24u,
	                                  .shrinksEdgeOn = true,
	                                  .edgeShrink = layout.edgeShrink,
	                                  .counter = mists::StartCounter(Locator::gameRandom::value().CrtRandom(0.0f, 16.0f)),
	                                  .counterRemainder = 0.0f,
	                              });
	return entity;
}

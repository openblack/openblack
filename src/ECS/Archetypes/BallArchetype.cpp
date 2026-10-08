/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "BallArchetype.h"

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

#include "ECS/Components/Ball.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourceManager.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity BallArchetype::Create(const glm::vec3& position)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	const auto& info = Locator::infoConstants::value().ball;

	registry.Assign<Transform>(entity, position, glm::mat3(1.0f), glm::vec3(1.0f));
	registry.Assign<Mobile>(entity);
	registry.Assign<Ball>(entity);
	registry.Assign<Mesh>(entity, resources::HashIdentifier(info.meshId), static_cast<int8_t>(0), static_cast<int8_t>(1));
	// A ball no side plays with belongs to no one: the neutral player's call to come and play
	if (Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().Create(
		    {.initiator = entity, .type = Reaction::ReactToBall, .player = PlayerNames::NEUTRAL, .position = position});
	}
	return entity;
}

/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "HandSystem.h"

#include <glm/gtc/constants.hpp>

#include "ECS/Archetypes/HandArchetype.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

bool HandSystem::Initialize(entt::entity playerEntity) noexcept
{
	auto& registry = Locator::entitiesRegistry::value();
	auto& playerHand = registry.Get<PlayerHand>(playerEntity);

	playerHand.hands[static_cast<size_t>(PlayerHand::Side::Left)] =
	    HandArchetype::Create(glm::vec3(0.0f), glm::half_pi<float>(), 0.0f, glm::half_pi<float>(), 0.01f, false);
	playerHand.hands[static_cast<size_t>(PlayerHand::Side::Right)] =
	    HandArchetype::Create(glm::vec3(0.0f), glm::half_pi<float>(), 0.0f, glm::half_pi<float>(), 0.01f, true);

	return false;
}

std::array<entt::entity, static_cast<size_t>(PlayerHand::Side::_Count)>
HandSystem::GetPlayerHands(entt::entity playerEntity) const noexcept
{
	auto& registry = Locator::entitiesRegistry::value();
	auto& playerHand = registry.Get<PlayerHand>(playerEntity);
	return playerHand.hands;
}

std::array<std::optional<glm::vec3>, static_cast<size_t>(PlayerHand::Side::_Count)>
HandSystem::GetPlayerHandPositions(entt::entity playerEntity) const noexcept
{
	const auto& registry = Locator::entitiesRegistry::value();
	auto& playerHand = registry.Get<PlayerHand>(playerEntity);
	std::array<std::optional<glm::vec3>, static_cast<size_t>(PlayerHand::Side::_Count)> result = {
	    registry.Get<Transform>(playerHand.hands[static_cast<size_t>(PlayerHand::Side::Left)]).position,
	    registry.Get<Transform>(playerHand.hands[static_cast<size_t>(PlayerHand::Side::Right)]).position,
	};
	// TODO(#693): Hand Getter should return an optional if the hand doesn't have a valid position
	// When the position is zero, it probably means it's not on the map (e.g. mouse is in the sky)
	if (result[static_cast<size_t>(PlayerHand::Side::Left)] == glm::zero<glm::vec3>() ||
	    (playerHand.showHands & static_cast<size_t>(PlayerHand::ActiveHand::Left)) == 0)
	{
		result[static_cast<size_t>(PlayerHand::Side::Left)] = std::nullopt;
	}
	if (result[static_cast<size_t>(PlayerHand::Side::Right)] == glm::zero<glm::vec3>() ||
	    (playerHand.showHands & static_cast<size_t>(PlayerHand::ActiveHand::Right)) == 0)
	{
		result[static_cast<size_t>(PlayerHand::Side::Right)] = std::nullopt;
	}
	return result;
}

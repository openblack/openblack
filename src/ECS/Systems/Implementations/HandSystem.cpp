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

#include <ECS/Systems/TimeSystemInterface.h>
#include <glm/gtc/constants.hpp>

#include "ECS/Archetypes/HandArchetype.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Swayable.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

// TODO(#708): Add to global configurations
// Direction x for tree swaying
constexpr auto k_SwayX = 1.0f;
// Direction z for tree swaying
constexpr auto k_SwayZ = 1.0f;
// Only affect trees within a small radius of the hand
constexpr auto k_SwayEffectRadius = 5.0f;
// Larger radius for transition area to return to natural sway and original direction xz
constexpr auto k_SwayTransitionRadius = 10.0f;
// Maximum amplified sway strength when hand is close
constexpr auto k_MaxSwayStrength = 10.0f;
// Applies exponential factor to make the effect more pronounced closer to hand
constexpr auto k_SwayStrengthBase = 6.0f;
constexpr auto k_SwayStrengthExponent = 4.0f;

bool HandSystem::Initialize() noexcept
{
	_hands[static_cast<size_t>(Side::Left)] =
	    HandArchetype::Create(glm::vec3(0.0f), glm::half_pi<float>(), 0.0f, glm::half_pi<float>(), 0.01f, false);
	_hands[static_cast<size_t>(Side::Right)] =
	    HandArchetype::Create(glm::vec3(0.0f), glm::half_pi<float>(), 0.0f, glm::half_pi<float>(), 0.01f, true);

	return false;
}

void HandSystem::Update() noexcept
{
	auto handPositions = Locator::handSystem::value().GetPlayerHandPositions();
	std::optional<glm::vec3> handPosition;
	for (const auto& position : handPositions)
	{
		if (position.has_value())
		{
			SwayNearbyEntities(position.value());
		}
	}
}

std::array<entt::entity, static_cast<size_t>(HandSystemInterface::Side::_Count)> HandSystem::GetPlayerHands() const noexcept
{
	return _hands;
}

std::array<std::optional<glm::vec3>, static_cast<size_t>(HandSystemInterface::Side::_Count)>
HandSystem::GetPlayerHandPositions() const noexcept
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto hands = GetPlayerHands();
	std::array<std::optional<glm::vec3>, static_cast<size_t>(Side::_Count)> result = {
	    registry.Get<Transform>(hands[static_cast<size_t>(Side::Left)]).position,
	    registry.Get<Transform>(hands[static_cast<size_t>(Side::Right)]).position,
	};
	// TODO(#693): Hand Getter should return an optional if the hand doesn't have a valid position
	// When the position is zero, it probably means it's not on the map (e.g. mouse is in the sky)
	if (result[static_cast<size_t>(Side::Left)] == glm::zero<glm::vec3>())
	{
		result[static_cast<size_t>(Side::Left)] = std::nullopt;
	}
	if (result[static_cast<size_t>(Side::Right)] == glm::zero<glm::vec3>())
	{
		result[static_cast<size_t>(Side::Right)] = std::nullopt;
	}
	return result;
}

void HandSystem::SwayNearbyEntities(const glm::vec3& handPosition)
{
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<const Transform, Swayable>([this, &handPosition](const Transform& transform, Swayable& swayable) {
		auto gameSpeed = Game::Instance()->GetGameSpeed();
		auto elapsedTime = Locator::time::value().GetElapsedTime();
		auto naturalSwayTime = std::sin((elapsedTime.count() / 1000.0f * gameSpeed) + (swayable.swayOffset * 100.0f));
		auto position = transform.position;
		// Calculate distance from hand to tree
		auto treeToHand = position - handPosition;
		// The player's hand is always near to the ground
		treeToHand.y = 0.0f;
		auto distanceSquared = glm::dot(treeToHand, treeToHand);
		// Calculate the natural sway time using sin function (what we'd use if no hand influence)
		auto baseSwayStrength = swayable.swayStrengthBase; // Store the tree's natural sway strength

		if (distanceSquared > 0.0f && distanceSquared < k_SwayTransitionRadius * k_SwayTransitionRadius)
		{
			// Calculate 2D direction (XZ plane) from hand to tree
			glm::vec2 direction(treeToHand.x, treeToHand.z);
			direction = glm::normalize(direction);

			// Calculate distance
			auto distance = std::sqrt(distanceSquared);

			// Calculate strength factor that scales from 1.0 when distance=0 to 0.0 at effectRadius
			float distanceFactor = 1.0f - (distance / k_SwayEffectRadius);
			distanceFactor = glm::clamp(distanceFactor, 0.0f, 1.0f);

			// Calculate transition factor - from 0.0 (natural sway) to 1.0 (hand influence)
			// Without this transition, swayed objects snap back
			float transitionFactor = 1.0f;
			if (distance > k_SwayEffectRadius && distance < k_SwayTransitionRadius)
			{
				// Smooth transition between effect radius and transition radius
				transitionFactor = 1.0f - ((distance - k_SwayEffectRadius) / (k_SwayTransitionRadius - k_SwayEffectRadius));
			}

			// Blend between natural swaying and hand-influenced swayTime based on transition factor
			auto handSwayTime = glm::clamp(distanceFactor, 0.0f, 1.0f);
			swayable.swayTime = glm::mix(naturalSwayTime, handSwayTime, transitionFactor);

			// Increase sway strength based on proximity to hand
			auto proximityFactor = k_SwayStrengthBase * std::pow(distanceFactor, k_SwayStrengthExponent);

			// Clamped to prevent excessive sway
			auto amplifiedProximityValue =
			    glm::clamp(baseSwayStrength * (1.0f + proximityFactor), baseSwayStrength, k_MaxSwayStrength);

			// Calculate amplified sway strength, clamped between base and maximum
			auto amplifiedStrength = glm::mix(baseSwayStrength, amplifiedProximityValue, transitionFactor);

			// Apply the smoothly transitioning sway strength
			swayable.swayStrength = glm::clamp(amplifiedStrength, 0.0f, k_MaxSwayStrength);
			swayable.swayDirection.x = glm::mix(k_SwayX, direction.x, distanceFactor * transitionFactor);
			swayable.swayDirection.y = glm::mix(k_SwayZ, direction.y, distanceFactor * transitionFactor);
			swayable.swayedByHand = true;
		}
		else
		{
			swayable.swayedByHand = false;
		}
	});
}

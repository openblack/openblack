/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "VillagerShieldShelter.h"

#include <cmath>

#include <numbers>

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/vec_swizzle.hpp>

#include "Common/GameRandom.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/TownDesire.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/TownDesire.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/ShieldRules.h"

using namespace openblack;
using namespace openblack::ecs::components;
namespace villager_shield = openblack::ecs::villager_shield;
namespace shield = openblack::magic::shield;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
constexpr size_t k_ForProtection = 3;

/// The way a villager faces, as an angle across the land, from how it is turned: walking turns it by
/// eulerAngleY(-angle - a quarter turn), so that angle is undone here
float FacingOf(const Transform& transform)
{
	const auto x = transform.rotation * glm::vec3(1.0f, 0.0f, 0.0f);
	return -std::atan2(-x.z, x.x) - glm::radians(90.0f);
}

void Face(Transform& transform, float angle)
{
	transform.rotation = glm::eulerAngleY(-angle - glm::radians(90.0f));
}

/// Turns a villager towards a point by at most its turn a game turn; whether it faces it
bool LookAtPos(Transform& transform, glm::vec2 point)
{
	const auto towards = point - glm::xz(transform.position);
	const float target = std::atan2(towards.y, towards.x);
	const float step = shield::k_LookTurnPerTurn * k_TwoPi;
	const float facing = FacingOf(transform);
	const float diff = std::remainder(target - facing, k_TwoPi);
	if (std::abs(diff) < step)
	{
		Face(transform, target);
		return true;
	}
	Face(transform, facing + (diff > 0.0f ? step : -step));
	return false;
}

void SetupWaitForCounter(LivingAction& action, uint16_t counter, VillagerStates state)
{
	auto& living = Locator::livingActionSystem::value();
	living.VillagerSetState(action, LivingAction::Index::Final, state, true);
	living.VillagerSetState(action, LivingAction::Index::Top, VillagerStates::WaitForCounter, false);
	action.turnsUntilStateChange = counter;
}
} // namespace

float villager_shield::ProtectionSignificance(entt::entity town)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* desire = town != entt::null && registry.Valid(town) ? registry.TryGet<const TownDesire>(town) : nullptr;
	if (desire == nullptr || !Locator::infoConstants::has_value())
	{
		return 0.0f;
	}
	return town_desire::GetDesireSignificanceToVillager(*desire, Locator::infoConstants::value().townDesire.at(k_ForProtection),
	                                                    k_ForProtection);
}

uint32_t villager_shield::AmazedByMagicShield(LivingAction& action)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto& random = Locator::gameRandom::value();
	const auto villager = registry.ToEntity(action);
	auto* reacting = registry.TryGet<VillagerShieldReaction>(villager);
	const auto* person = registry.TryGet<const Villager>(villager);
	const auto town = person != nullptr ? person->town : entt::null;
	const auto* standing = reacting != nullptr && registry.Valid(reacting->shield)
	                           ? registry.TryGet<const MagicShield>(reacting->shield)
	                           : nullptr;
	if (town != entt::null && registry.Valid(town) && standing != nullptr && standing->spell != entt::null &&
	    ProtectionSignificance(town) > 0.0f)
	{
		auto& transform = registry.Get<Transform>(villager);
		const auto centre = glm::xz(registry.Get<const Transform>(reacting->shield).position);
		// It looks out through the shield, now and then somewhere else once it faces where it looked
		if (LookAtPos(transform, reacting->lookAt) && random.GameRand(4) == 0)
		{
			reacting->lookAt =
			    shield::LookOut(centre, glm::xz(transform.position), random.GameFloatRand(shield::k_ShelterTurnRange));
		}
		registry.SetDirty();
		// A new animation every 20 to 30 seconds, talking and pointing after pointing up: chosen as the game chooses it,
		// though villagers play no animations yet
		static_cast<void>(random.GameFloatRand(shield::k_AnimationSecondsRange));
		auto animation = shield::AmazedAnimation(random.GameRand(5));
		if (reacting->animation == shield::k_PointingAnimation)
		{
			animation = shield::k_TalkingAndPointingAnimation;
		}
		reacting->animation = animation;
		return 1;
	}
	// The shield has gone, or the town no longer wants protection: it waits a while, then decides what to do
	const auto wait = static_cast<uint16_t>(static_cast<int32_t>(
	    static_cast<float>(random.GameRand(shield::k_GiveUpWaitRange)) + static_cast<float>(shield::k_GiveUpWaitLeast)));
	SetupWaitForCounter(action, wait, VillagerStates::DecideWhatToDo);
	return 1;
}

uint32_t villager_shield::WaitForCounter(LivingAction& action)
{
	auto& living = Locator::livingActionSystem::value();
	action.turnsUntilStateChange = static_cast<uint16_t>(action.turnsUntilStateChange - 1);
	if (static_cast<int16_t>(action.turnsUntilStateChange) < 1)
	{
		auto final = living.VillagerGetState(action, LivingAction::Index::Final);
		if (final == VillagerStates::InvalidState)
		{
			final = VillagerStates::DecideWhatToDo;
		}
		living.VillagerSetState(action, LivingAction::Index::Final, VillagerStates::InvalidState, true);
		living.VillagerSetState(action, LivingAction::Index::Top, final, false);
	}
	return 1;
}

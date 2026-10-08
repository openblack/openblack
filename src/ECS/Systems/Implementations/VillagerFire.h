/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <entt/entity/entity.hpp>

#include "Enums.h"

namespace openblack::ecs::components
{
struct LivingAction;
}

/// The villagers and fire: those near a blaze react, their own town's come to beat it out when it needs them and the
/// rest go round it; a villager the heat reaches runs from it until it is out of reach, and one that burns to death dies
/// and leaves its skeleton. The state functions are the living action system's.
namespace openblack::ecs::villager_fire
{

/// How urgently a villager reacts to an object's fire, 0 to 255; 0 when it shouldn't, or when a fireman takes the fire
/// into the blaze it already fights
[[nodiscard]] uint8_t ReactToFirePriority(entt::entity villager, entt::entity object);
/// A villager takes up the reaction to an object's fire
void SetupReactToFire(entt::entity villager, entt::entity object);
/// A villager the heat of an object's fire reaches runs from it; with none, from its own fire
void SetupOnFire(entt::entity villager, entt::entity fire);
/// Whether a villager is running from a fire
[[nodiscard]] bool IsRunningOnFire(entt::entity villager);
/// Whether a fire can take hold of a villager: not one dying, at home, held, or hiding in a building
[[nodiscard]] bool CanCatchFire(entt::entity villager);
/// Whether a villager is fighting a fire or reacting to one, which keeps the fire's heat off it
[[nodiscard]] bool IsFireMan(entt::entity villager);
/// A fireman stops fighting and goes back to what it was doing
void StopFireFighting(entt::entity villager);
/// The fire a villager fights or runs from, none for anybody else
[[nodiscard]] entt::entity FireOf(entt::entity villager);
/// A villager killed by an effect such as fire dies: it falls, lies dead as a skeleton for a while and goes
void DieByEffect(entt::entity villager);

uint32_t ReactToFire(components::LivingAction& action);
uint32_t PutOutFireByBeating(components::LivingAction& action);
uint32_t OnFire(components::LivingAction& action);
uint32_t MoveAroundFire(components::LivingAction& action);
/// The water states are never chosen: a villager there goes back to deciding what to do
uint32_t PutOutFireWithWater(components::LivingAction& action);
uint32_t Dying(components::LivingAction& action);
uint32_t Dead(components::LivingAction& action);

bool EnterPutOutFire(components::LivingAction& action, VillagerStates from, VillagerStates to);
bool ExitPutOutFire(components::LivingAction& action, VillagerStates next);
bool EnterOnFire(components::LivingAction& action, VillagerStates from, VillagerStates to);
bool ExitOnFire(components::LivingAction& action, VillagerStates next);
/// Leaving the reaction to a fire for a state that isn't a reaction lets the reaction go
bool ExitReaction(components::LivingAction& action, VillagerStates next);
/// A villager reacting to a fire whose object has gone, or is held, goes back to what it was doing
bool ReactionValidate(components::LivingAction& action);

} // namespace openblack::ecs::villager_fire

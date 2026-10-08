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

#include <optional>

#include <entt/entity/entity.hpp>

#include "Enums.h"

namespace openblack::ecs
{
struct PhysicsEntry;
} // namespace openblack::ecs

namespace openblack::ecs::components
{
struct LivingAction;
} // namespace openblack::ecs::components

/// Villagers as the physics throws, knocks and lands them: flying, coming down on their feet, back or front, getting up,
/// dying of the fall, drowning in the sea, and pointing at or running from things flying by
namespace openblack::ecs::villager_physics
{

/// A villager starts to fly: it remembers what it was doing, unless it was in a hand. Whether it flies: a state it
/// can't leave keeps it on the ground.
bool StartFlying(entt::entity villager);
/// A villager is taken into a hand, the player's or a creature's: it remembers what it was doing and is held
void IntoHand(entt::entity villager);
/// A villager's body came to rest, or it was let go without a body: it stands up facing the way its fall leaves it, and
/// lands, drowns or dies where it came down. Called once it is back in the map.
void Land(PhysicsEntry* entry, entt::entity villager);
/// A villager's body sank in the sea: it starts drowning, or a dead one sinks dying. Whether it has sunk.
bool Sink(PhysicsEntry& entry);
/// It points at a thing flying by, or runs from one coming too near
void SetupReactToFlyingObject(entt::entity villager, entt::entity object);
/// Whether a villager takes a flying-object reaction at all: not while flying or landing
[[nodiscard]] bool TakesFlyingObjectReaction(entt::entity villager);

/// The player who threw what a villager was hit by or dropped it, as the hand let it go
[[nodiscard]] std::optional<PlayerNames> DropperOf(entt::entity object);

uint32_t Flying(components::LivingAction& action);
/// Leaving flight or the hand, refused for anything but the hand or flight, landing, death and drowning
bool ExitFlying(components::LivingAction& action, VillagerStates next);
bool ExitInHand(components::LivingAction& action, VillagerStates next);
uint32_t Landed(components::LivingAction& action);
uint32_t Drowning(components::LivingAction& action);
uint32_t PointAtFlyingObject(components::LivingAction& action);

} // namespace openblack::ecs::villager_physics

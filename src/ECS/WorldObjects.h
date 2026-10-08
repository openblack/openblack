/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack
{
struct GObjectInfo;
struct GAbodeInfo;
} // namespace openblack

/// What the world's objects have in common however they are made: the table row of their kind, how big they stand, their
/// life and what happens as they lose it, and taking one away. Fire and the miracles that break things share these.
namespace openblack::ecs::world_objects
{

/// The table row of an object's kind: a villager's, a creature's species', a tree's, a building's and so on; none for
/// an object the tables don't cover
[[nodiscard]] const GObjectInfo* InfoOf(entt::entity object);
/// A building's own row, found by its number and model
[[nodiscard]] const GAbodeInfo* AbodeInfoOf(entt::entity object);

/// How big an object stands: its radius across the ground and its height, from its model as it is scaled
struct Size
{
	float radius;
	float height;
};
[[nodiscard]] Size SizeOf(entt::entity object);

/// Its life, 0 to 1: a villager's health, a creature's life, any other object's own (1 until it is hurt)
[[nodiscard]] float LifeOf(entt::entity object);
/// Life taken from an object, as each kind takes it: a building's people come out, and it stops working once low; a
/// field loses food rather than life; a creature that runs out is knocked out. Its life after.
float ReduceLife(entt::entity object, float damage);

/// Whether a building, a field, a wonder, a spell dispenser and the like: what a miracle that destroys things leaves
/// standing at no life
[[nodiscard]] bool IsBuilding(entt::entity object);
/// Whether a miracle may destroy an object, as the blast's wave shatters what it reaches and the tornado carries off
/// what it picks up. Not a creature, a field, the temple, a teleport stone, a totem or a one-off spell seed, nor a pot
/// with nothing in it. (Nor, in the game, something a script made indestructible, or held while the advisors speak:
/// neither is kept here yet.)
[[nodiscard]] bool CanBeDestroyedBySpell(entt::entity object);
[[nodiscard]] bool IsVillager(entt::entity object);
[[nodiscard]] bool IsCreature(entt::entity object);

/// What an effect that destroys things does to an object: a building is left standing with no life, a villager dies,
/// anything else goes from the world. A creature is never destroyed.
void Destroy(entt::entity object);
/// The object goes from the world at once, with its physics and its place in its building or town
void Remove(entt::entity object);
/// A ghost of an object about to go flickers out where it is drawn for half a second, as things taken into a store or
/// poured out of a pot vanish
void LeaveGhost(entt::entity object);
/// What an effect such as fire does to an object it has burnt down: a building flickers out as a ghost of itself and
/// goes, leaving no ruin; a field loses its crop and its fire; a villager dies; anything else goes from the world. A
/// creature is never destroyed.
void DestroyedByEffect(entt::entity object);

/// The player an object belongs to, whom a fire a script lights on it is put down to: a creature's owner; a villager's,
/// a building's or a field's town's owner; a magic tree's caster. A villager or field without a town, a tree and a dead
/// tree the land made belong to nobody; a building without a town and anything else to the neutral player, whom a
/// fire credits no more than nobody.
[[nodiscard]] std::optional<PlayerNames> PlayerOf(entt::entity object);

/// Harm done to a town's villager or building is an attack on its town by whoever did it
void AttackTown(entt::entity object, float damage, PlayerNames aggressor);

/// Life given back to an object other than a living thing, as a miracle's heal gives it: never above 1. Its life after.
float IncreaseLife(entt::entity object, float amount);
/// Whether crushing an object makes the people nearby react to it: anything living, and anything fixed to the land, such
/// as a building, a tree, a field or a rock, but not what can be picked up and moved
[[nodiscard]] bool CanBeCrushed(entt::entity object);

} // namespace openblack::ecs::world_objects

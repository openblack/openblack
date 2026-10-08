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

#include <entt/entity/fwd.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs
{
struct PhysicsEntry;
struct ImpactInfo;
namespace systems
{
class DynamicsSystemInterface;
}

/// What trees, dead trees, rocks and pots do in the physics: a tree put down gently takes root again and finds a forest,
/// one thrown falls dead; dead trees call people to their wood; rocks wear away under hard knocks and break in two, as
/// they do when the hand taps them or a creature smashes them; a handful of food or wood that comes to rest on the land
/// spills into the stores and piles round it.
namespace object_physics
{

/// A tree's flight ends: it takes root again, stays, or becomes a dead tree lying as it came down, which is returned
entt::entity EndTree(systems::DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity tree, bool insert);
/// A dead tree's flight ends, and people are called to its wood
entt::entity EndDeadTree(systems::DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity deadTree, bool insert);
/// A handful of food or wood comes to rest: on the land it spills into the stores and piles round it and is gone
entt::entity EndPot(systems::DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity pot, bool insert);
/// The sound a tree makes as it takes root, one of three chosen by the system clock's milliseconds
void TreeDropSound(entt::entity tree, uint64_t ticks);

/// Whether a thing is a bonfire, which is a kind of rock
[[nodiscard]] bool IsBonfire(entt::entity object);
/// Whether a thing is a rock, bonfires included: what a creature's blow smashes, and what doesn't wear another rock
[[nodiscard]] bool IsRock(entt::entity object);
/// A rock's reaction to a knock: worn away by a hard one, and broken in two once worn out
void KnockRock(systems::DynamicsSystemInterface& dynamics, entt::entity rock, const ImpactInfo& impact);
/// A rock breaks into two halves that carry on as it was moving, its fire spreading to both
void SplitRock(systems::DynamicsSystemInterface& dynamics, entt::entity rock);
/// Whether the hand may break a rock by tapping it
[[nodiscard]] bool CanTapRock(entt::entity rock);
/// The hand taps a rock at a point: it breaks with a knock, and the player's creature sees the fun of it
void TapRock(systems::DynamicsSystemInterface& dynamics, entt::entity rock, glm::vec3 handPoint,
             std::optional<PlayerNames> player);
/// A creature's blow smashes a rock in two
void SmashRock(systems::DynamicsSystemInterface& dynamics, entt::entity rock);

} // namespace object_physics
} // namespace openblack::ecs

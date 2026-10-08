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
/// A rock, static or dead tree comes to rest put down gently from a player's hand: the nearest building within reach
/// makes it its town's artefact
void ConsiderArtefact(const PhysicsEntry* entry, entt::entity object, bool insert);
/// An artefact taken into a hand leaves its town, remembering who took it
void ArtefactTaken(entt::entity object, PlayerNames player);
/// A forester fells a tree: a dead tree of it, the neutral player's, falls away from the feller into the physics, and
/// people are called to its wood; the tree itself is left for the feller to remove. The felled tree, none if it could
/// not be made.
entt::entity FellTree(systems::DynamicsSystemInterface& dynamics, entt::entity tree, entt::entity feller);
/// A footballer kicks the football at a point at a speed: one flying or lying in the physics, but not one a tornado
/// carries, is first taken out of it; then, standing in the map, it flies at that point with no spin and no thrower,
/// laid onto the land, the people's bodies letting it through as something a living thing pushed, and it remembers
/// where it was kicked at. Whether it was set flying.
bool KickBall(systems::DynamicsSystemInterface& dynamics, entt::entity ball, glm::vec3 destination, float speed);

} // namespace object_physics
} // namespace openblack::ecs

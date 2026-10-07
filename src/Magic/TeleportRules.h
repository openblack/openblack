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
#include <span>

#include <glm/vec3.hpp>

#include "Enums.h"

// The teleport miracle's rules. Each cast leaves an invisible stone on the land; a player's stones form a network. A
// villager or creature of that player walking past one of their stones takes a detour into it when jumping to another
// of the player's stones would save it enough of its walk, and comes out of the stone that leaves it closest to where it
// was going. A useful jump gives the miracle back prayer power for the way it saved; only a jump forced the wrong way
// costs. Pure functions of points on the land, tested without the game.

namespace openblack::magic::teleport
{

/// The stone's reach: nothing that stands fixed on the land may be this close to a new one, and no building may be put
/// on it
inline constexpr float k_StoneRadius = 6.0f;
/// The hand touches a stone within this of it
inline constexpr float k_HandTouchRadius = 3.0f;
/// The detour must be this much shorter than the walk it replaces, as a factor of its length
inline constexpr float k_DetourFactor = 1.2f;
/// A forced jump takes any other stone, even one that leaves the traveller further from where it was going
inline constexpr float k_ForcedSaving = -1000000.0f;
/// The prayer power a jump pays per metre saved, from the tables' cost per kilometre
inline constexpr float k_MetresPerKilometre = 0.001f;
/// A creature walks to a stone, and carries on to where it was going, arriving within this of it
inline constexpr float k_CreatureArriveDistance = 5.0f;
/// A creature the stone carries fades out over this long, comes out, then fades back in over as long again
inline constexpr float k_CreatureFadeSeconds = 2.0f;
/// The land's fixed point positions count this many units a metre
inline constexpr float k_FixedUnitsPerMetre = 6553.6f;

/// Whether a walker is within a step of a point across the land, by the land's fixed point units
[[nodiscard]] bool WithinAStep(glm::vec3 walker, glm::vec3 point, float stepMetres);
/// The turns a creature the stone carries takes to fade out, and again to fade in
[[nodiscard]] int32_t CreatureFadeTurns(float turnMilliseconds);
/// How far a creature the stone carries has faded, 0 to nearly 1, with so many turns of its fading out left; fading back
/// in, the share of the turns left
[[nodiscard]] float CreatureFadeOut(int32_t turnsLeft, int32_t turns);
[[nodiscard]] float CreatureFadeIn(int32_t turnsLeft, int32_t turns);

/// A cheap distance between two points across the land, in the land's fixed point units: the longer of the two sides
/// plus half the shorter, as the game measures whether a detour is worth taking
[[nodiscard]] int32_t FastDistance(glm::vec3 a, glm::vec3 b);

/// Whether a traveller at a point going to a destination should turn aside into the stone at `stone`, given another of
/// the same player's stones at `other`: the way through the two stones, with a fifth more for the trouble, is shorter
/// than the walk
[[nodiscard]] bool IsWorthTheDetour(glm::vec3 traveller, glm::vec3 destination, glm::vec3 stone, glm::vec3 other);

/// Whether any other of the player's stones makes the detour worth it. `stones` are all of the player's stones; the one
/// at index `self` is the stone being passed.
[[nodiscard]] bool ShouldReact(glm::vec3 traveller, glm::vec3 destination, std::span<const glm::vec3> stones, size_t self);

/// The stone a traveller jumps to and how many metres closer it brings it
struct Jump
{
	size_t stone;
	float saving;
};

/// The stone, other than the one jumped from, that brings the traveller closest to its destination: only a jump that
/// brings it closer unless forced, the first stone kept when two are as good. Stones are given newest first.
[[nodiscard]] std::optional<Jump> ChooseTarget(glm::vec3 traveller, glm::vec3 destination, std::span<const glm::vec3> stones,
                                               size_t from, bool forced);

/// What a jump costs the miracle: the metres saved paid back at the cost per kilometre, so a useful jump gives prayer
/// power back and only a backwards jump costs
[[nodiscard]] float JumpCost(float saving, float costPerKilometre);

/// Whether a stone may be put at a point: nothing fixed on the land (a building, a field, a feature, another stone)
/// stands within its reach
[[nodiscard]] bool CanPlaceStone(glm::vec3 point, std::span<const glm::vec3> fixedObjects);

/// The stone a worshipper heads for to reach a far worship site: the stone nearest it, when going through the stones
/// (to the nearest stone, then from the stone nearest the site) is shorter than the limit; none otherwise
[[nodiscard]] std::optional<size_t> FindRouteStone(glm::vec3 worshipper, glm::vec3 site, std::span<const glm::vec3> stones,
                                                   float maxDistance);

/// Whether a villager dropped by the hand onto a stone jumps at once: the villager is the stone owner's, and the player
/// has another stone to jump to
/// The state a villager keeps to come back to as it starts reacting: the state it was to take, unless its state table
/// says that state keeps the one kept before
[[nodiscard]] VillagerStates PreviousToKeep(VillagerStates final, VillagerStates keptBefore, bool finalKeepsPrevious);
/// The state a villager takes once it stops reacting: the state its table says to come back to from the one it kept,
/// deciding afresh when there is none
[[nodiscard]] VillagerStates StateAfterReacting(VillagerStates comeBackTo);

[[nodiscard]] bool CanDropOnStone(bool samePlayer, size_t playerStones);

} // namespace openblack::magic::teleport

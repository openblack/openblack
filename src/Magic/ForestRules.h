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

#include <array>
#include <optional>

#include <glm/vec2.hpp>

#include "Enums.h"

// The forest miracle's rules. As its seed lands the forest plants all its trees at once, on a spiral
// out from where it was cast, each of the kind the ground there grows; they grow a little each turn to a size that
// shrinks with their distance from the middle. While the miracle lasts it pays for itself and for each tree; once it has
// gone, its trees wither away and the forest with them. Like any tree short of its size, they also grow by themselves
// now and then (see TreeGrowth.h). Pure rules, tested without the game.

namespace openblack::magic::forest
{

/// The spiral starts this far from the middle and ends this far out, turning 17/13 of a turn for each tree
inline constexpr float k_InnerRadius = 2.0f;
inline constexpr float k_OuterRadius = 11.0f;
inline constexpr float k_TurnsPerTree = 1.307692289f;
/// Where tree `index` of `count` goes, from the middle
[[nodiscard]] glm::vec2 SpiralOffset(uint32_t index, uint32_t count);
/// The size a tree grows to at a distance from the middle: full in the middle, half at the spiral's end
[[nodiscard]] float TargetScale(float distance);

/// How many trees the forest may have: as many as it may still make (its tables' number for no limit), none once it
/// has no strength left
[[nodiscard]] uint32_t TreesWeCanAfford(int maxObjectsToCreate, uint32_t finalTrees, float strength);
/// How many more it may make before it is planted, or none once it is: what is left of a limit, the tables' number
/// without one
[[nodiscard]] uint32_t MaxObjectsToCreate(int maxObjectsToCreate, uint32_t finalTrees, bool planted,
                                          std::optional<uint32_t> trees);
/// Its upkeep: its plain upkeep and the cost of an event for each tree
[[nodiscard]] constexpr float Upkeep(float plainCost, float costPerTree, uint32_t trees)
{
	return plainCost + (costPerTree * static_cast<float>(trees));
}

/// The kinds of tree the ground's material grows by magic, one picked by a roll of 0 to 3
[[nodiscard]] TreeInfo SpeciesFor(const std::array<TreeInfo, 4>& kinds, uint32_t roll);

/// A tree grows by an amount, never past its size
[[nodiscard]] float Grow(float scale, float amount, float target);
/// A tree withers by an amount; none once it has withered away
[[nodiscard]] std::optional<float> Wither(float scale, float amount);

/// What a magic tree's wood is worth against an ordinary tree's, by the caster's tribal power
[[nodiscard]] constexpr float WoodMultiplier(float woodValueMultiplier, float tribalPower)
{
	return woodValueMultiplier * tribalPower;
}
/// The wood a tree gives: its size, its multiplier (1 for an ordinary tree), its life and its kind's value
[[nodiscard]] constexpr float WoodValue(float scale, float multiplier, float life, float kindValue)
{
	return scale * multiplier * life * kindValue;
}

/// Bats fly from the forest of a player this evil, butterflies from any other
inline constexpr float k_EvilAlignment = -0.5f;
[[nodiscard]] constexpr bool BatsFor(float alignment)
{
	return alignment < k_EvilAlignment;
}

} // namespace openblack::magic::forest

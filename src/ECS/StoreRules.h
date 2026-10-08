/*******************************************************************************
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

/// How much the things a store takes are worth, and what giving to a town's store and taking from it counts for
namespace openblack::ecs::store_rules
{

/// A tree's wood: its life, its wood value and its scale (once), times what a forest miracle's tree is worth over an
/// ordinary one and the land's balance for wood, truncated
[[nodiscard]] uint32_t TreeWood(float life, float multiplier, uint32_t woodValue, float scale, float landBalance);
/// A dead tree's wood: its scale and its tree's wood value, times what the tree was worth when it died; its life
/// doesn't count
[[nodiscard]] uint32_t DeadTreeWood(float scale, uint32_t woodValue, float multiplier);
/// A fence's wood: its life and wood value, by the cube of its scale
[[nodiscard]] uint32_t FenceWood(float life, uint32_t woodValue, float scale);
/// An animal is food worth its kind's food value when it is meat or vegetable, whatever its size or life
[[nodiscard]] uint32_t AnimalFood(float foodValue, uint32_t foodType);

/// How much belief giving a town a resource counts for, by how lately the giver took that resource from the town: in
/// full when they never took any, else growing with the cube of the share of the turns since that must pass
[[nodiscard]] float LastTakenModifier(std::optional<uint32_t> turnTaken, uint32_t now, uint32_t turnsToForget);

/// How much of a resource a storage pit holds over what its piles can hold: wood over five full piles, food over one
[[nodiscard]] int64_t AmountOverMaximum(bool wood, uint32_t held, uint32_t pileMaximum);
/// What a pile that is part of a store gives itself of what is asked: what is over the store's maximum comes from its
/// other piles
[[nodiscard]] uint32_t AskedOfPile(uint32_t asked, int64_t overMaximum);

} // namespace openblack::ecs::store_rules

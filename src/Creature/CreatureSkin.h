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

/// A creature's skin shows how evil or good it has become: each skin of its base mesh is blended towards the matching
/// skin of its evil or good mesh, by how far along the evil to good axis its body is drawn. The skins are 4 bits a
/// channel and blended a channel at a time in whole steps, as the game does. A neutral creature shows its base skins.
namespace openblack::creature_skin
{
/// The most the variant skin is weighed, of 255
constexpr uint8_t k_MaxWeight = 255;

/// How much of the evil or good skin shows, 0 to 255, for a body drawn at evilGood, -1 to 1
[[nodiscard]] uint8_t BlendWeight(float evilGood);

/// A 4-bit channel of the base skin moved towards the variant's by weight of 255, rounded down
[[nodiscard]] constexpr uint8_t BlendChannel(uint8_t base, uint8_t other, uint8_t weight)
{
	return static_cast<uint8_t>(((base * (k_MaxWeight - weight)) + (other * weight)) / k_MaxWeight);
}

/// A texel of 4 bits a channel blended a channel at a time
[[nodiscard]] uint16_t BlendTexel(uint16_t base, uint16_t other, uint8_t weight);

/// The variant mesh's skin a base skin is blended with: the one at the same place in the variant's list of skins, or
/// none when the variant has fewer skins
[[nodiscard]] std::optional<uint32_t> PairedSkin(std::span<const uint32_t> baseSkins, std::span<const uint32_t> variantSkins,
                                                 uint32_t baseSkin);
} // namespace openblack::creature_skin

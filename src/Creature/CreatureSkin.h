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
#include <span>
#include <vector>

#include <entt/core/hashed_string.hpp>

#include "Creature/CreatureMarks.h"
#include "Creature/CreatureTattoo.h"

/// A creature's skin shows how evil or good it has become: each skin of its base mesh is blended towards the matching
/// skin of its evil or good mesh, by how far along the evil to good axis its body is drawn. The skins are 4 bits a
/// channel and blended a channel at a time in whole steps, as the game does. A neutral creature shows its base skins.
/// Its tattoos are painted over that, then its wounds, then its blood.
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

/// What tattoos and marks are painted with: the designs, the damage atlases and the tattoo palette
struct Art
{
	std::array<creature_tattoo::Design, creature_tattoo::k_DesignCount> designs;
	creature_marks::DamageArt damage;
	/// k_PaletteColumns by k_PaletteRows colours
	std::vector<std::array<uint8_t, 3>> palette;
};

/// The id the art is loaded under
constexpr entt::id_type k_ArtId = entt::hashed_string("creature/skin_art");

/// What is painted over one of a creature's skins
struct Layers
{
	/// Which of the base mesh's skins it is, in the order it lists them, which the tattoos' sites and the marks name
	uint8_t skinIndex;
	const creature_tattoo::Slots& tattoos;
	/// The species' places for tattoos, if it has any
	const std::optional<creature_tattoo::Sites>& sites;
	const creature_marks::Marks& marks;
	/// None before the art is loaded, which leaves the tattoos and marks off
	const Art* art;
};

/// One of a creature's skins, 256 by 256 texels: the base skin blended towards the variant's by weight (the base alone
/// without a variant), then the tattoos on enabled sites of this skin in slot order, the wounds and the blood on it
void Compose(std::span<uint16_t> out, std::span<const uint16_t> base, std::span<const uint16_t> variant, uint8_t weight,
             const Layers& layers);
} // namespace openblack::creature_skin

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>

#include <entt/core/hashed_string.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// Rings on the water: a flat square of the smoke texture lying on the surface where something splashed, growing and
/// fading out over 0.7 seconds of game time, added over what is behind it.
namespace openblack::water_rings
{

/// The most rings there are at once: when full, a new one isn't made
inline constexpr size_t k_MostRings = 1024;
/// A ring's life, in milliseconds of game time before its rate
inline constexpr uint32_t k_Life = 700;
/// The textures the rings are cut from, 8 by 8 cells
inline constexpr entt::hashed_string k_TextureId = entt::hashed_string("raw/smoke");
inline constexpr entt::hashed_string k_AlphaTextureId = entt::hashed_string("raw/smokea");

struct Ring
{
	glm::vec3 position {0.0f};
	/// Milliseconds of game time it has grown for, at its rate
	uint32_t age {0};
	/// How big it grows: its half width is its age times this over its life
	float growth {1.0f};
	/// How it is turned about the vertical
	float angle {0.0f};
	/// Its depth over its width
	float aspect {1.0f};
	/// How fast it ages
	float rate {1.0f};
	/// Its cell of the smoke texture
	uint8_t cell {0x30};
	/// Its colour, 0xAARRGGBB, as it was made: one taking the land's light keeps that of when it was made
	uint32_t argb {0xFFFFFFFFu};
};

/// The splash the hand makes going into the water where it grips the land: on the surface, a little above, turned at
/// `angle`, its colour the land's brightest light of the moment, 0xRRGGBB
[[nodiscard]] Ring HandSplash(glm::vec2 xz, float angle, uint32_t landLight);

/// A ring after `milliseconds` more of game time; false once it has gone
[[nodiscard]] bool Advance(Ring& ring, float milliseconds);

/// Half the ring's width, and its opacity, 0 to 255: it fades out as it grows
[[nodiscard]] float HalfWidth(const Ring& ring);
[[nodiscard]] uint8_t Alpha(const Ring& ring);

/// The ring's four corners, flat about its position, and the corners of its cell of the texture in the same order
[[nodiscard]] std::array<glm::vec3, 4> Corners(const Ring& ring);
[[nodiscard]] std::array<glm::vec2, 4> CellUvs(uint8_t cell);

} // namespace openblack::water_rings

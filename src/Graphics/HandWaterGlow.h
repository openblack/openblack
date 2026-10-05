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

#include <functional>
#include <optional>

#include <glm/vec2.hpp>

/// At night the hand's light glows warm on the water beneath it: a square of 120 units on the sea level under the hand,
/// added to what lies under the sea, so the sea is blended over it.
namespace openblack::graphics::hand_water_glow
{

inline constexpr float k_HalfSize = 60.0f;
/// How far around the hand the land is looked at for water, in each direction
inline constexpr float k_Reach = 70.0f;
/// Land under this altitude is low enough for the water to show
inline constexpr uint8_t k_LowAltitude = 5;
inline constexpr uint32_t k_MaxAlpha = 190;
/// Where the glow is in the atmosphere texture
inline constexpr glm::vec2 k_UvMinimum {0.75f, 0.375f};
inline constexpr glm::vec2 k_UvMaximum {0.796875f, 0.421875f};
/// The hand's light must be stronger than this for the glow to show
inline constexpr float k_MinimumStrength = 0.01f;

/// The glow's colour, 0xAARRGGBB: the palette's warm colour (0xRRGGBB) a quarter of the way to orange, rounding down,
/// and as opaque as the hand's light is strong, up to 190
[[nodiscard]] uint32_t Colour(uint32_t warmColour, float strength);

/// The altitude of the land's cell at a cell's x and z, none where there is no land
using AltitudeAt = std::function<std::optional<uint8_t>(int x, int z)>;

/// Whether there is water near the hand: any cell within reach is low or not land at all. The cells start no lower
/// than the map's first and no higher than its last, but may run past its last.
[[nodiscard]] bool NearLowLand(glm::vec2 xz, const AltitudeAt& altitudeAt);

} // namespace openblack::graphics::hand_water_glow

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

#include <glm/vec3.hpp>

/// Dust thrown up from the land takes the colour of the land's light where snow lies: the deeper the snow, the nearer
/// its colour comes to the land's colour of the moment
namespace openblack::ecs::snow_dust
{

/// The land's colour of the moment, by the time of day, the camera's alignment and the overcast at the camera (0xRRGGBB);
/// none before the palette is loaded
[[nodiscard]] std::optional<uint32_t> CurrentLandColour();

/// How much snow lies at a point, from 0 to 255
[[nodiscard]] int32_t SnowAt(glm::vec3 point);

/// A dust colour where it is made: blended towards the land's colour by the snow there, its alpha kept
[[nodiscard]] uint32_t Tint(uint32_t argb, int32_t snow);

} // namespace openblack::ecs::snow_dust

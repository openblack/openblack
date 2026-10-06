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

#include <span>
#include <vector>

#include <glm/vec2.hpp>

#include "Enums.h"

/// The testbed's miracle dispensers: one of every miracle the player can be given, laid out in a grid on the camera's
/// side of the middle of the map, far enough apart for the hand to tap each bubble. Scenarios get them too, unless one
/// asks for none.
namespace openblack::testbed_dispensers
{

/// Where the grid's first dispenser stands, from the middle of the map (x east, y north): south of the middle, between
/// it and the testbed's starting camera, clear of the camera and of the lake to the north
inline constexpr glm::vec2 k_GridOrigin {-48.0f, -20.0f};
/// From one dispenser to the next along a row (east) and from one row to the next (south)
inline constexpr glm::vec2 k_GridSpacing {12.0f, -12.0f};
inline constexpr size_t k_GridColumns = 9;
/// The dispensers face the camera, to the south
inline constexpr float k_GridYawRadians = 0.0f;
/// How far round the grid a scenario's creatures and things must stand for it to keep the grid
inline constexpr float k_GridClearance = 8.0f;

/// Every miracle a dispenser can give the player, the creature spells among them, in the tables' order
[[nodiscard]] std::span<const MagicType> GridMagicTypes();
/// Where each of a number of dispensers stands, from the middle of the map, row by row
[[nodiscard]] std::vector<glm::vec2> GridOffsets(size_t count);

/// Lays the grid out round the middle of the map
void PlaceGrid(glm::vec2 middle);
/// Clears the grid away, with its bubbles and swirls
void RemoveGrid();
/// Whether a point, from the middle of the map, is on the grid or too near it to stand there
[[nodiscard]] bool InGridArea(glm::vec2 offset);

} // namespace openblack::testbed_dispensers

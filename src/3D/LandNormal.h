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

#include <glm/vec3.hpp>

/// The land's normal as the game works it out: the flat normal of the cell triangle a point is on, from the cell's raw
/// corner heights (land at sea level isn't flattened), normalised through two lookup tables rather than a square root.
namespace openblack::land_normal
{

constexpr float k_HeightUnit = 0.67f;

/// The inverse length of a cell edge that climbs the given number of height units: 1 / sqrt((0.67 i)^2 + 100), from a
/// table of 256 entries
[[nodiscard]] float EdgeScale(uint32_t climb);
/// The inverse square root of a squared length in 1023rds, from a table of 1024 entries (1 for 0)
[[nodiscard]] float LengthScale(uint32_t index);

/// The normal of a cell, given the fractions of the point in it (16 bits each), whether the cell is split from x + 1 to
/// z + 1, and its corner heights in height units (h01 at z + 1, h10 at x + 1). It points up.
[[nodiscard]] glm::vec3 OfCell(uint32_t fracX, uint32_t fracZ, bool split, int32_t h00, int32_t h01, int32_t h10, int32_t h11);

} // namespace openblack::land_normal

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
#include <glm/vec3.hpp>

/// Where a line meets the land, as the game finds it: the line runs over the map's cells nearest first, and each cell is
/// tested as two triangles of its corners' heights. The game uses this for what is under the cursor, where the camera's
/// moves meet the land and whether the land hides the sun.
namespace openblack::land_line
{

/// The height units of the land's corners, from the corner at the cell itself across x, across z and across both
struct CellHeights
{
	uint8_t here;
	uint8_t acrossX;
	uint8_t acrossZ;
	uint8_t acrossBoth;
};

/// The corner heights of the map cell at x, z (each 0 to 511), none for a cell the land doesn't have
using CellLookup = std::function<std::optional<CellHeights>(int32_t x, int32_t z)>;

/// Cells along each side of the map
inline constexpr float k_MapCells = 512.0f;
/// The line is kept this far, in cells, inside the map's edges
inline constexpr float k_EdgeMargin = 0.1f;
inline constexpr float k_FarEdge = 511.9f;
/// Metres along a cell's side, and up a height unit of the land
inline constexpr float k_CellSize = 10.0f;
inline constexpr float k_HeightUnit = 0.67f;

/// Where a stretch of the line, in cell units (x and z in cells, y in height units), meets the land of a cell: its x and
/// z in cell units, none when it misses. The cell is the triangle on the corners at the cell, across x and across z,
/// then the one on the corners across x, across both and across z, each reaching a tenth of a cell past its edges.
/// Nothing behind the stretch's start counts.
[[nodiscard]] std::optional<glm::vec2> HitInCell(int32_t x, int32_t z, glm::vec3 from, glm::vec3 to, const CellLookup& cells);

/// Where the line from one point to another, in cell units, first meets the land, the line carried on to the map's edge:
/// its x and z in cell units
[[nodiscard]] std::optional<glm::vec2> FirstHit(glm::vec3 from, glm::vec3 to, const CellLookup& cells);

/// Where the line from one point to another, in metres, meets the land: its x and z in metres
[[nodiscard]] std::optional<glm::vec2> LandAlong(glm::vec3 from, glm::vec3 to, const CellLookup& cells);

/// The same, or else where the line meets the sea's level when it goes down, no further across than 7500 from the camera
[[nodiscard]] std::optional<glm::vec2> LandOrSeaAlong(glm::vec3 from, glm::vec3 to, glm::vec3 camera, const CellLookup& cells);

/// The land or sea the interface takes to be under a pixel, from the camera through the pixel's point on the near plane:
/// where that line meets the land (or the sea near the camera), else, with the sea, the sea's level wherever the line
/// goes down. Its y is the land's height there (heightAt).
[[nodiscard]] std::optional<glm::vec3> UnderPixel(glm::vec3 camera, glm::vec3 nearPoint, bool withSea, const CellLookup& cells,
                                                  const std::function<float(glm::vec2)>& heightAt);

/// A point picked under the cursor, kept within 7680 of the map's middle at (2560, 0, 2560)
[[nodiscard]] glm::vec3 KeptInReach(glm::vec3 point);

/// The interface counts the land under the cursor this much further than its depth in the view, so a thing standing on
/// it is nearer
inline constexpr float k_LandDistanceAllowance = 2.3f;

} // namespace openblack::land_line

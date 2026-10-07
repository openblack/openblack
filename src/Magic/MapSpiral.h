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

#include <vector>

#include <glm/vec2.hpp>

// The cells a search walks round a point, in the game's square spiral (map_coords::Spiral) out from the point's cell. A
// search that visits fewer cells than a whole ring covers only the first few neighbours, and the game's searches keep
// that quirk.

namespace openblack::magic
{

/// The first count cells of the spiral out from a cell, the cell itself first
[[nodiscard]] std::vector<glm::ivec2> SpiralCells(glm::ivec2 start, size_t count);

} // namespace openblack::magic

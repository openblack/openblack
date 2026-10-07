/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "MapSpiral.h"

#include "3D/MapCoords.h"

std::vector<glm::ivec2> openblack::magic::SpiralCells(glm::ivec2 start, size_t count)
{
	std::vector<glm::ivec2> cells;
	cells.reserve(count);
	map_coords::Spiral spiral;
	auto cell = start;
	while (cells.size() < count)
	{
		cells.push_back(cell);
		const auto& step = spiral.Next();
		cell += glm::ivec2(step.x, step.z);
	}
	return cells;
}

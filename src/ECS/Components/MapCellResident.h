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

#include <vector>

#include <glm/vec3.hpp>

#include "ECS/MapCells.h"

namespace openblack::ecs::components
{

/// A thing that stands in the map's cells, and where it went in them
struct MapCellResident
{
	map_cells::Placement placement {map_cells::Placement::MobileFront};
	/// A building covers every cell its outline touches; anything else stands in one
	bool coversOutline {false};
	/// Whether it can be moved, so that its cell is checked each turn
	bool moves {false};
	/// The cells it is in, by index
	std::vector<uint32_t> cells;
	/// Where it stood when it went into them
	glm::vec3 filedAt {0.0f};
};

/// A thing on the map that can move, whose cell is checked each turn
struct MapCellMover
{
};

} // namespace openblack::ecs::components

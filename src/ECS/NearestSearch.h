/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <functional>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec2.hpp>

#include "3D/MapCoords.h"

/// How the game finds the nearest thing of a kind about a point on the map
namespace openblack::ecs::nearest_search
{

/// A thing of the sought kind standing in a cell, and where
struct Candidate
{
	entt::entity entity {entt::null};
	map_coords::MapCoords at;
};

/// The things of the sought kind in a cell of the map, in the order the cell keeps them
using CellCandidates = std::function<std::vector<Candidate>(glm::ivec2 cell)>;

/// The nearest thing within a reach of a point: the cells in a spiral out from the point's own, at least three by three
/// of them and as many as twice the reach covers; a thing counts when it is strictly within the reach and strictly
/// nearer than the nearest so far, so the first met wins a tie; and once one is found the walk ends at the first cell
/// farther than half as far again as it, and ten more
[[nodiscard]] entt::entity FindNearest(const map_coords::MapCoords& from, float reach, const CellCandidates& inCell);

} // namespace openblack::ecs::nearest_search

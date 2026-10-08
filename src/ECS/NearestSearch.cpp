/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "NearestSearch.h"

#include <cmath>

#include <algorithm>

#include "Common/GUtilsDistance.h"

using namespace openblack;
using namespace openblack::ecs;

entt::entity nearest_search::FindNearest(const map_coords::MapCoords& from, float reach, const CellCandidates& inCell)
{
	auto side = static_cast<int32_t>(std::ceil((reach + reach) / map_coords::k_CellSize));
	side = std::max(side, 3);
	int32_t cells = side * side;
	entt::entity nearest = entt::null;
	float best = 0.0f;
	auto coords = from;
	map_coords::Spiral spiral;
	while (cells != 0 && (nearest == entt::null || gutils::GetDistanceInMetres(from, coords) <= best * 1.5f + 10.0f))
	{
		if (const glm::ivec2 cell = map_coords::Cell(coords); map_coords::InBounds(cell))
		{
			for (const auto& candidate : inCell(cell))
			{
				const float distance = gutils::GetDistanceInMetres(from, candidate.at);
				if (distance < reach && (distance < best || nearest == entt::null))
				{
					nearest = candidate.entity;
					best = distance;
				}
			}
		}
		--cells;
		map_coords::AddCells(coords, spiral.Next());
	}
	return nearest;
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ScenicForest.h"

#include <algorithm>

#include <glm/geometric.hpp>

#include "Common/GUtilsDistance.h"

using namespace openblack;
using namespace openblack::ecs;

void scenic_forest::TownArea::Add(glm::vec2 at, float radius)
{
	min = glm::min(min, at - radius);
	max = glm::max(max, at + radius);
}

glm::vec2 scenic_forest::TownArea::Centre() const
{
	return (min + max) * 0.5f;
}

std::vector<map_coords::MapCoords> scenic_forest::Walk(const map_coords::MapCoords& centre, float reach)
{
	std::vector<map_coords::MapCoords> cells;
	auto coords = centre;
	map_coords::Spiral spiral;
	for (int32_t left = k_MostCells; left != 0; --left)
	{
		if (gutils::GetDistanceInMetres(coords, centre) > reach)
		{
			break;
		}
		cells.push_back(coords);
		map_coords::AddCells(coords, spiral.Next());
	}
	return cells;
}

bool scenic_forest::Takes(const Tree& tree, glm::vec2 centre)
{
	if (!tree.inForest)
	{
		return true;
	}
	return tree.scenicCentre.has_value() &&
	       gutils::GetDistanceInMetres(centre, tree.at) < gutils::GetDistanceInMetres(*tree.scenicCentre, tree.at);
}

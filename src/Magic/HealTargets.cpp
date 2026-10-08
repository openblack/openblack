/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "HealTargets.h"

#include <cmath>

#include "3D/MapCoords.h"
#include "Common/GUtilsDistance.h"
#include "MapSpiral.h"

using namespace openblack::magic;

size_t openblack::magic::HealCellsAcross(float radius)
{
	if (!(radius > 0.0f))
	{
		return 0;
	}
	return static_cast<size_t>(std::ceil(2.0f * radius / map_coords::k_CellSize));
}

std::vector<entt::entity> openblack::magic::FindHealTargets(glm::vec3 point, float radius, size_t maximum,
                                                            const CellContents& cells)
{
	std::vector<entt::entity> targets;
	const size_t across = HealCellsAcross(radius);
	if (maximum == 0 || across == 0 || !cells)
	{
		return targets;
	}
	const auto cast = map_coords::FromMetres({point.x, point.z});
	const auto castCell = map_coords::Cell(cast);
	const float radiusSquared = radius * radius;
	for (const auto cell : SpiralCells(castCell, across * across))
	{
		// Each cell's people are measured from the cast point moved by whole cells into that cell, its place inside the
		// cell kept
		auto from = cast;
		map_coords::AddCells(from, {static_cast<int16_t>(cell.x - castCell.x), static_cast<int16_t>(cell.y - castCell.y)});
		const auto fromMetres = map_coords::ToMetres(from);
		for (const auto& candidate : cells(cell))
		{
			if (targets.size() >= maximum)
			{
				return targets;
			}
			// Closer than the radius across the land, measured in whole map units and then exactly; heights don't count
			const auto at = map_coords::FromMetres({candidate.position.x, candidate.position.z});
			if (!(gutils::GetDistanceInMetres(from, at) < radius) || !candidate.living || !candidate.healable)
			{
				continue;
			}
			const auto atMetres = map_coords::ToMetres(at);
			const float dx = fromMetres.x - atMetres.x;
			const float dz = fromMetres.y - atMetres.y;
			if (!(dx * dx + dz * dz < radiusSquared))
			{
				continue;
			}
			targets.push_back(candidate.entity);
		}
	}
	return targets;
}

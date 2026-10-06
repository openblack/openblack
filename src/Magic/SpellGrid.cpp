/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SpellGrid.h"

using namespace openblack::magic;

std::optional<glm::ivec2> SpellGrid::CellOf(glm::vec2 xz)
{
	if (xz.x < 0.0f || xz.y < 0.0f)
	{
		return std::nullopt;
	}
	const glm::ivec2 cell(static_cast<int>(xz.x / k_CellSize), static_cast<int>(xz.y / k_CellSize));
	if (cell.x >= k_Side || cell.y >= k_Side)
	{
		return std::nullopt;
	}
	return cell;
}

void SpellGrid::Mark(glm::vec2 xz, uint8_t value)
{
	if (const auto cell = CellOf(xz))
	{
		_cells.at(static_cast<size_t>(cell->x)).at(static_cast<size_t>(cell->y)) = value;
	}
}

void SpellGrid::Fade()
{
	for (auto& column : _cells)
	{
		for (auto& cell : column)
		{
			cell = cell <= k_FadePerTurn ? 0 : static_cast<uint8_t>(cell - k_FadePerTurn);
		}
	}
}

uint8_t SpellGrid::At(glm::vec2 xz) const
{
	const auto cell = CellOf(xz);
	return cell ? _cells.at(static_cast<size_t>(cell->x)).at(static_cast<size_t>(cell->y)) : 0;
}

void SpellGrid::Clear()
{
	for (auto& column : _cells)
	{
		column.fill(0);
	}
}

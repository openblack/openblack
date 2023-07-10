/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Map.h"

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <glm/vec3.hpp>

#include "ECS/Components/Fixed.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack::ecs;
using namespace openblack::ecs::components;

MapInterface::CellId MapInterface::GetGridCell(const glm::vec2& pos)
{
	assert(glm::compMin(pos) >= 0);
	const glm::u32vec2 coords = pos * k_PositionToGridFactor;
	const MapInterface::CellId result(coords.x >> 0x10, coords.y >> 0x10);
	assert(glm::all(glm::lessThan(result, k_GridSize))); // If not, clamp to 0, _gridSize
	return result;
}

MapInterface::CellId MapInterface::GetGridCell(const glm::vec3& pos)
{
	return GetGridCell(glm::xz(pos));
}

glm::vec2 MapInterface::GetCellCenter(const MapInterface::CellId& cellId)
{
	return glm::vec2(cellId.x << 0x10, cellId.y << 0x10) / k_PositionToGridFactor + 5.0f;
}

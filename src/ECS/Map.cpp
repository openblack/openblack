/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

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

Map::CellId Map::GetGridCell(const glm::vec2& pos)
{
	const glm::u32vec2 coords = pos * _positionToGridFactor;
	const Map::CellId result(coords.x >> 0x10, coords.y >> 0x10);
	assert(glm::all(glm::lessThan(result, _gridSize))); // If not, clamp to 0, _gridSize
	return result;
}

Map::CellId Map::GetGridCell(const glm::vec3& pos)
{
	return GetGridCell(glm::xz(pos));
}

glm::vec2 Map::GetCellCenter(const Map::CellId& cellId)
{
	return glm::vec2(cellId.x << 0x10, cellId.y << 0x10) / _positionToGridFactor + 5.0f;
}

const std::unordered_set<entt::entity>& Map::GetFixedInGridCell(const CellId& cellId) const
{
	return _fixedGrid[cellId.x + cellId.y * _gridSize.x];
}

const std::unordered_set<entt::entity>& Map::GetFixedInGridCell(const glm::vec3& pos) const
{
	const auto cellId = GetGridCell(pos);
	return GetFixedInGridCell(cellId);
}

const std::unordered_set<entt::entity>& Map::GetMobileInGridCell(const CellId& cellId) const
{
	return _mobileGrid[cellId.x + cellId.y * _gridSize.x];
}

const std::unordered_set<entt::entity>& Map::GetMobileInGridCell(const glm::vec3& pos) const
{
	const auto cellId = GetGridCell(pos);
	return GetMobileInGridCell(cellId);
}

void Map::Rebuild()
{
	Clear();
	Build();
}

void Map::Clear()
{
	for (auto& g : _fixedGrid)
	{
		g.clear();
	}
	for (auto& g : _mobileGrid)
	{
		g.clear();
	}
}

void Map::Build()
{
	auto& registry = Game::instance()->GetEntityRegistry();
	registry.Each<const Fixed, const Transform>(
	    [this](entt::entity entity, [[maybe_unused]] const Fixed& fixed, const Transform& transform) {
		    const auto cellId = GetGridCell(transform.position);
		    auto& cell = _fixedGrid[cellId.x + cellId.y * _gridSize.x];
		    cell.insert(entity);
	    });
	registry.Each<const Mobile, const Transform>(
	    [this](entt::entity entity, [[maybe_unused]] const Mobile& mobile, const Transform& transform) {
		    const auto cellId = GetGridCell(transform.position);
		    auto& cell = _mobileGrid[cellId.x + cellId.y * _gridSize.x];
		    cell.insert(entity);
	    });
}

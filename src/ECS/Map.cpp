/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Map.h"

#include <glm/gtx/vec_swizzle.hpp>
#include <glm/vec3.hpp>

#include "ECS/Components/Fixed.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack::ecs;
using namespace openblack::ecs::components;

glm::u16vec2 Map::GetGridCell(const glm::vec3& pos)
{
	const glm::u32vec2 coords = glm::xz(pos) * _positionToGridFactor;
	const glm::u16vec2 result(coords.x >> 0x10, coords.y >> 0x10);
	assert(glm::all(glm::lessThan(result, _gridSize)));
	return result;
}

const std::unordered_set<entt::entity>& Map::GetFixedInGridCell(const glm::vec3& pos) const
{
	const auto cellId = GetGridCell(pos);
	return _fixedGrid[cellId.x + cellId.y * _gridSize.x];
}

const std::unordered_set<entt::entity>& Map::GetMobileInGridCell(const glm::vec3& pos) const
{
	const auto cellId = GetGridCell(pos);
	return _mobileGrid[cellId.x + cellId.y * _gridSize.x];
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

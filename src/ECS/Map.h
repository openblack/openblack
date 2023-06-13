/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <unordered_set>

#include <entt/fwd.hpp>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>

namespace openblack::ecs
{

class Map
{
public:
	using CellId = glm::u16vec2;

	static constexpr float k_PositionToGridFactor = static_cast<float>(0x10000) * 0.1f;
	static constexpr glm::u16vec2 k_GridSize = {0x200, 0x200};

	static CellId GetGridCell(const glm::vec2& pos);
	static CellId GetGridCell(const glm::vec3& pos);
	static glm::vec2 GetCellCenter(const CellId& cellId);

	[[nodiscard]] const std::unordered_set<entt::entity>& GetFixedInGridCell(const CellId& cellId) const;
	[[nodiscard]] const std::unordered_set<entt::entity>& GetFixedInGridCell(const glm::vec3& pos) const;
	[[nodiscard]] const std::unordered_set<entt::entity>& GetMobileInGridCell(const CellId& cellId) const;
	[[nodiscard]] const std::unordered_set<entt::entity>& GetMobileInGridCell(const glm::vec3& pos) const;

	void Rebuild();

private:
	void Clear();
	void Build();

	std::array<std::unordered_set<entt::entity>, k_GridSize.x * k_GridSize.y> _fixedGrid;
	std::array<std::unordered_set<entt::entity>, k_GridSize.x * k_GridSize.y> _mobileGrid;
};

} // namespace openblack::ecs

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

#include <array>
#include <type_traits>
#include <unordered_set>

#include <entt/fwd.hpp>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>

namespace openblack::ecs
{

class MapInterface
{
public:
	using CellId = glm::u16vec2;

	static constexpr float k_PositionToGridFactor = static_cast<float>(0x10000) * 0.1f;
	static constexpr glm::u16vec2 k_GridSize = {0x200, 0x200};

	static CellId GetGridCell(const glm::vec2& pos);
	static CellId GetGridCell(const glm::vec3& pos);
	static glm::vec3 PointGridCellAdd(const glm::vec3& pos, const glm::i16vec2& cellIncrement);
	static glm::vec2 GetCellCenter(const CellId& cellId);

	[[nodiscard]] virtual const std::unordered_set<entt::entity>& GetFixedInGridCell(const CellId& cellId) const = 0;
	[[nodiscard]] virtual const std::unordered_set<entt::entity>& GetFixedInGridCell(const glm::vec3& pos) const = 0;
	[[nodiscard]] virtual const std::unordered_set<entt::entity>& GetMobileInGridCell(const CellId& cellId) const = 0;
	[[nodiscard]] virtual const std::unordered_set<entt::entity>& GetMobileInGridCell(const glm::vec3& pos) const = 0;

	virtual void Rebuild() = 0;

private:
	virtual void Clear() = 0;
	virtual void Build() = 0;
};

} // namespace openblack::ecs

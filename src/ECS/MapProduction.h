/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Map.h"

namespace openblack::ecs
{

class MapProduction: public MapInterface
{
	[[nodiscard]] const std::unordered_set<entt::entity>& GetFixedInGridCell(const CellId& cellId) const override;
	[[nodiscard]] const std::unordered_set<entt::entity>& GetFixedInGridCell(const glm::vec3& pos) const override;
	[[nodiscard]] const std::unordered_set<entt::entity>& GetMobileInGridCell(const CellId& cellId) const override;
	[[nodiscard]] const std::unordered_set<entt::entity>& GetMobileInGridCell(const glm::vec3& pos) const override;

	void Rebuild() override;

private:
	void Clear() override;
	void Build() override;

	std::array<std::unordered_set<entt::entity>, k_GridSize.x * k_GridSize.y> _fixedGrid;
	std::array<std::unordered_set<entt::entity>, k_GridSize.x * k_GridSize.y> _mobileGrid;
};

} // namespace openblack::ecs

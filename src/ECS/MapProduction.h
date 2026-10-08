/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

#include <cstdint>

#include <optional>
#include <vector>

#include <entt/entity/registry.hpp>
#include <entt/signal/sigh.hpp>

#include "Map.h"
#include "MapCells.h"

namespace openblack::ecs
{
class Registry;

/// The map's cells, filled as things are made and emptied as they go, through the registry's signals: nothing is
/// rebuilt from the whole registry. Things made since the last look are filed, in the order they were made, before any
/// search of a cell, and once a turn whatever can move is checked for a new cell.
class MapProduction final: public MapInterface
{
public:
	MapProduction();
	~MapProduction() override;
	MapProduction(const MapProduction&) = delete;
	MapProduction& operator=(const MapProduction&) = delete;
	MapProduction(MapProduction&&) = delete;
	MapProduction& operator=(MapProduction&&) = delete;

	[[nodiscard]] std::span<const entt::entity> GetFixedInGridCell(const CellId& cellId) const override;
	[[nodiscard]] std::span<const entt::entity> GetFixedInGridCell(const glm::vec3& pos) const override;
	[[nodiscard]] std::span<const entt::entity> GetMobileInGridCell(const CellId& cellId) const override;
	[[nodiscard]] std::span<const entt::entity> GetMobileInGridCell(const glm::vec3& pos) const override;
	[[nodiscard]] std::vector<entt::entity> GetAllInCell(glm::ivec2 cell) const override;

	void Sync() override;
	void Refile(entt::entity entity) override;

private:
	/// How a thing goes into the cells, by what it is; none for what isn't on the map
	struct Kind
	{
		map_cells::Placement placement;
		bool coversOutline;
		bool moves;
	};
	[[nodiscard]] static std::optional<Kind> KindOf(const Registry& registry, entt::entity entity);

	void OnMade(entt::registry& registry, entt::entity entity);
	void OnGone(entt::registry& registry, entt::entity entity);
	void OnResidentGone(entt::registry& registry, entt::entity entity);

	/// Files the things made since the last look, in the order they were made
	void FileMade() const;
	void File(entt::entity entity, const Kind& kind) const;
	void TakeOut(entt::entity entity) const;
	[[nodiscard]] std::vector<uint32_t> CellsFor(entt::entity entity, const Kind& kind) const;

	Registry* _registry {nullptr};
	std::vector<entt::connection> _connections;
	/// The cells are filled lazily by the searches, which are const
	mutable map_cells::CellLists _lists;
	mutable std::vector<entt::entity> _made;
};

} // namespace openblack::ecs

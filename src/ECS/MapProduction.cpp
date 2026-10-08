/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS
#include "MapProduction.h"

#include <algorithm>

#include <glm/geometric.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <glm/vec3.hpp>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "3D/MapCoords.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Flowers.h"
#include "ECS/Components/Forest.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/MapCellResident.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/TeleportStone.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using openblack::ecs::map_cells::Placement;

namespace
{
/// Calls a function with each kind of component that puts a thing on the map
template <typename Func>
void ForEachMapComponent(Func&& func)
{
	// Buildings and features, which cover their outline
	func.template operator()<Abode>();
	func.template operator()<Feature>();
	func.template operator()<Flowers>();
	func.template operator()<BigForest>();
	func.template operator()<MobileStatic>();
	func.template operator()<SpellDispenser>();
	func.template operator()<TeleportStone>();
	// Trees and shields, each in one cell
	func.template operator()<Tree>();
	func.template operator()<DeadTree>();
	func.template operator()<ShieldDome>();
	// Things that count as staying put but can be carried
	func.template operator()<Pot>();
	func.template operator()<OneOffSpellSeed>();
	// The living and the other things that move
	func.template operator()<Villager>();
	func.template operator()<Creature>();
	func.template operator()<Animal>();
	func.template operator()<MobileObject>();
}

/// A building's outline on the ground, from its model's box as it is placed
std::optional<map_cells::Outline> OutlineOf(const Transform& transform, entt::id_type meshId)
{
	if (!Locator::resources::has_value())
	{
		return std::nullopt;
	}
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(meshId))
	{
		return std::nullopt;
	}
	const auto box = meshes.Handle(meshId)->GetBoundingBox();
	const float scale = transform.scale.x;
	const glm::vec3 half = box.Size() * 0.5f;
	// The game keeps positions as map positions
	const glm::vec3 position {openblack::map_coords::Quantise(transform.position.x), transform.position.y,
	                          openblack::map_coords::Quantise(transform.position.z)};
	const glm::vec3 centre = position + transform.rotation * (box.Center() * scale);
	glm::vec2 axis = glm::xz(transform.rotation * glm::vec3(1.0f, 0.0f, 0.0f));
	axis = glm::length(axis) > 0.0f ? glm::normalize(axis) : glm::vec2(1.0f, 0.0f);
	return map_cells::Outline {.centre = glm::xz(centre),
	                           .halfSize = glm::vec2(half.x * scale, half.z * scale),
	                           .halfDiagonal = scale * glm::length(half),
	                           .axis = axis};
}
} // namespace

MapProduction::MapProduction()
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	_registry = &Locator::entitiesRegistry::value();
	ForEachMapComponent([this]<typename Component>() {
		_connections.push_back(_registry->OnConstruct<Component>().template connect<&MapProduction::OnMade>(*this));
		_connections.push_back(_registry->OnDestroy<Component>().template connect<&MapProduction::OnGone>(*this));
	});
	_connections.push_back(_registry->OnDestroy<Transform>().connect<&MapProduction::OnGone>(*this));
	_connections.push_back(_registry->OnDestroy<MapCellResident>().connect<&MapProduction::OnResidentGone>(*this));
	// Anything already made is filed too
	ForEachMapComponent([this]<typename Component>() {
		_registry->Each<const Component>([this](entt::entity entity, const Component& /*unused*/) {
			if (std::ranges::find(_made, entity) == _made.end())
			{
				_made.push_back(entity);
			}
		});
	});
}

MapProduction::~MapProduction()
{
	// The registry may have gone first, taking its signals with it
	if (Locator::entitiesRegistry::has_value() && &Locator::entitiesRegistry::value() == _registry)
	{
		for (auto& connection : _connections)
		{
			connection.release();
		}
	}
}

std::optional<MapProduction::Kind> MapProduction::KindOf(const Registry& registry, entt::entity entity)
{
	if (registry.AnyOf<Abode, Feature, Flowers, BigForest, SpellDispenser, TeleportStone>(entity))
	{
		return Kind {.placement = Placement::FixedFront, .coversOutline = true, .moves = false};
	}
	if (registry.AnyOf<MobileStatic>(entity))
	{
		return Kind {.placement = Placement::FixedFront, .coversOutline = true, .moves = true};
	}
	if (registry.AnyOf<Tree, DeadTree, ShieldDome>(entity))
	{
		return Kind {.placement = Placement::FixedFront, .coversOutline = false, .moves = false};
	}
	if (registry.AnyOf<Pot, OneOffSpellSeed>(entity))
	{
		return Kind {.placement = Placement::FixedBack, .coversOutline = false, .moves = true};
	}
	if (registry.AnyOf<Villager, Creature, Animal, MobileObject>(entity))
	{
		return Kind {.placement = Placement::MobileFront, .coversOutline = false, .moves = true};
	}
	return std::nullopt;
}

void MapProduction::OnMade(entt::registry& /*registry*/, entt::entity entity)
{
	if (std::ranges::find(_made, entity) == _made.end())
	{
		_made.push_back(entity);
	}
}

void MapProduction::OnGone(entt::registry& /*registry*/, entt::entity entity)
{
	std::erase(_made, entity);
	TakeOut(entity);
}

void MapProduction::OnResidentGone(entt::registry& /*registry*/, entt::entity entity)
{
	TakeOut(entity);
}

void MapProduction::FileMade() const
{
	if (_made.empty() || _registry == nullptr)
	{
		return;
	}
	auto made = std::move(_made);
	_made.clear();
	for (const auto entity : made)
	{
		// What is held by a hand or moving in the physics stays out of the map's cells until it is put back
		if (!_registry->Valid(entity) || _registry->AllOf<MapCellResident>(entity) ||
		    _registry->AnyOf<InHand, InPhysics>(entity))
		{
			continue;
		}
		if (!_registry->AllOf<Transform>(entity))
		{
			// Not placed yet: filed once it is
			_made.push_back(entity);
			continue;
		}
		if (const auto kind = KindOf(*_registry, entity))
		{
			File(entity, *kind);
		}
	}
}

std::vector<uint32_t> MapProduction::CellsFor(entt::entity entity, const Kind& kind) const
{
	const auto& transform = _registry->Get<const Transform>(entity);
	std::vector<uint32_t> cells;
	if (kind.coversOutline)
	{
		const auto* mesh = _registry->TryGet<const Mesh>(entity);
		const auto outline = mesh != nullptr ? OutlineOf(transform, mesh->id) : std::nullopt;
		if (!outline.has_value())
		{
			SPDLOG_LOGGER_WARN(spdlog::get("game"), "Map: a building without a model can't be put in the map's cells");
			return cells;
		}
		for (const auto cell : map_cells::CellsCovered(*outline, _lists.CellsPerSide()))
		{
			if (const auto index = _lists.IndexOf(cell))
			{
				cells.push_back(*index);
			}
		}
		return cells;
	}
	if (const auto index = _lists.IndexOf(map_coords::CellOf(transform.position)))
	{
		cells.push_back(*index);
	}
	return cells;
}

void MapProduction::File(entt::entity entity, const Kind& kind) const
{
	auto cells = CellsFor(entity, kind);
	for (const auto cell : cells)
	{
		_lists.Insert(cell, entity, kind.placement);
	}
	const auto at = _registry->Get<const Transform>(entity).position;
	_registry->AssignOrReplace<MapCellResident>(entity, MapCellResident {.placement = kind.placement,
	                                                                     .coversOutline = kind.coversOutline,
	                                                                     .moves = kind.moves,
	                                                                     .cells = std::move(cells),
	                                                                     .filedAt = at});
	if (kind.moves && !_registry->AllOf<MapCellMover>(entity))
	{
		_registry->Assign<MapCellMover>(entity);
	}
}

void MapProduction::TakeOut(entt::entity entity) const
{
	if (_registry == nullptr)
	{
		return;
	}
	auto* resident = _registry->TryGet<MapCellResident>(entity);
	if (resident == nullptr)
	{
		return;
	}
	for (const auto cell : resident->cells)
	{
		_lists.Remove(cell, entity);
	}
	resident->cells.clear();
}

std::span<const entt::entity> MapProduction::GetFixedInGridCell(const CellId& cellId) const
{
	FileMade();
	const auto index = _lists.IndexOf({cellId.x, cellId.y});
	return index.has_value() ? _lists.Fixed(*index) : std::span<const entt::entity> {};
}

std::span<const entt::entity> MapProduction::GetFixedInGridCell(const glm::vec3& pos) const
{
	return GetFixedInGridCell(GetGridCell(pos));
}

std::span<const entt::entity> MapProduction::GetMobileInGridCell(const CellId& cellId) const
{
	FileMade();
	const auto index = _lists.IndexOf({cellId.x, cellId.y});
	return index.has_value() ? _lists.Mobile(*index) : std::span<const entt::entity> {};
}

std::span<const entt::entity> MapProduction::GetMobileInGridCell(const glm::vec3& pos) const
{
	return GetMobileInGridCell(GetGridCell(pos));
}

std::vector<entt::entity> MapProduction::GetAllInCell(glm::ivec2 cell) const
{
	FileMade();
	const auto index = _lists.IndexOf(cell);
	return index.has_value() ? _lists.All(*index) : std::vector<entt::entity> {};
}

void MapProduction::Sync()
{
	if (_registry == nullptr)
	{
		return;
	}
	FileMade();
	// What can move goes into its new cell: a building as soon as it is anywhere else, anything else once it is in
	// another cell
	std::vector<entt::entity> moved;
	_registry->Each<const MapCellMover, const MapCellResident, const Transform>(
	    [&moved](entt::entity entity, const MapCellResident& resident, const Transform& transform) {
		    const bool changed = resident.coversOutline
		                             ? transform.position != resident.filedAt
		                             : map_coords::CellOf(transform.position) != map_coords::CellOf(resident.filedAt);
		    if (changed)
		    {
			    moved.push_back(entity);
		    }
	    });
	for (const auto entity : moved)
	{
		Refile(entity);
	}
}

void MapProduction::Refile(entt::entity entity)
{
	if (_registry == nullptr || !_registry->Valid(entity) || !_registry->AllOf<Transform>(entity))
	{
		return;
	}
	const auto kind = KindOf(*_registry, entity);
	if (!kind.has_value())
	{
		return;
	}
	TakeOut(entity);
	File(entity, *kind);
}

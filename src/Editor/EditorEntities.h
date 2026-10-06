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
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "3D/AxisAlignedBoundingBox.h"
#include "EditorOutline.h"
#include "EditorPalette.h"
#include "Enums.h"

namespace openblack::ecs
{
class Registry;
}

/// What the editor knows of the things on the land, through the registry and the game's tables: their kinds, names and
/// bounds, and placing, copying, moving and removing them through the archetypes the game makes them with.
namespace openblack::editor
{

/// The readable names of the game's tables of things, read once the tables are loaded
struct NameTables
{
	std::vector<std::string> creatures;
	std::vector<std::string> villagers;
	std::vector<std::string> buildings;
	std::vector<std::string> trees;
	std::vector<std::string> features;
	std::vector<std::string> mobileObjects;
	std::vector<std::string> mobileStatics;
	/// By magic type, for the dispensers and bubbles
	std::vector<std::string> miracles;

	/// The tables' names, or none before the game's tables are loaded
	[[nodiscard]] static std::optional<NameTables> Load();
	[[nodiscard]] const std::vector<std::string>& Of(PlaceKind kind) const;
	[[nodiscard]] std::string_view NameOf(PlaceKind kind, int32_t type) const;
};

[[nodiscard]] std::string_view SpeciesName(CreatureType species);
[[nodiscard]] EntityKind KindOf(const ecs::Registry& registry, entt::entity entity);
[[nodiscard]] std::string LabelOf(const ecs::Registry& registry, const NameTables& names, entt::entity entity);
/// What a thing on the land is as an item of the palette, so it can be copied, if the palette makes its kind
[[nodiscard]] std::optional<PlaceItem> ItemOf(const ecs::Registry& registry, entt::entity entity);

/// A mesh's box, by its id in the meshes' cache, once it is loaded
[[nodiscard]] std::optional<AxisAlignedBoundingBox> MeshBox(uint32_t meshId);
/// The mesh an item of the palette is drawn with, by its id in the meshes' cache
[[nodiscard]] std::optional<uint32_t> MeshOf(const PlaceItem& item);
/// The box round a thing on the land, from its mesh, its place, turn and scale
[[nodiscard]] std::optional<AxisAlignedBoundingBox> WorldBoundsOf(const ecs::Registry& registry, entt::entity entity);
/// About how tall a thing stands, for framing it with the camera
[[nodiscard]] float HeightOf(const ecs::Registry& registry, entt::entity entity);

/// Puts an item of the palette on the land, through its archetype, facing an angle about the up axis in radians; a
/// building goes to the nearest town, a new one of the building's tribe if there is none. Creatures start as their
/// species does.
entt::entity Place(const PlaceItem& item, glm::vec3 position, float yawRadians);
/// A copy of a thing beside it, made the way the original was; none if the palette doesn't make its kind
std::optional<entt::entity> Duplicate(entt::entity entity, glm::vec2 offset);
/// Removes a thing and what it alone owns, such as a store's piles
void Remove(entt::entity entity);
/// Moves a thing to a point, stopping it first if it is a creature that walks
void MoveTo(entt::entity entity, glm::vec3 position);
/// Turns a thing about the up axis by an angle in radians
void Turn(entt::entity entity, float radians);

/// The land's height under a point, or 0 with no land
[[nodiscard]] float LandHeight(glm::vec2 point);

} // namespace openblack::editor

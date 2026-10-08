/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

namespace openblack::ecs
{
class Registry;
}

// How big the things of the world are, as creatures measure them to walk up to them and cast at them (see
// creature_cast_moves): a thing's radius on the ground and height from its model and scale, a field's radius always 5, a
// creature's from its size and how far its bones reach.
namespace openblack::ecs::systems::object_measures
{

/// A thing's radius on the ground, 0 for one without a model
[[nodiscard]] float TwoDRadius(const Registry& registry, entt::entity entity);
/// A thing's height, 0 for one without a model
[[nodiscard]] float Height(const Registry& registry, entt::entity entity);
/// What a creature walking up to the thing keeps clear of
[[nodiscard]] float RoutePlanRadius(const Registry& registry, entt::entity entity, entt::entity creature);
/// A building or anything else of several map cells fixed to the land, or a tree: a creature walking up to one stops
/// where its walk takes it
[[nodiscard]] bool WalkedUpToItself(const Registry& registry, entt::entity entity);
/// Where a thing stands, none once it has gone
[[nodiscard]] std::optional<glm::vec3> PositionOf(const Registry& registry, entt::entity entity);

} // namespace openblack::ecs::systems::object_measures

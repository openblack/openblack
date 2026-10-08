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

#include <optional>

#include <entt/fwd.hpp>
#include <glm/vec3.hpp>

/// Marks left on a creature's skin where a line from a blow or a burn meets its body as it is posed now
namespace openblack::ecs::creature_scars
{

/// Where the creature's groin bone is, as its body is posed now; none for a creature without a posed rig
[[nodiscard]] std::optional<glm::vec3> GroinOf(entt::entity creature);

/// A wound of a kind and column where the line from a point to the groin first meets the creature's skin; nothing
/// where it misses
void MarkAlong(entt::entity creature, glm::vec3 from, glm::vec3 groin, uint8_t kind, uint8_t column);

/// A creature that catches fire is burnt on its skin in three tries, each from a random point about its groin
void BurnOnCatching(entt::entity creature);

} // namespace openblack::ecs::creature_scars

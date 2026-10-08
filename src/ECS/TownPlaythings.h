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

#include <functional>
#include <optional>
#include <span>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

/// The things a town's people play with, and the town a new one goes to
namespace openblack::ecs::town_playthings
{

/// A town takes a thing to play with unless one of the same kind it has is still about; the newest comes first. Whether
/// it took it.
bool Add(std::vector<entt::entity>& playthings, entt::entity thing,
         const std::function<bool(entt::entity)>& stillAboutOfSameKind);

/// A town a new plaything might go to
struct Candidate
{
	entt::entity town;
	PlayerNames owner;
	uint32_t id;
	glm::vec3 position;
};
/// The town nearest a point, in the game's whole metres, however far: the players' towns are looked at in turn and the
/// neutral ones last, each player's by id, and a later town is taken only when strictly nearer
[[nodiscard]] std::optional<entt::entity> Nearest(std::span<const Candidate> candidates, glm::vec3 point);

} // namespace openblack::ecs::town_playthings

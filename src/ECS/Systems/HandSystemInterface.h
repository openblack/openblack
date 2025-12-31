/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <optional>

#include <entt/fwd.hpp>
#include <glm/vec3.hpp>

#include "ECS/Components/PlayerHand.h"

namespace openblack::ecs::systems
{
class HandSystemInterface
{
public:
	virtual bool Initialize(entt::entity playerEntity) noexcept = 0;
	[[nodiscard]] virtual std::array<entt::entity, static_cast<size_t>(components::PlayerHand::Side::_Count)>
	GetPlayerHands(entt::entity playerEntity) const noexcept = 0;
	[[nodiscard]] virtual std::array<std::optional<glm::vec3>, static_cast<size_t>(components::PlayerHand::Side::_Count)>
	GetPlayerHandPositions(entt::entity playerEntity) const noexcept = 0;
};
} // namespace openblack::ecs::systems

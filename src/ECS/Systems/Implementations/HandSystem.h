/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/HandSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::ecs::systems
{

class HandSystem final: public HandSystemInterface
{
public:
	bool Initialize(entt::entity playerEntity) noexcept override;
	[[nodiscard]] std::array<entt::entity, static_cast<size_t>(components::PlayerHand::Side::_Count)>
	GetPlayerHands(entt::entity playerEntity) const noexcept override;
	[[nodiscard]] std::array<std::optional<glm::vec3>, static_cast<size_t>(components::PlayerHand::Side::_Count)>
	GetPlayerHandPositions(entt::entity playerEntity) const noexcept override;
};
} // namespace openblack::ecs::systems

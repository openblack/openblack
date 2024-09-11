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
#pragma message("Locator interface implementations should only be included in Locator.cpp, use interface instead.")
#endif

namespace openblack::ecs::systems
{

class HandSystem final: public HandSystemInterface
{
public:
	bool Initialize() noexcept override;
	[[nodiscard]] std::array<entt::entity, static_cast<size_t>(Side::_Count)> GetPlayerHands() const noexcept override;
	[[nodiscard]] std::array<std::optional<glm::vec3>, static_cast<size_t>(Side::_Count)>
	GetPlayerHandPositions() const noexcept override;

private:
	std::array<entt::entity, 2> _hands;
};
} // namespace openblack::ecs::systems

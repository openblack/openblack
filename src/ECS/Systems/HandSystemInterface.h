/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
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

#include "3D/MapCoords.h"

namespace openblack::ecs::systems
{
class HandSystemInterface
{
public:
	enum class Side : uint8_t
	{
		Left,
		Right,
		_Count
	};
	virtual bool Initialize() noexcept = 0;
	[[nodiscard]] virtual std::array<entt::entity, static_cast<size_t>(Side::_Count)> GetPlayerHands() const noexcept = 0;
	[[nodiscard]] virtual std::array<std::optional<glm::vec3>, static_cast<size_t>(Side::_Count)>
	GetPlayerHandPositions() const noexcept = 0;
	/// Each frame the hands catch up with their player's alignment once it has moved far enough, and blend their skin
	/// again as they go into or out of the player's influence, tested at the point picked under the cursor (see
	/// hand_morph::Pick and NextPoint)
	virtual void UpdateAlignmentMorph(std::optional<map_coords::MapCoords> picked, bool holdPoint) = 0;
};
} // namespace openblack::ecs::systems

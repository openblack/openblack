/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/fwd.hpp>
#include <glm/mat4x4.hpp>

#include "Enums.h"

namespace openblack::ecs::components
{

struct PlayerHand
{
	static constexpr auto in_place_delete = true;

	enum class Side : uint8_t
	{
		Left,
		Right,
		_Count
	};

	enum class ActiveHand : uint8_t
	{
		None = (1u << 0),
		Left = (1u << 1),
		Right = (1u << 2),
		Both = Left | Right, // VR
	};

	std::array<entt::entity, static_cast<size_t>(Side::_Count)> hands;
	std::array<glm::mat4, static_cast<size_t>(Side::_Count)> handPoses;
	ActiveHand primaryHand {ActiveHand::Right};
	uint32_t showHands {static_cast<uint32_t>(ActiveHand::Right)};
	uint32_t handGripping {static_cast<uint32_t>(ActiveHand::None)};
};
} // namespace openblack::ecs::components

/******************************************************************************
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

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "ECS/RewardRules.h"
#include "Enums.h"

namespace openblack::ecs::components
{

/// A reward chest a script gives: falling from the sky, then on the land where it can be knocked about
struct Reward
{
	enum class State : uint8_t
	{
		/// Coming down from the sky
		Falling,
		/// On the land
		Landed,
	};

	RewardObjectInfo type {RewardObjectInfo::None};
	std::optional<PlayerNames> player;
	/// The town it was given to, none for none
	entt::entity town {entt::null};
	State state {State::Landed};
	/// Its own clock since it was given, in seconds of the game's time
	float seconds {0.0f};
	/// Where it is to land: the point on the land under where it was given
	glm::vec3 landingPoint {0.0f};
	/// It has landed this frame, and goes into the map's cells at the next game turn
	bool goesIntoMap {false};
	/// How long its dust has left to show, once it has fallen
	float dustMilliseconds {0.0f};
	std::array<reward::DustSprite, reward::k_DustSprites> dust {};
};

/// A reward chest that has come to the land, and stands in the map's cells
struct RewardOnLand
{
};

} // namespace openblack::ecs::components

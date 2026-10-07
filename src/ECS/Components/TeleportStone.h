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

#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::components
{

/// The invisible stone a teleport miracle leaves on the land, with its swirling pool. The player's stones form a
/// network the player's villagers and creature jump between.
struct TeleportStone
{
	/// Someone who turned aside into the stone, and where they were going
	struct Traveller
	{
		entt::entity living {entt::null};
		glm::vec3 destination {0.0f};
		/// The state a villager was to take on arriving there
		VillagerStates finalState {VillagerStates::DecideWhatToDo};
	};

	PlayerNames player {PlayerNames::NEUTRAL};
	/// The miracle it belongs to; it goes with it
	entt::entity spell {entt::null};
	/// The reaction it spreads round itself, which the passers-by react to
	uint32_t reaction {0};
	/// Its pool's particle effect
	uint32_t pool {0};
	/// The order the stones were made in: the newest are first in their player's list
	uint32_t serial {0};
	/// The newest first
	std::vector<Traveller> travellers;
};

/// A villager or creature on its way into a teleport stone
struct TeleportTraveller
{
	entt::entity stone {entt::null};
	/// Where it was going, and the state a villager was to take there, to carry on after the jump
	glm::vec3 destination {0.0f};
	VillagerStates finalState {VillagerStates::DecideWhatToDo};
	/// The state a villager's reaction returns it to: the state it was to take, as its state table keeps it
	VillagerStates previousState {VillagerStates::InvalidState};
	/// A creature walking on to its own destination after the jump: it doesn't turn aside again until it gets there
	bool jumped {false};
	/// A creature the stone carries: where it comes out, and the turns left of its fading out, or once there of its
	/// fading back in
	glm::vec3 arrival {0.0f};
	int32_t transportTurns {0};
	bool transported {false};
	bool fadingIn {false};
};

} // namespace openblack::ecs::components

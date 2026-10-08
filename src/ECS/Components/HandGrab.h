/*******************************************************************************
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

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "Hand/HandGrabRules.h"

namespace openblack::ecs::components
{

/// A thing held in a hand: out of the map's cells and doing nothing of its own while it is
struct InHand
{
	/// The hand that holds it
	entt::entity hand {entt::null};
};

/// What a hand is doing with the things it picks up: taking hold of one, holding it, and letting it go
struct HandGrab
{
	enum class State : uint8_t
	{
		/// Holding nothing of its own
		Empty,
		/// The button is held on a thing, which the hand takes once it has waited or pulled it free
		Grabbing,
		/// Holding a thing, the button let go
		Holding,
		/// Holding a thing with the button held, the hand dragged after the cursor by its spring, ready to throw
		ReadyToThrow,
	};

	State state {State::Empty};
	/// What it is taking or holds
	entt::entity object {entt::null};
	/// When the press began, by the clock and by the game's turns
	uint32_t pressMs {0};
	uint32_t pressTurn {0};
	/// It waits before taking what can't be pulled and what is in flight
	bool waits {false};
	/// How long it has been pulling, for the fade into the pulling pose
	float pullSeconds {0.0f};

	/// How what it holds hangs
	hand_grab::HoldFacts hold {};
	/// How far below the hand it hung as it was taken
	float pickUpLowering {0.0f};
	/// How far the hand rises for it, and the point picked on the land with it
	float rise {0.0f};

	hand_grab::HandSpring spring {};
	/// Where the hand was last meant to be while holding, for the twist of a throw
	glm::vec3 lastTarget {0.0f};

	/// What it last threw, which gets a twist once the hand has moved on a little, and how long until it does
	entt::entity released {entt::null};
	std::optional<int32_t> releaseSpinMs;

	/// What it last picked up and last dropped, while they still exist
	entt::entity lastPickedUp {entt::null};
	entt::entity lastDropped {entt::null};
};

} // namespace openblack::ecs::components

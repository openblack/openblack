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

#include "Common/Zoomer.h"
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

/// A thing a script has said the hand may not pick up
struct CannotBePickedUp
{
};

/// What a hand is doing with the things it picks up: taking hold of one, holding it, and letting it go
struct HandGrab
{
	enum class State : uint8_t
	{
		/// Holding nothing of its own
		Empty,
		/// The button is held on a thing, which the hand takes once it has waited, or pulled it free
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
	/// It waits before taking what can't be pulled
	bool waits {false};
	/// How long it has been in its pulling pose, for the fade into it
	float pullSeconds {0.0f};
	/// It still pulls at the thing: the thing hasn't come free
	bool pulling {false};
	/// The pull on a rooted thing once the fade is over, leaning and stretching it
	std::optional<hand_grab::Tug> tug;
	/// How far it stretches towards the hand
	Zoomer stretch {1.0f};
	/// How far up the thing the hand grips it as it pulls
	float holdDistance {0.0f};
	/// The plane of the land under the thing the hand's point moves in as it pulls: a point of it and its normal
	glm::vec3 pullPlanePoint {0.0f};
	glm::vec3 pullPlaneNormal {0.0f, 1.0f, 0.0f};

	/// How what it holds hangs
	hand_grab::HoldFacts hold {};
	/// How far below the hand what it holds hangs (its lowering times its height), as last measured
	float lowering {0.0f};
	/// How far the hand rises for it, and the point picked on the land with it
	float rise {0.0f};

	/// The spring, and that it is to take hold of the hand the next frame the hand holds something ready to throw
	hand_grab::HandSpring spring {};
	bool springOn {false};
	bool springPending {false};
	/// Where the hand was last meant to be while holding, before it rose, for the twist of a throw
	glm::vec3 lastTarget {0.0f};

	/// What it scoops a handful from while the button is held, the game turns it has scooped for, the stream of what it
	/// scoops flowing into it, and where over the land the hand stays as it scoops
	entt::entity scoopSource {entt::null};
	uint32_t scoopTurns {0};
	std::optional<uint32_t> scoopStream;
	glm::vec3 scoopAnchor {0.0f};

	/// What it last let go, which gets a twist once the hand has moved on a little, and how long until it does
	entt::entity released {entt::null};
	std::optional<int32_t> releaseSpinMs;

	/// The pour of a handful let go slowly, and how long it has poured for: it ends after three quarters of a second
	std::optional<uint32_t> pourEffect;
	float pourSeconds {0.0f};
	/// How long the stream of a scoop has flowed for: it ends after a minute
	float scoopStreamSeconds {0.0f};

	/// The hand's size last frame, which sets how far up a pulled thing it grips, and the point it picked on the land
	/// then, none over nothing
	float handSize {1.0f};
	std::optional<glm::vec3> handPoint;

	/// What it last picked up and last dropped, while they still exist
	entt::entity lastPickedUp {entt::null};
	entt::entity lastDropped {entt::null};
};

} // namespace openblack::ecs::components

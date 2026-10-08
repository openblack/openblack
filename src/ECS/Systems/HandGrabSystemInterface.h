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
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::systems
{

/// The player's hand picking things up, carrying them and letting them go: putting them down where it is, or throwing
/// them with the speed its spring drags it at. Miracles in the hand and creatures held to it are the magic's and the
/// creature hand's.
class HandGrabSystemInterface
{
public:
	virtual ~HandGrabSystemInterface() = default;

	/// What the frame tells the hand
	struct Frame
	{
		/// Where the hand should be, before it rises for what it holds
		glm::vec3 target {0.0f};
		/// The line through the cursor
		glm::vec3 rayOrigin {0.0f};
		glm::vec3 rayDirection {0.0f};
		/// Where the camera is: the pull measures its plane from it
		glm::vec3 camera {0.0f};
		/// The point picked on the land under the cursor, none over nothing
		std::optional<glm::vec3> cursorGround;
		/// The hand's size against its standard
		float handSize {1.0f};
		/// The frame's time: the clock's for the hand's fades, the game's for the spring
		float seconds {0.0f};
		uint32_t gameMs {0};
		/// The clock now, and the game's turn
		uint32_t nowMs {0};
		uint32_t turn {0};
	};

	/// The Action button is pressed with the cursor along a line: whether the hand took the press, to take hold of a thing
	/// under it or to make ready to throw what it holds
	virtual bool Press(glm::vec3 rayOrigin, glm::vec3 rayDirection, uint32_t nowMs, uint32_t turn) = 0;
	/// The Action button is let go: what the hand was taking is let be, or what it holds is put down or thrown. The thing a
	/// press tapped, a press too short to take it.
	virtual std::optional<entt::entity> Release(uint32_t nowMs, uint32_t turn) = 0;
	/// Each frame: the hand takes what it waited for, its spring drags it, and the twist of a throw is given. The hand's
	/// position, risen for what it holds, or dragged by its spring.
	virtual glm::vec3 UpdateFrame(const Frame& frame) = 0;
	/// Each game turn: what it holds stays where the hand is for the game, and is dropped once it is gone
	virtual void ProcessTurn() = 0;
	/// The hand is made to let go of what it holds, which is put down where it is and never planted again
	virtual void ForceDrop() = 0;
	/// The world is cleared: the hand holds nothing
	virtual void Reset() = 0;

	/// What the hand holds, none when it holds nothing of its own
	[[nodiscard]] virtual std::optional<entt::entity> GetHeld() const = 0;
	/// Whether the hand is taking, holding or about to throw a thing, so that other uses of the button leave it be
	[[nodiscard]] virtual bool IsBusy() const = 0;
	/// How what it holds hangs: its hold, how far below the hand its model hangs, and how far it spreads out of the hand
	struct HeldPose
	{
		entt::entity object {entt::null};
		HoldType hold {HoldType::Above};
		float hang {0.0f};
		float reach {0.0f};
	};
	[[nodiscard]] virtual std::optional<HeldPose> GetHeldPose() const = 0;
	/// How far the point the hand picks on the land is raised for what it holds
	[[nodiscard]] virtual float GetCursorRaise() const = 0;
};

} // namespace openblack::ecs::systems

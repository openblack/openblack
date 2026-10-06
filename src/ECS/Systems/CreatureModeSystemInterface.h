/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <optional>

#include <entt/entity/fwd.hpp>

#include "Camera/CreatureFollow.h"
#include "Creature/CreatureMode.h"

namespace openblack::ecs::systems
{

/// Creature Mode: the camera locks onto a creature and follows it, and the creature's status panel shows how damaged,
/// hungry and tired it is (see creature_mode and creature_follow). C locks onto the player's own creature and lets go of
/// it again; a double click locks onto any creature, another god's too. While locked on, Shift and the cursor keys
/// turn the camera round the creature, and Ctrl and Shift together swing it clear of the land in the way. The cursor
/// keys alone, dragging the land, the temple, the editor or a script's cinema bars give the camera back.
///
/// It takes the camera over from the player's camera and hands it back as it was, waiting while the temple or the
/// editor has the camera.
class CreatureModeSystemInterface
{
public:
	/// What the game tells it each frame
	struct Frame
	{
		/// The hand is gripping the land to drag or turn the camera
		bool handGripping {false};
	};

	virtual ~CreatureModeSystemInterface() = default;

	/// Each frame, before the camera moves: reads C, keeps the camera on the creature, and ends the mode when it must
	virtual void Update(std::chrono::microseconds dt, const Frame& frame) = 0;
	/// A press of the left button over the land; true when it is the second of a double click on a creature, which
	/// then is locked onto
	virtual bool Press(const creature_mode::Press& press) = 0;

	/// As C does: locks onto the player's creature, or lets go of it when already on it
	virtual void PressCreatureKey() = 0;
	/// Locks onto a creature, any player's; false when the camera can't be taken now
	virtual bool Enter(entt::entity creature) = 0;
	/// Gives the camera back, as soon as it has it again
	virtual void Leave() = 0;
	[[nodiscard]] virtual bool IsActive() const = 0;
	/// The creature locked onto
	[[nodiscard]] virtual std::optional<entt::entity> GetCreature() const = 0;
	/// The player's own creature, which C locks onto
	[[nodiscard]] virtual std::optional<entt::entity> PlayersCreature() const = 0;
	/// The camera's view of the creature, while it has the camera
	[[nodiscard]] virtual std::optional<creature_follow::View> GetView() const = 0;

	/// As if keys were held for some seconds: the cursor keys' directions (-1, 0 or 1 across and along) with Shift or
	/// Ctrl, for the testbed's scenarios
	virtual void HoldKeys(int across, int along, bool shift, bool ctrl, float seconds) = 0;
	/// As if Ctrl and Shift were pressed together: swings the camera clear of the land in the way
	virtual void ClearView() = 0;
};

} // namespace openblack::ecs::systems

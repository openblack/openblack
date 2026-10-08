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

#include "Enums.h"

namespace openblack::ecs::systems
{

/// The one source of how good or evil the world looks. Each player has an alignment, from -1, evil, to 1, good
/// (components::Alignment). Every game turn the camera takes the alignment of the player whose influence it is in,
/// which lights the temple's insides and picks the ambience, and the sky turns to it a whole a second of game time.
/// Each temple's outside shows its own player's.
class AlignmentSystemInterface
{
public:
	virtual ~AlignmentSystemInterface() = default;

	/// None for a player who isn't in the game
	[[nodiscard]] virtual float GetPlayerAlignment(PlayerNames player) const = 0;
	/// The alignment, held between -1 and 1
	virtual void SetPlayerAlignment(PlayerNames player, float alignment) = 0;
	/// Added to the alignment, held between -1 and 1, as the script's SET_ALIGNMENT does
	virtual void AddPlayerAlignment(PlayerNames player, float change) = 0;
	/// A change the player's deeds made, such as a miracle hurting or healing, which moves the alignment over the turns to
	/// come no faster than the game's limit a turn
	virtual void AddPendingAlignment(PlayerNames player, float change) = 0;
	/// The change still to come
	[[nodiscard]] virtual float GetPendingAlignment(PlayerNames player) const = 0;

	/// At the end of a game turn the camera takes the alignment of the player of most influence where it is
	virtual void UpdateTurn() = 0;
	/// The sky turns toward the camera's alignment, by game time, none while paused
	virtual void Update(std::chrono::duration<float, std::milli> gameTime) = 0;

	/// The alignment the camera is in, as of the last turn, from -1, evil, to 1, good, which the temple's insides are lit by
	/// and the ambience follows
	[[nodiscard]] virtual float GetCameraAlignment() const = 0;
	/// The alignment the sky shows, from -1, evil, to 1, good
	[[nodiscard]] virtual float GetSkyAlignment() const = 0;
};

} // namespace openblack::ecs::systems

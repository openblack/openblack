/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <glm/vec2.hpp>

namespace openblack::input
{

/// Holds the game's cursor still while the camera is turned with the mouse.
///
/// Turning the camera around the hand (the middle button) or turning and zooming it with both buttons is driven by the
/// mouse's movement alone. While it lasts the game stops following the pointer: the cursor, and the hand drawn at it,
/// stay where they were when the turning started. When it ends the pointer is put back at that spot, so the hand
/// doesn't jump to wherever the pointer had wandered.
class CursorFreeze
{
public:
	struct Result
	{
		/// Where the game's cursor is this frame
		glm::ivec2 cursor;
		/// The turning started this frame: the pointer should be hidden and held
		bool started {false};
		/// The turning ended this frame: the pointer should be shown again and moved here
		std::optional<glm::ivec2> warpTo;
	};

	/// Once a frame, with whether the camera is being turned with the mouse and where the pointer is
	[[nodiscard]] Result Update(bool freeze, glm::ivec2 pointer);

	[[nodiscard]] bool IsFrozen() const { return _frozenAt.has_value(); }
	[[nodiscard]] std::optional<glm::ivec2> GetFrozenAt() const { return _frozenAt; }

private:
	std::optional<glm::ivec2> _frozenAt;
};

} // namespace openblack::input

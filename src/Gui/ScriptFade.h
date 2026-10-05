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

namespace openblack::gui
{

/// The scripts' fade of the picture to a colour and back, moving once a game turn. Its colour covers the picture between
/// the cinema bars.
class ScriptFade
{
public:
	/// Fades from clear to a colour over whole seconds, at once for none
	void FadeTo(uint8_t red, uint8_t green, uint8_t blue, int8_t seconds);
	/// Fades from the colour back to clear over whole seconds, at once for none
	void FadeBackToNormal(int8_t seconds);
	/// Moves the fade on by a game turn
	void ProcessTurn();
	/// Whether the fade has stopped moving
	[[nodiscard]] bool IsFinished() const { return _rate == 0.0f; }
	/// 0xAARRGGBB, nothing drawn for an alpha of 0
	[[nodiscard]] uint32_t GetColour() const { return _colour; }

private:
	/// Alpha a turn, 0 when still
	float _rate {0.0f};
	/// 0 to 255
	float _alpha {0.0f};
	uint32_t _colour {0};
};

} // namespace openblack::gui

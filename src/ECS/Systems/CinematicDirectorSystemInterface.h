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

#include <chrono>

namespace openblack::ecs::systems
{

/// What the scripts do to the picture for their cut scenes: fading it to a colour and back, and the cinema bars
class CinematicDirectorSystemInterface
{
public:
	virtual ~CinematicDirectorSystemInterface() = default;

	/// Fades the picture from clear to a colour over whole seconds, at once for none
	virtual void FadeTo(uint8_t red, uint8_t green, uint8_t blue, int8_t seconds) = 0;
	/// Fades the picture from the colour back to clear over whole seconds, at once for none
	virtual void FadeBackToNormal(int8_t seconds) = 0;
	[[nodiscard]] virtual bool IsFadeFinished() const = 0;
	/// The fade's colour over the picture, 0xAARRGGBB, nothing for an alpha of 0
	[[nodiscard]] virtual uint32_t GetFadeColour() const = 0;

	/// Slides the cinema bars in or out
	virtual void SetWideScreen(bool on) = 0;
	[[nodiscard]] virtual bool IsWideScreenTransitionFinished() const = 0;
	/// How far in the bars are, 0 for none and 1 for a 16:9 picture
	[[nodiscard]] virtual float GetWideScreenFraction() const = 0;

	/// Moves the fade on by a game turn
	virtual void ProcessTurn() = 0;
	/// Slides the bars on by the game time of the frame, which stops while the game is paused
	virtual void Update(std::chrono::duration<float, std::milli> gameTime) = 0;
};

} // namespace openblack::ecs::systems

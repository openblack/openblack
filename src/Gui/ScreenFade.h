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

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace openblack::gui
{

/// A colour over the whole screen that fades towards an amount at a whole of it a second, as the temple's fade does.
/// Fading up, it turns at the amount and fades back down; fading down to nothing, its colour goes back to black. Beyond
/// a whole amount the screen stays covered, so a fade from more than one waits that long before it shows anything.
class ScreenFade
{
public:
	/// Covers the screen by an amount and fades it away, in the colour it has
	void FadeFrom(float amount);
	/// Covers the screen by an amount of a colour and fades it away
	void FadeFrom(float amount, glm::vec3 colour);
	/// Fades a colour over the screen, then back away
	void FadeThrough(glm::vec3 colour);
	void Update(float seconds);

	/// The colour over the screen, its alpha how much of the screen it covers
	[[nodiscard]] glm::vec4 GetColour() const;
	/// How many times the fade has reached where it was going
	[[nodiscard]] uint32_t GetTurns() const noexcept { return _turns; }

private:
	float _amount {0.0f};
	float _target {0.0f};
	glm::vec3 _colour {0.0f};
	uint32_t _turns {0};
};

} // namespace openblack::gui

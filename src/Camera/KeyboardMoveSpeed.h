/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cmath>

#include <algorithm>

namespace openblack
{

/// How much faster than the game's own speed the movement keys move the camera over the land. At the default the
/// camera moves exactly as the game moves it; turning, tilting and zooming keep the game's speed whatever this is.
constexpr float k_KeyboardMoveSpeedDefault = 1.0f;
constexpr float k_KeyboardMoveSpeedMin = 0.25f;
constexpr float k_KeyboardMoveSpeedMax = 8.0f;
/// Each step of the speed up and down keys is a half of a doubling, so two steps double or halve the speed
constexpr float k_KeyboardMoveSpeedStepsPerDoubling = 2.0f;

/// Keeps a speed within the allowed range; anything that is not a number falls back to the default
[[nodiscard]] inline float ClampKeyboardMoveSpeed(float speed)
{
	if (std::isnan(speed))
	{
		return k_KeyboardMoveSpeedDefault;
	}
	return std::clamp(speed, k_KeyboardMoveSpeedMin, k_KeyboardMoveSpeedMax);
}

/// The speed a number of steps up (or down, for negative steps) from the given one. Steps land on whole halves of a
/// doubling, so stepping back always returns to exactly the default.
[[nodiscard]] inline float StepKeyboardMoveSpeed(float speed, int steps)
{
	const auto current = std::round(std::log2(ClampKeyboardMoveSpeed(speed)) * k_KeyboardMoveSpeedStepsPerDoubling);
	return ClampKeyboardMoveSpeed(std::exp2((current + static_cast<float>(steps)) / k_KeyboardMoveSpeedStepsPerDoubling));
}

/// How far the movement keys move the camera this frame, given how far the game's own speed moves it
[[nodiscard]] inline float ScaleKeyboardMove(float distance, float speed)
{
	return distance * ClampKeyboardMoveSpeed(speed);
}

} // namespace openblack

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

#include <algorithm>

/// When the player takes the camera back from a miracle's camera path. The path keeps the camera until a key moving it
/// left, right, forwards or backwards would move it this frame, or the hand grips the land. A movement key moves the
/// camera 400 units a second of the camera's frame time, and only a step of at least a whole unit counts: the camera's
/// frame time is the frame's whole milliseconds as seconds, at most a tenth of a second, so a key held through a frame
/// of 2 ms or less doesn't take the camera back. Rotating, tilting and zooming never do.
namespace openblack::camera_path
{

/// The camera's frame time is no longer than this, in seconds
inline constexpr float k_MaxFrameSeconds = 0.1f;
/// Seconds in a millisecond, as the game's single precision constant
inline constexpr float k_SecondsPerMillisecond = 0.001f;
/// How far a movement key moves the camera in a second of its frame time
inline constexpr float k_KeyStepPerSecond = 400.0f;

/// The camera's frame time for a frame of whole milliseconds
[[nodiscard]] constexpr float FrameSeconds(uint32_t frameMilliseconds)
{
	return std::min(static_cast<float>(frameMilliseconds) * k_SecondsPerMillisecond, k_MaxFrameSeconds);
}

/// Whether a movement key held this frame would move the camera at least a whole unit
[[nodiscard]] constexpr bool KeyStepMoves(uint32_t frameMilliseconds)
{
	return static_cast<int32_t>(FrameSeconds(frameMilliseconds) * k_KeyStepPerSecond) != 0;
}

/// Whether the player takes the camera back: a movement key that would move it, or the hand gripping the land
[[nodiscard]] constexpr bool TakesCameraBack(bool movementKey, uint32_t frameMilliseconds, bool grippingLand)
{
	return (movementKey && KeyStepMoves(frameMilliseconds)) || grippingLand;
}

} // namespace openblack::camera_path

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// Black & White's puffs of mist and cloud: their animation, a counter that runs to 900 picking one of sixteen frames of
/// the smoke texture of 8 by 8 frames, and their shape.
namespace openblack::mists
{

inline constexpr int k_CounterWrap = 900;
/// The counter moves this much each millisecond of game time
inline constexpr float k_Rate = 0.255f;

/// The counter's frame: a twentieth of it, in sixteen frames. The counter reaches 900 itself, so the frames run 0 to 15
/// twice, then 0 to 14, before starting again.
[[nodiscard]] constexpr int Frame(int counter) noexcept
{
	return (counter * 45 / k_CounterWrap) & 15;
}

/// Where a frame is in the texture: a row of eight frames to each eighth. The map's mists use its first two rows, and
/// the clouds and the mists that shrink edge on the next two.
[[nodiscard]] constexpr glm::vec2 FrameOffset(int frame, bool lowerRows) noexcept
{
	return {static_cast<float>(frame & 7) * 0.125f,
	        (static_cast<float>((frame >> 3) & 7) * 0.125f) + (lowerRows ? 0.25f : 0.0f)};
}

/// A new mist starts at a random counter of the first sixteen, from a random number between 0 and 16
[[nodiscard]] constexpr int StartCounter(float random0To16) noexcept
{
	return static_cast<int>(random0To16) & 15;
}

/// The size of a mist that shrinks edge on, seen along `toMist` from the camera: its full size seen from straight above
/// or below, `edgeShrink` times smaller seen level
[[nodiscard]] float EdgeOnSize(float size, float edgeShrink, const glm::vec3& toMist);

/// Moves the counter on by the milliseconds of game time. The game drops the fraction each frame; it is kept here, so
/// the animation runs at the same pace however fast the frames come.
void Advance(int& counter, float& remainder, float milliseconds) noexcept;

} // namespace openblack::mists

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

#include <vector>

#include "CreatureIdleMind.h"

// How a creature casts a miracle at something. One time in five it first shows how it feels: angry before a lightning
// bolt, kind before a helpful miracle, playful before one it casts on another creature. Then it goes near the thing,
// gets away from it to a distance, turns to face it and draws the miracle's gesture, if it has one. Last it takes up
// the casting pose and casts the miracle as the pose's loop begins, holds it three seconds and lets it go as the pose
// ends.
//
// A lightning bolt is cast from fifty away, backing off to ten more than the creature's height; a helpful miracle from
// twice its height; a creature spell from five times its height, backing off to twice it. Pure, tested on its own.

namespace openblack::creature_mind
{

/// The kinds of casting, by the miracles cast that way
enum class CastStyle : uint8_t
{
	/// The lightning bolts
	Lightning,
	/// The heals, and other miracles that help
	Helpful,
	/// The spells cast on another creature
	Playful,
};

/// It shows how it feels first one time in this many
inline constexpr uint32_t k_CastEmoteLots = 5;
/// The casting pose's start, loop and end
inline constexpr std::array<size_t, 3> k_CastPose {40, 41, 42};
/// The miracle is held this long once cast
inline constexpr float k_CastHoldSeconds = 3.0f;
/// Turning to face what it casts at, it holds still this long once facing it
inline constexpr float k_CastSettleSeconds = 0.1f;
/// A lightning bolt is cast from this far, backing off to this much more than its height
inline constexpr float k_LightningCastDistance = 50.0f;
inline constexpr float k_LightningBackOff = 10.0f;

/// Casting a miracle of a magic type, by its number, with a gesture to draw (0 for none), at an object by its entity's
/// number, by a creature of a height
[[nodiscard]] std::vector<Step> CastAt(CastStyle style, uint32_t magicType, uint32_t gesture, uint32_t object, float height,
                                       const Random& random);

} // namespace openblack::creature_mind

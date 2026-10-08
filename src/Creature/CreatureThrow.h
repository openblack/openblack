/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

/// How a creature lets go of what it holds: thrown at something, tossed away, lobbed or put down, and how the thing then
/// flies until it comes to rest on the ground
namespace openblack::creature_throw
{
/// The pull of gravity on what is thrown, in units a second each second
constexpr float k_Gravity = 9.81f;

/// The throwing animations, flat and high, blended by how high the target is; and those that let go of what is held:
/// tossing it away, eating it, putting it down and lobbing it gently
constexpr size_t k_HurlFlat = 91;
constexpr size_t k_HurlHigh = 92;
constexpr size_t k_Discard = 95;
constexpr size_t k_Eat = 96;
constexpr size_t k_PutDown = 97;
constexpr size_t k_GentleLob = 98;

/// Tossing something away, it leaves with this share of the hand's own speed at that moment
constexpr float k_DiscardSpeedShare = 0.6f;
/// The hand's speed is measured over this many milliseconds of the animation before it lets go
constexpr float k_HandSpeedSpanMs = 100.0f;
/// A creature throws nothing at anything nearer than this share of its height, 15 units at size 1
constexpr float k_MinThrowHeightShare = 0.66f;
constexpr float k_HeightAtSizeOne = 15.0f;

/// How long something thrown at a target a distance away takes to get there: the time it would take to fall that far
[[nodiscard]] float FlightTime(float distance);

/// The velocity that takes something from where the hand lets go of it to the target in a time, falling as it goes
[[nodiscard]] glm::vec3 ReleaseVelocity(const glm::vec3& target, const glm::vec3& release, float seconds);

/// Whether a target is far enough along the ground to throw at, for a creature of a size
[[nodiscard]] bool FarEnoughToThrow(float groundDistance, float size);

/// How much of the high throw to blend into the flat one, 0 to 1. Each slope is height over distance ahead: the
/// target's, and the hand's where it lets go in the flat and in the high throw.
[[nodiscard]] float HighThrowWeight(float targetSlope, float flatSlope, float highSlope);

/// The hand's velocity in units a second, from where it is when it lets go and where it was a span of milliseconds
/// before
[[nodiscard]] glm::vec3 HandVelocity(const glm::vec3& atRelease, const glm::vec3& before, float spanMs);

/// The velocity something tossed away leaves with: a share of the hand's own, measured in the creature's space, flipped
/// to the other side when tossed by the other hand and turned with the creature
[[nodiscard]] glm::vec3 TossVelocity(const glm::vec3& handVelocity, bool mirrored, const glm::mat3& rotation, float share);

} // namespace openblack::creature_throw

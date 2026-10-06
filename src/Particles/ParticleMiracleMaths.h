/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <span>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// The formulas of the miracles' particle rules: how a fireball is thrown and bounces, where a lightning bolt looks for
/// things to strike and how its forks split, and how shields stop what crosses them. Free of state, so they are tested
/// on their own.
namespace openblack::particles::maths
{

// Throwing a fireball

/// A throw from the hand goes this far above the hand's movement, in radians, unless its file says otherwise
inline constexpr float k_ThrowLift = 0.3f;
/// How a hand's speed becomes a throw's: three hand speeds and the throw speeds they give, eased between, the last held
/// above. The files give their own; these are the usual.
struct ThrowSpeeds
{
	std::array<float, 3> hand {0.0f, 50.0f, 450.0f};
	std::array<float, 3> thrown {0.0f, 50.0f, 250.0f};
};
/// A thrown fireball of another caster aims at this share of the way to its target
inline constexpr float k_LobAimShare = 0.8f;
/// It takes a fortieth of a second for each metre of its flight, and at least half a second
inline constexpr float k_LobSecondsPerMetre = 0.025f;
inline constexpr float k_LobMinimumSeconds = 0.5f;
/// A lob never leaves steeper than this, in radians
inline constexpr float k_LobSteepest = 0.52370351552963257f;

/// Where a throw goes and how fast
struct Launch
{
	glm::vec3 direction {0.0f};
	float speed {0.0f};
};

/// A direction lifted by an angle above itself, no steeper than straight up or down
[[nodiscard]] glm::vec3 LiftDirection(glm::vec3 direction, float lift);
/// The speed of a throw by the hand's speed
[[nodiscard]] float ThrowSpeedFromHand(float handSpeed, const ThrowSpeeds& speeds = {});
/// A human player's throw: lifted a little above the hand's movement, at a speed eased from the hand's
[[nodiscard]] Launch HandThrow(glm::vec3 handVelocity, float lift = k_ThrowLift, const ThrowSpeeds& speeds = {});
/// Another caster's throw from a point towards a target: an arc under a gravity to most of the way there, taking time by
/// its distance, or a flatter one when that would leave too steeply
[[nodiscard]] Launch Lob(glm::vec3 from, glm::vec3 target, float gravity);

/// A particle's speed after striking a slope: its slide along the slope slowed by the drag over the step and scaled by
/// the horizontal bounce, its speed into the slope turned back and scaled by the vertical bounce
[[nodiscard]] glm::vec3 BounceOffSlope(glm::vec3 velocity, glm::vec3 normal, float groundDrag, float dt, float horizontalBounce,
                                       float verticalBounce);

// Lightning

/// Whether a point lies within a cone about a heading on the ground, of a half angle by its cosine
[[nodiscard]] bool InStrikeCone(glm::vec3 origin, float heading, float cosHalfAngle, glm::vec3 point);
/// How many of a bolt's targets it strikes at once: at most the limit and at most half of them, rounded up
[[nodiscard]] int StrikesAtOnce(int limit, int targets);
/// The two sides a fork's targets are split into, by which side of its split point they lie along its way; neither side
/// is left empty when there are two or more
struct ForkSplit
{
	std::vector<size_t> ahead;
	std::vector<size_t> behind;
};
[[nodiscard]] ForkSplit SplitTargets(std::span<const glm::vec3> tips, glm::vec3 origin, glm::vec3 centroid, glm::vec3 split);
/// The scale of a fork joint: from the bolt's scale over its depth plus one at its start to over its depth plus two at
/// its end
[[nodiscard]] float ForkJointScale(float scale, int depth, float t);

// Shields

/// Whether a point lies inside a sphere grown by a margin
[[nodiscard]] bool InsideSphere(glm::vec3 point, glm::vec3 centre, float radius, float margin);
/// Where a way from one point to another first goes into a sphere grown by a margin: the start when it is inside
/// already, else the first crossing, or the end when it never crosses
[[nodiscard]] glm::vec3 SphereEntry(glm::vec3 from, glm::vec3 to, glm::vec3 centre, float radius, float margin);
/// A speed reflected off a sphere's surface at a point
[[nodiscard]] glm::vec3 DeflectOffSphere(glm::vec3 point, glm::vec3 centre, glm::vec3 velocity);

} // namespace openblack::particles::maths

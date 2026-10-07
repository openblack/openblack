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

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// The tornado's shape and motion, free of any state so they can be tested on their own. The funnel stands from a base
/// on the land to a top under the storm's clouds; things caught in it orbit its axis, are pulled to its wall and rise.
/// Every length scales with the tornado's size, which grows with the storm's radius.
namespace openblack::particles::tornado
{

/// The funnel's shape and spin, as the effect file gives them
struct Funnel
{
	/// The wall's radius at the foot and at the top, before scaling
	float baseRadius {4.38053f};
	float topRadius {36.7624f};
	/// The size of what is drawn on the wall at the foot and at the top
	float baseScale {1.53319f};
	float topScale {5.13938f};
	/// Radians a second that things spin round at the foot and at the top, and the bias of the spin up the funnel
	float baseSpin {12.5699f};
	float topSpin {5.89912f};
	float spinBias {0.632743f};
	/// How the axis bends from the foot to the top: the gain of its height
	float bend {0.668142f};
	/// The axis wiggles this many half turns up the funnel, this far to the side
	int wiggleCount {5};
	float wiggleAmplitude {3.2208f};
	/// How big the tornado is: every length is scaled by it
	float scale {1.0f};
	/// Spin multiplier of the whole tornado
	float spinMultiplier {1.0f};
};

/// The bias curve: t raised to the power that makes 0.5 come out at b
[[nodiscard]] float Bias(float t, float b);
/// The gain curve: two bias curves back to back, flat in the middle for a gain over 0.5
[[nodiscard]] float Gain(float t, float g);

/// The radius of the funnel's wall at a share of its height (0 the foot, 1 the top)
[[nodiscard]] float WallRadius(const Funnel& funnel, float h);
/// The size of what is drawn on the wall at a share of its height
[[nodiscard]] float WallScale(const Funnel& funnel, float h);
/// The point of the funnel's axis at a share of its height: straight up between the base and the top, bent sideways by
/// the gain and wiggled
[[nodiscard]] glm::vec3 AxisCentre(const Funnel& funnel, glm::vec3 base, glm::vec3 top, float h);
/// Radians a second something at a share of the height spins round the axis, by its own orbit factor
[[nodiscard]] float SpinRate(const Funnel& funnel, float h, float orbitFactor);
/// The spin of something outside the wall, slowed by how far out it is
[[nodiscard]] float SpinOutside(float spin, float wallRadius, float distance);
/// How fast something's distance from the axis changes over a step, pulled to the wall's radius at half its distance a
/// second, by the midpoint of the step
[[nodiscard]] float RadialRate(float wallRadius, float distance, float dt);
/// How fast something rises towards the height it is drawn to, a share of the way from the base to the top, by a spring
[[nodiscard]] float RiseSpeed(float y, float baseY, float topY, float target, float spring);
/// The share of the height something is at, within 0..1
[[nodiscard]] float HeightShare(float y, float baseY, float topY);

/// How far the foot wanders from under the clouds at an age, from the noise at four rates of the time: x from the
/// noises at t and 2t, z from those at 1.3t and 2.6t, the faster at half weight, all times the amplitude and the
/// tornado's size
[[nodiscard]] glm::vec2 FootWander(float n1, float n2, float n3, float n4, float amplitude, float scale);
/// The times the noise is read at for the foot's wander at an age, at the wander's rate: t, 1.3t, 2t and 2.6t
struct WanderTimes
{
	float a;
	float b;
	float c;
	float d;
};
[[nodiscard]] WanderTimes FootWanderTimes(float age, float frequency);

/// How far the tornado has faded out since it closed down: 1 before, 0 once its fading time has passed
[[nodiscard]] float CloseFade(float sinceClose, float fadeOut);
/// The tornado's alpha as a byte: faded in over its time from its start, never above its close fade
[[nodiscard]] uint8_t FadeAlpha(float age, float fadeIn, float closeFade);

/// A puff of dust or a pretend object thrown from the foot: up at an angle from the vertical, round at a heading, at a
/// share of the speed scaled by the tornado's size, plus the foot's own movement
[[nodiscard]] glm::vec3 Launch(float fromVertical, float heading, float speedShare, float speed, float scale,
                               glm::vec3 footVelocity);

/// How far round its foot the tornado reaches for things to pick up: the wall's radius at the foot and at the top, times
/// the caster's tribal power held between 1 and 5
[[nodiscard]] float Reach(const Funnel& funnel, float tribalPower);
/// Whether something of a radius across the ground fits in the funnel: under twice the wall's radius at the foot and
/// under the radius at the top
[[nodiscard]] bool Fits(const Funnel& funnel, float radius);
/// How much of a pile's resource the tornado takes: between the least and most by its size, held within 0..1
[[nodiscard]] float PileTake(float scale, float least, float most);
/// The size of the pot it makes of what it took, from a random number of 0.7..1.2: smaller for a small tornado
[[nodiscard]] float PotScale(float scale, float random);

} // namespace openblack::particles::tornado

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

#include <span>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// The maths of the creature spells' particles: the wisps that fly from the hand and wind round the creature, the flies
// that circle an itchy creature's head. Free of state, so they are tested on their
// own.

namespace openblack::particles::maths
{

/// The box round a creature's bones the wisps wind about: its middle across the land, its foot and height, and half
/// its width across the land's diagonal
struct WispBox
{
	float centreX {0.0f};
	float centreZ {0.0f};
	float bottom {0.0f};
	float height {0.0f};
	float radius {0.0f};
};
[[nodiscard]] WispBox WispBoxOf(std::span<const glm::vec3> bones);

/// A wisp winds this much wider and higher than the box, wobbling by this share of its radius
inline constexpr float k_WispReach = 1.2f;
inline constexpr float k_WispWobble = 0.2f;
/// Up to this share of its height a wisp winds at full width; above it narrows by the taper
inline constexpr float k_WispFullWidthBelow = 0.7f;
/// Where a wisp winds about the box, at its angles round and up and its age in seconds
[[nodiscard]] glm::vec3 WispOrbit(const WispBox& box, float theta, float phi, float taper, float age);

/// A wisp flies out of the hand to where it winds over this many seconds
inline constexpr float k_WispFlightSeconds = 2.0f;
/// Where a wisp is, flying from the hand to its place round the creature
[[nodiscard]] glm::vec3 WispPosition(glm::vec3 hand, glm::vec3 orbit, float age);

/// A wisp's opacity, 0 to 255: from 20 to 50 over its first half second
[[nodiscard]] uint8_t WispAlpha(float age);

/// How many wisps are due after a step: they come up to the most over the emitting time
[[nodiscard]] float WispsDue(float due, int most, float dt, float emitSeconds);

/// The point an itchy creature's flies circle: round the middle of its eyes, half as far again as each eye is, the
/// circle level and turned at the orbiting speed
inline constexpr float k_ItchReach = 1.5f;
[[nodiscard]] glm::vec3 ItchOrbit(glm::vec3 rightEye, glm::vec3 leftEye, float seconds, float orbitSpeed);

/// Whether the way from a creature casting to its target has swung too far from the way it began: the two ways, flat,
/// made unit length, meet at less than the cosine of the limit, taken as radians. A way of no length never has.
[[nodiscard]] bool SwungTooFar(glm::vec2 began, glm::vec2 now, float limit);

} // namespace openblack::particles::maths

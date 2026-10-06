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

#include <array>

#include <glm/vec3.hpp>

/// How a creature reaches for something on the ground to pick it up or strike it. Each species has four reaching
/// animations, to the front and back on its left and right. They are not chosen between but blended, weighted by where
/// the thing lies between the four places the hand gets to in them when it takes hold. Something nearer the creature's
/// left than its right is reached for with the animations played mirrored, by the other hand.
namespace openblack::creature_reach
{
/// The corners the hand reaches to, in the order the blend weights them
enum class Corner : uint8_t
{
	BackLeft,
	BackRight,
	FrontLeft,
	FrontRight,
};
constexpr size_t k_CornerCount = 4;

/// The reaching animations for picking up and for striking, by their place in the creature spec, in corner order
constexpr std::array<size_t, k_CornerCount> k_PickUpAnimations {84, 83, 82, 81};
constexpr std::array<size_t, k_CornerCount> k_DestroyAnimations {116, 115, 114, 113};

/// Where the hand is when it takes hold in each reaching animation, in the creature's own space scaled to the world, in
/// corner order
using Points = std::array<glm::vec3, k_CornerCount>;

/// The points reflected to the creature's other side, for reaching with the animations mirrored
[[nodiscard]] Points Mirrored(const Points& points);

/// Whether something at a point in the creature's own space is reached for mirrored: when it is nearer the middle of
/// the reflected points than of the points themselves. Height is ignored.
[[nodiscard]] bool ReachesMirrored(const Points& points, const glm::vec3& target);

/// Where between the corners a point lies, and so how much each reaching animation counts
struct Blend
{
	/// Left to right across the back corners, left to right across the front ones, and back to front
	float acrossBack;
	float acrossFront;
	float backToFront;
	/// Whether the hand can get there: the animations are stretched only so far past their corners
	bool inRange;
	/// The weights of the four animations, in corner order, adding up to 1; outside the corners some are negative
	std::array<float, k_CornerCount> weights;
};

/// How far past the corners a reach can stretch: half the width beyond either side, and 0.8 of the depth beyond the
/// front or back
constexpr float k_SideLimit = 0.5f;
constexpr float k_DepthLimit = 0.8f;

/// Where a point in the creature's own space lies between the corners. Height is ignored.
[[nodiscard]] Blend Solve(const Points& points, const glm::vec3& target);

/// How far from the creature it can reach at most, along the ground: to the front, stretched as far as it goes
[[nodiscard]] float MaxReach(const Points& points);

/// The middle of the corners, along the ground: where something is easiest to reach
[[nodiscard]] glm::vec3 Centre(const Points& points);
} // namespace openblack::creature_reach

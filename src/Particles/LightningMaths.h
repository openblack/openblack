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
#include <optional>
#include <span>
#include <vector>

#include <glm/vec3.hpp>

// The lightning bolt's maths: how many of the land's cells it searches, how a creature in its cone draws every fork to
// itself, where two players' bolts meet when they clash, and the electric arcs a strike leaves crawling over what it hit.
// Free of state, so they are tested on their own.

namespace openblack::particles::maths
{

/// A bolt searches this many of the land's cells, spiralling out from the hand's: a square twice its radius across
inline constexpr float k_StrikeCellSize = 10.0f;
[[nodiscard]] size_t StrikeSearchCells(float radius);
/// A bolt from a cloud or a parent searches a quarter as many: a square its radius across
[[nodiscard]] size_t CloudSearchCells(float radius);
/// The forks a bolt keeps ready: two for each target it may strike at once, and two more
[[nodiscard]] size_t ForkCount(int atOnce, size_t targets);

/// A creature less faded away than this draws the bolt
inline constexpr float k_DrawsBoltBelowFizz = 0.5f;
/// Every one of a bolt's targets is pointed at the creatures among them in turn, when there are any: a creature in the
/// cone takes all the forks. Returns, for each target, the index of the target it becomes.
[[nodiscard]] std::vector<size_t> PreferCreatures(const std::vector<bool>& isCreature);

/// What a bolt shows another to see whether they clash
struct BoltPose
{
	glm::vec3 origin {0.0f};
	/// The middle of its targets' tips
	glm::vec3 centroid {0.0f};
	/// Where it points across the land, in radians from east towards north
	float heading {0.0f};
};
/// How far a bolt's trunk reaches towards another's middle before they meet, and the widest angle off its heading the
/// meeting may be at, in radians
inline constexpr float k_ClashShare = 0.7f;
inline constexpr float k_ClashWidestAngle = 1.036725640296936f;
/// Where a newer bolt meets an older one, if they clash: on the way between the older one's middle and its hand, pulled
/// towards the newer one's heading, within the newer one's reach and in front of it
[[nodiscard]] std::optional<glm::vec3> ClashPoint(const BoltPose& newer, const BoltPose& older, float reach);
/// The older bolt carries on past where they meet this much thicker, fully opaque
inline constexpr float k_ClashForkScale = 3.0f;
/// Each strike of a clash hits twice as hard, for both miracles
inline constexpr float k_ClashStrength = 2.0f;

/// An electric arc crawling over what a bolt struck: from one point of its model to another, leaving and arriving along
/// the model's normals there, the tangents scaled by the arc's length
struct ArcEnds
{
	glm::vec3 from {0.0f};
	glm::vec3 to {0.0f};
	glm::vec3 fromNormal {0.0f};
	glm::vec3 toNormal {0.0f};
};
/// The arc's joints along the curve: a cubic from one end to the other with the given tangents, each joint jittered by a
/// point of the unit ball times the arc's length times the jitter
[[nodiscard]] std::vector<glm::vec3> ArcJoints(const ArcEnds& ends, glm::vec3 fromTangent, glm::vec3 toTangent,
                                               std::span<const glm::vec3> jitters, float randomFrac);
/// An end's tangent: its normal, nudged by a point of the unit ball times the random tangent share, times the arc's
/// length times the tangent scale
[[nodiscard]] glm::vec3 ArcTangent(glm::vec3 normal, glm::vec3 nudge, float randomTangents, float length, float scale);

} // namespace openblack::particles::maths

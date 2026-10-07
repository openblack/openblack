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

#include <functional>
#include <optional>
#include <span>
#include <vector>

#include <glm/mat2x2.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "3D/SkeletalAnimation.h"

// How a creature moves about something it acts on, such as a creature it casts a miracle at, as the game measures it.
//
// Sizes: a thing's radius on the ground is the larger of its model's half width and half length times its scale (a
// field's is always 5), and its height is its model's full height times its scale. A creature's is its height of 15 a
// size times how far its bones reach out from its middle in the first frame of its first animation. Walking up to a
// thing, it keeps clear of it by the thing's radius less some of its own: seven tenths of it for a thing much lower than
// four fifths of the creature, less the taller the thing, down to two fifths of it for a thing as tall as that or taller
// (a tree only a tenth of its radius, at most a quarter). It has arrived once it is within a tenth more than both their
// radii and the distance it was to keep.
//
// Getting away from a thing, it is far enough once it is the distance it was to keep plus a fifth more than the thing's
// radius away; else it goes that far from where it stands, straight away from the thing, to the nearest clear area as
// wide as it is tall: of the cells of the land within a hundred and thirty or so of the point, the middle of a square
// of cells whose every cell with its middle within half the width of the square's middle is clear.
//
// Turning to face a thing, it is facing it once it is within an eighth of a turn of it, or right on top of it.
//
// Pure, tested on made-up things.

namespace openblack::creature_cast_moves
{

/// A creature of size 1 is this tall
inline constexpr float k_HeightOfSizeOne = 15.0f;
/// A field's radius on the ground
inline constexpr float k_FieldRadius = 5.0f;
/// It has arrived within this much more than both radii and the distance
inline constexpr float k_ArrivalMargin = 1.1f;
/// It is far enough away at the distance plus this many times the thing's radius
inline constexpr float k_GetAwayRadii = 1.2f;
/// It faces a thing within this angle of it, and is on top of it within this distance
inline constexpr float k_FacingRadians = 0.3926991f;
inline constexpr float k_OnTopDistance = 0.1f;
/// The land's cells are this wide, and the clear area is looked for over this many of them across
inline constexpr float k_CellSize = 10.0f;
inline constexpr int32_t k_ClearAreaCells = 26;
/// Every thing a clear cell is kept clear of keeps this far from the cell's middle, past its own radius
inline constexpr float k_ClearOfThings = 5.0f;

/// How far a creature's bones reach out from its middle on the ground, in its model's units: the furthest of them in the
/// first frame of an animation
[[nodiscard]] float BoneReach(const skeletal_animation::Animation& animation, const skeletal_animation::Skeleton& skeleton);
/// A creature's radius on the ground, by its bones' reach on its model and the scale its model is drawn at: the game's
/// 15 a size times the reach in its own units comes to the same
[[nodiscard]] float CreatureRadius(float modelScale, float boneReach);

/// What walking up to a thing keeps clear of: its radius less some of the creature's own, by how tall each is; a tree's
/// is only a little of its radius
[[nodiscard]] float RoutePlanRadius(float radius, float height, bool tree, float creatureHeight, float creatureRadius);
/// Whether a creature walking up to a thing, so far away on the ground, has arrived
[[nodiscard]] bool Arrived(float distance, float creatureRadius, float routeRadius, float keep);

/// How far from a thing a creature getting away from it must be
[[nodiscard]] float GetAwayDistance(float keep, std::optional<float> thingRadius);
/// Where it goes: that far from where it stands, straight away from the thing (x along when right on top of it)
[[nodiscard]] glm::vec3 GetAwayPoint(const glm::vec3& creature, const glm::vec3& thing, float distance);

/// A circle a thing on the land keeps creatures off, on the ground
struct CollideCircle
{
	glm::vec2 centre;
	float radius;
};
/// The circles a building, feature, field or other fixed thing keeps clear, from its model's half width and length
/// times its scale (at least 1 each), the middle of its model's box placed on the land, and its turn: one circle as wide
/// as the longer, or for a thing more than 1.4 times as long as wide a row of circles as wide as the shorter along it
[[nodiscard]] std::vector<CollideCircle> CollideCircles(glm::vec2 halfSize, glm::vec2 centre, const glm::mat2& turn);
/// A tree keeps clear only a little circle where it stands
inline constexpr float k_TreeCollideRadius = 0.3f;
/// Whether a circle comes within the reach of a cell's middle that makes the cell not clear
[[nodiscard]] bool BlocksCell(const CollideCircle& circle, int32_t x, int32_t z);

/// Whether a cell of the land, by its column and row, is clear
using ClearCell = std::function<bool(int32_t x, int32_t z)>;
/// The middle of the nearest area so wide about a point whose cells are clear, if any
[[nodiscard]] std::optional<glm::vec2> FindClearArea(glm::vec2 point, float width, const ClearCell& clear);

/// Whether a creature at a heading (radians, as the creatures' movement measures it) faces a point from where it stands
[[nodiscard]] bool Facing(glm::vec2 creature, float heading, glm::vec2 point);

} // namespace openblack::creature_cast_moves

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

#include <array>
#include <span>
#include <vector>

#include <glm/vec3.hpp>

#include "Body.h"

/// The shapes bodies are built from: a model's own points and triangles, or the hand-made shapes of trees, villagers and
/// animals, building pieces and the creature.
namespace openblack::physics::shapes
{

/// The lightest a body may be
inline constexpr float k_MinMass = 0.01f;
/// Pieces of buildings weigh this much per unit of area
inline constexpr float k_FragmentMassPerArea = 30.0f;
/// How far behind its face each point of a building piece has its twin, giving the piece thickness
inline constexpr float k_FragmentThickness = 0.45f;
inline constexpr float k_CreatureMass = 1000.0f;
inline constexpr float k_BuildingMass = 2000.0f;
inline constexpr float k_TempleHeartMass = 10000.0f;

/// One part of a model as the physics sees it
struct ModelPart
{
	std::span<const glm::vec3> positions;
	/// Three vertex indices per triangle
	std::span<const uint32_t> indices;
	/// Marked as the model's collision shape
	bool isPhysics {false};
	/// Drawn at the nearest level of detail
	bool nearestDetail {false};
};

/// Which parts of a model make its body: those marked as its collision shape, or else those of the nearest level of
/// detail
[[nodiscard]] std::vector<std::size_t> BodyParts(std::span<const ModelPart> parts);

/// A body from a model's parts: every vertex a point, every triangle a face, the centre of mass at the mean of the
/// points and the radius the farthest scaled point from it. Empty when no part makes a body.
[[nodiscard]] Shape FromModel(std::span<const ModelPart> parts, float scale);

/// A tree's spindle of 16 points along its trunk. A rooted tree's centre of mass is at 0.4 of its height and its spindle
/// reaches a little below its base; a fallen one balances at half its height. Height and radius are scaled.
[[nodiscard]] Shape Tree(float height, float radius, float scale, bool rooted);

enum class Living : uint8_t
{
	Villager,
	Animal,
};
/// A villager's or an animal's box of 12 points on three levels (head, waist and feet), turned a quarter about its up
/// axis, with twice the air drag of objects. Height and radius are scaled.
[[nodiscard]] Shape LivingBody(Living kind, float height, float radius, float scale);

/// A building piece: a slab of its triangles (three points each, in the piece's own frame)
struct FragmentShape
{
	Shape shape;
	float area {0.0f};
	float mass {0.0f};
	/// Too thin a piece is thrown away as soon as it is made
	bool tooThin {false};
};
[[nodiscard]] FragmentShape Fragment(std::span<const std::array<glm::vec3, 3>> triangles);

/// A part of a skeleton's box in the part's own space
struct BoneBox
{
	glm::vec3 min {0.0f};
	glm::vec3 max {0.0f};
};
/// Each bone's box about the vertices it moves, in its own space; every box also holds the bone's origin, as both its
/// corners start there. Vertices without a bone (k_NoBone) are left out; a model without bones puts every vertex in the
/// first box.
inline constexpr uint16_t k_NoBone = 0xFFFF;
[[nodiscard]] std::vector<BoneBox> BoneBoxes(std::span<const glm::vec3> positions, std::span<const uint16_t> bones,
                                             size_t boneCount);

/// The creature as an obstacle: a point for each part of its skeleton, all on the centre of the body's own frame, as
/// wide as its bounding sphere, no faces. The points are placed at the parts' origins with Body::PlacePoints.
[[nodiscard]] Shape Creature(float sphereRadius, size_t parts);

} // namespace openblack::physics::shapes

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
#include <cstdint>

#include <array>
#include <functional>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// The leash's rope: a chain of masses on springs between the hand (or what the leash is tied to) and the creature's
/// collar. It sags under its own weight, swings, is kept off the ground, and is drawn as a ribbon facing the camera with
/// a shadow strip on the land beneath it. How hard the rope is pulled, from slack to fully taut, is its tension.
namespace openblack::leash_rope
{

/// The masses between the two ends
constexpr size_t k_NodeCount = 40;
/// The rope is stepped at 200 times a second, however long the frame
constexpr float k_StepSeconds = 0.005f;
constexpr float k_StepsPerSecond = 200.0f;
/// At most this many steps a frame: a long stall moves the ends without making the rope catch up all at once
constexpr uint32_t k_MaxSteps = 200;
/// Each mass and how the springs and air act on it
constexpr float k_Stiffness = 20000.0f;
constexpr float k_Drag = 0.1f;
/// The drag is divided by the rest length of a segment, never less than this
constexpr float k_MinDragLength = 0.5f;
constexpr float k_Gravity = -7.848f;
constexpr float k_Mass = 0.8f;
constexpr float k_MaxSpeed = 150.0f;
/// The masses keep this much over the ground beyond the ribbon's half width
constexpr float k_GroundClearance = 0.5f;
/// The ends are kept within the world: x and z from -1000 to 6120, height from 0 to 6120
constexpr float k_MinAcross = -1000.0f;
constexpr float k_MaxAcross = 6120.0f;
constexpr float k_MinHeight = 0.0f;
constexpr float k_MaxHeight = 6120.0f;

/// How the rope looks: half its width, the band of the leash texture it shows across its width, and how fast the
/// texture repeats along it
struct Look
{
	float halfWidth {0.15f};
	float v0 {0.125f};
	float v1 {0.25f};
	float uScale {2.5f};
};

struct Node
{
	glm::vec3 position {0.0f};
	glm::vec3 velocity {0.0f};
	/// How far past its rest length the segment to the node before is stretched, as a fraction
	float stretch {0.0f};
};

struct Rope
{
	glm::vec3 start {0.0f};
	glm::vec3 end {0.0f};
	std::array<Node, k_NodeCount> nodes {};
	/// The rope's length at rest and its length pulled fully taut
	float slackLength {32.5f};
	float maxLength {77.0f};
	Look look {};
	/// How hard the rope is pulled at its start, 0 slack to 1 taut
	float tension {0.0f};
};

/// The land's height under a point across it
using GroundHeight = std::function<float(glm::vec2)>;

/// The length of each of the rope's segments at rest
[[nodiscard]] float RestLength(float slackLength);
/// A rope straight from start to end, its masses spread evenly along it and still
[[nodiscard]] Rope Create(const glm::vec3& start, const glm::vec3& end, float slackLength, float maxLength, const Look& look);
/// An end kept within the world
[[nodiscard]] glm::vec3 ClampEnd(const glm::vec3& point);
/// The rope some seconds on, its ends moving to start and end: in steps of 1/200 s, each moving the ends that much
/// closer to where they go, then pulling each mass towards its neighbours, slowing it by the air, dropping it under
/// gravity and moving it. The ends then sit exactly where they go and the tension is worked out again.
void Step(Rope& rope, const glm::vec3& start, const glm::vec3& end, float seconds, const GroundHeight& ground);
/// How hard the rope is pulled: how far the first segment is stretched past its rest length, as a share of how far it
/// stretches when the rope is at its full length, 0 to 1
[[nodiscard]] float Tension(const Rope& rope);

/// The rope's points from start to end, its two ends included
constexpr size_t k_PointCount = k_NodeCount + 2;
/// A corner of the rope's ribbon or of its shadow. The ribbon's corners are fully opaque; the shadow's are faint and
/// fade out at both ends.
struct RibbonVertex
{
	glm::vec3 position;
	glm::vec2 uv;
	float alpha;
};
/// The shadow's opacity, out of 255
constexpr uint32_t k_ShadowAlpha = 0x41;
/// How far the shadow lies over the land
constexpr float k_ShadowLift = 0.1f;
/// Two corners at each point, either side of it
constexpr size_t k_RibbonVertexCount = k_PointCount * 2;
/// Two triangles between each pair of points
constexpr size_t k_RibbonIndexCount = (k_PointCount - 1) * 6;
struct Ribbon
{
	std::array<RibbonVertex, k_RibbonVertexCount> rope;
	std::array<RibbonVertex, k_RibbonVertexCount> shadow;
};
/// The rope's point i, from its start to its end
[[nodiscard]] glm::vec3 Point(const Rope& rope, size_t i);
/// The ribbon facing an eye, half the rope's width either side of each point across the line from it to the eye and
/// the rope's direction there, and the shadow strip flat on the land beneath it. The texture runs along the rope by its
/// length and across it through the look's band.
[[nodiscard]] Ribbon BuildRibbon(const Rope& rope, const glm::vec3& eye, const GroundHeight& ground);
/// The triangles of a ribbon, corner indices from the first of its corners
[[nodiscard]] std::array<uint16_t, k_RibbonIndexCount> RibbonIndices();

} // namespace openblack::leash_rope

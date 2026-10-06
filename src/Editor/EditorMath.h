/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "3D/AxisAlignedBoundingBox.h"

/// The editor's maths, free of the game so it can be tested on its own: snapping, the orbit and follow cameras, picking
/// things with the mouse's ray and dragging them over the land.
namespace openblack::editor
{

/// Snapping for the move and rotate tools, as the toolbar sets it
struct Snapping
{
	bool enabled {false};
	/// Units of the land
	float move {5.0f};
	float angleDegrees {15.0f};
};

/// The value rounded to the nearest whole step, or as it is with no step
[[nodiscard]] float Snap(float value, float step);
/// A point on the land snapped on x and z, keeping its height
[[nodiscard]] glm::vec3 SnapPoint(glm::vec3 point, const Snapping& snapping);
/// An angle in radians snapped to the snapping's degrees
[[nodiscard]] float SnapAngle(float radians, const Snapping& snapping);
/// The angle brought into -pi to pi
[[nodiscard]] float WrapAngle(float radians);

/// Where an orbiting camera is round its target: yaw about the up axis from +z, pitch up from the ground and the distance
struct Orbit
{
	float yaw {0.0f};
	float pitch {0.5f};
	float distance {40.0f};
};
constexpr float k_MinOrbitPitch = -0.2f;
constexpr float k_MaxOrbitPitch = 1.5f;
constexpr float k_MinOrbitDistance = 3.0f;
constexpr float k_MaxOrbitDistance = 2000.0f;
/// Where the camera stands for an orbit round a target
[[nodiscard]] glm::vec3 OrbitOrigin(glm::vec3 target, const Orbit& orbit);
/// The orbit that puts the camera at the origin, looking at the target
[[nodiscard]] Orbit OrbitFrom(glm::vec3 origin, glm::vec3 target);
/// The orbit turned by a drag, in radians across and up, with its pitch kept within bounds
[[nodiscard]] Orbit Turn(Orbit orbit, glm::vec2 radians);
/// The orbit pulled in by steps of the wheel, each a tenth closer, and pushed out by negative steps
[[nodiscard]] Orbit Zoom(Orbit orbit, float steps);

/// The yaw of an orbit that puts the camera behind a thing facing a way on the land, turned further by an angle
[[nodiscard]] float BehindYaw(glm::vec2 facing, float turned);
/// Eases a value towards a goal: the share of the way it goes in a second, so the same however the frames fall
[[nodiscard]] glm::vec3 EaseTowards(glm::vec3 current, glm::vec3 goal, float seconds, float sharePerSecond);
/// The way a rotation faces on the land, from its +z axis
[[nodiscard]] glm::vec2 FacingOf(const glm::mat3& rotation);
/// The rotation that turns +z to face an angle about the up axis, as the archetypes give it
[[nodiscard]] glm::mat3 YawRotation(float yawRadians);
/// The angle about the up axis a rotation faces
[[nodiscard]] float YawOf(const glm::mat3& rotation);

/// How far along a ray, from its origin, it enters a box, if it meets it at all
[[nodiscard]] std::optional<float> RayBox(glm::vec3 origin, glm::vec3 direction, const AxisAlignedBoundingBox& box);
/// A mesh's box carried into the world by a position, rotation and scale: the box round the turned box's corners
[[nodiscard]] AxisAlignedBoundingBox WorldBox(const AxisAlignedBoundingBox& box, glm::vec3 position, const glm::mat3& rotation,
                                              glm::vec3 scale);
/// Where a ray meets a level plane at a height, if it points down or up towards it
[[nodiscard]] std::optional<glm::vec3> RayLevel(glm::vec3 origin, glm::vec3 direction, float height);

} // namespace openblack::editor

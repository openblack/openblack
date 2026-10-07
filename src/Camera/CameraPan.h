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

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// Dragging the land the way the game does: the land gripped stays under the cursor. Both the line of sight through
/// the cursor now and the one through it where the land was gripped are cast from the camera as it was then, onto a
/// plane through the gripped land, and the camera moves by the difference. Pure maths, tested on its own.
namespace openblack::camera_pan
{

/// The plane the land is dragged across, through the gripped land. It holds the direction from the ground under the
/// camera to the gripped land and the level direction square to it, so it faces up and back towards the camera.
struct GripPlane
{
	glm::vec3 normal {0.0f, 1.0f, 0.0f};
	float distance {0.0f};
};

/// The plane through the gripped land, from the camera and the height of the ground under it
[[nodiscard]] GripPlane PlaneThrough(glm::vec3 gripped, glm::vec3 cameraOrigin, float groundUnderCamera);

/// A drag further than this from the camera, along its line of sight, is given up
constexpr float k_MaxGripDepth = 3000.0f;
/// The most the land moves, as a share of the distance to the focus for each pixel the cursor moved, and at least
constexpr float k_StepPerPixel = 0.011f;
constexpr float k_MinStep = 50.0f;

/// The camera's origin and focus after a drag
struct CameraPlace
{
	glm::vec3 origin;
	glm::vec3 focus;
};

/// Where the camera goes for the cursor's line of sight now and at the grip, both cast from the camera as it was at the
/// grip, at originAtGrip looking at focusAtGrip from distance away. The land moves at most 0.011 of the distance for
/// every pixel the cursor has moved since the grip, and at least 50. None when either line misses the plane.
[[nodiscard]] std::optional<CameraPlace> Pan(const GripPlane& plane, glm::vec3 originAtGrip, glm::vec3 focusAtGrip,
                                             float distance, glm::vec3 rayNow, glm::vec3 rayAtGrip, glm::ivec2 cursorNow,
                                             glm::ivec2 cursorAtGrip);

/// The camera stops this far short, across the land, of land in its way
constexpr float k_LandStopDistance = 3.0f;

/// The camera's move from where it was at the grip is cut short before land in its way: landHit is where the line from
/// the old origin through the new one meets the land (or the sea)
[[nodiscard]] CameraPlace StopShortOfLand(const CameraPlace& place, glm::vec3 originAtGrip, glm::vec3 landHit);

/// Where the line from the camera at the grip through its new origin meets the sea, if the land doesn't stop it: only
/// when it goes down, and no further than 7500 across from where the camera is now
[[nodiscard]] std::optional<glm::vec3> SeaHit(glm::vec3 originAtGrip, glm::vec3 newOrigin, glm::vec3 cameraNow);

} // namespace openblack::camera_pan

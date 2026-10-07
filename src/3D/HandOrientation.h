/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

namespace openblack::hand_orientation
{

/// The hand's up eases to the land's slope over this many seconds
constexpr float k_UpEaseSeconds = 0.4f;
/// The line of sight must lean at least this far from straight down, east-west or north-south, to turn the hand
constexpr float k_MinHeadingLean = 0.01f;

/// Which way the hand faces across the land: along the line of sight through the cursor, flattened. Looking straight
/// down it keeps the way it faced.
[[nodiscard]] glm::vec3 HeadingAlongRay(glm::vec3 rayDirection, glm::vec3 previousHeading);

/// The hand turned to face along a heading on level land, from how it is turned facing along the camera's own
/// heading. Both headings are level.
[[nodiscard]] glm::mat3 TurnToHeading(const glm::mat3& facingCamera, glm::vec3 cameraHeading, glm::vec3 heading);

/// The hand stood up on a slope: its up becomes the slope's up and its front the heading laid along the slope, from
/// how it stands facing along the heading on level land
[[nodiscard]] glm::mat3 StandOnSlope(const glm::mat3& onLevelLand, glm::vec3 heading, glm::vec3 up);

} // namespace openblack::hand_orientation

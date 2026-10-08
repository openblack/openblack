/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "HandOrientation.h"

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace openblack::hand_orientation
{

namespace
{
constexpr glm::vec3 k_Up {0.0f, 1.0f, 0.0f};

/// A frame of a front and an up square to it, with the side across them
glm::mat3 Frame(glm::vec3 front, glm::vec3 up)
{
	return {glm::normalize(glm::cross(front, up)), up, front};
}
} // namespace

glm::vec3 HeadingAlongRay(glm::vec3 rayDirection, glm::vec3 previousHeading)
{
	if (std::abs(rayDirection.x) <= k_MinHeadingLean && std::abs(rayDirection.z) <= k_MinHeadingLean)
	{
		return previousHeading;
	}
	return glm::normalize(glm::vec3(rayDirection.x, 0.0f, rayDirection.z));
}

glm::mat3 TurnToHeading(const glm::mat3& facingCamera, glm::vec3 cameraHeading, glm::vec3 heading)
{
	const auto angle = std::atan2(glm::cross(cameraHeading, heading).y, glm::dot(cameraHeading, heading));
	return glm::mat3(glm::rotate(glm::mat4(1.0f), angle, k_Up)) * facingCamera;
}

glm::mat3 StandOnSlope(const glm::mat3& onLevelLand, glm::vec3 heading, glm::vec3 up)
{
	const auto upright = glm::normalize(up);
	auto front = heading - glm::dot(heading, upright) * upright;
	if (glm::length(front) <= 0.0f)
	{
		// The slope faces along the heading itself: there is no front to lay along it
		return onLevelLand;
	}
	front = glm::normalize(front);
	return Frame(front, upright) * glm::transpose(Frame(heading, k_Up)) * onLevelLand;
}

glm::mat3 TipForwards(const glm::mat3& onLevelLand, glm::vec3 heading, float angle)
{
	const auto across = glm::cross(k_Up, heading);
	if (glm::length(across) <= 0.0f)
	{
		return onLevelLand;
	}
	return glm::mat3(glm::rotate(glm::mat4(1.0f), angle, glm::normalize(across))) * onLevelLand;
}

} // namespace openblack::hand_orientation

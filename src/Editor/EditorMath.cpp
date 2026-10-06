/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "EditorMath.h"

#include <cmath>

#include <algorithm>
#include <array>
#include <limits>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace openblack::editor
{

float Snap(float value, float step)
{
	if (step <= 0.0f)
	{
		return value;
	}
	return std::round(value / step) * step;
}

glm::vec3 SnapPoint(glm::vec3 point, const Snapping& snapping)
{
	if (!snapping.enabled)
	{
		return point;
	}
	return {Snap(point.x, snapping.move), point.y, Snap(point.z, snapping.move)};
}

float SnapAngle(float radians, const Snapping& snapping)
{
	if (!snapping.enabled)
	{
		return radians;
	}
	return glm::radians(Snap(glm::degrees(radians), snapping.angleDegrees));
}

float WrapAngle(float radians)
{
	const auto turn = glm::two_pi<float>();
	auto wrapped = std::fmod(radians + glm::pi<float>(), turn);
	if (wrapped < 0.0f)
	{
		wrapped += turn;
	}
	return wrapped - glm::pi<float>();
}

glm::vec3 OrbitOrigin(glm::vec3 target, const Orbit& orbit)
{
	const auto across = std::cos(orbit.pitch);
	return target +
	       glm::vec3(std::sin(orbit.yaw) * across, std::sin(orbit.pitch), std::cos(orbit.yaw) * across) * orbit.distance;
}

Orbit OrbitFrom(glm::vec3 origin, glm::vec3 target)
{
	const auto offset = origin - target;
	const auto distance = glm::length(offset);
	if (distance <= std::numeric_limits<float>::epsilon())
	{
		return {.yaw = 0.0f, .pitch = 0.5f, .distance = k_MinOrbitDistance};
	}
	const auto pitch = std::asin(std::clamp(offset.y / distance, -1.0f, 1.0f));
	return {
	    .yaw = std::atan2(offset.x, offset.z),
	    .pitch = std::clamp(pitch, k_MinOrbitPitch, k_MaxOrbitPitch),
	    .distance = std::clamp(distance, k_MinOrbitDistance, k_MaxOrbitDistance),
	};
}

Orbit Turn(Orbit orbit, glm::vec2 radians)
{
	orbit.yaw = WrapAngle(orbit.yaw + radians.x);
	orbit.pitch = std::clamp(orbit.pitch + radians.y, k_MinOrbitPitch, k_MaxOrbitPitch);
	return orbit;
}

Orbit Zoom(Orbit orbit, float steps)
{
	orbit.distance = std::clamp(orbit.distance * std::pow(0.9f, steps), k_MinOrbitDistance, k_MaxOrbitDistance);
	return orbit;
}

float BehindYaw(glm::vec2 facing, float turned)
{
	if (glm::length(facing) <= std::numeric_limits<float>::epsilon())
	{
		return WrapAngle(turned);
	}
	return WrapAngle(std::atan2(-facing.x, -facing.y) + turned);
}

glm::vec3 EaseTowards(glm::vec3 current, glm::vec3 goal, float seconds, float sharePerSecond)
{
	if (seconds <= 0.0f)
	{
		return current;
	}
	const auto share = std::clamp(sharePerSecond, 0.0f, 1.0f);
	if (share >= 1.0f)
	{
		return goal;
	}
	// What is left after a second is 1 - share; after t seconds, its t-th power
	const auto left = std::pow(1.0f - share, seconds);
	return goal + ((current - goal) * left);
}

glm::vec2 FacingOf(const glm::mat3& rotation)
{
	return {rotation[2][0], rotation[2][2]};
}

glm::mat3 YawRotation(float yawRadians)
{
	return glm::mat3(glm::eulerAngleY(yawRadians));
}

float YawOf(const glm::mat3& rotation)
{
	return std::atan2(rotation[2][0], rotation[2][2]);
}

std::optional<float> RayBox(glm::vec3 origin, glm::vec3 direction, const AxisAlignedBoundingBox& box)
{
	float near = 0.0f;
	float far = std::numeric_limits<float>::max();
	for (int axis = 0; axis < 3; ++axis)
	{
		if (std::abs(direction[axis]) <= std::numeric_limits<float>::epsilon())
		{
			if (origin[axis] < box.minima[axis] || origin[axis] > box.maxima[axis])
			{
				return std::nullopt;
			}
			continue;
		}
		auto t0 = (box.minima[axis] - origin[axis]) / direction[axis];
		auto t1 = (box.maxima[axis] - origin[axis]) / direction[axis];
		if (t0 > t1)
		{
			std::swap(t0, t1);
		}
		near = std::max(near, t0);
		far = std::min(far, t1);
		if (near > far)
		{
			return std::nullopt;
		}
	}
	return near;
}

AxisAlignedBoundingBox WorldBox(const AxisAlignedBoundingBox& box, glm::vec3 position, const glm::mat3& rotation,
                                glm::vec3 scale)
{
	AxisAlignedBoundingBox world {
	    .minima = glm::vec3(std::numeric_limits<float>::max()),
	    .maxima = glm::vec3(std::numeric_limits<float>::lowest()),
	};
	for (int corner = 0; corner < 8; ++corner)
	{
		const glm::vec3 local {
		    (corner & 1) != 0 ? box.maxima.x : box.minima.x,
		    (corner & 2) != 0 ? box.maxima.y : box.minima.y,
		    (corner & 4) != 0 ? box.maxima.z : box.minima.z,
		};
		const auto point = position + (rotation * (local * scale));
		world.minima = glm::min(world.minima, point);
		world.maxima = glm::max(world.maxima, point);
	}
	return world;
}

std::optional<glm::vec3> RayLevel(glm::vec3 origin, glm::vec3 direction, float height)
{
	if (std::abs(direction.y) <= std::numeric_limits<float>::epsilon())
	{
		return std::nullopt;
	}
	const auto along = (height - origin.y) / direction.y;
	if (along < 0.0f)
	{
		return std::nullopt;
	}
	return origin + (direction * along);
}

} // namespace openblack::editor

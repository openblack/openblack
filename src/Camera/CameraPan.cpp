/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CameraPan.h"

#include <cmath>

#include <glm/geometric.hpp>

namespace openblack::camera_pan
{

namespace
{
/// Lines of sight this close to running along the plane, or facing away from it, can't drag the land
constexpr float k_ParallelLimit = 0.001f;
/// The plane must be ahead along both lines of sight, at least this far
constexpr float k_MinAhead = 0.0001f;
/// The camera only stops short of land once it moves this far across
constexpr float k_SmallestMove = 1e-4f;
/// The sea stops the camera only this close, squared, across from where it is
constexpr float k_SeaReachSquared = 5.625e7f;
constexpr float k_LevelLimit = 0.0001f;

glm::vec3 UnitOrZero(glm::vec3 v)
{
	return v != glm::vec3(0.0f) ? v / std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z) : v;
}
} // namespace

GripPlane PlaneThrough(glm::vec3 gripped, glm::vec3 cameraOrigin, float groundUnderCamera)
{
	// From the ground under the camera, no higher than the gripped land, to the gripped land
	const auto ground = glm::vec3(cameraOrigin.x, std::min(groundUnderCamera, gripped.y), cameraOrigin.z);
	const auto towards = UnitOrZero(gripped - ground);
	const auto across = UnitOrZero({towards.z, 0.0f, -towards.x});
	const auto normal = UnitOrZero(glm::cross(towards, across));
	return {.normal = normal, .distance = glm::dot(normal, gripped)};
}

std::optional<CameraPlace> Pan(const GripPlane& plane, glm::vec3 originAtGrip, glm::vec3 focusAtGrip, float distance,
                               glm::vec3 rayNow, glm::vec3 rayAtGrip, glm::ivec2 cursorNow, glm::ivec2 cursorAtGrip)
{
	const auto now = glm::dot(plane.normal, rayNow);
	const auto atGrip = glm::dot(plane.normal, rayAtGrip);
	if ((-k_ParallelLimit <= now || -k_ParallelLimit <= atGrip) && (now <= k_ParallelLimit || atGrip <= k_ParallelLimit))
	{
		return std::nullopt;
	}
	const auto toPlane = plane.distance - glm::dot(plane.normal, originAtGrip);
	const auto alongNow = toPlane / now;
	const auto alongAtGrip = toPlane / atGrip;
	if (!(k_MinAhead < alongNow && k_MinAhead < alongAtGrip))
	{
		return std::nullopt;
	}

	// The land under the cursor now moves to where the land under it at the grip was seen
	auto moved = rayNow * alongNow - rayAtGrip * alongAtGrip;
	const auto pixels = cursorNow - cursorAtGrip;
	auto most = std::sqrt(static_cast<float>(pixels.x * pixels.x + pixels.y * pixels.y)) * distance * k_StepPerPixel;
	if (most < k_MinStep)
	{
		most = k_MinStep;
	}
	const auto movedSquared = glm::dot(moved, moved);
	if (most * most < movedSquared)
	{
		moved *= most / std::sqrt(movedSquared);
	}

	const auto focus = focusAtGrip - moved;
	const auto origin = focus + UnitOrZero((originAtGrip - moved) - focus) * distance;
	return CameraPlace {.origin = origin, .focus = focus};
}

CameraPlace StopShortOfLand(const CameraPlace& place, glm::vec3 originAtGrip, glm::vec3 landHit)
{
	const auto toLand = landHit - originAtGrip;
	const auto move = place.origin - originAtGrip;
	const auto acrossSquared = move.x * move.x + move.z * move.z;
	if (!(k_LandStopDistance * k_LandStopDistance < acrossSquared))
	{
		return place;
	}
	const auto stop = k_LandStopDistance / std::sqrt(acrossSquared);
	float share = 0.0f;
	if (std::abs(move.x) > std::abs(move.z))
	{
		if (std::abs(move.x) <= k_SmallestMove)
		{
			return place;
		}
		share = toLand.x / move.x;
	}
	else
	{
		if (std::abs(move.z) <= k_SmallestMove)
		{
			return place;
		}
		share = toLand.z / move.z;
	}
	share -= stop;
	if (!(share > 0.0f) || !(share < 1.0f))
	{
		return place;
	}
	const auto back = move * (1.0f - share);
	return {.origin = place.origin - back, .focus = place.focus - back};
}

std::optional<glm::vec3> SeaHit(glm::vec3 originAtGrip, glm::vec3 newOrigin, glm::vec3 cameraNow)
{
	if (originAtGrip.y < newOrigin.y || std::abs(newOrigin.y - originAtGrip.y) < k_LevelLimit)
	{
		return std::nullopt;
	}
	const auto share = -(originAtGrip.y / (newOrigin.y - originAtGrip.y));
	const auto hit = glm::vec3((newOrigin.x - originAtGrip.x) * share + originAtGrip.x, 0.0f,
	                           (newOrigin.z - originAtGrip.z) * share + originAtGrip.z);
	const auto across = glm::vec2(hit.x - cameraNow.x, hit.z - cameraNow.z);
	if (k_SeaReachSquared < across.x * across.x + across.y * across.y)
	{
		return std::nullopt;
	}
	return hit;
}

} // namespace openblack::camera_pan

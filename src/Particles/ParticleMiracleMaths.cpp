/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ParticleMiracleMaths.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include <glm/geometric.hpp>

using namespace openblack::particles;

namespace
{
constexpr float k_HalfPi = std::numbers::pi_v<float> / 2.0f;
/// A launch slower than this (squared) doesn't count as moving
constexpr float k_StillLaunchSquared = 0.01f;
/// A slide slower than this (squared) has no direction of its own
constexpr float k_StillSlideSquared = 1e-4f;
/// A lob whose target is closer than this (squared) across the ground is nudged away
constexpr float k_TooCloseSquared = 0.1f;
constexpr float k_TooCloseNudge = 0.1f;
/// The shortest flight a lob re-solved for its steepest angle takes, squared seconds
constexpr float k_ShortestFlightSquared = 0.1f;

glm::vec3 NormaliseOrZero(glm::vec3 v)
{
	const float length = glm::length(v);
	return length > 0.0f ? v / length : glm::vec3(0.0f);
}
} // namespace

glm::vec3 maths::LiftDirection(glm::vec3 direction, float lift)
{
	const float yaw = std::atan2(direction.z, direction.x);
	const float pitch = std::clamp(
	    std::atan2(direction.y, std::sqrt(direction.x * direction.x + direction.z * direction.z)) + lift, -k_HalfPi, k_HalfPi);
	return {std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch)};
}

float maths::ThrowSpeedFromHand(float handSpeed, const ThrowSpeeds& speeds)
{
	const auto& in = speeds.hand;
	const auto& out = speeds.thrown;
	if (handSpeed >= in[2])
	{
		return out[2];
	}
	const size_t i = handSpeed < in[1] ? 0 : 1;
	const float span = in[i + 1] - in[i];
	const float share = span > 0.0f ? (handSpeed - in[i]) / span : 1.0f;
	return out[i] + share * (out[i + 1] - out[i]);
}

maths::Launch maths::HandThrow(glm::vec3 handVelocity, float lift, const ThrowSpeeds& speeds)
{
	const float handSpeed = glm::length(handVelocity);
	if (handSpeed <= 0.0f)
	{
		return {};
	}
	return {.direction = LiftDirection(handVelocity / handSpeed, lift), .speed = ThrowSpeedFromHand(handSpeed, speeds)};
}

maths::Launch maths::Lob(glm::vec3 from, glm::vec3 target, float gravity)
{
	const glm::vec2 across(target.x - from.x, target.z - from.z);
	if (glm::dot(across, across) < k_TooCloseSquared)
	{
		target = glm::vec3(from.x + k_TooCloseNudge, from.y, from.z + k_TooCloseNudge);
	}
	const glm::vec3 aim = from + (target - from) * k_LobAimShare;
	const glm::vec3 delta = aim - from;
	const float distance = std::sqrt(delta.x * delta.x + delta.z * delta.z);
	float seconds = std::max(distance * k_LobSecondsPerMetre, k_LobMinimumSeconds);
	// Starting at v, gravity brings it down by half g t squared: it reaches the aim after t
	const auto velocityFor = [&](float t) { return delta / t + glm::vec3(0.0f, gravity * 0.5f * t, 0.0f); };
	glm::vec3 v = velocityFor(seconds);
	const float horizontalSquared = v.x * v.x + v.z * v.z;
	const float steepest = std::tan(k_LobSteepest);
	if (horizontalSquared + v.y * v.y > k_StillLaunchSquared && v.y / std::sqrt(horizontalSquared) > steepest)
	{
		// The flight that leaves at the steepest angle instead
		const float flightSquared = std::max((delta.y - distance * steepest) * (-2.0f / gravity), 0.0f);
		seconds = std::sqrt(flightSquared > 0.0f ? flightSquared : k_ShortestFlightSquared);
		v = velocityFor(seconds);
	}
	const float speed = glm::length(v);
	return {.direction = speed > 0.0f ? v / speed : glm::vec3(0.0f), .speed = speed};
}

glm::vec3 maths::BounceOffSlope(glm::vec3 velocity, glm::vec3 normal, float groundDrag, float dt, float horizontalBounce,
                                float verticalBounce)
{
	const float into = glm::dot(velocity, normal);
	if (into >= 0.0f)
	{
		return velocity;
	}
	const glm::vec3 normalPart = normal * into;
	glm::vec3 slide = velocity - normalPart;
	const float slideSpeed = glm::length(slide);
	const glm::vec3 slideDirection =
	    glm::dot(slide, slide) < k_StillSlideSquared ? glm::vec3(1.0f, 0.0f, 0.0f) : slide / slideSpeed;
	slide -= slideDirection * std::clamp(dt * groundDrag, 0.0f, slideSpeed);
	return slide * horizontalBounce - normalPart * verticalBounce;
}

bool maths::InStrikeCone(glm::vec3 origin, float heading, float cosHalfAngle, glm::vec3 point)
{
	const auto across = NormaliseOrZero(glm::vec3(point.x - origin.x, 0.0f, point.z - origin.z));
	return across.x * std::cos(heading) + across.z * std::sin(heading) > cosHalfAngle;
}

int maths::StrikesAtOnce(int limit, int targets)
{
	return std::max(0, std::min(limit, (targets + 1) / 2));
}

maths::ForkSplit maths::SplitTargets(std::span<const glm::vec3> tips, glm::vec3 origin, glm::vec3 centroid, glm::vec3 split)
{
	ForkSplit result;
	const glm::vec2 way(centroid.x - origin.x, centroid.z - origin.z);
	for (size_t i = 0; i < tips.size(); ++i)
	{
		const float along = (tips[i].x - split.x) * way.x + (tips[i].z - split.z) * way.y;
		(along > 0.0f ? result.ahead : result.behind).push_back(i);
	}
	if (tips.size() >= 2)
	{
		if (result.ahead.empty())
		{
			result.ahead.push_back(result.behind.back());
			result.behind.pop_back();
		}
		else if (result.behind.empty())
		{
			result.behind.push_back(result.ahead.back());
			result.ahead.pop_back();
		}
	}
	return result;
}

float maths::ForkJointScale(float scale, int depth, float t)
{
	const float from = scale / static_cast<float>(depth + 1);
	const float to = scale / static_cast<float>(depth + 2);
	return from + (to - from) * t;
}

bool maths::InsideSphere(glm::vec3 point, glm::vec3 centre, float radius, float margin)
{
	const glm::vec3 d = point - centre;
	const float r = radius + margin;
	return glm::dot(d, d) < r * r;
}

glm::vec3 maths::SphereEntry(glm::vec3 from, glm::vec3 to, glm::vec3 centre, float radius, float margin)
{
	if (InsideSphere(from, centre, radius, margin))
	{
		return from;
	}
	const glm::vec3 way = to - from;
	const float length = glm::length(way);
	if (length <= 0.0f)
	{
		return to;
	}
	const glm::vec3 direction = way / length;
	const glm::vec3 w = from - centre;
	const float r = radius + margin;
	const float b = glm::dot(w, direction);
	const float disc = b * b - (glm::dot(w, w) - r * r);
	if (disc < 0.0f)
	{
		return to;
	}
	const float t = -b - std::sqrt(disc);
	return t >= 0.0f && t <= length ? from + direction * t : to;
}

glm::vec3 maths::DeflectOffSphere(glm::vec3 point, glm::vec3 centre, glm::vec3 velocity)
{
	const glm::vec3 n = NormaliseOrZero(point - centre);
	return velocity - 2.0f * glm::dot(velocity, n) * n;
}

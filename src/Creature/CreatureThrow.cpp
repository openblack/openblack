/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureThrow.h"

#include <cmath>

#include <algorithm>

using namespace openblack;
using namespace openblack::creature_throw;

namespace
{
constexpr float k_Tiny = 1e-4f;
} // namespace

float creature_throw::FlightTime(float distance)
{
	return std::sqrt(std::max(distance, 0.0f) / k_Gravity);
}

glm::vec3 creature_throw::ReleaseVelocity(const glm::vec3& target, const glm::vec3& release, float seconds)
{
	const auto time = std::max(seconds, k_Tiny);
	// Gravity takes half g t squared off the height on the way, which the throw makes up
	const glm::vec3 fall {0.0f, -0.5f * k_Gravity * time * time, 0.0f};
	return (target - release - fall) / time;
}

bool creature_throw::FarEnoughToThrow(float groundDistance, float size)
{
	return groundDistance >= size * k_HeightAtSizeOne * k_MinThrowHeightShare;
}

float creature_throw::HighThrowWeight(float targetSlope, float flatSlope, float highSlope)
{
	const auto span = highSlope - flatSlope;
	if (std::abs(span) < k_Tiny)
	{
		return 0.0f;
	}
	return std::clamp((targetSlope - flatSlope) / span, 0.0f, 1.0f);
}

glm::vec3 creature_throw::HandVelocity(const glm::vec3& atRelease, const glm::vec3& before, float spanMs)
{
	return spanMs > 0.0f ? (atRelease - before) * (1000.0f / spanMs) : glm::vec3(0.0f);
}

glm::vec3 creature_throw::TossVelocity(const glm::vec3& handVelocity, bool mirrored, const glm::mat3& rotation, float share)
{
	auto local = handVelocity * share;
	if (mirrored)
	{
		local.x = -local.x;
	}
	return rotation * local;
}

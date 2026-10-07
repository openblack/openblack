/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureSpellMaths.h"

#include <cmath>

#include <algorithm>
#include <limits>
#include <numbers>

#include <glm/geometric.hpp>

using namespace openblack::particles;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// How sharply a wisp narrows above its full width
constexpr float k_TaperSteepness = 3.3333333f;
/// A wisp's opacity comes up from the least by the rise over this many seconds
constexpr float k_WispFadeInSeconds = 0.5f;
constexpr float k_WispLeastAlpha = 20.0f;
constexpr float k_WispAlphaRise = 30.0f;
} // namespace

maths::WispBox maths::WispBoxOf(std::span<const glm::vec3> bones)
{
	if (bones.empty())
	{
		return {};
	}
	glm::vec3 low(std::numeric_limits<float>::max());
	glm::vec3 high(std::numeric_limits<float>::lowest());
	for (const auto& bone : bones)
	{
		low = glm::min(low, bone);
		high = glm::max(high, bone);
	}
	const float dx = high.x - low.x;
	const float dz = high.z - low.z;
	return {.centreX = (low.x + high.x) * 0.5f,
	        .centreZ = (low.z + high.z) * 0.5f,
	        .bottom = low.y,
	        .height = high.y - low.y,
	        .radius = 0.5f * std::sqrt(dx * dx + dz * dz)};
}

glm::vec3 maths::WispOrbit(const WispBox& box, float theta, float phi, float taper, float age)
{
	const float up = (std::cos(phi) + 1.0f) * 0.5f;
	float width = 1.0f;
	if (!(up < k_WispFullWidthBelow))
	{
		const float over = (up - k_WispFullWidthBelow) * k_TaperSteepness;
		width = 1.0f - over * over * taper;
	}
	glm::vec3 point(box.centreX + std::cos(theta) * width * box.radius * k_WispReach,
	                box.bottom + up * box.height * k_WispReach,
	                box.centreZ + std::sin(theta) * width * box.radius * k_WispReach);
	// It wobbles on its way round
	const float t = age;
	point.x += (std::sin(1.3f * t) + std::sin(t)) * k_WispWobble * box.radius;
	point.y += (std::sin(1.13f * t) + std::sin(0.53f * t)) * k_WispWobble * box.radius;
	point.z += (std::sin(0.9f * t) + std::sin(2.72f * t)) * k_WispWobble * box.radius;
	return point;
}

glm::vec3 maths::WispPosition(glm::vec3 hand, glm::vec3 orbit, float age)
{
	const float flown = std::clamp(age / k_WispFlightSeconds, 0.0f, 1.0f);
	return hand + (orbit - hand) * flown;
}

uint8_t maths::WispAlpha(float age)
{
	const float risen = std::clamp(age / k_WispFadeInSeconds, 0.0f, 1.0f);
	return static_cast<uint8_t>(static_cast<int32_t>(risen * k_WispAlphaRise + k_WispLeastAlpha));
}

float maths::WispsDue(float due, int most, float dt, float emitSeconds)
{
	const auto mostDue = static_cast<float>(most);
	if (!(due < mostDue))
	{
		return due;
	}
	return std::min(due + mostDue * dt / emitSeconds, mostDue);
}

glm::vec3 maths::ItchOrbit(glm::vec3 rightEye, glm::vec3 leftEye, float seconds, float orbitSpeed)
{
	const auto centre = (rightEye + leftEye) * 0.5f;
	const auto out = (rightEye - centre) * k_ItchReach;
	glm::vec3 across(out.z, 0.0f, -out.x);
	const float acrossLength = glm::length(across);
	if (acrossLength > 0.0f)
	{
		across *= glm::length(out) / acrossLength;
	}
	const float angle = std::fmod(seconds * orbitSpeed, k_TwoPi);
	return centre + out * std::cos(angle) + across * std::sin(angle);
}

bool maths::SwungTooFar(glm::vec2 began, glm::vec2 now, float limit)
{
	const float a = glm::length(began);
	const float b = glm::length(now);
	if (!(a > 0.0f) || !(b > 0.0f))
	{
		return false;
	}
	return glm::dot(began / a, now / b) < std::cos(limit);
}

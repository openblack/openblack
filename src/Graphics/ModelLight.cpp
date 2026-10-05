/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ModelLight.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>

using namespace openblack;

namespace
{
constexpr float k_HandLift = 10.0f;
/// Darker than this, the light moves to the hand
constexpr float k_NightSkyType = 0.5f;
constexpr float k_DistanceFromHand = 3.0f;
} // namespace

glm::vec3 model_light::FrameLight(glm::vec3 hand, float groundUnderHand, const glm::vec3& camera, float skyType)
{
	if (!(skyType < k_NightSkyType))
	{
		return k_Sun;
	}
	hand.y = std::max(hand.y, groundUnderHand + k_HandLift);
	const auto toCamera = camera - hand;
	const auto lengthSquared = glm::dot(toCamera, toCamera);
	const auto direction = lengthSquared > 0.0f ? toCamera / std::sqrt(lengthSquared) : toCamera;
	return hand + direction * k_DistanceFromHand;
}

glm::vec4 model_light::Uniform(const glm::vec3& light, float ambient)
{
	return {light, ambient};
}

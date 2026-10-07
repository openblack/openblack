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

glm::vec3 model_light::FrameLight(glm::vec3 hand, float groundUnderHand, const glm::vec3& camera, float skyType, bool inTemple)
{
	if (inTemple || !(skyType < k_NightSkyType))
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

glm::vec3 model_light::LocalDirection(const glm::vec3& light, const glm::vec3& axisX, const glm::vec3& axisY,
                                      const glm::vec3& axisZ, const glm::vec3& origin)
{
	const auto toLight = light - origin;
	const auto row0 = glm::cross(axisY, axisZ);
	const auto row1 = glm::cross(axisZ, axisX);
	const auto row2 = glm::cross(axisX, axisY);
	const float determinant = glm::dot(axisX, row0);
	auto local = glm::vec3(glm::dot(row0, toLight), glm::dot(row1, toLight), glm::dot(row2, toLight));
	local *= determinant < 0.0f ? -1.0f : 1.0f;
	const float lengthSquared = glm::dot(local, local);
	return lengthSquared > 0.0f ? local / std::sqrt(lengthSquared) : glm::vec3(0.0f);
}

float model_light::Factor(const glm::vec3& normal, const glm::vec3& localLight, float ambient)
{
	// Rounded to the nearest whole number, halves to even, as the game's FPU stores it
	const float intensity = std::nearbyint(255.0f * glm::dot(normal, localLight));
	return intensity < 0.0f ? ambient : ambient + std::floor((255.0f - ambient) * intensity / 256.0f);
}

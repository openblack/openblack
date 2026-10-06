/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "HandShadow.h"

#include <cmath>

#include <algorithm>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

namespace openblack::graphics
{

namespace
{
/// The sun's height above the horizon at sunrise and sunset and at midday
constexpr float k_HorizonElevation = glm::radians(10.0f);
constexpr float k_MiddayElevation = glm::radians(60.0f);
/// Black & White's light sits at (-500000, 500000, -500000): the midday sun comes from that side
const glm::vec3 k_MiddaySunSide = glm::normalize(glm::vec3(-1.0f, 0.0f, -1.0f));
/// The bones are joints, the fingertips reach past the outermost ones
constexpr float k_BoneSpreadToRadius = 1.4f;
} // namespace

glm::vec3 HandShadow::SunlightDirection(float hours, const SkyInterface::DayNightTimes& times)
{
	const auto sunrise = times.duskStart;
	const auto sunset = 24.0f - times.duskStart;
	const auto day = std::clamp((hours - sunrise) / std::max(sunset - sunrise, 0.001f), 0.0f, 1.0f);

	// Over the day the sun turns half way round the sky, through the midday side
	const auto turn = (day - 0.5f) * glm::pi<float>();
	const auto side = glm::vec3((k_MiddaySunSide.x * std::cos(turn)) - (k_MiddaySunSide.z * std::sin(turn)), 0.0f,
	                            (k_MiddaySunSide.x * std::sin(turn)) + (k_MiddaySunSide.z * std::cos(turn)));
	const auto elevation = k_HorizonElevation + ((k_MiddayElevation - k_HorizonElevation) * std::sin(day * glm::pi<float>()));
	const auto towardsSun = side * std::cos(elevation) + glm::vec3(0.0f, std::sin(elevation), 0.0f);
	return -glm::normalize(towardsSun);
}

float HandShadow::Daylight(float hours, const SkyInterface::DayNightTimes& times)
{
	// The sky's times are those of the morning, the evening mirrors them around midday
	const auto morning = hours <= 12.0f ? hours : 24.0f - hours;
	return std::clamp((morning - times.duskStart) / std::max(times.dayFull - times.duskStart, 0.001f), 0.0f, 1.0f);
}

float HandShadow::DistanceFade(float cameraDistance, float handRadius)
{
	if (handRadius <= 0.0f)
	{
		return 0.0f;
	}
	const auto radii = cameraDistance / handRadius;
	if (radii < k_FadeStart)
	{
		return 1.0f;
	}
	if (radii <= k_FadeEnd)
	{
		return 1.0f - ((radii - k_FadeStart) / (k_FadeEnd - k_FadeStart));
	}
	return 0.0f;
}

std::optional<HandShadow> HandShadow::Compute(const std::vector<glm::mat4>& handBones, glm::vec3 cameraPosition,
                                              float groundHeight, float hours, const SkyInterface::DayNightTimes& times,
                                              bool originBottomLeft, bool homogeneousDepth)
{
	if (handBones.empty())
	{
		return std::nullopt;
	}

	// Bounding sphere of the hand
	auto minimum = glm::vec3(handBones.front()[3]);
	auto maximum = minimum;
	for (const auto& bone : handBones)
	{
		minimum = glm::min(minimum, glm::vec3(bone[3]));
		maximum = glm::max(maximum, glm::vec3(bone[3]));
	}
	const auto centre = (minimum + maximum) * 0.5f;
	float spread = 0.0f;
	for (const auto& bone : handBones)
	{
		spread = std::max(spread, glm::distance(centre, glm::vec3(bone[3])));
	}
	const auto radius = std::max(spread * k_BoneSpreadToRadius, 0.001f);

	const auto ground = glm::vec3(centre.x, groundHeight, centre.z);
	const auto strength = Daylight(hours, times) * DistanceFade(glm::distance(cameraPosition, ground), radius);
	if (strength <= 0.0f)
	{
		return std::nullopt;
	}

	HandShadow shadow {};
	shadow.lightDirection = SunlightDirection(hours, times);
	shadow.centre = centre;
	shadow.radius = radius;
	shadow.strength = strength;
	shadow.startDepth = -0.5f * radius;

	// Look down the sunlight at the hand, fitting it in the silhouette texture
	const auto up = std::abs(shadow.lightDirection.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
	const auto eye = centre - shadow.lightDirection * (radius * 2.0f);
	shadow.view = glm::lookAtRH(eye, centre, up);
	shadow.projection = homogeneousDepth ? glm::orthoRH_NO(-radius, radius, -radius, radius, 0.0f, radius * 4.0f)
	                                     : glm::orthoRH_ZO(-radius, radius, -radius, radius, 0.0f, radius * 4.0f);

	// From clip space to texture coordinates, which run downwards on some renderers
	auto bias = glm::mat4(1.0f);
	bias[0][0] = 0.5f;
	bias[1][1] = originBottomLeft ? 0.5f : -0.5f;
	bias[3][0] = 0.5f;
	bias[3][1] = 0.5f;
	shadow.receiverMatrix = bias * shadow.projection * shadow.view;
	// Distance past the hand's centre along the light
	for (glm::length_t column = 0; column < 3; ++column)
	{
		shadow.receiverMatrix[column][2] = shadow.lightDirection[column];
	}
	shadow.receiverMatrix[3][2] = -glm::dot(centre, shadow.lightDirection);
	return shadow;
}

} // namespace openblack::graphics

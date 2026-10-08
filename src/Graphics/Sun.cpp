/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Sun.h"

#include <cmath>

#include <algorithm>

using namespace openblack::graphics;

std::optional<sun::Placement> sun::Place(float scriptHour)
{
	if (scriptHour < 3.0f || scriptHour > 21.0f)
	{
		return std::nullopt;
	}
	float alpha = 255.0f;
	if (scriptHour < 6.0f)
	{
		alpha = (scriptHour - 3.0f) * 85.0f;
	}
	else if (scriptHour > 18.0f)
	{
		alpha = 255.0f - ((scriptHour - 18.0f) * 85.0f);
	}
	if (alpha <= 0.0f)
	{
		return std::nullopt;
	}
	const float height = 7500.0f * (std::clamp(std::min(scriptHour, 24.0f - scriptHour), 6.0f, 12.0f) - 6.0f) / 6.0f;
	return Placement {.position = {-30000.0f, height, -30000.0f}, .alpha = alpha};
}

std::array<glm::vec3, 5> sun::GlareSamples(glm::vec3 sun, glm::vec3 camera, float near)
{
	auto towards = sun - camera;
	if (towards.x != 0.0f || towards.y != 0.0f || towards.z != 0.0f)
	{
		towards *= near / std::sqrt(towards.x * towards.x + towards.y * towards.y + towards.z * towards.z);
	}
	std::array<glm::vec3, 5> samples {};
	for (size_t i = 0; i < samples.size(); ++i)
	{
		samples[i] = sun + glm::vec3(k_GlareSamples[i].x, k_GlareSamples[i].y, 0.0f) + towards;
		samples[i].y = std::max(samples[i].y, k_GlareLowestSample);
	}
	return samples;
}

float sun::GlareHidingDepth(float near)
{
	// Hidden where the depth buffer's 16 bits, rounded from (1 - near / depth) * 65535, are below 65000
	constexpr float k_Hidden = 64999.5f / 65535.0f;
	return near / (1.0f - k_Hidden);
}

float sun::EaseGlare(float glare, int hiddenSamples, uint32_t frameMilliseconds)
{
	const float target = (1.0f - (0.2f * static_cast<float>(hiddenSamples))) * 255.0f;
	const float step = static_cast<float>(frameMilliseconds) * 0.01f;
	return std::clamp(((target - glare) * step) + glare, 0.0f, 255.0f);
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CameraShake.h"

#include <algorithm>

#include <glm/geometric.hpp>

using namespace openblack;
using namespace openblack::camera_shake;

void camera_shake::Advance(std::vector<Shake>& shakes, float milliseconds)
{
	std::erase_if(shakes, [milliseconds](Shake& shake) {
		shake.millisecondsLeft -= milliseconds;
		return shake.millisecondsLeft <= 0.0f;
	});
}

float camera_shake::Amplitude(std::span<const Shake> shakes, const glm::vec3& camera)
{
	const Shake* nearest = nullptr;
	float best = 0.0f;
	for (const auto& shake : shakes)
	{
		const float distance = glm::distance(shake.position, camera);
		if (nearest == nullptr || distance < best)
		{
			nearest = &shake;
			best = distance;
		}
	}
	if (nearest == nullptr || best > nearest->radius || nearest->milliseconds <= 0.0f)
	{
		return 0.0f;
	}
	return nearest->strength * std::max(nearest->millisecondsLeft, 0.0f) / nearest->milliseconds;
}

Offsets camera_shake::Jitter(float amplitude, const std::function<float(float, float)>& random)
{
	if (amplitude <= 0.0f)
	{
		return {};
	}
	const float eye = random(-amplitude, amplitude);
	const float focus = random(-amplitude, amplitude);
	return {.eye = eye, .focus = focus};
}

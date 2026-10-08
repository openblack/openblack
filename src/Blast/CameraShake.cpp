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
#include <ranges>

#include <glm/geometric.hpp>

using namespace openblack;
using namespace openblack::camera_shake;

void camera_shake::Advance(std::vector<Shake>& shakes, float milliseconds)
{
	std::erase_if(shakes, [milliseconds](Shake& shake) {
		shake.millisecondsLeft -= milliseconds;
		return shake.millisecondsLeft < 1.0f;
	});
}

const Shake* camera_shake::Nearest(std::span<const Shake> shakes, const glm::vec3& camera)
{
	// The newest shake is met first, and only a strictly nearer one replaces the nearest so far
	const Shake* nearest = nullptr;
	float best = 0.0f;
	for (const auto& shake : std::ranges::reverse_view(shakes))
	{
		const float distance = glm::distance(shake.position, camera);
		if (nearest == nullptr || distance < best)
		{
			nearest = &shake;
			best = distance;
		}
	}
	return nearest;
}

float camera_shake::Amplitude(std::span<const Shake> shakes, const glm::vec3& camera)
{
	const Shake* nearest = Nearest(shakes, camera);
	const float best = nearest != nullptr ? glm::distance(nearest->position, camera) : 0.0f;
	// Only a camera strictly inside the shake's radius shakes
	if (nearest == nullptr || best >= nearest->radius || nearest->milliseconds <= 0.0f)
	{
		return 0.0f;
	}
	return nearest->strength * std::max(nearest->millisecondsLeft, 0.0f) / nearest->milliseconds;
}

Offsets camera_shake::Jitter(float amplitude, bool verticalOnly, const std::function<float(float, float)>& random)
{
	if (amplitude <= 0.0f)
	{
		return {};
	}
	if (verticalOnly)
	{
		const float eye = random(-amplitude, amplitude);
		const float focus = random(-amplitude, amplitude);
		return {.eye = {0.0f, eye, 0.0f}, .focus = {0.0f, focus, 0.0f}};
	}
	Offsets offsets;
	offsets.eye.z = random(-amplitude, amplitude);
	offsets.eye.y = random(-amplitude, amplitude);
	offsets.eye.x = random(-amplitude, amplitude);
	offsets.focus.z = random(-amplitude, amplitude);
	offsets.focus.y = random(-amplitude, amplitude);
	offsets.focus.x = random(-amplitude, amplitude);
	return offsets;
}

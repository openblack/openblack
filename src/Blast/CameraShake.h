/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <functional>
#include <optional>
#include <span>
#include <vector>

#include <glm/vec3.hpp>

/// The camera shaking as a blast lands near it: each shake has a place, a radius it reaches, a strength and a time it
/// lasts. Only the nearest shake counts, at full strength anywhere within its radius and none beyond, its strength
/// falling steadily to nothing over its time. The camera's eye and what it looks at each jump up or down by a fresh
/// random amount up to that strength every frame.
namespace openblack::camera_shake
{

struct Shake
{
	glm::vec3 position {0.0f};
	float radius {0.0f};
	float strength {1.0f};
	float milliseconds {0.0f};
	float millisecondsLeft {0.0f};
};

/// The shakes count down by the frame's time; those finished go
void Advance(std::vector<Shake>& shakes, float milliseconds);
/// How hard the camera shakes where it is: by the nearest shake, when within its radius
[[nodiscard]] float Amplitude(std::span<const Shake> shakes, const glm::vec3& camera);

/// How far the eye and what it looks at jump up this frame
struct Offsets
{
	float eye {0.0f};
	float focus {0.0f};
};
/// random(a, b) draws a number between them
[[nodiscard]] Offsets Jitter(float amplitude, const std::function<float(float, float)>& random);

} // namespace openblack::camera_shake

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

/// The game lights its models with a single point light and an ambient level, per vertex and in whole numbers: a vertex
/// facing the light at an intensity I = round(255 n.l) has its colour times f / 256, where f is the ambient when it
/// faces away and ambient + (255 - ambient) I / 256 when it faces it. The light is a distant sun by day; in the darker
/// half of the night it moves next to the god hand, a little towards the camera. assets/shaders/model_light.sh is the
/// shader side.
namespace openblack::model_light
{

/// The sun, far off at the same height as it is away along -x and -z
constexpr glm::vec3 k_Sun {-500000.0f, 500000.0f, -500000.0f};
/// The ambient level, of 255
constexpr float k_Ambient = 90.0f;

/// Where the light is this frame. `skyType` runs from 0 at night to 2 by day: at night the hand carries the light, never
/// below 10 over the ground under it. Inside the temple the world's night doesn't reach, and the light stays the sun.
[[nodiscard]] glm::vec3 FrameLight(glm::vec3 hand, float groundUnderHand, const glm::vec3& camera, float skyType,
                                   bool inTemple);

/// The uniform the object shaders read: the light's position, and the ambient
[[nodiscard]] glm::vec4 Uniform(const glm::vec3& light, float ambient = k_Ambient);

} // namespace openblack::model_light

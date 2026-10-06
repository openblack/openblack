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

namespace openblack::tree_brightness
{

/// The game's trees take the land's light scaled down by a brightness worked out once a frame, of 256: 200, rising to
/// 255 as the camera looks away from the model light, by how far its facing on the ground lines up with the light's
/// direction past what it looks at. Trees with the sun behind the camera are lit brightest.
[[nodiscard]] int Factor(const glm::vec3& cameraFocus, const glm::vec3& cameraForward, const glm::vec3& light);

} // namespace openblack::tree_brightness

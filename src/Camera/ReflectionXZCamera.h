/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Camera.h"

namespace openblack
{
// Reflection of distance 0 about the y-axis
class ReflectionXZCamera final: public Camera
{
public:
	ReflectionXZCamera();
	[[nodiscard]] glm::mat4 GetViewMatrix(Interpolation interpolation) const final;
};
} // namespace openblack

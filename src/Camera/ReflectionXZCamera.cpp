/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ReflectionXZCamera.h"

#include <glm/gtc/constants.hpp>

using namespace openblack;

ReflectionXZCamera::ReflectionXZCamera()
    : Camera(glm::zero<glm::vec3>())
{
}

glm::mat4 ReflectionXZCamera::GetViewMatrix() const
{
	return Camera::GetViewMatrix() *
	       // Reflection matrix when the normal is (0, 1, 0)
	       glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

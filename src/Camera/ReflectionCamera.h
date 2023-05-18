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
class ReflectionCamera: public Camera
{
public:
	ReflectionCamera()
	    : ReflectionCamera(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec4(0.0f))
	{
	}
	ReflectionCamera(glm::vec3 position, glm::vec3 rotation, glm::vec4 reflectionPlane)
	    : Camera(position, rotation)
	    , _reflectionPlane(reflectionPlane)
	{
	}
	[[nodiscard]] glm::mat4 GetViewMatrix() const override;

private:
	glm::vec4 _reflectionPlane;
	void ReflectMatrix(glm::mat4& m, const glm::vec4& plane) const;
};
} // namespace openblack

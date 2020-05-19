/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace openblack
{

struct Transform
{
	glm::vec3 position;
	glm::mat3 rotation;
	glm::vec3 scale;

	explicit operator glm::mat4() const
	{
		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, position);
		modelMatrix *= glm::mat4(rotation);
		modelMatrix = glm::scale(modelMatrix, scale);
		return modelMatrix;
	}
};

} // namespace openblack

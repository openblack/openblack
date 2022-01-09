/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <glm/glm.hpp>

namespace openblack::ecs::components
{

struct Transform
{
	glm::vec3 position;
	glm::mat3 rotation;
	glm::vec3 scale;
};

} // namespace openblack::ecs::components

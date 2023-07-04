/*******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/mat4x4.hpp>

namespace openblack
{
	struct Extent2D
	{
		glm::vec2 minimum;
		glm::vec2 maximum;
	};
    struct IndexExtent
	{
	    glm::u16vec2 minimum;
	    glm::u16vec2 maximum;
	};
}

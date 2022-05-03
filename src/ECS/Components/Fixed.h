/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <glm/vec2.hpp>

namespace openblack::ecs::components
{

struct Fixed
{
	Fixed(const glm::vec2& boundingCenter, float boundingRadius)
	    : boundingCenter(boundingCenter)
	    , boundingRadius(boundingRadius)
	{
	}

	glm::vec2 boundingCenter;
	float boundingRadius;
};

} // namespace openblack::ecs::components

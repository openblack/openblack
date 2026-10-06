/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ObjectShadows.h"

#include <algorithm>

#include <glm/vec2.hpp>

namespace openblack::graphics
{

glm::vec3 ObjectShadows::Project(glm::vec3 point, glm::vec3 origin)
{
	// vs_object_shadow_instanced does the same
	const auto relative = point - origin;
	const auto height = std::max(relative.y, 0.0f);
	constexpr auto k_SunAcross = glm::vec2(k_Sun.x, k_Sun.z);
	// The line from the sun through the point meets the plane height / (sun height - height) of the way back past the
	// point again. Taken as an offset from the point, as the sun is too far away to subtract its position precisely.
	const auto across = glm::vec2(relative.x, relative.z);
	const auto cast = across + (height / (k_Sun.y - height)) * (across - k_SunAcross);
	return origin + glm::vec3(cast.x, 0.0f, cast.y);
}

} // namespace openblack::graphics

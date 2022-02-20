/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <glm/vec3.hpp>
#include <glm/vector_relational.hpp>

namespace openblack
{

struct AxisAlignedBoundingBox
{
	glm::vec3 minima;
	glm::vec3 maxima;

	[[nodiscard]] inline glm::vec3 Center() const { return (maxima + minima) * 0.5f; }
	[[nodiscard]] inline glm::vec3 Size() const { return maxima - minima; }
	[[nodiscard]] inline bool Contains(const glm::vec3& point) const
	{
		return glm::all(glm::greaterThanEqual(point, minima) && glm::lessThanEqual(point, maxima));
	}
};

} // namespace openblack

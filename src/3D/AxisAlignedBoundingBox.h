/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <glm/vec3.hpp>

namespace openblack
{

struct AxisAlignedBoundingBox
{
	glm::vec3 minima;
	glm::vec3 maxima;

	[[nodiscard]] inline glm::vec3 center() const { return (maxima + minima) * 0.5f; }
	[[nodiscard]] inline glm::vec3 size() const { return maxima - minima; }
};

} // namespace openblack

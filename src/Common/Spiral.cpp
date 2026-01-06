/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Spiral.h"

#include <array>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace openblack::utils::spiral
{

constexpr std::array<glm::i16vec2, 4> k_Points = {
    glm::i16vec2 {1, 0},
    glm::i16vec2 {0, 1},
    glm::i16vec2 {-1, 0},
    glm::i16vec2 {0, -1},
};

const glm::i16vec2& Spiral(int& index, int& subIndex)
{
	--subIndex;
	if (subIndex == 0)
	{
		++index;
		subIndex = index / 2;
	}

	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-constant-array-index)
	return k_Points[index % k_Points.size()];
}

int GetIncrementSpiralSizeFromRadius(float radius, float step)
{
	const auto iVar1 = 1 - static_cast<int>(-2.0f * radius / step);
	return iVar1 * iVar1;
}

void SpiralIncrement(glm::vec3& point, int& index, int& subIndex, float size)
{
	const auto spiralPoint = Spiral(index, subIndex);
	point += size * glm::vec3(spiralPoint.x, 0.0f, spiralPoint.y);
}

int GetMapCellSpiralSizeFromRadius(float radius)
{
	const int fifth = static_cast<int>(radius / 5.0f);
	if (fifth == 0)
	{
		return 1;
	}
	return fifth * fifth;
}

} // namespace openblack::utils::spiral

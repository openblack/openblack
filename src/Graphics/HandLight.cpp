/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "HandLight.h"

#include <cmath>

#include <algorithm>
#include <utility>

#include <glm/common.hpp>

namespace openblack::graphics
{

glm::vec2 HandLight::GetOrigin(glm::vec3 handPosition)
{
	constexpr auto k_HalfWidth = static_cast<float>(k_Size - 1) * k_Spacing * 0.5f;
	return glm::vec2(handPosition.x, handPosition.z) - k_HalfWidth;
}

float HandLight::GetStrength(uint32_t landColour)
{
	// The mean is a whole number, rounded down
	const auto mean =
	    static_cast<int32_t>(((landColour >> 16) & 0xFFu) + ((landColour >> 8) & 0xFFu) + (landColour & 0xFFu)) / 3;
	if (mean >= 120)
	{
		return 0.0f;
	}
	return std::clamp(static_cast<float>(120 - mean) * (1.0f / 15.0f), 0.0f, 1.0f);
}

float HandLight::GetBrightness(std::span<const uint8_t> map, glm::vec3 handPosition, glm::vec2 point)
{
	if (map.size() < static_cast<size_t>(k_Size) * k_Size)
	{
		return 0.0f;
	}
	// fs_terrain samples the map the same way, between the brightnesses of the four vertices around the point
	const auto cell = (point - GetOrigin(handPosition)) / k_Spacing;
	const auto at = [&map](int row, int column) {
		if (row < 0 || column < 0 || std::cmp_greater_equal(row, k_Size) || std::cmp_greater_equal(column, k_Size))
		{
			return 0.0f;
		}
		return static_cast<float>(map[(static_cast<size_t>(row) * k_Size) + static_cast<size_t>(column)]) / 255.0f;
	};
	const auto row = static_cast<int>(std::floor(cell.x));
	const auto column = static_cast<int>(std::floor(cell.y));
	const auto fraction = cell - glm::floor(cell);
	const auto first = at(row, column) + ((at(row, column + 1) - at(row, column)) * fraction.y);
	const auto second = at(row + 1, column) + ((at(row + 1, column + 1) - at(row + 1, column)) * fraction.y);
	return first + ((second - first) * fraction.x);
}

} // namespace openblack::graphics

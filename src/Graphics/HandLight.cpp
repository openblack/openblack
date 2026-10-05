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

} // namespace openblack::graphics

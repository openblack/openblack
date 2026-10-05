/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "HandWaterGlow.h"

#include <algorithm>

using namespace openblack::graphics;

namespace
{
/// The map's last cell along each side
constexpr int k_LastCell = 511;
} // namespace

uint32_t hand_water_glow::Colour(uint32_t warmColour, float strength)
{
	const auto towards = [warmColour](uint32_t shift, uint32_t target) {
		const uint32_t c = (warmColour >> shift) & 0xFFu;
		return ((3 * c + target) >> 2) & 0xFFu;
	};
	// The alpha is truncated towards zero
	const auto alpha = static_cast<uint32_t>(
	    std::clamp(static_cast<int>(strength * static_cast<float>(k_MaxAlpha)), 0, static_cast<int>(k_MaxAlpha)));
	return alpha << 24 | towards(16, 255) << 16 | towards(8, 128) << 8 | towards(0, 64);
}

bool hand_water_glow::NearLowLand(glm::vec2 xz, const AltitudeAt& altitudeAt)
{
	// Truncated towards zero, then divided rounding towards zero
	const auto cellOf = [](float v) { return static_cast<int>(v) / 10; };
	const int x0 = std::clamp(cellOf(xz.x - k_Reach), 0, k_LastCell);
	const int z0 = std::clamp(cellOf(xz.y - k_Reach), 0, k_LastCell);
	const int x1 = cellOf(xz.x + k_Reach);
	const int z1 = cellOf(xz.y + k_Reach);
	for (int z = z0; z <= z1; ++z)
	{
		for (int x = x0; x <= x1; ++x)
		{
			const auto altitude = altitudeAt(x, z);
			if (!altitude.has_value() || *altitude < k_LowAltitude)
			{
				return true;
			}
		}
	}
	return false;
}

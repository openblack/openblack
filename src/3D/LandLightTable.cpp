/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LandLightTable.h"

#include <algorithm>
#include <stdexcept>

using namespace openblack;

namespace
{
constexpr size_t k_PaletteSide = LandLightPalette::k_Side;
constexpr uint32_t k_DarkLevels = 48;
/// The ramps divide by this rather than 255
constexpr uint32_t k_RampDivisor = 200;

// The haze's distances as their inverses, exactly as the game's floats: 400 and 900 from the camera, drawn in by dusk
// to 100 and 800
constexpr float k_NearInverse = 0x1.47AE14p-9f;     // 0.0025
constexpr float k_NearInverseDusk = 0x1.EB851Ep-8f; // 0.0075
constexpr float k_FarInverse = 0x1.234568p-10f;     // 1 / 900
constexpr float k_FarInverseDusk = 0x1.234560p-13f; // 0.00013888883

enum Row : size_t
{
	k_Good = 0,
	k_Neutral = 1,
	k_Evil = 2,
	k_Dark = 3,
	k_Warm = 6,
	k_RowCount = 8,
};

/// Each channel of a towards b by t of 256, in whole steps; the alpha of b
uint32_t Lerp(uint32_t a, uint32_t b, uint32_t t)
{
	const uint32_t r = (((((b & 0xFF0000u) - (a & 0xFF0000u)) * t) >> 8) + (a & 0xFFFF0000u)) & 0xFF0000u;
	const uint32_t g = (((((b & 0xFF00u) - (a & 0xFF00u)) * t) >> 8) + (a & 0xFFFFFF00u)) & 0xFF00u;
	const uint32_t bl = (((((b & 0xFFu) - (a & 0xFFu)) * t) >> 8) + a) & 0xFFu;
	return r | g | bl | (b & 0xFF000000u);
}

/// Each channel (a (255 - t) + b t) / 200, at most 255; the alpha of a
uint32_t Ramp(uint32_t a, uint32_t b, uint32_t t)
{
	uint32_t result = a & 0xFF000000u;
	for (const uint32_t shift : {16u, 8u, 0u})
	{
		const uint32_t value = (((a >> shift) & 0xFFu) * (255 - t) + ((b >> shift) & 0xFFu) * t) / k_RampDivisor;
		result |= std::min(255u, value) << shift;
	}
	return result;
}
} // namespace

LandLightPalette::LandLightPalette(std::span<const uint8_t> bytes)
{
	if (bytes.size() != k_Side * k_Side * 4)
	{
		throw std::runtime_error("The land's light palette isn't 32 by 32 colours");
	}
	_colours.resize(k_Side * k_Side);
	for (size_t i = 0; i < _colours.size(); ++i)
	{
		const auto texel = bytes.subspan(i * 4, 4);
		_colours.at(i) = static_cast<uint32_t>(texel[0]) << 16 | static_cast<uint32_t>(texel[1]) << 8 |
		                 static_cast<uint32_t>(texel[2]) | static_cast<uint32_t>(texel[3]) << 24;
	}
}

void LandLightTable::Build(const LandLightPalette& palette, float skyType, float alignment) noexcept
{
	// The palette's columns: the time of day from midnight to noon, and the alignment from good to evil
	const float timeColumn = std::clamp(skyType, 0.0f, 2.0f) * 15.0f;
	const float evil = std::clamp(1.0f - alignment, 0.0f, 2.0f);
	const float alignmentColumn = evil * 15.0f;

	std::array<uint32_t, k_RowCount> colours {};
	for (size_t row = 0; row < colours.size(); ++row)
	{
		const float column = row <= k_Evil ? timeColumn : alignmentColumn;
		const auto index = std::min(static_cast<size_t>(column), k_PaletteSide - 2);
		const auto t = static_cast<uint32_t>((column - static_cast<float>(index)) * 256.0f);
		colours.at(row) = Lerp(palette.At(row, index), palette.At(row, index + 1), t);
	}

	// The land's colour, from good through neutral to evil
	const auto towardsEvil = static_cast<int32_t>(evil * 255.0f);
	const auto land = evil < 1.0f ? Lerp(colours[k_Good], colours[k_Neutral], static_cast<uint32_t>(towardsEvil))
	                              : Lerp(colours[k_Neutral], colours[k_Evil], static_cast<uint32_t>(towardsEvil - 256));

	// The haze: a third of the land's colour, k by its brightness, and its distances drawn in at dusk
	{
		const uint32_t r = (land >> 16) & 0xFFu;
		const uint32_t g = (land >> 8) & 0xFFu;
		const uint32_t b = land & 0xFFu;
		_haze.k = static_cast<float>(std::min(255u, (r + 4 * g + 3 * b) / 8 + 8));
		_haze.colour = glm::vec3(static_cast<float>(r / 3), static_cast<float>(g / 3), static_cast<float>(b / 3));
		// 0 by day and at night, 1 at dusk
		const float dusk = std::clamp(skyType < 1.0f ? skyType : 2.0f - skyType, 0.0f, 1.0f);
		const float duskSquared = dusk * dusk;
		float nearInverse = k_NearInverse;
		float farInverse = k_FarInverse;
		if (duskSquared > 0.0f)
		{
			nearInverse = duskSquared * k_NearInverseDusk + k_NearInverse;
			farInverse = duskSquared * k_FarInverseDusk + k_FarInverse;
		}
		_haze.nearDistance = 1.0f / nearInverse;
		_haze.farDistance = 1.0f / farInverse;
	}

	std::array<uint32_t, k_Size> table {};
	// The darkest levels reach the land's colour at a level by its green, then go on to the warm colour
	const uint32_t landLevel = (((land >> 8) & 0xFFu) * k_DarkLevels) >> 8;
	for (uint32_t i = 0; i < landLevel; ++i)
	{
		table.at(i) = Ramp(colours[k_Dark], land, (i * 256) / landLevel);
	}
	for (uint32_t i = landLevel; i < k_DarkLevels; ++i)
	{
		table.at(i) = Ramp(land, colours[k_Warm], ((i - landLevel) * 256) / (k_DarkLevels - landLevel));
	}
	for (uint32_t i = k_DarkLevels; i < k_Size; ++i)
	{
		table.at(i) = Ramp(colours[k_Dark], land, i);
	}

	for (size_t i = 0; i < k_Size; ++i)
	{
		const uint32_t c = table.at(i);
		_texels.at(i) = ((c >> 16) & 0xFFu) | (c & 0xFF00u) | ((c & 0xFFu) << 16) | 0xFF000000u;
	}
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "VillageLights.h"

#include <bit>

namespace openblack::village_lights
{

namespace
{
constexpr float k_On = 16.5f;
constexpr float k_Off = 7.0f;
constexpr float k_Ramp = 1.0f;
/// The game's single precision 1/255 and 1/30
constexpr float k_InverseByte = std::bit_cast<float>(0x3B808081u);
constexpr float k_InverseFlicker = std::bit_cast<float>(0x3D088889u);
} // namespace

bool IsDark(uint32_t landColour)
{
	const auto mean = (((landColour >> 16u) & 0xFFu) + ((landColour >> 8u) & 0xFFu) + (landColour & 0xFFu)) / 3u;
	return mean < 120u;
}

float Intensity(float scriptHour)
{
	const double hour = scriptHour;
	if (hour > k_On)
	{
		return k_On + k_Ramp < hour ? 255.0f : static_cast<float>((hour - k_On) * 255.0 / k_Ramp);
	}
	if (hour > k_Off)
	{
		return 0.0f;
	}
	return k_Off - k_Ramp > hour ? 255.0f : static_cast<float>((k_Off - hour) * 255.0 / k_Ramp);
}

int32_t Strength(float intensity)
{
	return static_cast<int32_t>(static_cast<double>(intensity) * 255.0);
}

int32_t VillageStrength(float intensity)
{
	// The brightness is taken to 0 to 1 in single precision first
	return Strength(static_cast<float>(static_cast<double>(intensity) * k_InverseByte));
}

Placement Place(glm::vec2 xz)
{
	// A cell is 10 units; each axis gives its cell, rounded towards zero and then down, and the weight of the next texel
	const auto axis = [](float position, int32_t& cell, int32_t& weight) {
		const double cells = static_cast<double>(position) * static_cast<double>(0.1f);
		cell = static_cast<int32_t>(cells);
		if (cells >= 0.0)
		{
			weight = static_cast<int32_t>(255.0 - (cells - cell) * 255.0);
		}
		else
		{
			weight = static_cast<int32_t>((cell - cells) * 255.0);
			--cell;
		}
		weight &= 0xFF;
	};
	Placement placement {};
	axis(xz.x, placement.cell.x, placement.weight.x);
	axis(xz.y, placement.cell.y, placement.weight.y);
	return placement;
}

FlickerStep AdvanceFlicker(float timer, float milliseconds)
{
	timer = static_cast<float>(static_cast<double>(milliseconds) + timer);
	if (timer <= k_FlickerMilliseconds)
	{
		return {.timer = timer, .flickers = false};
	}
	const auto periods = static_cast<int32_t>(static_cast<double>(timer) * k_InverseFlicker);
	timer = static_cast<float>(timer - static_cast<double>(periods) * k_FlickerMilliseconds);
	return {.timer = timer, .flickers = true};
}

int32_t Threshold(uint8_t fullLightGreen)
{
	return (static_cast<int32_t>(fullLightGreen) * k_WarmLevels) >> 8;
}

} // namespace openblack::village_lights

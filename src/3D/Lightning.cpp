/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Lightning.h"

#include <algorithm>

namespace openblack::lightning
{

Flash Strike(const glm::vec3& position, float radius, float strength)
{
	return {.position = position, .radius = radius, .strength = strength, .time = 0.0f, .active = true};
}

Flash Advance(Flash flash, float seconds)
{
	flash.time += seconds;
	if (flash.active && flash.time > k_FlashSeconds)
	{
		flash.active = false;
	}
	return flash;
}

float Brightness(const Flash& flash)
{
	if (!flash.active)
	{
		return 0.0f;
	}
	float brightness = 1.0f - flash.time;
	if (flash.time < 0.5f && flash.time > 0.1f)
	{
		brightness = 0.1f;
	}
	return brightness * flash.strength;
}

uint8_t LandLightFlash(float brightness)
{
	return static_cast<uint8_t>(static_cast<int32_t>(std::clamp(brightness, 0.0f, 1.0f) * 255.0f));
}

} // namespace openblack::lightning

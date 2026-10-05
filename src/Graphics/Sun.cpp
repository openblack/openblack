/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Sun.h"

#include <algorithm>

using namespace openblack::graphics;

std::optional<sun::Placement> sun::Place(float scriptHour)
{
	if (scriptHour < 3.0f || scriptHour > 21.0f)
	{
		return std::nullopt;
	}
	float alpha = 255.0f;
	if (scriptHour < 6.0f)
	{
		alpha = (scriptHour - 3.0f) * 85.0f;
	}
	else if (scriptHour > 18.0f)
	{
		alpha = 255.0f - ((scriptHour - 18.0f) * 85.0f);
	}
	if (alpha <= 0.0f)
	{
		return std::nullopt;
	}
	const float height = 7500.0f * (std::clamp(std::min(scriptHour, 24.0f - scriptHour), 6.0f, 12.0f) - 6.0f) / 6.0f;
	return Placement {.position = {-30000.0f, height, -30000.0f}, .alpha = alpha};
}

float sun::EaseGlare(float glare, int hiddenSamples, uint32_t frameMilliseconds)
{
	const float target = (1.0f - (0.2f * static_cast<float>(hiddenSamples))) * 255.0f;
	const float step = static_cast<float>(frameMilliseconds) * 0.01f;
	return std::clamp(((target - glare) * step) + glare, 0.0f, 255.0f);
}

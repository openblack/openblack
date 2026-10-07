/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ParticleSoundRelease.h"

#include <algorithm>

using namespace openblack::particles;

SoundLetGo openblack::particles::LetGoOf(bool softRelease, bool sampleLoops, int fadeStep)
{
	// A sound the file doesn't soft-release is stopped at once, looping or not, whatever its fade step
	if (!softRelease)
	{
		return {.stopNow = true};
	}
	// Otherwise a loop plays to the end of its pass and a sample that plays once plays out, fading as it goes when it has
	// a step
	const int step = std::max(fadeStep, 0);
	return {.releaseLoop = sampleLoops, .fadeStep = step, .stopWhenSilent = step > 0};
}

uint32_t openblack::particles::FadedVolume(uint32_t volume, int fadeStep)
{
	if (fadeStep <= 0)
	{
		return volume;
	}
	const auto step = static_cast<uint32_t>(fadeStep);
	return volume > step ? volume - step : 0;
}

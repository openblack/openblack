/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SkyDome.h"

#include <cmath>

#include <algorithm>

namespace openblack::sky_dome
{

namespace
{
constexpr uint8_t k_Night = 0;
constexpr uint8_t k_Dusk = 1;
constexpr uint8_t k_Day = 2;

/// A fraction of 255, cut to a whole number
uint8_t Weight(float fraction)
{
	return static_cast<uint8_t>(static_cast<int>(fraction * 255.0f));
}
} // namespace

Follow::Follow(float skyType)
    : _wholeDome(skyType)
{
}

FrameRows Follow::Advance(float skyType)
{
	FrameRows frame;
	if (_wholeDome)
	{
		frame.rows.at(frame.count++) = {.skyType = *_wholeDome, .first = 0, .count = k_Rows};
		_wholeDome.reset();
	}
	if (_rowsDone >= k_Rows)
	{
		if (std::abs(static_cast<double>(skyType - _following)) <= k_Hysteresis)
		{
			return frame;
		}
		_following = skyType;
		_rowsDone = 0;
	}
	frame.rows.at(frame.count++) = {.skyType = _following, .first = _rowsDone, .count = k_RowsPerFrame};
	_rowsDone += k_RowsPerFrame;
	return frame;
}

void Follow::Jump(float skyType)
{
	_following = skyType;
	_rowsDone = 0;
	_wholeDome = skyType;
}

Pair TimePair(float skyType)
{
	// Counted from day, as the dome's blend takes it
	const float fromDay = 2.0f - skyType;
	if (fromDay > 1.0f)
	{
		return {.lower = k_Dusk, .upper = k_Night, .weight = Weight(fromDay - 1.0f)};
	}
	return {.lower = k_Day, .upper = k_Dusk, .weight = Weight(fromDay)};
}

Pair AlignmentPair(float alignment)
{
	if (alignment > 1.0f)
	{
		return {.lower = 1, .upper = 2, .weight = Weight(std::min(alignment, 2.0f) - 1.0f)};
	}
	return {.lower = 0, .upper = 1, .weight = Weight(std::max(alignment, 0.0f))};
}

uint8_t BlendChannel(uint8_t lower, uint8_t upper, uint8_t weight)
{
	return static_cast<uint8_t>(((lower * (255 - weight)) >> 8) + ((upper * weight) >> 8));
}

uint8_t Darkness(float alignment)
{
	const float evil = 1.0f - (alignment * 0.5f);
	if (!(evil > 0.6f))
	{
		return 0;
	}
	const auto darkness = static_cast<int>(static_cast<double>(evil - 0.6f) * 225.0);
	return static_cast<uint8_t>(std::clamp(darkness, 0, 90));
}

Tint TintOf(const TintInputs& inputs)
{
	const int overcast = static_cast<int>(std::clamp(inputs.overcast, 0.0f, 1.0f) * 255.0f);
	// Both colours lose this much of 256 under an evil sky, rounded up
	const int dark = inputs.weather ? (inputs.darkness * inputs.darkness) / 150 : 0;
	const int halfFlash = inputs.flash / 2;
	Tint tint;
	for (glm::length_t c = 0; c < 3; ++c)
	{
		int modulate = 255;
		int add = 0;
		if (inputs.fog)
		{
			// From white towards the haze's colour, and the haze's colour added, by the overcast
			const int haze = static_cast<int>(inputs.hazeColour[c]);
			modulate = 255 + (((haze - 255) * overcast) >> 8);
			add = (haze * overcast) >> 8;
		}
		modulate += (-modulate * dark) >> 8;
		add += (-add * dark) >> 8;
		// A flash takes the first towards white, and the second half as far
		modulate += ((255 - modulate) * inputs.flash) >> 8;
		add += ((255 - add) * halfFlash) >> 8;
		tint.modulate[c] = static_cast<uint8_t>(modulate);
		tint.add[c] = static_cast<uint8_t>(add);
	}
	return tint;
}

} // namespace openblack::sky_dome

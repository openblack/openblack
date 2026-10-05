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

} // namespace openblack::sky_dome

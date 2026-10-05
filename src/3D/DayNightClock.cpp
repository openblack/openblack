/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "DayNightClock.h"

#include <cmath>

#include <algorithm>

using namespace openblack;

namespace
{
/// A turn is a tenth of a second of game time
constexpr float k_TurnSeconds = 0.1f;
/// How many hours a second the visual time moves at most towards a new hour, outside a move
constexpr float k_DefaultStep = 2.5f;

float WrapHours(float hour)
{
	while (hour < 0.0f)
	{
		hour += 24.0f;
	}
	while (hour >= 24.0f)
	{
		hour -= 24.0f;
	}
	return hour;
}

/// Stretches the first half of the day so the hours `from` fall on `to`, between them evenly; the second half mirrors
/// the first about noon
float MapHours(float hour, const std::array<float, 4>& from, const std::array<float, 4>& to)
{
	const bool afternoon = hour > 12.0f;
	if (afternoon)
	{
		hour = 24.0f - hour;
	}
	float fromLow = 0.0f;
	float toLow = 0.0f;
	float fromHigh = 12.0f;
	float toHigh = 12.0f;
	size_t i = 0;
	while (i < from.size() && hour >= from.at(i))
	{
		++i;
	}
	if (i > 0)
	{
		fromLow = from.at(i - 1);
		toLow = to.at(i - 1);
	}
	if (i < from.size())
	{
		fromHigh = from.at(i);
		toHigh = to.at(i);
	}
	const float mapped = ((hour - fromLow) / (fromHigh - fromLow) * (toHigh - toLow)) + toLow;
	return afternoon ? 24.0f - mapped : mapped;
}
} // namespace

void DayNightClock::Reset()
{
	_scale = 1.0f;
	_moveSeconds = 0.0f;
	SetCycle(k_DefaultDuration, k_DefaultNight, k_DefaultChange);
	SetScriptTime(12.0f);
}

void DayNightClock::SetCycle(float duration, float night, float change)
{
	// The day is counted in whole tenths of a second, in an hour of the day, rounding down
	const auto tenths = static_cast<int>(duration * 0.41666666f);
	_dayRate = tenths != 0 ? 10.0f / static_cast<float>(tenths) : 0.0f;
	_nightRate = _dayRate;

	// The night lasts `night` of the half day and the change follows it; the sky turns a quarter of the change either
	// side of each, never longer than the night
	const float nightEnd = night * 12.0f;
	const float changeEnd = (change * 12.0f) + nightEnd;
	const float turning = std::min((changeEnd - nightEnd) * 0.25f, nightEnd);
	_times = {nightEnd - turning, turning + nightEnd, changeEnd - turning, changeEnd + turning};
}

void DayNightClock::SetCycleFromLand(float duration, float night, float change)
{
	night = std::min(night, 1.0f);
	change = std::min(change, 1.0f - night);
	SetCycle(duration, night, change);
}

void DayNightClock::SetScriptTime(float hour)
{
	_moveSeconds = 0.0f;
	SetTarget(ScriptToVisual(hour), 0.0f);
	_visualTime = _target;
}

void DayNightClock::MoveScriptTime(float hour, float seconds)
{
	SetTarget(ScriptToVisual(hour), seconds);
}

void DayNightClock::SetTarget(float hour, float seconds)
{
	if (hour < -1000.0f || hour > 1000.0f)
	{
		hour = 0.0f;
	}
	_target = WrapHours(hour);
	if (seconds != 0.0f)
	{
		_moveSeconds = seconds;
		float difference = std::abs(_visualTime - _target);
		if (difference > 12.0f)
		{
			difference -= 24.0f;
		}
		_step = std::abs(difference) / seconds;
	}
}

void DayNightClock::ProcessTurn()
{
	if (_moveSeconds != 0.0f)
	{
		// A move ends once the time has reached its hour
		if (_visualTime == _target)
		{
			_moveSeconds = 0.0f;
		}
	}
	else
	{
		// The time moves on by day's or night's rate, mixed by how far the sky has turned to night
		_step = k_DefaultStep;
		const float towardsNight = 2.0f - SkyType(_visualTime);
		const float rate = ((_nightRate - _dayRate) * towardsNight * 0.5f) + _dayRate;
		SetTarget(_visualTime + (rate * _scale * k_TurnSeconds), 0.0f);
	}

	// Towards the target the short way round, at most a turn's step
	float target = _target;
	if (_visualTime - target > 12.0f)
	{
		target += 24.0f;
	}
	if (_visualTime - target < -12.0f)
	{
		target -= 24.0f;
	}
	const float step = _step * k_TurnSeconds;
	if (target < _visualTime)
	{
		_visualTime = std::max(_visualTime - step, target);
	}
	else if (target > _visualTime)
	{
		_visualTime = std::min(_visualTime + step, target);
	}
	_visualTime = WrapHours(_visualTime);
}

float DayNightClock::SkyType(float visualHour) const
{
	const float hour = visualHour > 12.0f ? 24.0f - visualHour : visualHour;
	const auto& [nightFull, duskStart, duskEnd, dayFull] = _times;
	if (hour < nightFull)
	{
		return 0.0f;
	}
	if (hour < duskStart)
	{
		return (hour - nightFull) / (duskStart - nightFull);
	}
	if (hour < duskEnd)
	{
		return 1.0f;
	}
	if (hour < dayFull)
	{
		return 1.0f + ((hour - duskEnd) / (dayFull - duskEnd));
	}
	return 2.0f;
}

float DayNightClock::ScriptToVisual(float hour) const
{
	return MapHours(hour, k_ScriptTimes, _times);
}

float DayNightClock::VisualToScript(float hour) const
{
	return MapHours(hour, _times, k_ScriptTimes);
}

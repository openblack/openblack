/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CameraZoomer.h"

using namespace openblack::camera;

namespace
{
constexpr float k_Instant = 0.001f;
} // namespace

void Zoomer::SetPosition(float value)
{
	*this = Zoomer {};
	_value = value;
	_destination = value;
	_startValue = value;
}

void Zoomer::SetDestination(float destination, float speed, float seconds)
{
	if (seconds < k_Instant)
	{
		SetPosition(destination);
		return;
	}
	_startSpeed = _speed;
	_startValue = _value;
	_destination = destination;
	_destinationSpeed = speed;
	_duration = seconds;
	_time = 0.0f;
	// Where it must be, how fast and no longer accelerating when it arrives
	const float t = seconds;
	const float d = destination - _startValue - (t * _startSpeed);
	const float e = speed - _startSpeed;
	_c1 = 6.0f * ((2.0f * d) - (t * e)) / (t * t);
	_c2 = 6.0f * ((5.0f * t * e) - (8.0f * d)) / (t * t * t);
	_c3 = 24.0f * ((3.0f * d) - (2.0f * t * e)) / (t * t * t * t);
}

void Zoomer::Update(float seconds)
{
	_time += seconds;
	if (!(_time < _duration))
	{
		_value = _destination;
		_speed = _destinationSpeed;
		_time = _duration;
		return;
	}
	const float t = _time;
	const float half = t * t * 0.5f;
	const float sixth = half * t * (1.0f / 3.0f);
	const float twentyFourth = half * half * (1.0f / 6.0f);
	_speed = _startSpeed + (_c1 * t) + (_c2 * half) + (_c3 * sixth);
	_value = _startValue + (_startSpeed * t) + (_c1 * half) + (_c2 * sixth) + (_c3 * twentyFourth);
}

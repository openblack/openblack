/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Zoomer.h"

#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

using namespace openblack;

namespace
{
constexpr float k_MinimumDuration = 0.001f;
}

void Zoomer::Reset(float value)
{
	_value = value;
	_destination = value;
	_startValue = value;
	_speed = 0.0f;
	_destinationSpeed = 0.0f;
	_startSpeed = 0.0f;
	_elapsed = 0.0f;
	_duration = 0.0f;
	_coefficients = glm::vec3(0.0f);
}

void Zoomer::SetDestination(float destination, float seconds, float speed)
{
	if (seconds < k_MinimumDuration)
	{
		Reset(destination);
		return;
	}
	_startValue = _value;
	_startSpeed = _speed;
	_destination = destination;
	_destinationSpeed = speed;
	_duration = seconds;
	_elapsed = 0.0f;

	// The value and speed it is to have at the end, and the last term of its path, which is to be nothing there
	const float t = seconds;
	const float t2 = t * t * 0.5f;
	const float t3 = t * t2 / 3.0f;
	const float t4 = t2 * t2 / 6.0f;
	const glm::mat3 equations(glm::vec3(t2, t, 1.0f), glm::vec3(t3, t2, t), glm::vec3(t4, t3, t2));
	const glm::vec3 ends {destination - _startValue - (t * _startSpeed), speed - _startSpeed, 0.0f};
	_coefficients = glm::inverse(equations) * ends;
}

void Zoomer::Update(float deltaSeconds)
{
	_elapsed += deltaSeconds;
	if (_duration <= _elapsed)
	{
		_value = _destination;
		_speed = _destinationSpeed;
		_elapsed = _duration;
		return;
	}
	const float t = _elapsed;
	const float t2 = t * t * 0.5f;
	const float t3 = t * t2 / 3.0f;
	_speed = _startSpeed + (t * _coefficients.x) + (t2 * _coefficients.y) + (t3 * _coefficients.z);
	_value =
	    _startValue + (t * _startSpeed) + (t2 * _coefficients.x) + (t3 * _coefficients.y) + (t2 * t2 / 6.0f * _coefficients.z);
}

Zoomer3::Zoomer3(glm::vec3 point)
    : x(point.x)
    , y(point.y)
    , z(point.z)
{
}

void Zoomer3::Reset(glm::vec3 point)
{
	x.Reset(point.x);
	y.Reset(point.y);
	z.Reset(point.z);
}

void Zoomer3::SetDestination(glm::vec3 destination, float seconds)
{
	x.SetDestination(destination.x, seconds);
	y.SetDestination(destination.y, seconds);
	z.SetDestination(destination.z, seconds);
}

void Zoomer3::Update(float deltaSeconds)
{
	x.Update(deltaSeconds);
	y.Update(deltaSeconds);
	z.Update(deltaSeconds);
}

glm::vec3 Zoomer3::GetValue() const
{
	return {x.GetValue(), y.GetValue(), z.GetValue()};
}

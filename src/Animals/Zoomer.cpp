/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Zoomer.h"

using namespace openblack::animals;

namespace
{
/// A glide shorter than this is over at once
constexpr float k_Instant = 0.001f;
} // namespace

void Zoomer::SetTarget(float target, float targetSpeed, float seconds)
{
	if (seconds < k_Instant)
	{
		*this = Zoomer(target);
		return;
	}
	_start = _value;
	_startSpeed = _speed;
	_target = target;
	_targetSpeed = targetSpeed;
	_duration = seconds;
	_elapsed = 0.0f;
	// The coefficients that bring the curve to the target at the target's speed and without acceleration at the end:
	// the three conditions solved by Cramer's rule
	const double t = seconds;
	const double h = t * t * 0.5;
	const double c = h * t / 3.0;
	const double q = h * h / 6.0;
	const double position = static_cast<double>(target) - _start - (static_cast<double>(_startSpeed) * t);
	const double velocity = static_cast<double>(targetSpeed) - _startSpeed;
	// c4 q + c3 c + c2 h = position, c4 c + c3 h + c2 t = velocity, c4 h + c3 t + c2 = 0
	const auto det = [](double a, double b, double cc, double d, double e, double f, double g, double hh, double i) {
		return (a * ((e * i) - (f * hh))) - (b * ((d * i) - (f * g))) + (cc * ((d * hh) - (e * g)));
	};
	const double d = det(q, c, h, c, h, t, h, t, 1.0);
	_c4 = static_cast<float>(det(position, c, h, velocity, h, t, 0.0, t, 1.0) / d);
	_c3 = static_cast<float>(det(q, position, h, c, velocity, t, h, 0.0, 1.0) / d);
	_c2 = static_cast<float>(det(q, c, position, c, h, velocity, h, t, 0.0) / d);
}

float Zoomer::Step(float seconds)
{
	_elapsed += seconds;
	if (_elapsed < _duration)
	{
		const float t = _elapsed;
		const float h = t * t * 0.5f;
		const float c = t * h * (1.0f / 3.0f);
		_speed = _startSpeed + (_c2 * t) + (_c3 * h) + (_c4 * c);
		_value = _start + (_startSpeed * t) + (_c2 * h) + (_c3 * c) + (_c4 * (h * h * (1.0f / 6.0f)));
	}
	else
	{
		_value = _target;
		_speed = _targetSpeed;
		_elapsed = _duration;
	}
	return _value;
}

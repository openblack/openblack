/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ZoomInterpolator.h"

namespace openblack
{

/// Vanilla's Zoomer: a value that eases to a destination over a set time, setting off from its current value and speed
/// so that it can be given a new destination every frame.
class Zoomer
{
public:
	explicit Zoomer(float value = 0.0f)
	    : _curve(value)
	    , _value(value)
	{
	}

	/// Zoomer::SetPosition
	void Reset(float value)
	{
		_curve = ZoomInterpolator<float>(value);
		_value = value;
		_speed = 0.0f;
		_elapsed = 0.0f;
		_duration = 0.0f;
	}

	/// Zoomer::SetDestinationWithSpeedAndTime, arriving at rest
	void SetDestination(float destination, float seconds)
	{
		if (seconds < k_MinimumDuration)
		{
			Reset(destination);
			return;
		}
		// ZoomInterpolator's velocities are per unit of its 0 to 1 interpolation factor
		_curve = ZoomInterpolator<float>(_value, destination, _speed * seconds, 0.0f);
		_elapsed = 0.0f;
		_duration = seconds;
	}

	/// Zoomer::Update
	void Update(float deltaSeconds)
	{
		if (_duration <= 0.0f)
		{
			return;
		}
		_elapsed += deltaSeconds;
		if (_elapsed >= _duration)
		{
			Reset(_curve.p1);
			return;
		}
		const auto t = _elapsed / _duration;
		_value = _curve.PositionAt(t);
		_speed = _curve.VelocityAt(t) / _duration;
	}

	[[nodiscard]] float GetValue() const { return _value; }

private:
	static constexpr float k_MinimumDuration = 0.001f;

	ZoomInterpolator<float> _curve;
	float _value;
	float _speed = 0.0f;
	float _elapsed = 0.0f;
	float _duration = 0.0f;
};

} // namespace openblack

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

namespace openblack::animals
{

/// A value that glides to a target over a time, as the game eases a bird into its bank or a miracle's animal out of
/// sight: it sets off from where it is at the speed it has, and arrives at the target's speed with no acceleration,
/// along a curve of the fourth degree in time
class Zoomer
{
public:
	explicit Zoomer(float value = 0.0f)
	    : _value(value)
	    , _target(value)
	    , _start(value)
	{
	}

	/// Sets off towards a target, arriving at a speed (per second) after so many seconds; under a thousandth of a second
	/// it is there at once, at rest
	void SetTarget(float target, float targetSpeed, float seconds);
	/// The value after a further step of so many seconds
	float Step(float seconds);

	[[nodiscard]] float Value() const { return _value; }
	[[nodiscard]] float Speed() const { return _speed; }
	[[nodiscard]] float Target() const { return _target; }
	/// It has reached its target
	[[nodiscard]] bool Done() const { return !(_elapsed < _duration); }

private:
	float _value {0.0f};
	float _speed {0.0f};
	float _target {0.0f};
	float _targetSpeed {0.0f};
	float _elapsed {0.0f};
	float _duration {0.0f};
	float _start {0.0f};
	float _startSpeed {0.0f};
	/// The curve's coefficients of t^2/2, t^3/6 and t^4/24
	float _c2 {0.0f};
	float _c3 {0.0f};
	float _c4 {0.0f};
};

} // namespace openblack::animals

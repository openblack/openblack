/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

// One coordinate of the camera gliding to where it is sent, as the game's camera paths move it: on a curve of the
// fourth degree in time that starts from where it is at the speed it has, and arrives at the destination at the speed
// asked, no longer speeding up or slowing down, after the time given. Sent somewhere new, it starts afresh from where it
// is. Pure, tested without the game.

namespace openblack::camera
{

class Zoomer
{
public:
	/// Puts it at a value, still
	void SetPosition(float value);
	/// Sends it to a destination, to arrive at a speed after a number of seconds; in under a thousandth of a second it
	/// is there at once, still
	void SetDestination(float destination, float speed, float seconds);
	/// It moves on by some seconds
	void Update(float seconds);

	[[nodiscard]] float Value() const { return _value; }
	[[nodiscard]] float Speed() const { return _speed; }

private:
	float _value {0.0f};
	float _speed {0.0f};
	float _destination {0.0f};
	float _destinationSpeed {0.0f};
	float _time {0.0f};
	float _duration {0.0f};
	float _startValue {0.0f};
	float _startSpeed {0.0f};
	/// The acceleration at the start, and how it changes and how that changes
	float _c1 {0.0f};
	float _c2 {0.0f};
	float _c3 {0.0f};
};

} // namespace openblack::camera

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec3.hpp>

namespace openblack
{

/// A value that eases to a destination over a set time, as the game's do, setting off from its current value and speed
/// so that it can be given a new destination every frame. Its path is a quartic in time that arrives at the destination
/// at the destination's speed, its last term ending at nothing, and its speed is kept as the game keeps it.
class Zoomer
{
public:
	explicit Zoomer(float value = 0.0f) { Reset(value); }

	/// At value, still
	void Reset(float value);
	/// Eases to destination over seconds, arriving at speed. Under a millisecond puts it there.
	void SetDestination(float destination, float seconds, float speed = 0.0f);
	/// Moves the value on along its path
	void Update(float deltaSeconds);

	[[nodiscard]] float GetValue() const { return _value; }
	[[nodiscard]] float GetSpeed() const { return _speed; }
	[[nodiscard]] float GetDestination() const { return _destination; }

private:
	float _value {0.0f};
	float _speed {0.0f};
	float _destination {0.0f};
	float _destinationSpeed {0.0f};
	float _startValue {0.0f};
	float _startSpeed {0.0f};
	float _elapsed {0.0f};
	float _duration {0.0f};
	/// The path's coefficients of t^2/2, t^3/6 and t^4/24 for the value, and of t, t^2/2 and t^3/6 for the speed
	glm::vec3 _coefficients {0.0f};
};

/// A point of three zoomers
struct Zoomer3
{
	Zoomer x;
	Zoomer y;
	Zoomer z;

	explicit Zoomer3(glm::vec3 point = glm::vec3(0.0f));
	void Reset(glm::vec3 point);
	void SetDestination(glm::vec3 destination, float seconds);
	void Update(float deltaSeconds);
	[[nodiscard]] glm::vec3 GetValue() const;
};

} // namespace openblack

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TribalPowerSpin.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include <glm/geometric.hpp>

using namespace openblack::magic::tribal_spin;

namespace
{
/// The ring round the hand: its radius swells slowly about this, by this much, at this rate in radians a second
constexpr float k_RingRadius = 1.3f;
constexpr float k_RingSwell = 0.5f;
constexpr float k_SwellRate = 0.1f;
/// It flies in from the camera over this many seconds, and its letters close up from trailing by a sample over two
constexpr float k_FlyInSeconds = 1.0f;
constexpr float k_TrailClosing = 0.5f;
/// The rising column: its letters trail by this many samples, it turns by this times its age squared, it is fully seen
/// for three seconds and then fades by 256 a second out of 255, and it is gone after this many seconds
constexpr float k_ColumnTrail = 0.8f;
constexpr float k_ColumnTurn = 1.4f;
constexpr float k_ColumnFadeFrom = 3.0f;
constexpr float k_ColumnFadeRate = 256.0f;
constexpr float k_ColumnSeconds = 4.8f;
/// A ring's axis this close to the x axis is squared up against the z axis instead
constexpr float k_AlongX = 0.01f;

glm::vec3 UnitOrSame(glm::vec3 v)
{
	const float length = glm::length(v);
	return length != 0.0f ? v / length : v;
}

/// The ring's radius for its age
float RingRadius(float age)
{
	return std::sin(age * k_SwellRate) * k_RingSwell + k_RingRadius;
}
} // namespace

Spin::Spin(std::u16string_view text, glm::vec3 position)
    : _text(text)
{
	_samples[0] = Sample {.position = position, .axis = {0.0f, -1.0f, 0.0f}, .radius = 0.0f, .angle = 0.0f, .alpha = 0};
}

void Spin::Record(const Sample& now, float steps)
{
	_owed += steps;
	while (_owed >= 1.0f)
	{
		_owed -= 1.0f;
		_samples.at(static_cast<size_t>(_head)) = now;
		_head = (_head + 1) & static_cast<int>(k_SampleCount - 1);
		_count = std::min(_count + 1, static_cast<int>(k_SampleCount));
	}
}

std::vector<Letter> Spin::Letters() const
{
	std::vector<Letter> letters;
	if (_text.empty())
	{
		return letters;
	}
	const float spacing = 2.0f * std::numbers::pi_v<float> / static_cast<float>(_text.size());
	for (size_t i = 0; i < _text.size(); ++i)
	{
		const auto character = _text[i];
		if (character == u' ')
		{
			continue;
		}
		// Each letter is where the ring was some samples ago, between the last two samples
		float at = static_cast<float>(_head - 2) + _owed - static_cast<float>(i) * trail;
		if (!(static_cast<float>(_head - _count) < at))
		{
			continue;
		}
		while (at < 0.0f)
		{
			at += static_cast<float>(k_SampleCount);
		}
		const int index = static_cast<int>(at);
		const float t = at - static_cast<float>(index);
		const auto& a = _samples.at(static_cast<size_t>(index) & (k_SampleCount - 1));
		const auto& b = _samples.at(static_cast<size_t>(index + 1) & (k_SampleCount - 1));
		const float radius = (b.radius - a.radius) * t + a.radius;
		const float angle = (b.angle - a.angle) * t + a.angle - static_cast<float>(i) * spacing;
		const auto position = a.position + (b.position - a.position) * t;
		const auto axis = UnitOrSame(a.axis + (b.axis - a.axis) * t);
		const int alpha = static_cast<int>(static_cast<float>(b.alpha - a.alpha) * t + static_cast<float>(a.alpha));

		// Square the ring up round its axis
		glm::vec3 reference {1.0f, 0.0f, 0.0f};
		if (std::abs(axis.y) < k_AlongX && std::abs(axis.z) < k_AlongX)
		{
			reference = {0.0f, 0.0f, 1.0f};
		}
		const auto side = UnitOrSame(glm::cross(reference, axis));
		const auto other = down == 2 ? glm::cross(axis, side) : glm::cross(side, axis);
		const auto outward = side * std::cos(angle) + other * std::sin(angle);
		const auto along = other * std::cos(angle) - side * std::sin(angle);

		letters.push_back(Letter {
		    .character = character,
		    .origin = position - outward * radius,
		    .axes = {along, axis, outward},
		    .down = down,
		    .size = radius * k_LetterShare,
		    .alpha = static_cast<uint8_t>(alpha < 1 ? 0 : std::min(alpha, 255)),
		});
	}
	return letters;
}

Runner::Runner(std::u16string_view text, glm::vec3 position, glm::u8vec4 colour, bool held)
    : _spin(text, position)
    , _position(position)
    , _colour(colour)
    , _held(held)
{
}

void Runner::Release(glm::vec3 handPosition)
{
	_held = false;
	_age = 0.0f;
	_position = handPosition;
}

bool Runner::Update(const Frame& frame, float seconds)
{
	_age += seconds;
	const float steps = seconds * k_SamplesPerSecond;
	if (_held)
	{
		// Round the hand, half its height above where it is drawn from
		auto centre = frame.handPosition - frame.handZ * 0.5f;
		auto axis = frame.handZ;
		// Flying in from the camera over the first second, facing it
		if (_age < k_FlyInSeconds)
		{
			const auto view = UnitOrSame(frame.cameraFocus - frame.camera);
			const float remaining = 1.0f - _age * _age;
			centre += (frame.camera - centre) * remaining;
			axis += (view - frame.handZ) * remaining;
		}
		_spin.trail = std::max(1.0f - _age * k_TrailClosing, 0.0f);
		_spin.down = 2;
		_spin.Record({.position = centre, .axis = axis, .radius = RingRadius(_age), .angle = _age, .alpha = 255}, steps);
		return true;
	}

	// Rising from where the hand cast, widening and turning faster and faster, fading at the end
	_spin.trail = k_ColumnTrail;
	_spin.down = 1;
	int alpha = 255;
	if (_age > k_ColumnFadeFrom)
	{
		alpha = std::max(static_cast<int>(255.0f - (_age - k_ColumnFadeFrom) * k_ColumnFadeRate), 0);
	}
	_spin.Record({.position = _position + glm::vec3(0.0f, _age * _age * _age, 0.0f),
	              .axis = {0.0f, -1.0f, 0.0f},
	              .radius = _age * RingRadius(_age),
	              .angle = _age * _age * k_ColumnTurn,
	              .alpha = alpha},
	             steps);
	return _age <= k_ColumnSeconds;
}

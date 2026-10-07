/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

// A tribe's power behind a miracle, shown as the tribe's name ("Norse Power") spinning in the world. While the miracle is
// in the hand its name is a ring of letters round the hand, which flies in from the camera over the first second; when
// the miracle is cast the ring is let go and becomes a widening column of letters rising from where the hand was, which
// fades after three seconds. Each letter trails the one before through the recent path of the ring, kept as samples twenty
// times a second. Pure value types, tested on their own; MiracleFxSystem runs them and the renderer draws their letters.

namespace openblack::magic::tribal_spin
{

/// The ring keeps this many recent samples of where it was
inline constexpr size_t k_SampleCount = 64;
/// Samples taken a second
inline constexpr float k_SamplesPerSecond = 20.0f;
/// A letter is this share of the ring's radius high
inline constexpr float k_LetterShare = 0.6f;

/// Where the ring was at a sample
struct Sample
{
	glm::vec3 position {0.0f};
	glm::vec3 axis {0.0f, -1.0f, 0.0f};
	float radius {0.0f};
	float angle {0.0f};
	int alpha {0};
};

/// A letter to draw: laid out in a frame whose first axis it runs along and whose down axis it hangs down
struct Letter
{
	char16_t character {u' '};
	glm::vec3 origin {0.0f};
	/// The along, axis and outward directions
	std::array<glm::vec3, 3> axes {};
	/// Which axis the letter hangs down: the outward one round the hand, the ring's axis in the rising column
	int down {2};
	float size {0.0f};
	uint8_t alpha {0};
};

/// The spinning letters: the text, how far each letter trails the one before in samples, and the samples
class Spin
{
public:
	Spin(std::u16string_view text, glm::vec3 position);

	/// The ring is now at a place, turned about an axis, of a radius and turned round it by an angle, at an alpha; taken
	/// as samples steps times a second's worth of twenty
	void Record(const Sample& now, float steps);
	/// The letters as the samples have them now
	[[nodiscard]] std::vector<Letter> Letters() const;

	/// Which axis the letters hang down, and how far each trails the one before, in samples
	int down {2};
	float trail {0.2f};

private:
	std::u16string _text;
	std::array<Sample, k_SampleCount> _samples {};
	/// Where the next sample goes, and how many there are
	int _head {1};
	int _count {1};
	/// Steps owed towards the next sample
	float _owed {0.0f};
};

/// A tribe's name round the hand or rising from where it cast
class Runner
{
public:
	/// The name, where the column would rise from, the player's colour (alpha ignored), and whether it starts as the
	/// ring round the hand
	Runner(std::u16string_view text, glm::vec3 position, glm::u8vec4 colour, bool held);

	/// What the hand is like this frame: its position and its model's z axis (scaled with it), and the camera
	struct Frame
	{
		glm::vec3 handPosition {0.0f};
		glm::vec3 handZ {0.0f, -1.0f, 0.0f};
		glm::vec3 camera {0.0f};
		glm::vec3 cameraFocus {0.0f, 0.0f, 1.0f};
	};
	/// Some seconds of game time on; false once it is over
	[[nodiscard]] bool Update(const Frame& frame, float seconds);
	/// The ring is let go where the hand is: it rises as a column from there, starting its time again
	void Release(glm::vec3 handPosition);

	[[nodiscard]] bool Held() const { return _held; }
	[[nodiscard]] float Age() const { return _age; }
	[[nodiscard]] glm::u8vec4 Colour() const { return _colour; }
	[[nodiscard]] const Spin& GetSpin() const { return _spin; }

private:
	Spin _spin;
	glm::vec3 _position;
	glm::u8vec4 _colour;
	bool _held;
	float _age {0.0f};
};

} // namespace openblack::magic::tribal_spin

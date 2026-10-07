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
#include <optional>

#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// The small formulas the particle rules share, free of any state so they can be tested on their own
namespace openblack::particles::maths
{

/// The value a timed rule sets at an age: a straight line from `from` at `start` to `to` at `stop`, and `to` once more on
/// the first step past `stop` so a step that jumps over the end still lands on it. nullopt when the rule leaves the value
/// alone.
[[nodiscard]] std::optional<float> TimedValue(float age, float dt, float start, float stop, float from, float to);

/// The scale a particle is drawn at for its distance from the camera: the near scale closer than the near distance, the
/// far scale beyond the far distance, and a straight line between them
[[nodiscard]] float ScaleAtCameraDistance(float distance, float nearDistance, float farDistance, float nearScale,
                                          float farScale);

/// An alpha or colour channel as the game stores it from a float: truncated, and only its low byte kept
[[nodiscard]] uint8_t TruncateToByte(float value);

/// A colour tinted by a player's colour (0xRRGGBB): the neutral player's black counts as white, and a blend below 1 moves
/// the player's colour towards white, each channel 255 + ((c - 255) * b >> 8) with b the blend as a byte. Each channel of
/// the colour, alpha too, is then multiplied by the matching channel of that and shifted down a byte.
[[nodiscard]] std::array<uint8_t, 4> TintWithPlayerColour(std::array<uint8_t, 4> rgba, uint32_t playerRgb, float blend);

/// How far the burst under a heal chakra has faded in at an age: rising from 0 to 1 by ageMax, then back to 0 by
/// ageZero, held within 0..1
[[nodiscard]] float ChakraFade(float age, float ageMax, float ageZero);

/// How big a sound is by how far its effect reaches: 3 (small) under the small radius, 2 (medium) under the medium one,
/// else 1 (large)
[[nodiscard]] int SoundSizeFromRadius(float radius, float small, float medium);

/// The emitters' schedule: when the next atom is due and how many have been made
struct EmitterClock
{
	float next {0.0f};
	int emitted {0};
	bool started {false};
};
struct EmitterLimits
{
	/// Atoms a second
	float frequency {1.0f};
	/// At most this many alive in the collection before another is made, -1 for no limit
	int maxAlive {-1};
	/// At most this many made in all, -1 for no limit
	int maxTotal {-1};
	/// The gap between atoms varies between half and all of 1 / frequency
	bool randomise {true};
};
/// Whether an emitter makes an atom this step, and if so moves its schedule on. The first call starts the schedule at the
/// collection's age. random05 draws a random number up to 0.5, only called when the gap is randomised.
template <typename Random>
[[nodiscard]] bool ShouldEmit(EmitterClock& clock, const EmitterLimits& limits, float collectionAge, float dt, int alive,
                              Random&& random05)
{
	if (!clock.started)
	{
		clock.started = true;
		clock.next = collectionAge;
	}
	if (limits.maxTotal != -1 && clock.emitted >= limits.maxTotal)
	{
		return false;
	}
	if (!(clock.next < collectionAge + dt))
	{
		return false;
	}
	if (limits.maxAlive >= 0 && alive > limits.maxAlive)
	{
		return false;
	}
	float gap = 1.0f / limits.frequency;
	if (limits.randomise)
	{
		gap *= random05() + 0.5f;
	}
	++clock.emitted;
	clock.next += gap;
	return true;
}

/// An atom's animation frame over a step: the previous frame becomes the current one, then while playing the current one
/// moves on by dt * rate, both kept within [0, 2 * frames) by whole cycles. A paused atom neither moves nor wraps.
void AdvanceFrame(float& previous, float& current, float dt, float rate, int frames, bool playing);
/// The frame drawn between two steps, t of the way from the previous: t may run up to 5 steps ahead for a looping
/// animation, only to the current frame for one that plays once
[[nodiscard]] float LerpFrame(float previous, float current, float t, bool loop);
/// The whole frame drawn: wrapped into the animation when it loops, held at its ends when it plays once
[[nodiscard]] int FrameIndex(float frame, int frames, bool loop);

/// A sprite sheet cell's corner in texture space and its size: the sheet is cellsPerRow cells across, and the cell number
/// keeps only its low six bits
struct UvRect
{
	glm::vec2 corner;
	glm::vec2 size;
};
[[nodiscard]] UvRect SpriteCellUv(int cell, int cellsPerRow);

/// The game's rotations, in glm's column convention: its angles turn the other way to glm's.
/// A turn of a about the vertical
[[nodiscard]] glm::mat3 AngleY(float a);
/// Turns of x, then y, then z about the three axes
[[nodiscard]] glm::mat3 AngleXYZ(float x, float y, float z);
/// A rotation turned by a about one of the world axes (0 x, 1 y, 2 z), as a spinning rule turns its atoms
[[nodiscard]] glm::mat3 TurnAboutAxis(const glm::mat3& rotation, int axis, float a);

/// The roll of a sprite drawn from an atom's rotation: the angle its local x axis makes in the ground plane
[[nodiscard]] float SpriteAngle(const glm::mat3& rotation);
/// The roll that lines a sprite's top up with a velocity as the camera sees it, from the camera's right and up
[[nodiscard]] float ScreenVelocityAngle(const glm::vec3& velocity, const glm::vec3& right, const glm::vec3& up);

/// The particles' value noise: a lattice of 256 values in -1..1 fixed for every game, read through a permutation of
/// the lattice points and blended linearly between them
class ValueNoise
{
public:
	ValueNoise();
	/// About -1..1 at a point, the eight lattice values round it blended along x, then y, then z
	[[nodiscard]] float At(const glm::vec3& point) const;
	/// Two independent values for a gust of wind across the ground: the noise at (x, y, z) and at (y, z, x)
	[[nodiscard]] glm::vec2 Wind(const glm::vec3& point) const;
	[[nodiscard]] float Lattice(int x, int y, int z) const;
	/// About -1..1 along a line: a smooth curve through the lattice values at the whole numbers round x, each read
	/// through the permutation once, as the tornado's foot wanders
	[[nodiscard]] float Smooth(float x) const;
	/// About -1..1 along a line: the lattice's values through its permutation alone, blended linearly between them, as
	/// the fires' light flickers
	[[nodiscard]] float Line(float x) const;

private:
	std::array<float, 256> _values {};
};

} // namespace openblack::particles::maths

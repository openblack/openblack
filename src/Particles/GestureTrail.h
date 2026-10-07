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

#include <vector>

#include <glm/vec3.hpp>

/// The trail a recognised gesture leaves on the land: the path the hand drew, and the gesture's own shape laid over the
/// land where it was drawn. Its particles flow from the first to the second.
namespace openblack::particles
{

/// A path through points, walked by the distance along it
class TrailPath
{
public:
	TrailPath() = default;
	explicit TrailPath(std::vector<glm::vec3> points);

	[[nodiscard]] const std::vector<glm::vec3>& Points() const { return _points; }
	[[nodiscard]] size_t Size() const { return _points.size(); }
	/// Moves a point; the distances along the path are worked out again
	void SetPoint(size_t index, glm::vec3 point);
	/// The distance along it from its first point to its last, 0 for an empty path
	[[nodiscard]] float Length() const;
	/// The point a fraction of the way along it, between the points either side of that distance: the last point past
	/// its end. A path of fewer than three points always gives its first point, as the game's does.
	[[nodiscard]] glm::vec3 At(float fraction) const;

private:
	void Measure();

	std::vector<glm::vec3> _points;
	/// The distance along the path to each point
	std::vector<float> _distances;
};

/// How the trail's particles move and fade
namespace gesture_trail
{
/// The particles grow with the shape: this much for each unit of its length
inline constexpr float k_ScalePerLength = 0.01f;
/// The shape is raised towards the camera by its particles' size over how steeply the camera looks down on it, but by no
/// more than this share of the way to the camera
inline constexpr float k_MostLift = 0.5f;
/// The light sheet stands on this many points taken evenly along the shape, and its strength moves one point along this
/// often
inline constexpr int k_SheetPoints = 50;
inline constexpr float k_SheetShiftSeconds = 0.03f;
/// The particles flash and the hand glows this long after they appear, the flash dying away over this long
inline constexpr float k_FlashStart = 2.4f;
inline constexpr float k_FlashSeconds = 2.1f;
/// Particles carried round the shape go round it every this many seconds, and fade out over the last this many
inline constexpr float k_CircuitSeconds = 2.0f;
inline constexpr float k_CircuitFadeSeconds = 2.0f;
/// The wiggles across the shape's depth and upwards are read further along the noise than those across its width
inline constexpr float k_WiggleOffsetZ = 0.3f;
inline constexpr float k_WiggleOffsetY = 0.7f;

/// How far the particles have gone from the drawn path to the shape, 0 to 1, a time of 0 to 1 into the move: eased in and
/// out by the gain (none at 0, fully at 1)
[[nodiscard]] float Transition(float t, float gain);
/// A particle's alpha (0 to 255) a share of the way along the shape, the trail grown from both ends a share of the way
/// in: brightest at its ends, fading to nothing at the share
[[nodiscard]] uint8_t RevealAlpha(float along, float grown, float maxAlpha);
/// How far the particles wiggle at an age, 0 to 1: rising and falling with a phase, then shrinking to nothing over a time
/// once they disperse
[[nodiscard]] float WiggleAmount(float age, float phaseSpeed, float dispersalTime, float shrinkTime);
/// A point of the shape raised towards the camera, when the camera looks down on it, by a size over how steeply it looks
/// down, no more than half the way to the camera
[[nodiscard]] glm::vec3 Lift(glm::vec3 point, glm::vec3 camera, float size);
/// The light sheet's strength at an age, rising from nothing and falling back to it over its life
[[nodiscard]] float SheetStrength(float age, float lifetime);
/// What is left of a flash a time after it started, 1 when it starts down to 0 a duration later
[[nodiscard]] double FlashLeft(float since, float duration);
/// Each channel of a colour (0xAARRGGBB) times a level of 0 to 255, over 256
[[nodiscard]] uint32_t Dimmed(uint32_t argb, uint8_t level);
/// The chain behind a gesturing hand is sized by how far the hand is from the camera: a fifth of its size up close,
/// whole from 50 to 500 units away and half as big again from 1500 on, in straight lines between
[[nodiscard]] float ChainDistanceScale(float distance);
} // namespace gesture_trail

struct GestureTrail
{
	/// The path the hand drew, through the points of it that lay on the land
	TrailPath drawn;
	/// The gesture's shape laid over the land where it was drawn, through as many points
	TrailPath ideal;
};

} // namespace openblack::particles

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

#include <functional>
#include <vector>

#include <glm/vec3.hpp>

/// Black & White's clouds: 70 puffs of mist drifting with the wind along a track across the island, fading in and out
/// at its ends, with two huge ones pinned at its ends on the horizon.
namespace openblack::clouds
{

inline constexpr int k_Count = 70;
/// The track runs this far either side of its middle
inline constexpr float k_TrackHalfLength = 8000.0f;

/// A cloud as it is laid out: where it is on the track (x along it), its size and how much it shrinks edge on
struct Layout
{
	glm::vec3 track;
	float size;
	float edgeShrink;
	bool pinned;
};

/// A random number between two others
using Random = std::function<float(float low, float high)>;

/// The clouds of a new land: each at random along the track, 300 to 500 high and up to 5000 either side of it, 13 to
/// 50 times the mist's size and shrinking 2.5 to 5 times edge on; the first two then pinned at the track's ends,
/// 300 times the size and shrinking 20 times
[[nodiscard]] std::vector<Layout> MakeLayout(const Random& random);

/// Moves a cloud along the track by the milliseconds of game time, 70 units a second, back to its start past its end.
/// Pinned clouds stay.
[[nodiscard]] glm::vec3 Move(const glm::vec3& track, bool pinned, float milliseconds);

/// Where a point of the track is in the world: the track turned three eighths of a turn about the point 1280, 1280
[[nodiscard]] glm::vec3 WorldPosition(const glm::vec3& track);

/// How opaque a cloud is at its place on the track, 0 to 255: fading in over the first 2000 units and out over the
/// last. Pinned clouds are 192.
[[nodiscard]] int EdgeAlpha(const glm::vec3& track, bool pinned);

/// The clouds' colour, 0xAARRGGBB, for the sky's alignment (-1 evil to 1 good) and the land's light at full
/// luminosity (0xRRGGBB): clear and white for good, grey for neutral and dark orange for evil, in that light, each
/// channel then drawn a little towards 35
[[nodiscard]] uint32_t Colour(float skyAlignment, uint32_t fullLight);

} // namespace openblack::clouds

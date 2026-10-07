/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <functional>
#include <optional>
#include <span>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Gestures/GestureRecorder.h"
#include "Particles/GestureTrail.h"

/// The trail a recognised gesture leaves on the land, as the game lays it out the moment the gesture is recognised: the
/// points of the hand's path that lie on the land, and the gesture's own shape (its symbol, a path of points in a square
/// 200 units across) fitted to where the path was drawn on the screen and put on the land under it. Seen from a camera
/// low over the land, the shape stretches away into the distance, so it is squashed up the screen a quarter at a time
/// until it is no more than twice as deep as it is wide, or fifteen tries have been made.
namespace openblack::gesture
{

/// A recorded point lies on the land when it is further than this from the world's origin along an axis
inline constexpr double k_TrailOnLand = 1e-4;
/// The most the shape is squashed, and by how much each time
inline constexpr int k_TrailSquashTries = 15;
inline constexpr float k_TrailSquash = 0.75f;
/// The shape stops being squashed once no deeper across the land than this against its width
inline constexpr float k_TrailMostDepth = 2.0f;
/// A camera looking almost straight down measures depth along the world's x axis
inline constexpr float k_TrailLevelEpsilon = 1e-4f;
/// Where the screen shows no land, the shape's point is this far from the camera towards it
inline constexpr float k_TrailSkyDistance = 400.0f;

/// A gesture's symbol: its points put in a square 0 to 1 across (x to the right, z down the screen), as many again taken
/// evenly along its length, and the box round the points it was made from
struct TrailSymbol
{
	std::vector<glm::vec3> points;
	float minX {0.0f};
	float maxX {0.0f};
	float minZ {0.0f};
	float maxZ {0.0f};
};

/// The symbol from the points of its file: x and z from -100 to 100 put in 0 to 1, z turned to run down the screen
[[nodiscard]] TrailSymbol MakeTrailSymbol(std::span<const glm::vec3> filePoints);

/// The points of the hand's path that lie on the land, the oldest first
[[nodiscard]] std::vector<glm::vec3> TrailLandPoints(const GestureRecorder& recorder);

/// How the screen meets the land as the trail is laid out
struct TrailView
{
	/// Which way the camera looks
	glm::vec3 cameraForward {0.0f, 0.0f, 1.0f};
	/// The point on the land under a pixel of the screen, the land's height there; k_TrailSkyDistance from the camera
	/// towards the pixel where there is no land under it
	std::function<glm::vec3(glm::ivec2 pixel)> pointUnder;
};

/// The trail of a gesture whose path on the screen, in pixels, had a box, from the points of the path on the land; none
/// when fewer than two of them lie on the land
[[nodiscard]] std::optional<particles::GestureTrail> BuildTrail(std::span<const glm::vec3> landPoints,
                                                                const ScreenBox& boxInPixels, const TrailSymbol& symbol,
                                                                const TrailView& view);

} // namespace openblack::gesture

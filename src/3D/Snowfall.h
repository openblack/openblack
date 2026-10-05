/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <functional>
#include <optional>
#include <span>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// The snow the game draws falling: one set of 256 flakes over an 80 unit square, each a small square of the
/// atmosphere texture tumbling as it drifts down in circles. The set is repeated over every quarter of a land block
/// near the camera where it snows, more of its flakes and more opaque the more it snows there.
namespace openblack::snowfall
{

inline constexpr size_t k_Flakes = 256;
/// The square the flakes fall over, and a quarter of a land block, which is how far apart the sets are drawn
inline constexpr float k_Span = 80.0f;
/// The flakes fall to this far under the ground before they start again at the top
inline constexpr float k_Bottom = -20.0f;
/// Half a flake's side
inline constexpr float k_HalfSize = 0.3f;
/// The cell of the atmosphere texture a flake shows, its corners in the order of Corners
inline constexpr std::array<glm::vec2, 4> k_Uvs = {glm::vec2(0.12890625f, 0.00390625f), glm::vec2(0.12890625f, 0.12109375f),
                                                   glm::vec2(0.24609375f, 0.12109375f), glm::vec2(0.24609375f, 0.00390625f)};

/// Draws a number between two, as the game's random numbers for the snow
using Random = std::function<float(float, float)>;

struct Flake
{
	glm::vec3 position {0.0f};
	/// How fast it falls, before the snow's fall speed
	float fall {0.0f};
	/// It drifts round in a circle: how fast, the way it is heading and how far round the circle it is
	float swaySpeed {0.0f};
	float heading {0.0f};
	float sway {0.0f};
	/// How it is turned, and how fast it turns, about each axis
	glm::vec3 spin {0.0f};
	glm::vec3 spinSpeed {0.0f};
};

/// A flake at a new place, anywhere up to the snow's height, in the order the game draws its numbers
[[nodiscard]] Flake Place(const Random& random, float height);
/// The flakes when the snow starts: each at a new place, at a height of its own
void Scatter(std::span<Flake> flakes, const Random& random, float height);
/// Moves every flake on by some seconds at the snow's fall speed: one that has fallen through the ground starts again
void Step(std::span<Flake> flakes, float seconds, float fallSpeed, float height, const Random& random);

/// A flake's four corners about the middle of its square, turned as it is
[[nodiscard]] std::array<glm::vec3, 4> Corners(const Flake& flake);

/// A quarter's snow: where its flakes fall about, how many of them are drawn and how opaque they are, 0 to 255
struct Tile
{
	glm::vec2 corner {0.0f};
	int32_t flakes {0};
	int32_t alpha {0};
	/// The ground's height under its corner, which its flakes fall to
	float ground {0.0f};

	bool operator==(const Tile&) const = default;
};
/// The snow over the quarter of a land block centred at `centre`, the most snow about it being `snowiest` percent: none
/// for a little snow or a quarter too far from the camera, and fainter from 50 units away
[[nodiscard]] std::optional<Tile> TileOf(glm::vec2 centre, int32_t snowiest, glm::vec2 camera);

} // namespace openblack::snowfall

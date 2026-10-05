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
#include <functional>
#include <optional>
#include <span>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// The rain the game draws: one set of 128 streaks, each a line from under the ground up to the rain's height with a
/// dashed row of the atmosphere texture scrolling down it. The set is repeated over every land block near the camera
/// where it rains, as dense and as opaque as the rain over the block's middle.
namespace openblack::rain
{

inline constexpr size_t k_Streaks = 128;
/// The streaks' bottom, under the ground
inline constexpr float k_Bottom = -50.0f;
/// The row of the atmosphere texture the streaks are drawn with
inline constexpr float k_TextureRow = 0.50390625f;

/// Draws a number between two, as the game's random numbers for the rain
using Random = std::function<float(float, float)>;

/// One streak, about the middle of a block
struct Streak
{
	/// From 0 to 1 over its life: it fades in over the first twentieth and out over the last
	float phase {0.0f};
	float x {0.0f};
	float z {0.0f};
	/// Its top's offset from its bottom: the slant
	float dx {0.0f};
	float dz {0.0f};
	/// The texture's position along it, which scrolls from 0 to 1
	float scroll {0.0f};
	/// How fast the texture scrolls, in turns a second before the rain's fall speed
	float speed {0.0f};
};

/// A streak at a new place, in the order the game draws its numbers
[[nodiscard]] Streak Place(const Random& random);
/// Moves every streak on by some seconds: the texture scrolls, and a streak whose life ends starts again elsewhere
void Step(std::span<Streak> streaks, float seconds, float fallSpeed, const Random& random);

/// How high the streaks reach and how fast they fall: they follow the nearest storm's, or the calm air's
struct Fall
{
	float height {160.0f};
	float speed {1.0f};

	bool operator==(const Fall&) const = default;
};
/// A frame's step towards a storm's fall, or the calm air's without one, kept in bounds
[[nodiscard]] Fall Follow(Fall current, std::optional<Fall> storm);

/// A block's rain: how many streaks are drawn over it and their opacity, 0 to 255, at the bottom and at the top
struct Tile
{
	glm::vec2 centre {0.0f};
	int32_t streaks {0};
	int32_t alpha {0};
	int32_t alphaTop {0};
	/// The ground's height under its centre, which its streaks stand on
	float ground {0.0f};

	bool operator==(const Tile&) const = default;
};
/// The rain over a block with its centre at `centre`, the most rain over its middle being `wettest` percent: none for
/// a little rain or a block too far from the camera, and less of it from 100 units away
[[nodiscard]] std::optional<Tile> TileOf(glm::vec2 centre, int32_t wettest, glm::vec2 camera);

/// A streak's opacity, from an opacity of the tile's, as it fades in and out over its life
[[nodiscard]] int32_t StreakAlpha(int32_t alpha, float phase);
/// A streak's two ends, about its tile's centre on the ground
[[nodiscard]] std::array<glm::vec3, 2> Ends(const Streak& streak, float height);

} // namespace openblack::rain

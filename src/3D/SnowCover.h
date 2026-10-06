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

#include <span>

#include <entt/core/hashed_string.hpp>
#include <glm/vec2.hpp>

/// Snow lying on the island: storms that snow pile it up on a grid of 40 unit cells, and it melts away slowly
/// everywhere. The land turns white where it is deep enough, raggedly by a noise image, and objects standing in it
/// show a snow texture over themselves, more of it the deeper it lies.
namespace openblack::snow_cover
{

/// The texture objects show where snow lies on them, with its alpha, and the noise image the land's snow is ragged by,
/// once across each land block
inline constexpr entt::hashed_string k_TextureId = entt::hashed_string("raw/SNOW");
inline constexpr entt::hashed_string k_AlphaTextureId = entt::hashed_string("raw/SNOWA");
inline constexpr entt::hashed_string k_NoiseTextureId = entt::hashed_string("raw/snowmap");
inline constexpr float k_NoiseSpan = 160.0f;

/// The grid's cells along each side, and their size
inline constexpr int32_t k_GridSize = 128;
inline constexpr size_t k_Cells = static_cast<size_t>(k_GridSize) * k_GridSize;
inline constexpr float k_CellSize = 40.0f;
/// The deepest the snow lies
inline constexpr float k_MaxDepth = 255.0f;

/// The snow a storm lays on each cell in a turn of 0.1 seconds, from its snow in percent and its strength, 0 to 1
[[nodiscard]] float StormSnowPerTurn(int8_t snow, float strength);

/// A storm snowing on the cells within its outer radius of its centre: as much as `amount` within its inner radius,
/// less beyond, and towards its edge it takes snow away. The depth stays between 0 and k_MaxDepth.
void AddStorm(std::span<float> grid, glm::vec2 centre, float innerRadius, float outerRadius, float amount);

/// The snow melts by 2 in one of eight bands of the grid's rows every 0.3 seconds, so all of it in 2.4 seconds
struct Melting
{
	float clock {0.0f};
	int32_t band {0};
};
/// The melting clock after `seconds` more, melting each band whose turn has come
void Melt(std::span<float> grid, Melting& melting, float seconds);

/// How deep the snow lies at a point in the world, blended between the four cells around it; none off the grid
[[nodiscard]] float DepthAt(std::span<const float> grid, glm::vec2 xz);

/// How much snow shows on an object standing where the snow is `depth` deep, 0 to `cap`: none below 20, then as deep
/// as it lies beyond that, scaled by `rate` of 256
inline constexpr int32_t k_ObjectRate = 255;
[[nodiscard]] int32_t ObjectLevel(float depth, int32_t rate, int32_t cap);
/// Objects show snow from this much of it
inline constexpr int32_t k_ObjectLevelShown = 5;
/// The snow texture's alpha, 0 to 255, at which an object shows snow at a level: the more snow, the more of it
[[nodiscard]] int32_t ObjectThreshold(int32_t level);

/// How white the land is, 0 to 16, where the snow is `depth` deep over a texel of the noise image
[[nodiscard]] int32_t LandLevel(float depth, uint8_t noise);
/// The land turns no whiter where no corner of its cell has more snow than this
inline constexpr float k_LandLeast = 5.0f;
/// The grey the land turns to under snow, of 15, by the noise image's texel: 14 or 15
[[nodiscard]] uint8_t LandWhite(uint8_t noise);

} // namespace openblack::snow_cover

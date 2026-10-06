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

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// Light stamped in colour on the land's cells for a frame, as lightning lights the ground: an image of colours, a texel a
/// cell, blended between texels and scaled by a strength, then added to each cell's colour or kept where it is brighter.
/// The land shows its cells' colours as a light added to it, and so do the models standing on it.
namespace openblack::land_colour_stamps
{

/// The most stamps the game lays in a frame
inline constexpr size_t k_MaxStamps = 200;

/// How a stamp meets the cells' colours
enum class Combine : uint8_t
{
	Add,
	Brighter,
};

/// Where a stamp's first texel falls on the land's cells, from a point in the world's x and z: its cell, and the
/// weights, of 255, its image is blended with along x and z, rounded to the nearest
struct Placement
{
	glm::ivec2 cell;
	glm::ivec2 weight;
};
[[nodiscard]] Placement Place(glm::vec2 xz);

/// The corner of a stamp `side` texels across centred on a point
[[nodiscard]] glm::vec2 CentredCorner(const glm::vec3& centre, int32_t side);

/// A stamp's strength, 0 to 255, from 0 to 1
[[nodiscard]] uint8_t Strength(float strength);

/// The glow lightning stamps: 64 by 64 grey texels, as RGB bytes, bright at the centre and none beyond 32 texels from it
inline constexpr int32_t k_LightningSide = 64;
[[nodiscard]] std::vector<uint8_t> LightningImage();

} // namespace openblack::land_colour_stamps

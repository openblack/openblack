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

#include "3D/LandData.h"

/// The testbed's land: a flat plane over the whole map, 512 by 512 cells, for trying out how creatures move and are
/// animated. It is painted as a checkerboard of cells, ten units a side, with a darker line along each block's edge
/// every 160 units, so that sliding feet and distances show.
namespace openblack::flat_land
{
/// The plane's altitude, well clear of the sea
constexpr uint8_t k_Altitude = 32;
/// Fully lit
constexpr uint8_t k_Luminosity = 0xFF;
/// The ambient sound of every cell: countryside
constexpr uint8_t k_SoundFlags = 7 << 2;
/// The terrain type of the plane's material: grass
constexpr uint16_t k_GrassMaterial = 18;
/// The bump map's value that leaves the materials' colours as they are
constexpr uint8_t k_FlatBump = 0x80;

/// The material's colours, 5 bits a channel
struct Colour
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
};
constexpr Colour k_LightSquare {20, 22, 17};
constexpr Colour k_DarkSquare {14, 16, 12};
constexpr Colour k_BlockEdge {7, 8, 6};

/// The colour of texel (x, z) of the material, which every block shows whole
[[nodiscard]] Colour MaterialColour(int x, int z);

/// Low enough over the sea for what stands on it to show in a pool of it, and just high enough above sea level for
/// creatures to walk on
constexpr uint8_t k_LowAltitude = 4;
/// The pool's cells, from the first to before the last, in cells of the map: in front of the testbed's camera, just
/// short of the middle of the map
constexpr int k_PoolMinX = 240;
constexpr int k_PoolMaxX = 272;
constexpr int k_PoolMinZ = 244;
constexpr int k_PoolMaxZ = 254;

/// The plane; with a pool, lower, just above the sea, with a pool of the sea let into it, for checking what the water
/// reflects
[[nodiscard]] LandData Build(bool pool = false);
} // namespace openblack::flat_land

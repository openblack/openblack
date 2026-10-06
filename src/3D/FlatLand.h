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

#include <glm/vec2.hpp>

#include "3D/LandData.h"

/// The testbed's land: a flat plane over the whole map, 512 by 512 cells, for trying out how creatures move and are
/// animated, with a lake let into it to the north of the middle. It is painted as a checkerboard of cells, ten units a
/// side, with a darker line along each block's edge every 160 units, so that sliding feet and distances show.
namespace openblack::flat_land
{
/// The plane's altitude, well clear of the sea
constexpr uint8_t k_Altitude = 32;
/// Fully lit
constexpr uint8_t k_Luminosity = 0xFF;
/// The ambient sound of the plane: countryside
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

/// The ambient sound types of the lake's water and of its shore
constexpr uint8_t k_FreshWaterSoundFlags = 2 << 2;
constexpr uint8_t k_CoastalSoundFlags = 3 << 2;
/// The flag of a cell of open water, which shows the sea rather than land
constexpr uint8_t k_OpenWaterFlag = 0x02;

/// The cells of the map along each side, and their size in units of the map (metres)
constexpr int k_CellsPerSide = 512;
constexpr float k_CellSize = 10.0f;
/// The middle of the map, where the testbed's camera looks and where scenarios and the spawn area are
constexpr glm::vec2 k_MapMiddle {static_cast<float>(k_CellsPerSide) * k_CellSize * 0.5f};

/// The lake: 10 by 10 cells (100 by 100 units) of open water at sea level, from the first cell to before the last, in
/// cells of the map. It lies north of the middle, beyond what the scenarios and the spawn area use, and in view of the
/// testbed's camera, which looks north over the middle.
constexpr int k_LakeMinX = 251;
constexpr int k_LakeMaxX = 261;
constexpr int k_LakeMinZ = 273;
constexpr int k_LakeMaxZ = 283;
/// The lake's centre and half its width, in units of the map
constexpr glm::vec2 k_LakeCentre {static_cast<float>(k_LakeMinX + k_LakeMaxX) * k_CellSize * 0.5f,
                                  static_cast<float>(k_LakeMinZ + k_LakeMaxZ) * k_CellSize * 0.5f};
constexpr glm::vec2 k_LakeHalfExtent {static_cast<float>(k_LakeMaxX - k_LakeMinX) * k_CellSize * 0.5f,
                                      static_cast<float>(k_LakeMaxZ - k_LakeMinZ) * k_CellSize * 0.5f};
/// The shore round the open water, by how many cells out from it each corner of a cell lies: two cells of shallows at
/// sea level that can be waded, then a bank climbing to the plane no steeper than a creature can walk
constexpr std::array<uint8_t, 6> k_ShoreAltitudes {0, 3, 3, 12, 22, k_Altitude};
/// How far the shore reaches out from the open water, in cells; the lake takes this much more on every side
constexpr int k_ShoreCells = static_cast<int>(k_ShoreAltitudes.size()) - 1;
/// The highest altitude the game counts as sea level, which is drawn at height 0
constexpr uint8_t k_SeaLevelAltitude = 3;

/// The altitude of the map's corner (x, z), the corner at the low x and z of the cell (x, z)
[[nodiscard]] uint8_t Altitude(int x, int z);

/// What a cell of the map is, by the altitudes of its corners
enum class CellKind : uint8_t
{
	/// Open water: every corner at the bottom, deep, showing the sea
	OpenWater,
	/// Shallows: every corner at sea level but not all at the bottom; water that can be waded
	Shallows,
	/// The bank: some corners at sea level, dry land by the water
	Shore,
	/// The plane and the rest of the bank
	Land,
};
[[nodiscard]] CellKind KindOf(int x, int z);

/// The cell (x, z) of the map: its altitude, and its water, ambient sound and open water flag by its kind
[[nodiscard]] lnd::LNDCell CellAt(int x, int z);

/// The plane with its lake
[[nodiscard]] LandData Build();
} // namespace openblack::flat_land

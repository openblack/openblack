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
#include <glm/vec3.hpp>

namespace openblack
{
class LandIslandInterface;
}

/// The game's map positions: x and z in 16.16 fixed point, a 10 m cell to 0x10000 (the high word is the cell, the low
/// word the fraction), and the altitude above the ground. Conversions, the 512 by 512 cell grid, the neighbour tables
/// and the spiral search, exactly as the game does them: it computes at single precision, so every product is a float
/// product.
///
/// The ground height is the island's: FromWorld and ToWorld ask it at the fixed position.
namespace openblack::map_coords
{

constexpr float k_FixedPerMetre = 6553.6f;           ///< 0x45CCCCCD, 6553.60009765625
constexpr float k_MetresPerFixed = 10.0f / 65536.0f; ///< exactly 10 / 65536
constexpr int32_t k_FixedPerCell = 0x10000;          ///< the high word is the cell
constexpr float k_CellSize = 10.0f;                  ///< metres per cell
/// The map's width and height in cells
constexpr uint32_t k_MapCells = 512;

/// A step in whole cells
struct JustMapXZ
{
	int16_t x;
	int16_t z;

	constexpr bool operator==(const JustMapXZ& other) const { return x == other.x && z == other.z; }
};

/// A map position: x and z in 16.16 fixed point, and the altitude above the ground
struct MapCoords
{
	int32_t x {0};
	int32_t z {0};
	float altitude {0.0f};

	constexpr bool operator==(const MapCoords& other) const = default;

	/// x and z added as integers, wrapping, and the altitude too
	constexpr MapCoords& operator+=(const MapCoords& other)
	{
		x = static_cast<int32_t>(static_cast<uint32_t>(x) + static_cast<uint32_t>(other.x));
		z = static_cast<int32_t>(static_cast<uint32_t>(z) + static_cast<uint32_t>(other.z));
		altitude = other.altitude + altitude;
		return *this;
	}
	constexpr MapCoords& operator-=(const MapCoords& other)
	{
		x = static_cast<int32_t>(static_cast<uint32_t>(x) - static_cast<uint32_t>(other.x));
		z = static_cast<int32_t>(static_cast<uint32_t>(z) - static_cast<uint32_t>(other.z));
		altitude = altitude - other.altitude;
		return *this;
	}
	[[nodiscard]] constexpr MapCoords operator+(const MapCoords& other) const
	{
		MapCoords sum = *this;
		sum += other;
		return sum;
	}
	[[nodiscard]] constexpr MapCoords operator-(const MapCoords& other) const
	{
		MapCoords difference = *this;
		difference -= other;
		return difference;
	}
};

/// The game's float to integer conversion: truncated towards 0, and 0x80000000 for a NaN or a value out of the int32
/// range
[[nodiscard]] constexpr int32_t FtoL(float value)
{
	if (!(value > -2147483648.0f && value < 2147483648.0f))
	{
		return static_cast<int32_t>(0x80000000u);
	}
	return static_cast<int32_t>(value);
}

/// Metres to 16.16: a float product, truncated towards 0
[[nodiscard]] constexpr int32_t ToFixed(float metres)
{
	return FtoL(metres * k_FixedPerMetre);
}

/// 16.16 to metres: the exact integer times 10 / 65536, rounded once to a float. Above 2^24 a float of the integer would
/// round twice, so the exact product is taken in double and rounded once
[[nodiscard]] constexpr float ToMetres(int32_t fixed)
{
	return static_cast<float>(static_cast<double>(fixed) * (10.0 / 65536.0));
}

/// Metres to 16.16 the way the game's angle and spiral helpers do it: times 65536, divided by 10, truncated. Not
/// ToFixed: 6553.6f is not 65536 / 10, so a value on a boundary truncates one unit apart
[[nodiscard]] constexpr int32_t ToFixedGUtils(float metres)
{
	return FtoL(metres * 65536.0f / 10.0f);
}

/// A metre value through a map position and back: what a position stored as one becomes. Another round trip may lose
/// one more unit (ToFixed(ToMetres(8090858)) = 8090857), as in the game
[[nodiscard]] constexpr float Quantise(float metres)
{
	return ToMetres(ToFixed(metres));
}

/// The cell of a 16.16 value, the high word read unsigned: a negative value is cell 0xFFFF, off the map
[[nodiscard]] constexpr uint16_t CellOf(int32_t fixed)
{
	return static_cast<uint16_t>(static_cast<uint32_t>(fixed) >> 16u);
}
/// The high word read signed, as a cell step holds it
[[nodiscard]] constexpr int16_t SignedCellOf(int32_t fixed)
{
	return static_cast<int16_t>(CellOf(fixed));
}
[[nodiscard]] constexpr uint16_t CellX(const MapCoords& coords)
{
	return CellOf(coords.x);
}
[[nodiscard]] constexpr uint16_t CellZ(const MapCoords& coords)
{
	return CellOf(coords.z);
}
/// The cell (CellX, CellZ)
[[nodiscard]] constexpr glm::ivec2 Cell(const MapCoords& coords)
{
	return {CellX(coords), CellZ(coords)};
}
/// The cell of a world point, made a map position first
[[nodiscard]] constexpr glm::ivec2 CellOf(glm::vec2 metres)
{
	return {CellOf(ToFixed(metres.x)), CellOf(ToFixed(metres.y))};
}
[[nodiscard]] constexpr glm::ivec2 CellOf(glm::vec3 point)
{
	return CellOf(glm::vec2(point.x, point.z));
}

/// Whether a position is on the map, its cells compared unsigned
[[nodiscard]] constexpr bool InBounds(const MapCoords& coords, uint32_t cells = k_MapCells)
{
	return CellX(coords) < cells && CellZ(coords) < cells;
}
/// The same on a cell: a negative cell is a large unsigned value, off the map
[[nodiscard]] constexpr bool InBounds(glm::ivec2 cell, uint32_t cells = k_MapCells)
{
	return static_cast<uint32_t>(cell.x) < cells && static_cast<uint32_t>(cell.y) < cells;
}
[[nodiscard]] constexpr bool InBounds(glm::vec3 point, uint32_t cells = k_MapCells)
{
	return InBounds(CellOf(point), cells);
}

/// The index cx * cells + cz of a position's map cell, -1 off the map
[[nodiscard]] constexpr int32_t CellIndex(const MapCoords& coords, uint32_t cells = k_MapCells)
{
	return InBounds(coords, cells) ? static_cast<int32_t>(CellX(coords) * cells + CellZ(coords)) : -1;
}

/// Moves a position by whole cells, adding to the 16-bit cells: the fractions stay and a carry out of the cell is lost
constexpr void AddCells(MapCoords& coords, JustMapXZ step)
{
	const auto move = [](int32_t value, int16_t d) {
		const auto word = static_cast<uint16_t>(CellOf(value) + static_cast<uint16_t>(d));
		return static_cast<int32_t>((static_cast<uint32_t>(word) << 16u) | (static_cast<uint32_t>(value) & 0xFFFFu));
	};
	coords.x = move(coords.x, step.x);
	coords.z = move(coords.z, step.z);
}

/// The 4 neighbours, +x, +z, -x, -z: the spiral's steps, also used on their own
constexpr std::array<JustMapXZ, 4> k_Neighbours4 {{{1, 0}, {0, 1}, {-1, 0}, {0, -1}}};
/// The 8 neighbours and the cell itself, anticlockwise from +x, then (0, 0)
constexpr std::array<JustMapXZ, 9> k_Neighbours8 {
    {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}, {0, 0}}};

/// The game's spiral search: every search does the centre cell first and then steps by Next(), checking each cell is on
/// the map. The steps from the start are (-1, 0), (0, -1), (+1, 0) x 2, (0, +1) x 2, (-1, 0) x 3, ...: a square growing
/// outwards. How many cells it covers is up to the search (CellSpiralSize)
struct Spiral
{
	int32_t dir {1};
	int32_t count {1};

	/// When the run of steps ends, the direction turns and the next run is half the turns made so far; the step is read
	/// after the state is updated
	constexpr const JustMapXZ& Next()
	{
		if (--count == 0)
		{
			++dir;
			count = dir / 2;
		}
		return k_Neighbours4[static_cast<size_t>(dir & 3)];
	}
};

/// The spiral in steps of `step` metres rather than cells, as towns look for clear ground: each axis moves through
/// metres and is truncated again with ToFixedGUtils
constexpr void SpiralIncrement(MapCoords& coords, Spiral& spiral, float step)
{
	const auto& d = spiral.Next();
	coords.x = ToFixedGUtils(static_cast<float>(d.x) * step + ToMetres(coords.x));
	coords.z = ToFixedGUtils(static_cast<float>(d.z) * step + ToMetres(coords.z));
}

/// The cells a spiral covers out to a radius: n = r * 0.2, truncated, at least 1 (compared unsigned); n^2
[[nodiscard]] constexpr int32_t CellSpiralSize(float radius)
{
	auto n = FtoL(radius * 0.2f);
	if (static_cast<uint32_t>(n) < 1u)
	{
		n = 1;
	}
	return n * n;
}

/// The steps a SpiralIncrement covers out to a radius: n = r * -2 / step, truncated; (1 - n)^2
[[nodiscard]] constexpr int32_t IncrementSpiralSize(float radius, float step)
{
	const auto n = FtoL(radius * -2.0f / step);
	return (1 - n) * (1 - n);
}

/// A world point as a map position: x and z by ToFixed, and the altitude above the ground at the truncated position
[[nodiscard]] MapCoords FromWorld(const LandIslandInterface& island, glm::vec3 point);
/// A map position as a world point: x and z by ToMetres, and y the ground there plus the altitude
[[nodiscard]] glm::vec3 ToWorld(const LandIslandInterface& island, const MapCoords& coords);

/// A map position of an (x, z) pair in metres, at no altitude
[[nodiscard]] constexpr MapCoords FromMetres(glm::vec2 metres)
{
	return {ToFixed(metres.x), ToFixed(metres.y), 0.0f};
}
[[nodiscard]] constexpr glm::vec2 ToMetres(const MapCoords& coords)
{
	return {ToMetres(coords.x), ToMetres(coords.z)};
}

} // namespace openblack::map_coords

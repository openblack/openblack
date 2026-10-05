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

/// The original's one map position: MapCoords {int32 x, z; float altitude} (bw1-decomp MapCoords.h), x and z in 16.16
/// fixed point with a 10 m cell = 0x10000 (the high word is the cell, the low word the fraction) and altitude the height
/// above the ground. Conversions, the 512 x 512 cell grid (ToMap 0x603430, InBounds 0x6042C0), the neighbour tables and
/// GUtils::Spiral 0x74D7E0, exactly as runblack.exe does them: the game logic runs with the FPU at 24 bits
/// (fn_007DEE00, and 0xFCFF at 0x7DEE0D), so every product is a float product.
///
/// The ground height (LH3DIsland::GetAltitude 0x803090) is the island's: FromWorld / ToWorld ask it at the fixed position.
namespace openblack::map_coords
{

constexpr float k_FixedPerMetre = 6553.6f;           ///< [0x8AC400] = 0x45CCCCCD (6553.60009765625), MapCoords::Set 0x603340
constexpr float k_MetresPerFixed = 10.0f / 65536.0f; ///< [0x8AA3A4] = 0x39200000, exactly 10 / 65536 (GetLHPoint 0x605C40)
constexpr int32_t k_FixedPerCell = 0x10000;          ///< the high word is the cell
constexpr float k_CellSize = 10.0f;                  ///< metres per cell (0x10000 * [0x8AA3A4])
/// GMap::Init 0x6014C0's width and height, both 0x200 (pushed by GGame::Init): stored in g_game+0x59C4 (the row width,
/// the limit of cz) and g_game+0x59C8 (the limit of cx)
constexpr uint32_t k_MapCells = 512;

/// JustMapXZ: one cell step {int16 x, z}
struct JustMapXZ
{
	int16_t x;
	int16_t z;

	constexpr bool operator==(const JustMapXZ& other) const { return x == other.x && z == other.z; }
};

/// MapCoords: x, z 16.16 fixed (+0, +4), altitude above the ground (+8)
struct MapCoords
{
	int32_t x {0};
	int32_t z {0};
	float altitude {0.0f};

	constexpr bool operator==(const MapCoords& other) const = default;

	/// MapCoords::operator+= 0x605410: x and z added as integers, the altitude too (fld [b + 8]; fadd [a + 8])
	constexpr MapCoords& operator+=(const MapCoords& other)
	{
		x = static_cast<int32_t>(static_cast<uint32_t>(x) + static_cast<uint32_t>(other.x));
		z = static_cast<int32_t>(static_cast<uint32_t>(z) + static_cast<uint32_t>(other.z));
		altitude = other.altitude + altitude;
		return *this;
	}
	/// MapCoords::operator-= 0x6054A0: the same with sub and fsub
	constexpr MapCoords& operator-=(const MapCoords& other)
	{
		x = static_cast<int32_t>(static_cast<uint32_t>(x) - static_cast<uint32_t>(other.x));
		z = static_cast<int32_t>(static_cast<uint32_t>(z) - static_cast<uint32_t>(other.z));
		altitude = altitude - other.altitude;
		return *this;
	}
	/// MapCoords::operator+ 0x605520 (a copy, then += 0x605410)
	[[nodiscard]] constexpr MapCoords operator+(const MapCoords& other) const
	{
		MapCoords sum = *this;
		sum += other;
		return sum;
	}
	/// MapCoords::operator- 0x6055C0 (a copy, then -= 0x6054A0)
	[[nodiscard]] constexpr MapCoords operator-(const MapCoords& other) const
	{
		MapCoords difference = *this;
		difference -= other;
		return difference;
	}
};

/// __ftol 0x7A1400, the branch every SSE2 CPU takes (HasSSE2 [0xE83A20] set): fstp qword; cvttsd2si eax, xmm0
/// (0x7A1414..0x7A141A), towards 0 and the "integer indefinite" 0x80000000 for a NaN or a value out of the int32 range.
/// The x87 branch (0x7A141F: fistp qword, then corrected towards 0, the low 32 bits of the int64) would give another
/// value out of that range; it is not reproduced
[[nodiscard]] constexpr int32_t FtoL(float value)
{
	if (!(value > -2147483648.0f && value < 2147483648.0f))
	{
		return static_cast<int32_t>(0x80000000u);
	}
	return static_cast<int32_t>(value);
}

/// Metres -> 16.16: fld; fmul [0x8AC400]; __ftol 0x7A1400 (truncated towards 0), MapCoords::Set 0x603346..0x603367 and
/// its 258 inline copies
[[nodiscard]] constexpr int32_t ToFixed(float metres)
{
	return FtoL(metres * k_FixedPerMetre);
}

/// 16.16 -> metres: fild; fmul [0x8AA3A4] (GetLHPoint 0x605C40 = ConvertToLHPoint 0x6041C0). fild is exact and the
/// product is rounded once to 24 bits; above 2^24 a float of the integer would round twice, so the exact product is
/// taken in double (fixed * 5 / 32768 needs at most 34 bits) and rounded to float once, as the FPU does
[[nodiscard]] constexpr float ToMetres(int32_t fixed)
{
	return static_cast<float>(static_cast<double>(fixed) * (10.0 / 65536.0));
}

/// GUtils' own metres -> 16.16: fmul 65536 [0x8AC408]; fdiv 10 [0x99A1BC]; __ftol (AddDistanceFromAngle 0x74D52F,
/// GetPosFromAngle 0x74D595, SpiralIncrement 0x74D85A ... 0x74F3A3; bw1-decomp MapCoords::SetX). Not ToFixed: 6553.6f
/// is not 65536 / 10, so a value on a boundary truncates one unit apart
[[nodiscard]] constexpr int32_t ToFixedGUtils(float metres)
{
	return FtoL(metres * 65536.0f / 10.0f);
}

/// A metre value through a MapCoords and back (ToMetres(ToFixed(m))): what a position stored in a MapCoords is. Not
/// idempotent: another round trip may lose one more unit (ToFixed(ToMetres(8090858)) = 8090857), as in the original
[[nodiscard]] constexpr float Quantise(float metres)
{
	return ToMetres(ToFixed(metres));
}

/// The cell of a 16.16 value: the high word read unsigned ("xor esi, esi; mov si, [ecx + 2]", ToMap 0x603433):
/// a negative value is cell 0xFFFF, off the map
[[nodiscard]] constexpr uint16_t CellOf(int32_t fixed)
{
	return static_cast<uint16_t>(static_cast<uint32_t>(fixed) >> 16u);
}
/// The high word read signed, as a JustMapXZ holds it (movsx, JustMapXZ::ToMap 0x5E1950; ApplyEffectToMapPos 0x525212)
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
/// The cell of a world point, MapCoords(LHPoint) 0x603160 then the high words
[[nodiscard]] constexpr glm::ivec2 CellOf(glm::vec2 metres)
{
	return {CellOf(ToFixed(metres.x)), CellOf(ToFixed(metres.y))};
}
[[nodiscard]] constexpr glm::ivec2 CellOf(glm::vec3 point)
{
	return CellOf(glm::vec2(point.x, point.z));
}

/// MapCoords::InBounds 0x6042C0 (= JustMapXZ::InBounds 0x5E1860): the unsigned high words "jae" against
/// [g_game+0x59C8] and [g_game+0x59C4] (0x6042CB, 0x6042D9), both k_MapCells
[[nodiscard]] constexpr bool InBounds(const MapCoords& coords, uint32_t cells = k_MapCells)
{
	return CellX(coords) < cells && CellZ(coords) < cells;
}
/// The same on a cell: a negative cell (a sign-extended JustMapXZ) is a large unsigned value, off the map
[[nodiscard]] constexpr bool InBounds(glm::ivec2 cell, uint32_t cells = k_MapCells)
{
	return static_cast<uint32_t>(cell.x) < cells && static_cast<uint32_t>(cell.y) < cells;
}
[[nodiscard]] constexpr bool InBounds(glm::vec3 point, uint32_t cells = k_MapCells)
{
	return InBounds(CellOf(point), cells);
}

/// MapCoords::ToMap 0x603430 (= JustMapXZ::ToMap 0x5E1950): the index cx * [g_game+0x59C4] + cz of the map cell
/// (g_game + 0x59FC + index * 8), -1 for NULL off the map
[[nodiscard]] constexpr int32_t CellIndex(const MapCoords& coords, uint32_t cells = k_MapCells)
{
	return InBounds(coords, cells) ? static_cast<int32_t>(CellX(coords) * cells + CellZ(coords)) : -1;
}

/// MapCoords::operator+=(JustMapXZ) 0x605470: 16-bit adds to the high words ("add word [ecx + 2]", "add word
/// [ecx + 6]"); the fractions stay and a carry out of the high word is lost
constexpr void AddCells(MapCoords& coords, JustMapXZ step)
{
	const auto move = [](int32_t value, int16_t d) {
		const auto word = static_cast<uint16_t>(CellOf(value) + static_cast<uint16_t>(d));
		return static_cast<int32_t>((static_cast<uint32_t>(word) << 16u) | (static_cast<uint32_t>(value) & 0xFFFFu));
	};
	coords.x = move(coords.x, step.x);
	coords.z = move(coords.z, step.z);
}

/// The 4 neighbours, 0xDA59FC..0xDA5A0B (static initialiser 0x74CA10): +x, +z, -x, -z. GUtils::Spiral's table; also read
/// without the spiral (fn_00602C70 0x602CC4, Villager::CalculateNearestFreeDestination 0x768374, the wallhug 0x770104)
constexpr std::array<JustMapXZ, 4> k_Neighbours4 {{{1, 0}, {0, 1}, {-1, 0}, {0, -1}}};
/// The 8 neighbours and the cell itself, 0xDA59D8..0xDA59FB (static initialiser 0x74CA60, read by fn_00504143
/// 0x50417E): anticlockwise from +x, then (0, 0). Another table, not the spiral's
constexpr std::array<JustMapXZ, 9> k_Neighbours8 {
    {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}, {0, 0}}};

/// GUtils::Spiral(long& dir, long& count) 0x74D7E0: every caller starts with dir = count = 1, does the centre cell first
/// and then steps by Next() (MapCoords += JustMapXZ 0x605470), checking InBounds on each cell. The steps from the start:
/// (-1, 0), (0, -1), (+1, 0) x 2, (0, +1) x 2, (-1, 0) x 3, ...: a square growing outwards. The number of cells is the
/// caller's own (CellSpiralSize, or its inline ceil(...)^2)
struct Spiral
{
	int32_t dir {1};
	int32_t count {1};

	/// dec [count]; jne (0x74D7E9); inc dir; count = dir / 2 (cdq; sub; sar 1: towards 0, 0x74D7EF..0x74D7F7); then
	/// the step &table[dir & 3] (0x74D7FE): the state is updated before the step is read
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

/// GUtils::SpiralIncrement(MapCoords&, long& dir, long& count, float step) 0x74D810, Town::FindClearArea's only: the
/// spiral's rule (0x74D81B..0x74D82B), then each axis moves `step` metres and is truncated again with the GUtils
/// formula: x = ftol((table.x * step + x * 10 * (1 / 65536)) * 65536 / 10) (0x74D836..0x74D8A8). x * 10 [0x99A1BC] then
/// * 1/65536 [0x8AC41C] is ToMetres (a power of 2 does not change the rounding)
constexpr void SpiralIncrement(MapCoords& coords, Spiral& spiral, float step)
{
	const auto& d = spiral.Next();
	coords.x = ToFixedGUtils(static_cast<float>(d.x) * step + ToMetres(coords.x));
	coords.z = ToFixedGUtils(static_cast<float>(d.z) * step + ToMetres(coords.z));
}

/// GUtils::GetMapCellSpiralSizeFromRadius 0x74F520: n = ftol(r * 0.2 [0x8AA3AC]); "cmp eax, 1; jae" (unsigned: only 0
/// becomes 1); n^2
[[nodiscard]] constexpr int32_t CellSpiralSize(float radius)
{
	auto n = FtoL(radius * 0.2f);
	if (static_cast<uint32_t>(n) < 1u)
	{
		n = 1;
	}
	return n * n;
}

/// GUtils::GetIncrementSpiralSizeFromRadius 0x74F540: n = ftol(r * -2 [0x8C7CE0] / step); (1 - n)^2
[[nodiscard]] constexpr int32_t IncrementSpiralSize(float radius, float step)
{
	const auto n = FtoL(radius * -2.0f / step);
	return (1 - n) * (1 - n);
}

/// MapCoords(LHPoint) 0x603160 / MapCoords::Set 0x603340: x, z = ToFixed; altitude = y - GetAltitude(this), the ground
/// height at the truncated position (0x603371..0x60337C)
[[nodiscard]] MapCoords FromWorld(const LandIslandInterface& island, glm::vec3 point);
/// MapCoords::GetLHPoint 0x605C40 (= ConvertToLHPoint 0x6041C0): x, z = ToMetres; y = GetAltitude(this) + altitude
[[nodiscard]] glm::vec3 ToWorld(const LandIslandInterface& island, const MapCoords& coords);

/// MapCoords without its altitude (x, z only), for the 2D callers: JustWholeMapXZ
[[nodiscard]] constexpr MapCoords FromMetres(glm::vec2 metres)
{
	return {ToFixed(metres.x), ToFixed(metres.y), 0.0f};
}
[[nodiscard]] constexpr glm::vec2 ToMetres(const MapCoords& coords)
{
	return {ToMetres(coords.x), ToMetres(coords.z)};
}

} // namespace openblack::map_coords

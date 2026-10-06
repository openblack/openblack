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
#include <bit>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "3D/MapCoords.h"

/// The game's distances. Every object measures with these, so they are kept as the game computes them.
///
/// Lengths go through an approximate inverse square root read from a table, off by at most 0.1%, and the map unit
/// lengths are then truncated: they are not sqrt(dx^2 + dz^2). All of them are on the ground (x, z); heights are never
/// part of a distance.
///
/// Every float operation is a single float operation, as the game's 24-bit FPU rounds it.
///
/// Lookalikes that are not interchangeable:
/// - Hypotenuse of map units (truncated, no cut-off near zero) and of metres (0 when both sides are within 1e-4).
/// - GetDistanceInMetres between map positions (quantised to map units) and GetDistance between points (metres).
/// - GetMetresDistanceSq: the exact square, without the table.
/// - FastDistance (the larger side plus half the smaller) and ChebyshevDistance (the larger side) aren't lengths.
/// - SigmoidThreshold takes the threshold first; CreatureSigmoidThreshold gives 0 for b <= 0.
namespace openblack::gutils
{

namespace detail
{
/// The bits of the game's sigmoid table. It is close to 1 / (1 + exp(-1.0232 (i - 20))), but it is data, so it is
/// kept as it is: the first entry is exactly 0 and the last four exactly 1
constexpr std::array<uint32_t, 41> k_SigmoidBits {
    0x00000000, 0x317763DF, 0x322BCC77, 0x32F084A7, 0x33A71301, 0x34684017, 0x35218B62, 0x35E0AE34, 0x369C419B,
    0x375955DE, 0x38172465, 0x38D235BD, 0x39922A17, 0x3A4B32F9, 0x3B0D1EB3, 0x3BC38892, 0x3C868D9B, 0x3D35D41D,
    0x3DEA5E18, 0x3E8762A1, 0x3F000000, 0x3F3C4EB0, 0x3F62B43D, 0x3F74A2BE, 0x3F7BCB93, 0x3F7E78EF, 0x3F7F72E1,
    0x3F7FCD33, 0x3F7FEDBB, 0x3F7FF96E, 0x3F7FFDA3, 0x3F7FFF27, 0x3F7FFFB2, 0x3F7FFFE4, 0x3F7FFFF6, 0x3F7FFFFC,
    0x3F7FFFFF, 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000};
} // namespace detail

/// The game's sigmoid table, from 0 to 1 in 41 steps
constexpr std::array<float, 41> k_Sigmoid = [] {
	std::array<float, 41> table {};
	for (size_t i = 0; i < table.size(); ++i)
	{
		table.at(i) = std::bit_cast<float>(detail::k_SigmoidBits.at(i));
	}
	return table;
}();

/// Sides of a metre hypotenuse within this are both taken as 0
constexpr float k_HypotenuseEpsilon = 1e-4f;
constexpr float k_MetresPerCell = 10.0f;

/// The inverse square root table: entry i keeps the top 10 mantissa bits of 1 / sqrt(f), for the float f in [0.5, 2)
/// whose exponent's low bit and top 9 mantissa bits are i. An exact 1 is kept as all ten bits set
[[nodiscard]] const std::array<uint32_t, 1024>& InvSqrtTable();

/// An approximate 1 / sqrt(x): the exponent halved and negated, and the mantissa from the table, truncated. The sign is
/// ignored, and 0 gives about 2^63, so that 1 / InvSqrt(0) is 0
[[nodiscard]] float InvSqrt(float value);

/// The length of a vector in map units (0x10000 to the cell), truncated, with no cut-off near zero
[[nodiscard]] int32_t Hypotenuse(int32_t dx, int32_t dz);

/// The length of a vector in metres, 0 when both sides are within 1e-4 or are NaN
[[nodiscard]] float Hypotenuse(float a, float b);

/// Map units to metres. The exact integer is scaled and rounded once, so above 2^24 units (2560 m) this is not
/// float(whole) * c
[[nodiscard]] constexpr float ConvertWholeDistanceToMeters(int32_t whole)
{
	return map_coords::ToMetres(whole);
}

/// Metres to map units: divided by 10, then scaled by 65536, truncated towards 0
[[nodiscard]] constexpr int32_t ConvertMetersToWholeDistance(float metres)
{
	return map_coords::FtoL(metres / k_MetresPerCell * 65536.0f);
}

/// The distance between two map positions in map units, ignoring the altitude
[[nodiscard]] int32_t GetDistance(const map_coords::MapCoords& a, const map_coords::MapCoords& b);

/// The distance from a map position to the centre of a map cell, in map units
[[nodiscard]] int32_t GetDistanceToCell(const map_coords::MapCoords& a, map_coords::JustMapXZ cell);

/// The distance between two map positions in metres, through the map unit distance
[[nodiscard]] float GetDistanceInMetres(const map_coords::MapCoords& a, const map_coords::MapCoords& b);
/// The same for two points, each made a map position first
[[nodiscard]] float GetDistanceInMetres(glm::vec3 a, glm::vec3 b);
/// The same for two (x, z) pairs in metres
[[nodiscard]] float GetDistanceInMetres(glm::vec2 a, glm::vec2 b);
/// The same for two map positions kept as (x, z) in map units: unlike the vec2 overload, not metres
[[nodiscard]] float GetDistanceInMetres(glm::ivec2 a, glm::ivec2 b);

/// The distance from a map position to the centre of a map cell, in metres
[[nodiscard]] float GetDistanceInMetresToCell(const map_coords::MapCoords& a, map_coords::JustMapXZ cell);

/// The distance between two points in metres, without going through map positions
[[nodiscard]] float GetDistance(glm::vec3 a, glm::vec3 b);

/// The square of the distance between two map positions in metres, exactly: not GetDistanceInMetres squared
[[nodiscard]] float GetMetresDistanceSq(const map_coords::MapCoords& a, const map_coords::MapCoords& b);

namespace detail
{
/// |v| with 32-bit wrapping, as the game's
[[nodiscard]] constexpr int32_t Abs(int32_t v)
{
	return v < 0 ? static_cast<int32_t>(0u - static_cast<uint32_t>(v)) : v;
}
/// a - b with 32-bit wrapping, as the game's
[[nodiscard]] constexpr int32_t Sub(int32_t a, int32_t b)
{
	return static_cast<int32_t>(static_cast<uint32_t>(a) - static_cast<uint32_t>(b));
}
} // namespace detail

/// The larger side of the vector between two map positions plus half the smaller, in map units: not a length
[[nodiscard]] constexpr int32_t FastDistance(const map_coords::MapCoords& a, const map_coords::MapCoords& b)
{
	const int32_t dx = detail::Abs(detail::Sub(b.x, a.x));
	const int32_t dz = detail::Abs(detail::Sub(b.z, a.z));
	return dx >= dz ? (dz >> 1) + dx : (dx >> 1) + dz;
}

/// The larger side of the vector between two map positions, the sides compared unsigned, in map units
[[nodiscard]] constexpr int32_t ChebyshevDistance(const map_coords::MapCoords& a, const map_coords::MapCoords& b)
{
	const int32_t dx = detail::Abs(detail::Sub(a.x, b.x));
	const int32_t dz = detail::Abs(detail::Sub(a.z, b.z));
	return static_cast<uint32_t>(dx) <= static_cast<uint32_t>(dz) ? dz : dx;
}

/// The chance that b passes the threshold a (the threshold comes first), in 41 steps: 0 when a is 1 or NaN; otherwise b
/// is clamped to [-1, 1], a is taken from it and the result clamped again (a NaN clamping to -1), and the step is
/// (v + 1) * 20.5, truncated
[[nodiscard]] float SigmoidThreshold(float a, float b);

/// How much a distance counts, falling from almost 1 when near to almost 0 at the maximum and beyond:
/// SigmoidThreshold(0.5, 1 - min(d, max) / max), the distance kept when it is NaN. A maximum of 0 gives 0
[[nodiscard]] float GetDistanceModifier(float distance, float maximum);

/// How much a change in distance changes belief: SigmoidThreshold(-0.9, -(x / y))
[[nodiscard]] float DistanceChangeToBelief(float x, float y);

/// The creature's sigmoid: 0 when b <= 0 or NaN, else SigmoidThreshold(a, b)
[[nodiscard]] float CreatureSigmoidThreshold(float a, float b);

} // namespace openblack::gutils

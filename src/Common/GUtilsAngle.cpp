/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GUtilsAngle.h"

#include <cmath>

#include <numbers>

#include "Common/GUtilsDistance.h"

using namespace openblack;
using map_coords::MapCoords;

namespace
{
/// The cosine and sine of a float angle times a distance, rounded once
float CosTimes(float radians, float metres)
{
	return static_cast<float>(std::cos(static_cast<double>(radians)) * static_cast<double>(metres));
}
float SinTimes(float radians, float metres)
{
	return static_cast<float>(std::sin(static_cast<double>(radians)) * static_cast<double>(metres));
}

/// 32-bit arithmetic that wraps, as the game's does
int32_t Wrap(int64_t value)
{
	return static_cast<int32_t>(static_cast<uint32_t>(static_cast<uint64_t>(value)));
}

/// table * (whole >> shift) >> (16 - shift), the product wrapping to 32 bits
int32_t ScaleByTable(int32_t table, int32_t whole, int shift)
{
	const auto product = static_cast<int32_t>(static_cast<uint32_t>(table) * static_cast<uint32_t>(whole >> shift));
	return product >> (16 - shift);
}
} // namespace

const std::array<uint16_t, 257>& gutils::ArcTanTable()
{
	static const auto s_table = [] {
		std::array<uint16_t, 257> table {};
		for (size_t i = 0; i < table.size(); ++i)
		{
			table.at(i) = static_cast<uint16_t>(
			    std::trunc(std::atan(static_cast<double>(i) / 256.0) * 2048.0 / (2.0 * std::numbers::pi)));
		}
		return table;
	}();
	return s_table;
}

const std::array<int32_t, 2560>& gutils::SinTable()
{
	static const auto s_table = [] {
		std::array<int32_t, 2560> table {};
		for (size_t i = 0; i < table.size(); ++i)
		{
			table.at(i) =
			    static_cast<int32_t>(std::trunc(65536.0 * std::sin(static_cast<double>(i) * 2.0 * std::numbers::pi / 2048.0)));
		}
		return table;
	}();
	return s_table;
}

int32_t gutils::Cos(uint16_t angle)
{
	return SinTable().at(static_cast<size_t>(angle & k_GameAngleMask) + 512);
}

int32_t gutils::Sin(uint16_t angle)
{
	return SinTable().at(static_cast<size_t>(angle & k_GameAngleMask));
}

uint16_t gutils::LHArcTan(int32_t dx, int32_t dz)
{
	const auto& table = ArcTanTable();
	const auto t = [&table](int32_t num, int32_t den) {
		return static_cast<int32_t>(table.at((static_cast<uint32_t>(num) << 8u) / static_cast<uint32_t>(den)));
	};
	const int32_t x = Wrap(-static_cast<int64_t>(dx));
	const int32_t z = dz;
	if (z == 0 && x == 0)
	{
		return 0;
	}
	const int32_t negX = Wrap(-static_cast<int64_t>(x));
	const int32_t negZ = Wrap(-static_cast<int64_t>(z));
	int32_t a = 0;
	if (z >= 0)
	{
		if (x >= 0)
		{
			a = z >= x ? 0x200 + t(x, z) : 0x400 - t(z, x);
		}
		else
		{
			a = z >= negX ? 0x200 - t(negX, z) : t(z, negX);
		}
	}
	else
	{
		if (x >= 0)
		{
			a = negZ >= x ? 0x600 - t(x, negZ) : 0x400 + t(negZ, x);
		}
		else
		{
			a = negZ >= negX ? 0x600 + t(negX, negZ) : 0x800 - t(negZ, negX);
		}
	}
	return static_cast<uint16_t>(a & k_GameAngleMask);
}

uint16_t gutils::GetAngleFromDXDZ(int32_t dx, int32_t dz)
{
	return LHArcTan(dx, dz);
}

uint16_t gutils::GetAngleFromXZ(const MapCoords& from, const MapCoords& to)
{
	return GetAngleFromDXDZ(Wrap(int64_t {to.x} - from.x), Wrap(int64_t {to.z} - from.z));
}

uint16_t gutils::GetAngleFromXZ(glm::ivec2 from, glm::ivec2 to)
{
	return GetAngleFromDXDZ(Wrap(int64_t {to.x} - from.x), Wrap(int64_t {to.y} - from.y));
}

uint16_t gutils::GetAngleFromXZ(glm::vec2 from, glm::vec2 to)
{
	return GetAngleFromXZ(map_coords::FromMetres(from), map_coords::FromMetres(to));
}

float gutils::Get3DAngleFromXZ(const MapCoords& from, const MapCoords& to)
{
	return ConvertGameAngleTo3D(GetAngleFromXZ(from, to));
}

float gutils::Get3DAngleFromXZ(glm::ivec2 from, glm::ivec2 to)
{
	return ConvertGameAngleTo3D(GetAngleFromXZ(from, to));
}

float gutils::Get3DAngleFromXZ(glm::vec2 from, glm::vec2 to)
{
	return ConvertGameAngleTo3D(GetAngleFromXZ(from, to));
}

uint32_t gutils::ConvertAngle3DToGame(float radians)
{
	const float scaled = radians * k_Angle3DToGame;
	return static_cast<uint32_t>(map_coords::FtoL(scaled)) & static_cast<uint32_t>(k_GameAngleMask);
}

float gutils::ConvertGameAngleTo3D(int32_t angle)
{
	return static_cast<float>(angle & k_GameAngleMask) * k_GameAngleTo3D;
}

uint32_t gutils::ConvertScawenAngleToGameAngle(float radians)
{
	const float shifted = radians - k_ScawenOffset;
	return ConvertAngle3DToGame(shifted);
}

float gutils::ConvertGameAngleToScawenAngle(uint16_t angle)
{
	const float scaled = static_cast<float>(static_cast<int32_t>(angle) << 1) * k_HalfGameAngleTo3D;
	return scaled + k_ScawenOffset;
}

float gutils::GetXByAngle(uint16_t angle, float distance)
{
	const float scaled = static_cast<float>(Cos(angle)) * distance;
	return scaled * (1.0f / 65536.0f);
}

float gutils::GetZByAngle(uint16_t angle, float distance)
{
	const float scaled = static_cast<float>(Sin(angle)) * distance;
	return scaled * (1.0f / 65536.0f);
}

int32_t gutils::GetXByAngleBigDistance(uint16_t angle, int32_t whole)
{
	return ScaleByTable(Cos(angle), whole, 4);
}

int32_t gutils::GetZByAngleBigDistance(uint16_t angle, int32_t whole)
{
	return ScaleByTable(Sin(angle), whole, 4);
}

int32_t gutils::GetXByAngleHugeDistance(uint16_t angle, int32_t whole)
{
	return ScaleByTable(Cos(angle), whole, 8);
}

int32_t gutils::GetZByAngleHugeDistance(uint16_t angle, int32_t whole)
{
	return ScaleByTable(Sin(angle), whole, 8);
}

int32_t gutils::GetXByAngleMetersDistance(uint16_t angle, float metres)
{
	const float cells = metres / k_MetresPerCell;
	return map_coords::FtoL(static_cast<float>(Cos(angle)) * cells);
}

int32_t gutils::GetZByAngleMetersDistance(uint16_t angle, float metres)
{
	const float cells = metres / k_MetresPerCell;
	return map_coords::FtoL(static_cast<float>(Sin(angle)) * cells);
}

MapCoords gutils::GetPosFromGameAngle(uint16_t angle, int32_t whole)
{
	return {GetXByAngleBigDistance(angle, whole), GetZByAngleBigDistance(angle, whole), 0.0f};
}

MapCoords gutils::GetPosFromGameAngle(uint16_t angle, float metres)
{
	return GetPosFromGameAngle(angle, ConvertMetersToWholeDistance(metres));
}

MapCoords gutils::GetPosFromAngle(float radians, float metres)
{
	return {map_coords::ToFixedGUtils(CosTimes(radians, metres)), map_coords::ToFixedGUtils(SinTimes(radians, metres)), 0.0f};
}

void gutils::AddDistanceFromAngle(MapCoords& pos, float radians, float metres)
{
	const float x = CosTimes(radians, metres) + map_coords::ToMetres(pos.x);
	pos.x = map_coords::ToFixedGUtils(x);
	const float z = SinTimes(radians, metres) + map_coords::ToMetres(pos.z);
	pos.z = map_coords::ToFixedGUtils(z);
}

glm::vec3 gutils::GetPointFromAngle(float radians, float metres)
{
	return {CosTimes(radians, metres), 0.0f, SinTimes(radians, metres)};
}

uint32_t gutils::GetAngleDifference(int32_t a, int32_t b)
{
	const auto d = static_cast<int64_t>(Wrap(int64_t {a} - b));
	const auto magnitude = static_cast<uint32_t>(d < 0 ? -d : d);
	return magnitude > 0x400u ? 0x800u - magnitude : magnitude;
}

int32_t gutils::GetAngleSign(int32_t from, int32_t to)
{
	int32_t d = Wrap(int64_t {to} - from);
	if (d == 0)
	{
		return 0;
	}
	const auto magnitude = static_cast<uint32_t>(d < 0 ? -static_cast<int64_t>(d) : d);
	if (magnitude > 0x400u)
	{
		d += d < 0 ? 0x800 : -0x800;
	}
	return d < 0 ? -1 : 1;
}

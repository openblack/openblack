/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GUtilsDistance.h"

#include <cmath>

#include <algorithm>

using namespace openblack;
using map_coords::JustMapXZ;
using map_coords::MapCoords;

namespace
{
/// The x or z of a map cell's centre in map units
int32_t CellCentre(int16_t cell)
{
	return static_cast<int32_t>(static_cast<uint32_t>(static_cast<int32_t>(cell)) << 16u) + 0x8000;
}

/// The sigmoid's clamp to [-1, 1], a NaN going to -1
float SigmoidClamp(float value)
{
	if (!(value >= -1.0f))
	{
		return -1.0f;
	}
	return value > 1.0f ? 1.0f : value;
}
} // namespace

const std::array<uint32_t, 1024>& gutils::InvSqrtTable()
{
	static const auto s_table = [] {
		std::array<uint32_t, 1024> table {};
		for (uint32_t i = 0; i < table.size(); ++i)
		{
			// f in [0.5, 1) for i < 512 and in [1, 2) above
			const auto f = std::bit_cast<float>(0x3F000000u | (i << 14u));
			const auto r = static_cast<float>(1.0 / std::sqrt(static_cast<double>(f)));
			table.at(i) = r == 1.0f ? 0x7FE000u : (std::bit_cast<uint32_t>(r) & 0x7FE000u);
		}
		return table;
	}();
	return s_table;
}

float gutils::InvSqrt(float value)
{
	const auto bits = std::bit_cast<uint32_t>(value);
	const uint32_t exponent = ((0xBE000000u - (bits & 0x7F800000u)) >> 1u) & 0x7F800000u;
	return std::bit_cast<float>(exponent | InvSqrtTable().at((bits >> 14u) & 0x3FFu));
}

int32_t gutils::Hypotenuse(int32_t dx, int32_t dz)
{
	// Scaling by a power of two, rounding the integer to a float first gives the same value as rounding once
	const float x = static_cast<float>(dx) * (1.0f / 65536.0f);
	const float z = static_cast<float>(dz) * (1.0f / 65536.0f);
	const float squared = z * z + x * x;
	return map_coords::FtoL(65536.0f / InvSqrt(squared));
}

float gutils::Hypotenuse(float a, float b)
{
	if (!(std::abs(a) > k_HypotenuseEpsilon) && !(std::abs(b) > k_HypotenuseEpsilon))
	{
		return 0.0f;
	}
	const float squared = a * a + b * b;
	return 1.0f / InvSqrt(squared);
}

int32_t gutils::GetDistance(const MapCoords& a, const MapCoords& b)
{
	return Hypotenuse(detail::Sub(b.x, a.x), detail::Sub(b.z, a.z));
}

int32_t gutils::GetDistanceToCell(const MapCoords& a, JustMapXZ cell)
{
	return Hypotenuse(detail::Sub(CellCentre(cell.x), a.x), detail::Sub(CellCentre(cell.z), a.z));
}

float gutils::GetDistanceInMetres(const MapCoords& a, const MapCoords& b)
{
	return ConvertWholeDistanceToMeters(GetDistance(a, b));
}

float gutils::GetDistanceInMetres(glm::vec3 a, glm::vec3 b)
{
	return GetDistanceInMetres(map_coords::FromMetres({a.x, a.z}), map_coords::FromMetres({b.x, b.z}));
}

float gutils::GetDistanceInMetres(glm::vec2 a, glm::vec2 b)
{
	return GetDistanceInMetres(map_coords::FromMetres(a), map_coords::FromMetres(b));
}

float gutils::GetDistanceInMetres(glm::ivec2 a, glm::ivec2 b)
{
	return GetDistanceInMetres(MapCoords {a.x, a.y, 0.0f}, MapCoords {b.x, b.y, 0.0f});
}

float gutils::GetDistanceInMetresToCell(const MapCoords& a, JustMapXZ cell)
{
	return ConvertWholeDistanceToMeters(GetDistanceToCell(a, cell));
}

float gutils::GetDistance(glm::vec3 a, glm::vec3 b)
{
	const float dx = b.x - a.x;
	const float dz = b.z - a.z;
	return Hypotenuse(dx, dz);
}

float gutils::GetMetresDistanceSq(const MapCoords& a, const MapCoords& b)
{
	const float mx = ConvertWholeDistanceToMeters(detail::Sub(b.x, a.x));
	const float mz = ConvertWholeDistanceToMeters(detail::Sub(b.z, a.z));
	return mz * mz + mx * mx;
}

float gutils::SigmoidThreshold(float a, float b)
{
	if (a == 1.0f || std::isnan(a))
	{
		return 0.0f;
	}
	const float v = SigmoidClamp(SigmoidClamp(b) - a);
	// Compared unsigned, so only steps past the end are cut
	const auto index = static_cast<uint32_t>(map_coords::FtoL((v + 1.0f) * 20.5f));
	return k_Sigmoid.at(std::min(index, 40u));
}

float gutils::GetDistanceModifier(float distance, float maximum)
{
	const float m = !(distance >= maximum) ? distance : maximum;
	const float b = 1.0f - m / maximum;
	return SigmoidThreshold(0.5f, b);
}

float gutils::DistanceChangeToBelief(float x, float y)
{
	const float b = -(x / y);
	return SigmoidThreshold(-0.9f, b);
}

float gutils::CreatureSigmoidThreshold(float a, float b)
{
	if (!(b > 0.0f))
	{
		return 0.0f;
	}
	return SigmoidThreshold(a, b);
}

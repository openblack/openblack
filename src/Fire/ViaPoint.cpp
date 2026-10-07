/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ViaPoint.h"

#include <cmath>

using namespace openblack;

fire::ViaPoint fire::GetViaPoint(const map_coords::MapCoords& from, const map_coords::MapCoords& to,
                                 const map_coords::MapCoords& centre, float radius, float margin, float side)
{
	ViaPoint result;
	const float rad = radius * map_coords::k_FixedPerMetre;
	const float mar = margin * map_coords::k_FixedPerMetre;
	const auto ux = static_cast<float>(centre.x - from.x);
	const auto uz = static_cast<float>(centre.z - from.z);
	const float d = std::sqrt(ux * ux + uz * uz);
	if (d <= rad)
	{
		result.inside = true;
		return result;
	}
	const float reach = rad + mar;
	const float s = reach / d;
	const float c = std::sqrt(1.0f - s * s);
	const float uxn = ux / d;
	const float uzn = uz / d;
	const auto tx = static_cast<float>(to.x - from.x);
	const auto tz = static_cast<float>(to.z - from.z);
	const float length = std::sqrt(tx * tx + tz * tz);
	constexpr float k_Degenerate = 0.0001f;
	if (length < k_Degenerate)
	{
		return result;
	}
	const float txn = tx / length;
	const float tzn = tz / length;
	const float dot = tzn * uzn + txn * uxn;
	if (dot <= c)
	{
		return result;
	}
	const float cross = txn * uzn - uxn * tzn;
	const bool sideA = side < 0.0f || (side == 0.0f && cross < 0.0f);
	float angle = 0.0f;
	if (sideA)
	{
		result.point.x = map_coords::FtoL((-c * uzn - s * uxn) * reach + static_cast<float>(centre.x));
		result.point.z = map_coords::FtoL((c * uxn - s * uzn) * reach + static_cast<float>(centre.z));
		angle = -std::acos(dot);
	}
	else
	{
		result.point.x = map_coords::FtoL((c * uzn - s * uxn) * reach + static_cast<float>(centre.x));
		result.point.z = map_coords::FtoL((-c * uxn - s * uzn) * reach + static_cast<float>(centre.z));
		angle = std::acos(dot);
	}
	result.detour = true;
	const auto ex = static_cast<float>(to.x - centre.x);
	const auto ez = static_cast<float>(to.z - centre.z);
	if (std::sqrt(ex * ex + ez * ez) < rad)
	{
		result.inside = true;
		result.angle = angle;
		return result;
	}
	const auto vx = static_cast<float>(result.point.x - from.x);
	const auto vz = static_cast<float>(result.point.z - from.z);
	if (std::sqrt(vx * vx + vz * vz) < length)
	{
		result.angle = angle;
	}
	return result;
}

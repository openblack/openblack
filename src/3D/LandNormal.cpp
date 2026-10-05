/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LandNormal.h"

#include <cmath>
#include <cstdlib>

#include <array>

using namespace openblack;

namespace
{
constexpr float k_EdgeSquared = 100.0f; ///< a cell edge of 10 m, squared
constexpr float k_CellEdge = 10.0f;
constexpr float k_LengthSteps = 1023.0f;
constexpr float k_LengthStep = 0.000977517106f; ///< 1 / 1023
constexpr uint32_t k_EdgeEntries = 0x100;
constexpr uint32_t k_LengthEntries = 0x400;

/// Every step rounded to a float
float EdgeFormula(uint32_t climb)
{
	const float rise = static_cast<float>(climb) * land_normal::k_HeightUnit;
	return 1.0f / std::sqrt(rise * rise + k_EdgeSquared);
}

const std::array<float, k_EdgeEntries>& EdgeTable()
{
	static const auto table = [] {
		std::array<float, k_EdgeEntries> t {};
		for (uint32_t i = 0; i < k_EdgeEntries; ++i)
		{
			t[i] = EdgeFormula(i);
		}
		return t;
	}();
	return table;
}

const std::array<float, k_LengthEntries>& LengthTable()
{
	static const auto table = [] {
		std::array<float, k_LengthEntries> t {};
		t[0] = 1.0f;
		for (uint32_t j = 1; j < k_LengthEntries; ++j)
		{
			t[j] = 1.0f / std::sqrt(static_cast<float>(j) * k_LengthStep);
		}
		return t;
	}();
	return table;
}
} // namespace

float land_normal::EdgeScale(uint32_t climb)
{
	return climb < k_EdgeEntries ? EdgeTable()[climb] : EdgeFormula(climb);
}

float land_normal::LengthScale(uint32_t index)
{
	// A unit normal keeps the index at 1023 or less
	return LengthTable()[index < k_LengthEntries ? index : k_LengthEntries - 1];
}

glm::vec3 land_normal::OfCell(uint32_t fracX, uint32_t fracZ, bool split, int32_t h00, int32_t h01, int32_t h10, int32_t h11)
{
	// B is the triangle's corner at its right angle, P and Q the other two
	int32_t hB;
	int32_t hP;
	int32_t hQ;
	float bx;
	float bz;
	float pz;
	float qz;
	if (split)
	{
		hP = h10;
		hQ = h01;
		pz = 0.0f;
		qz = k_CellEdge;
		if (static_cast<int32_t>(fracZ) > 0xFFFF - static_cast<int32_t>(fracX))
		{
			hB = h11;
			bx = k_CellEdge;
			bz = k_CellEdge;
		}
		else
		{
			hB = h00;
			bx = 0.0f;
			bz = 0.0f;
		}
	}
	else
	{
		hP = h11;
		hQ = h00;
		pz = k_CellEdge;
		qz = 0.0f;
		if (fracX > fracZ)
		{
			hB = h10;
			bx = k_CellEdge;
			bz = 0.0f;
		}
		else
		{
			hB = h01;
			bx = 0.0f;
			bz = k_CellEdge;
		}
	}
	const int32_t dP = hP - hB;
	const int32_t dQ = hQ - hB;

	const float scale = EdgeScale(static_cast<uint32_t>(std::abs(dP))) * EdgeScale(static_cast<uint32_t>(std::abs(dQ)));
	const float qx = -bx;
	const float qy = static_cast<float>(dQ) * k_HeightUnit;
	const float qzRel = qz - bz;
	const float px = k_CellEdge - bx;
	const float py = static_cast<float>(dP) * k_HeightUnit;
	const float pzRel = pz - bz;
	glm::vec3 n;
	n.x = (pzRel * qy - py * qzRel) * scale;
	n.y = (qzRel * px - pzRel * qx) * scale;
	n.z = (py * qx - qy * px) * scale;

	const float squared = ((n.z * n.z + n.x * n.x) + n.y * n.y) * k_LengthSteps;
	const auto index = static_cast<uint32_t>(std::lrint(squared));
	const float length = LengthScale(index);
	n.x = length * n.x;
	n.y = length * n.y;
	n.z = length * n.z;
	if (n.y < 0.0f)
	{
		n = -n;
	}
	return n;
}

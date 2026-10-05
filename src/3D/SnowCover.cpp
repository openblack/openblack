/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SnowCover.h"

#include <algorithm>

namespace openblack::snow_cover
{

namespace
{
constexpr float k_InverseCell = 0.025f;
/// A turn's time, and how much of a storm's snow settles in it
constexpr float k_Turn = 0.1f;
constexpr float k_Settles = 0.03f;
/// The rows of the grid that melt together, every so often, and by how much
constexpr int32_t k_Bands = 8;
constexpr int32_t k_BandRows = k_GridSize / k_Bands;
constexpr float k_MeltEvery = 0.3f;
constexpr float k_MeltBy = 2.0f;
constexpr float k_ObjectLeast = 20.0f;

size_t Index(int32_t x, int32_t z)
{
	return (static_cast<size_t>(z) * k_GridSize) + static_cast<size_t>(x);
}
} // namespace

float StormSnowPerTurn(int8_t snow, float strength)
{
	const auto whole = static_cast<int32_t>(static_cast<float>(snow) * strength);
	return static_cast<float>(whole) * k_Turn * k_Settles;
}

void AddStorm(std::span<float> grid, glm::vec2 centre, float innerRadius, float outerRadius, float amount)
{
	const auto middle = centre * k_InverseCell;
	const float inner = innerRadius * k_InverseCell;
	float outer = outerRadius * k_InverseCell;
	if (outer <= inner)
	{
		outer = inner + 0.02f;
	}
	const auto x0 = static_cast<int32_t>(middle.x - outer);
	const auto z0 = static_cast<int32_t>(middle.y - outer);
	const auto x1 = std::min(static_cast<int32_t>(middle.x + outer + 2.0f), k_GridSize);
	const auto z1 = std::min(static_cast<int32_t>(middle.y + outer + 2.0f), k_GridSize);
	if (x0 >= k_GridSize || z0 >= k_GridSize || x1 <= 0 || z1 <= 0)
	{
		return;
	}
	const float inner2 = inner * inner;
	const float outer2 = outer * outer;
	for (int32_t z = std::max(z0, 0); z < z1; ++z)
	{
		const float dz = static_cast<float>(z) - middle.y;
		for (int32_t x = std::max(x0, 0); x < x1; ++x)
		{
			const float dx = static_cast<float>(x) - middle.x;
			const float distance2 = (dx * dx) + (dz * dz);
			if (distance2 >= outer2)
			{
				continue;
			}
			// Beyond the inner radius the snow thins with the square of the distance, and turns to melting before the edge
			const float laid = distance2 > inner2 ? ((outer2 - distance2) - inner2) * (amount / (outer2 - inner2)) : amount;
			auto& depth = grid[Index(x, z)];
			depth = std::clamp(depth + laid, 0.0f, k_MaxDepth);
		}
	}
}

void Melt(std::span<float> grid, Melting& melting, float seconds)
{
	melting.clock += seconds;
	while (melting.clock > k_MeltEvery)
	{
		melting.clock -= k_MeltEvery;
		melting.band = (melting.band + 1) % k_Bands;
		const auto first = Index(0, melting.band * k_BandRows);
		const auto band = grid.subspan(first, static_cast<size_t>(k_BandRows) * k_GridSize);
		std::ranges::for_each(band, [](float& depth) { depth = depth > k_MeltBy ? depth - k_MeltBy : 0.0f; });
	}
}

float DepthAt(std::span<const float> grid, glm::vec2 xz)
{
	const auto cell = xz * k_InverseCell;
	constexpr auto k_Last = static_cast<float>(k_GridSize - 1);
	if (cell.x < 0.0f || cell.x >= k_Last || cell.y < 0.0f || cell.y >= k_Last)
	{
		return 0.0f;
	}
	const auto x = static_cast<int32_t>(cell.x);
	const auto z = static_cast<int32_t>(cell.y);
	const float wx = cell.x - static_cast<float>(x);
	const float wz = cell.y - static_cast<float>(z);
	const float d00 = grid[Index(x, z)];
	const float d10 = grid[Index(x + 1, z)];
	const float d01 = grid[Index(x, z + 1)];
	const float d11 = grid[Index(x + 1, z + 1)];
	const float near = d00 + ((d10 - d00) * wx);
	const float far = d01 + ((d11 - d01) * wx);
	return near + ((far - near) * wz);
}

int32_t ObjectLevel(float depth, int32_t rate, int32_t cap)
{
	const auto beyond = static_cast<int32_t>(std::max(depth - k_ObjectLeast, 0.0f));
	return std::min((beyond * rate) >> 8, cap);
}

int32_t ObjectThreshold(int32_t level)
{
	const auto clamped = std::clamp(level, 0, 255);
	return std::max(250 - clamped, 0);
}

int32_t LandLevel(float depth, uint8_t noise)
{
	const auto whole = static_cast<int32_t>(depth);
	return std::clamp((whole - static_cast<int32_t>(noise)) >> 3, 0, 16);
}

uint8_t LandWhite(uint8_t noise)
{
	return static_cast<uint8_t>((((noise >> 2) + (noise >> 1)) & 1) + 14);
}

} // namespace openblack::snow_cover

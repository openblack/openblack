/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TempleExteriorMorph.h"

#include <cassert>
#include <cmath>

#include <algorithm>

#include <fmt/format.h>

namespace openblack::TempleExteriorMorph
{

namespace
{
/// The size's index stops short of the largest
constexpr float k_SizeShort = 0.9999f;
/// From neutral the texture goes halfway either way: twice 255 a whole
constexpr float k_TextureWeight = 510.0f;
} // namespace

float Step(float current, float target)
{
	if (std::abs(current - target) <= k_Near)
	{
		return target;
	}
	return current < target ? std::min(current + k_Step, target) : std::max(current - k_Step, target);
}

std::array<Corner, 4> Corners(float size, float alignment)
{
	// Each the index below and the one after, as far as there are, and how much of the one below
	const auto sizeBelow = static_cast<int32_t>(2.0f * std::min(size, k_SizeShort));
	const auto sizeLow = static_cast<uint32_t>(std::clamp(sizeBelow, 0, static_cast<int32_t>(k_Sizes) - 1));
	const auto sizeHigh = static_cast<uint32_t>(std::clamp(sizeBelow + 1, 0, static_cast<int32_t>(k_Sizes) - 1));
	const float sizeWeight = std::clamp(static_cast<float>(sizeHigh) - (2.0f * size), 0.0f, 1.0f);

	const auto stageBelow = static_cast<int32_t>(4.0f * alignment);
	const auto stageLow = static_cast<uint32_t>(std::clamp(stageBelow, 0, static_cast<int32_t>(k_Stages) - 1));
	const auto stageHigh = static_cast<uint32_t>(std::clamp(stageBelow + 1, 0, static_cast<int32_t>(k_Stages) - 1));
	const float stageWeight = static_cast<float>(stageHigh) - (4.0f * alignment);

	return {{
	    {sizeLow, stageLow, sizeWeight * stageWeight},
	    {sizeHigh, stageLow, (1.0f - sizeWeight) * stageWeight},
	    {sizeLow, stageHigh, sizeWeight * (1.0f - stageWeight)},
	    {sizeHigh, stageHigh, (1.0f - sizeWeight) * (1.0f - stageWeight)},
	}};
}

std::string MeshName(uint32_t size, uint32_t stage)
{
	return fmt::format("b_temple{}{}_l3d", size, stage);
}

TextureBlend TextureOf(float alignment)
{
	const bool good = alignment > 0.5f;
	const float along = good ? alignment - 0.5f : alignment;
	const auto weight = std::clamp(static_cast<int32_t>(along * k_TextureWeight), 0, 255);
	return {
	    .from = good ? Look::Neutral : Look::Evil,
	    .to = good ? Look::Good : Look::Neutral,
	    .weight = static_cast<uint8_t>(weight),
	};
}

std::string ImageName(Look look, uint32_t set)
{
	constexpr std::array<std::string_view, 3> k_Looks {"evil", "neutral", "good"};
	return fmt::format("{}{}", k_Looks.at(static_cast<size_t>(look)), set);
}

void BlendTexels(std::span<const uint16_t> from, std::span<const uint16_t> to, uint8_t weight, std::span<uint16_t> blended)
{
	assert(from.size() == to.size() && to.size() == blended.size());
	const uint32_t toWeight = weight;
	const uint32_t fromWeight = 255 - toWeight;
	// Each channel in its place, the two by their weights over 255, rounded down
	const auto channel = [toWeight, fromWeight](uint32_t a, uint32_t b, uint32_t mask) {
		return ((((b & mask) * toWeight) + ((a & mask) * fromWeight)) / 255) & mask;
	};
	for (size_t i = 0; i < blended.size(); ++i)
	{
		const uint32_t a = from[i];
		const uint32_t b = to[i];
		blended[i] = static_cast<uint16_t>(channel(a, b, 0x00F) | channel(a, b, 0x0F0) | channel(a, b, 0xF00) | (a & 0xF000));
	}
}

} // namespace openblack::TempleExteriorMorph

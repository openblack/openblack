/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureSkin.h"

#include <cmath>

#include <algorithm>

using namespace openblack;
using namespace openblack::creature_skin;

namespace
{
/// The axis is scaled to 256 steps before it is capped at the most a weight can be
constexpr float k_WeightSteps = 256.0f;
} // namespace

uint8_t creature_skin::BlendWeight(float evilGood)
{
	// Truncated, as the game converts it
	const auto steps = static_cast<int32_t>(std::abs(evilGood) * k_WeightSteps);
	return static_cast<uint8_t>(std::clamp<int32_t>(steps, 0, k_MaxWeight));
}

uint16_t creature_skin::BlendTexel(uint16_t base, uint16_t other, uint8_t weight)
{
	uint16_t result = 0;
	for (uint32_t shift = 0; shift < 16; shift += 4)
	{
		const auto from = static_cast<uint8_t>((base >> shift) & 0xFu);
		const auto to = static_cast<uint8_t>((other >> shift) & 0xFu);
		result = static_cast<uint16_t>(result | (BlendChannel(from, to, weight) << shift));
	}
	return result;
}

std::optional<uint32_t> creature_skin::PairedSkin(std::span<const uint32_t> baseSkins, std::span<const uint32_t> variantSkins,
                                                  uint32_t baseSkin)
{
	const auto found = std::ranges::find(baseSkins, baseSkin);
	if (found == baseSkins.end())
	{
		return std::nullopt;
	}
	const auto index = static_cast<size_t>(std::distance(baseSkins.begin(), found));
	if (index >= variantSkins.size())
	{
		return std::nullopt;
	}
	return variantSkins[index];
}

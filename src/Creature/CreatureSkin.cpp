/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureSkin.h"

#include <cassert>
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

void creature_skin::Compose(std::span<uint16_t> out, std::span<const uint16_t> base, std::span<const uint16_t> variant,
                            uint8_t weight, const Layers& layers)
{
	assert(out.size() == base.size());
	if (variant.size() == base.size() && weight > 0)
	{
		std::ranges::transform(base, variant, out.begin(),
		                       [weight](uint16_t from, uint16_t to) { return BlendTexel(from, to, weight); });
	}
	else
	{
		std::ranges::copy(base, out.begin());
	}
	if (layers.art == nullptr || out.size() != static_cast<size_t>(creature_tattoo::k_SkinSize) * creature_tattoo::k_SkinSize)
	{
		return;
	}
	if (layers.sites.has_value())
	{
		for (const auto& slot : layers.tattoos)
		{
			if (slot.Empty() || slot.design >= layers.art->designs.size())
			{
				continue;
			}
			const auto& site = layers.sites->at(slot.site);
			if (site.enabled && site.skin == layers.skinIndex)
			{
				creature_tattoo::Paint(out, layers.art->designs.at(slot.design), slot.colour, site);
			}
		}
	}
	for (const auto& wound : layers.marks.wounds)
	{
		if (wound.skin == layers.skinIndex)
		{
			creature_marks::PaintWound(out, layers.art->damage, wound);
		}
	}
	for (const auto& blood : layers.marks.blood)
	{
		if (blood.skin == layers.skinIndex)
		{
			creature_marks::PaintBlood(out, blood);
		}
	}
}

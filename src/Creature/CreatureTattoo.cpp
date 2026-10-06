/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureTattoo.h"

#include <cassert>

#include <algorithm>

using namespace openblack;
using namespace openblack::creature_tattoo;

namespace
{
/// The masks' sizes by how big the tattoo is, in 64ths of the skin's width: at least 48, 24, 12 or 6, or smaller
constexpr std::array<int32_t, k_MaskLevels - 1> k_LevelThresholds {48, 24, 12, 6};
constexpr float k_SizeSteps = 64.0f;
/// The skin's channels, 4 bits each, blue the lowest
constexpr uint16_t k_Blue = 0x000F;
constexpr uint16_t k_Green = 0x00F0;
constexpr uint16_t k_Red = 0x0F00;
constexpr uint16_t k_Opaque = 0xF000;

Mask Halved(const Mask& mask)
{
	Mask half {.size = mask.size / 2, .levels = {}};
	half.levels.resize(static_cast<size_t>(half.size) * half.size);
	for (uint32_t y = 0; y < half.size; ++y)
	{
		for (uint32_t x = 0; x < half.size; ++x)
		{
			const auto sum = mask.At(x * 2, y * 2) + mask.At((x * 2) + 1, y * 2) + mask.At(x * 2, (y * 2) + 1) +
			                 mask.At((x * 2) + 1, (y * 2) + 1);
			half.levels[(y * half.size) + x] = static_cast<uint8_t>(sum / 4);
		}
	}
	return half;
}
} // namespace

Design creature_tattoo::DesignFromAtlas(std::span<const std::array<uint8_t, 3>> atlas, uint32_t atlasWidth, uint32_t design)
{
	Design result {};
	auto& full = result.front();
	full.size = k_DesignSize;
	full.levels.assign(static_cast<size_t>(k_DesignSize) * k_DesignSize, 0);
	const auto left = (design % k_DesignsPerRow) * k_DesignSize;
	const auto top = (design / k_DesignsPerRow) * k_DesignSize;
	for (uint32_t y = 0; y < k_DesignSize; ++y)
	{
		for (uint32_t x = 0; x < k_DesignSize; ++x)
		{
			const auto index = (static_cast<size_t>(top + y) * atlasWidth) + left + x;
			if (index < atlas.size() && left + x < atlasWidth)
			{
				full.levels[(y * k_DesignSize) + x] = static_cast<uint8_t>(atlas[index][2] >> 4u);
			}
		}
	}
	for (size_t level = 1; level < result.size(); ++level)
	{
		result.at(level) = Halved(result.at(level - 1));
	}
	return result;
}

size_t creature_tattoo::MaskLevel(float size)
{
	// Truncated, as the game converts it
	const auto steps = static_cast<int32_t>(std::min(size, 1.0f) * k_SizeSteps);
	for (size_t level = 0; level < k_LevelThresholds.size(); ++level)
	{
		if (steps >= k_LevelThresholds.at(level))
		{
			return level;
		}
	}
	return k_MaskLevels - 1;
}

Mask creature_tattoo::Oriented(const Mask& mask, uint8_t rotation, bool mirror)
{
	Mask result {.size = mask.size, .levels = std::vector<uint8_t>(mask.levels.size(), 0)};
	const auto last = mask.size - 1;
	for (uint32_t y = 0; y < mask.size; ++y)
	{
		for (uint32_t x = 0; x < mask.size; ++x)
		{
			// Where the texel at (x, y) of the turned mask comes from
			uint32_t fromX = x;
			uint32_t fromY = y;
			switch (rotation & 3u)
			{
			case 1:
				fromX = y;
				fromY = last - x;
				break;
			case 2:
				fromX = last - x;
				fromY = last - y;
				break;
			case 3:
				fromX = last - y;
				fromY = x;
				break;
			default:
				break;
			}
			const auto toX = mirror ? last - x : x;
			result.levels[(y * mask.size) + toX] = mask.At(fromX, fromY);
		}
	}
	return result;
}

uint16_t creature_tattoo::PaintTexel(uint16_t skin, uint8_t level, const glm::u8vec3& colour)
{
	const uint32_t m = std::min(level, k_MaxLevel);
	const uint32_t keep = k_MaxLevel - m;
	const auto blue = (((skin & k_Blue) * keep) + (m * (colour.b >> 4u))) / k_MaxLevel;
	const auto green = ((((skin & k_Green) * keep) + (m * (colour.g & 0xF0u))) / k_MaxLevel) & k_Green;
	const auto red = ((((skin & k_Red) * keep) + (m * (colour.r & 0xF0u) * 16u)) / k_MaxLevel) & k_Red;
	return static_cast<uint16_t>(red | green | blue | k_Opaque);
}

void creature_tattoo::Paint(std::span<uint16_t> skin, const Design& design, const glm::u8vec3& colour, const Site& site)
{
	assert(skin.size() == static_cast<size_t>(k_SkinSize) * k_SkinSize);
	if (site.size < 0.0f)
	{
		return;
	}
	const auto level = MaskLevel(std::min(site.size, 1.0f));
	const auto mask = Oriented(design.at(level), site.rotation, site.mirror);
	// The size doubles for each halving of the mask, so the tattoo spans the size times the largest mask's width
	auto scaled = std::min(site.size, 1.0f);
	for (size_t i = 0; i < level; ++i)
	{
		scaled += scaled;
	}
	const auto extent = static_cast<int32_t>(static_cast<float>(mask.size) * scaled);
	if (extent <= 0 || mask.size == 0)
	{
		return;
	}
	const auto half = extent / 2;
	const auto left = static_cast<int32_t>(site.u) - half;
	const auto top = static_cast<int32_t>(site.v) - half;
	constexpr auto k_Size = static_cast<int32_t>(k_SkinSize);
	if (left < 0 || top < 0 || site.u + half >= k_Size || site.v + half >= k_Size)
	{
		return;
	}
	for (int32_t y = 0; y < extent; ++y)
	{
		const auto maskY = static_cast<uint32_t>((y * static_cast<int32_t>(mask.size)) / extent);
		for (int32_t x = 0; x < extent; ++x)
		{
			const auto maskX = static_cast<uint32_t>((x * static_cast<int32_t>(mask.size)) / extent);
			const auto index = static_cast<size_t>(((top + y) * k_Size) + left + x);
			if (index >= skin.size())
			{
				continue;
			}
			auto& texel = skin[index];
			texel = PaintTexel(texel, mask.At(maskX, maskY), colour);
		}
	}
}

glm::u8vec3 creature_tattoo::PaletteColour(std::span<const std::array<uint8_t, 3>> palette, uint32_t column, uint32_t row,
                                           float brightness)
{
	const auto index =
	    (static_cast<size_t>(std::min(row, k_PaletteRows - 1)) * k_PaletteColumns) + std::min(column, k_PaletteColumns - 1);
	if (index >= palette.size())
	{
		return glm::u8vec3(255);
	}
	const glm::ivec3 colour {palette[index][0], palette[index][1], palette[index][2]};
	glm::ivec3 result;
	if (brightness >= 0.5f)
	{
		// Towards white
		const auto towards = static_cast<int32_t>((brightness - 0.5f) * 2.0f * 256.0f);
		result = colour + (((glm::ivec3(255) - colour) * towards) / 256);
	}
	else
	{
		// Towards black
		const auto factor = static_cast<int32_t>(brightness * 2.0f * 256.0f);
		result = (colour * factor) / 256;
	}
	return glm::u8vec3(glm::clamp(result, 0, 255));
}

std::optional<size_t> creature_tattoo::SlotFor(const Slots& slots, uint8_t site, uint8_t design)
{
	const auto index = [&slots](auto&& predicate) -> std::optional<size_t> {
		const auto found = std::ranges::find_if(slots, predicate);
		return found != slots.end() ? std::optional(static_cast<size_t>(std::distance(slots.begin(), found))) : std::nullopt;
	};
	if (const auto same = index([site, design](const Slot& slot) { return slot.site == site && slot.design == design; }))
	{
		return same;
	}
	if (const auto empty = index([](const Slot& slot) { return slot.Empty(); }))
	{
		return empty;
	}
	return index([site](const Slot& slot) { return slot.site == site; });
}

Slot creature_tattoo::FromWord(uint32_t word)
{
	return {
	    .design = static_cast<uint8_t>(word & 0xFu),
	    .site = static_cast<uint8_t>((word >> 4u) & 0xFu),
	    .colour = {static_cast<uint8_t>(word >> 24u), static_cast<uint8_t>(word >> 16u), static_cast<uint8_t>(word >> 8u)},
	};
}

uint32_t creature_tattoo::ToWord(const Slot& slot)
{
	return (slot.design & 0xFu) | ((slot.site & 0xFu) << 4u) | (static_cast<uint32_t>(slot.colour.b) << 8u) |
	       (static_cast<uint32_t>(slot.colour.g) << 16u) | (static_cast<uint32_t>(slot.colour.r) << 24u);
}

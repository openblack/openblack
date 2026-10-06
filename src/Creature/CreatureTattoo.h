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
#include <optional>
#include <span>
#include <vector>

#include <glm/gtc/type_precision.hpp>
#include <glm/vec3.hpp>

/// A creature can wear up to eight tattoos. Each is one of sixteen designs, in a colour, painted at one of the places its
/// species has for tattoos: a point on one of its skins, at a size, turned and perhaps flipped. A design is a mask of
/// sixteen levels, from Data/Textures/PlayersSymbols.raw, painted over the skin a 4-bit channel at a time. The tattoos are
/// painted after the skin has been blended towards evil or good.
namespace openblack::creature_tattoo
{
/// The places on a body a tattoo can go
struct Site
{
	bool enabled {false};
	/// The centre, in texels of a skin 256 wide
	uint8_t u {0};
	uint8_t v {0};
	/// Which of the base mesh's skins, in the order it lists them
	uint8_t skin {0};
	/// How wide a tattoo is, a fraction of the skin's width
	float size {0.0f};
	/// Flipped left to right, after it is turned
	bool mirror {false};
	/// Quarter turns
	uint8_t rotation {0};
};
constexpr size_t k_SlotCount = 8;
using Sites = std::array<Site, k_SlotCount>;
/// The site of an empty slot
constexpr uint8_t k_NoSite = 0xF;

/// A tattoo a creature wears
struct Slot
{
	/// The design, 0 to 15
	uint8_t design {0};
	/// The site, 0 to 7, or k_NoSite for an empty slot
	uint8_t site {k_NoSite};
	/// 0 to 255 a channel
	glm::u8vec3 colour {0};

	[[nodiscard]] bool Empty() const { return site >= k_SlotCount; }
	bool operator==(const Slot&) const = default;
};
using Slots = std::array<Slot, k_SlotCount>;

/// The designs, four to a row of 64 by 64 texel cells
constexpr uint32_t k_DesignCount = 16;
constexpr uint32_t k_DesignSize = 64;
constexpr uint32_t k_DesignsPerRow = 4;
/// A design's mask is kept at 64, 32, 16, 8 and 4 texels across
constexpr size_t k_MaskLevels = 5;
/// The most a mask's level can be: the tattoo's colour alone
constexpr uint8_t k_MaxLevel = 15;

/// A square mask of levels, 0 (the skin) to 15 (the tattoo), a row at a time
struct Mask
{
	uint32_t size {0};
	std::vector<uint8_t> levels;

	[[nodiscard]] uint8_t At(uint32_t x, uint32_t y) const { return levels.at((y * size) + x); }
};

/// A design at each of its sizes, the largest first
using Design = std::array<Mask, k_MaskLevels>;

/// A design cut from the atlas of designs, an image of red, green and blue a row at a time: its level at each texel is
/// the top four bits of the blue. Each smaller mask averages squares of four of the one before, rounded down.
[[nodiscard]] Design DesignFromAtlas(std::span<const std::array<uint8_t, 3>> atlas, uint32_t atlasWidth, uint32_t design);

/// Which of a design's masks a tattoo of a size is painted from: the largest for a quarter of the skin's width or more,
/// smaller for smaller tattoos
[[nodiscard]] size_t MaskLevel(float size);

/// A mask turned by quarter turns, clockwise as the skin is seen, then flipped left to right
[[nodiscard]] Mask Oriented(const Mask& mask, uint8_t rotation, bool mirror);

/// A texel of the skin, 4 bits a channel with blue the lowest, with the tattoo's colour painted over it at a level of
/// 15: each channel moved towards the top four bits of the colour's, rounded down. The texel comes out opaque.
[[nodiscard]] uint16_t PaintTexel(uint16_t skin, uint8_t level, const glm::u8vec3& colour);

/// The skin's width and height in texels
constexpr uint32_t k_SkinSize = 256;

/// A tattoo painted onto a skin of 256 by 256 texels: the design's mask for its size, turned and flipped, stretched to
/// the size times the largest mask's width (64 texels at size 1) and centred on the site. A tattoo that would run over
/// an edge isn't painted.
void Paint(std::span<uint16_t> skin, const Design& design, const glm::u8vec3& colour, const Site& site);

/// The tattoo palette, Data/tattoocols.raw, is 32 columns by 128 rows of colours
constexpr uint32_t k_PaletteColumns = 32;
constexpr uint32_t k_PaletteRows = 128;

/// A colour of the palette at a brightness, 0 to 1: a half leaves it as it is, more draws it towards white, less towards
/// black
[[nodiscard]] glm::u8vec3 PaletteColour(std::span<const std::array<uint8_t, 3>> palette, uint32_t column, uint32_t row,
                                        float brightness);

/// The slot a design put on a site goes in: the slot with that design on that site already, then the first empty one,
/// then the one with something else on that site; none when every slot holds another site's tattoo
[[nodiscard]] std::optional<size_t> SlotFor(const Slots& slots, uint8_t site, uint8_t design);

/// A tattoo as a saved creature keeps it, one 32-bit word a slot: the design in the lowest four bits, the site in the
/// next four, then the colour's blue, green and red bytes
[[nodiscard]] Slot FromWord(uint32_t word);
[[nodiscard]] uint32_t ToWord(const Slot& slot);
} // namespace openblack::creature_tattoo

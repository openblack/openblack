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
#include <span>
#include <vector>

#include <entt/core/hashed_string.hpp>

namespace openblack
{

/// The weather system's palette.raw: 32 by 32 colours. Its first three rows are the good, neutral and evil colours of the
/// land through the day; its next ones a dark colour, the moon's and a warm one, by alignment.
class LandLightPalette
{
public:
	static constexpr size_t k_Side = 32;
	static constexpr entt::hashed_string k_Id = entt::hashed_string("weather/palette");

	/// Red, green, blue and alpha bytes; throws for a palette of the wrong size
	explicit LandLightPalette(std::span<const uint8_t> bytes);

	/// 0xAARRGGBB
	[[nodiscard]] uint32_t At(size_t row, size_t column) const { return _colours.at(row * k_Side + column); }

private:
	std::vector<uint32_t> _colours;
};

/// The land's light: 256 colours, one for each level of a cell's luminosity, rebuilt every frame from the palette for
/// the time of day and the alignment the sky shows.
///
/// The darkest 48 levels ramp from the dark colour up to the land's colour and on to the warm one, the rest from the
/// dark colour up to the land's, all divided by 200 rather than 255, so the brightest cells come out brighter than the
/// palette.
class LandLightTable
{
public:
	static constexpr size_t k_Size = 256;

	/// skyType runs from 0 at night to 2 by day, and alignment from -1, evil, to 1, good
	void Build(const LandLightPalette& palette, float skyType, float alignment) noexcept;

	/// The table as RGBA8 texels
	[[nodiscard]] const std::array<uint32_t, k_Size>& GetTexels() const noexcept { return _texels; }

private:
	std::array<uint32_t, k_Size> _texels {};
};

} // namespace openblack

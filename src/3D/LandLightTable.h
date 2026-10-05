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
#include <glm/vec3.hpp>

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

	/// The distance haze of the frame: from `nearDistance` to `farDistance` from the camera things fade towards the haze
	/// colour, which is added to them, while their own colour is scaled down to k of 256. The colour is a third of the
	/// land's, k follows its brightness, and the haze closes in at dusk.
	struct Haze
	{
		float nearDistance {400.0f};
		float farDistance {900.0f};
		float k {256.0f};
		glm::vec3 colour {0.0f}; ///< 0 to 255
	};

	/// The table as RGBA8 texels
	[[nodiscard]] const std::array<uint32_t, k_Size>& GetTexels() const noexcept { return _texels; }
	[[nodiscard]] const Haze& GetHaze() const noexcept { return _haze; }
	/// The land's colour of the frame, by the time of day and alignment, 0xRRGGBB
	[[nodiscard]] uint32_t GetLandColour() const noexcept { return _landColour; }
	/// The palette's warm colour of the frame, which the darkest levels go on to, 0xRRGGBB
	[[nodiscard]] uint32_t GetWarmColour() const noexcept { return _warmColour; }
	/// The moon's colour of the frame, by the alignment, 0xRRGGBB
	[[nodiscard]] uint32_t GetMoonColour() const noexcept { return _moonColour; }

private:
	std::array<uint32_t, k_Size> _texels {};
	Haze _haze;
	uint32_t _landColour {0xFFFFFF};
	uint32_t _warmColour {0xFFFFFF};
	uint32_t _moonColour {0xFFFFFF};
};

} // namespace openblack

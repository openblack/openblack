/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

namespace openblack
{

/**
 * @brief A 32 bit integer color, where each component is an 8 bit 0-255 value.
 */
class Color
{
public:
	constexpr Color()
	    : r(0)
	    , g(0)
	    , b(0)
	    , a(255)
	{
	}
	constexpr Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
	    : r(red)
	    , g(green)
	    , b(blue)
	    , a(alpha)
	{
	}

	explicit constexpr Color(uint32_t rgba)
	    : r((rgba >> 24) & 0xFF)
	    , g((rgba >> 16) & 0xFF)
	    , b((rgba >> 8) & 0xFF)
	    , a(rgba & 0xFF)
	{
	}

	bool operator==(const Color& other) const { return r == other.r && g == other.g && b == other.b && a == other.a; }
	bool operator!=(const Color& other) const { return !(*this == other); }

	constexpr explicit operator uint32_t() const
	{
		return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) | (static_cast<uint32_t>(g) << 8) |
		       static_cast<uint32_t>(r);
	}

	uint8_t r, g, b, a;
};

// I'd rather have Color::White but it all bitches at me
namespace colors
{
constexpr Color k_White = Color(255, 255, 255);
constexpr Color k_Black = Color(0, 0, 0);
constexpr Color k_Good = Color(235, 235, 183);
constexpr Color k_Evil = Color(255, 180, 180);
}; // namespace colors

} // namespace openblack

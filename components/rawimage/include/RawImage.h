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

/// The game's .raw images: rows of pixels with no header, top row first, either three bytes a pixel (red, green, blue)
/// or one (an alpha or grey level). The size isn't in the file, so it is known from what the image is for, and an
/// alpha image sits beside its colours in a file of the same name ending in "a".
namespace openblack::rawimage
{

struct RgbImage
{
	uint32_t width {0};
	uint32_t height {0};
	/// Red, green and blue, a row at a time
	std::vector<std::array<uint8_t, 3>> pixels;

	[[nodiscard]] const std::array<uint8_t, 3>& At(uint32_t x, uint32_t y) const { return pixels.at((y * width) + x); }
};

struct GreyImage
{
	uint32_t width {0};
	uint32_t height {0};
	std::vector<uint8_t> pixels;

	[[nodiscard]] uint8_t At(uint32_t x, uint32_t y) const { return pixels.at((y * width) + x); }
};

/// An image of three bytes a pixel, or none when the bytes aren't exactly that many
[[nodiscard]] std::optional<RgbImage> DecodeRgb(std::span<const uint8_t> bytes, uint32_t width, uint32_t height);
/// An image of one byte a pixel, or none when the bytes aren't exactly that many
[[nodiscard]] std::optional<GreyImage> DecodeGrey(std::span<const uint8_t> bytes, uint32_t width, uint32_t height);

} // namespace openblack::rawimage

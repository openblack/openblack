/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "RawImage.h"

#include <cstring>

namespace openblack::rawimage
{

std::optional<RgbImage> DecodeRgb(std::span<const uint8_t> bytes, uint32_t width, uint32_t height)
{
	constexpr size_t k_BytesPerPixel = 3;
	const auto count = static_cast<size_t>(width) * height;
	if (count == 0 || bytes.size() != count * k_BytesPerPixel)
	{
		return std::nullopt;
	}
	RgbImage image {.width = width, .height = height, .pixels = {}};
	image.pixels.resize(count);
	std::memcpy(image.pixels.data(), bytes.data(), bytes.size());
	return image;
}

std::optional<GreyImage> DecodeGrey(std::span<const uint8_t> bytes, uint32_t width, uint32_t height)
{
	const auto count = static_cast<size_t>(width) * height;
	if (count == 0 || bytes.size() != count)
	{
		return std::nullopt;
	}
	return GreyImage {.width = width, .height = height, .pixels = {bytes.begin(), bytes.end()}};
}

} // namespace openblack::rawimage

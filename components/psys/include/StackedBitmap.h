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

#include <optional>
#include <span>
#include <vector>

/// The light maps the particle effects stamp on the land (Data/Spells/LightMaps): raw square frames of RGB or grey bytes,
/// laid out in the file as a grid of frames, as many to a row as the whole number below the square root of the count.
namespace openblack::psys
{

/// The frames of a light map one after the other, each pitch by pitch texels of `channels` bytes, rows along the land's x
struct StackedBitmap
{
	int pitch {0};
	int frames {0};
	/// 3 for colours, 1 for grey
	int channels {0};
	std::vector<uint8_t> data;

	/// The texels of a frame, the frame wrapped into those there are; empty for a bitmap without frames
	[[nodiscard]] std::span<const uint8_t> Frame(int frame) const noexcept;
	[[nodiscard]] size_t FrameSize() const noexcept;
};

/// The bitmap in a file's bytes. Nothing unless the file holds exactly framesInFile frames of pitch by pitch texels of
/// `channels` bytes; the first framesInUse of them are kept.
[[nodiscard]] std::optional<StackedBitmap> LoadStackedBitmap(std::span<const uint8_t> bytes, int pitch, int channels,
                                                             int framesInFile, int framesInUse);

} // namespace openblack::psys

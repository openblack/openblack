/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "StackedBitmap.h"

#include <cmath>

#include <algorithm>

using namespace openblack::psys;

size_t StackedBitmap::FrameSize() const noexcept
{
	return static_cast<size_t>(pitch) * static_cast<size_t>(pitch) * static_cast<size_t>(channels);
}

std::span<const uint8_t> StackedBitmap::Frame(int frame) const noexcept
{
	if (frames <= 0 || data.empty())
	{
		return {};
	}
	const auto index = static_cast<size_t>(static_cast<uint32_t>(frame) % static_cast<uint32_t>(frames));
	return std::span<const uint8_t>(data).subspan(index * FrameSize(), FrameSize());
}

std::optional<StackedBitmap> openblack::psys::LoadStackedBitmap(std::span<const uint8_t> bytes, int pitch, int channels,
                                                                int framesInFile, int framesInUse)
{
	if (pitch <= 0 || channels <= 0 || framesInFile <= 0)
	{
		return std::nullopt;
	}
	StackedBitmap bitmap;
	bitmap.pitch = pitch;
	bitmap.channels = channels;
	if (bytes.size() != bitmap.FrameSize() * static_cast<size_t>(framesInFile))
	{
		return std::nullopt;
	}
	bitmap.frames = std::max(std::min(framesInUse, framesInFile), 0);
	bitmap.data.reserve(bitmap.FrameSize() * static_cast<size_t>(bitmap.frames));
	const auto side = static_cast<size_t>(pitch);
	const auto texel = static_cast<size_t>(channels);
	const auto perRow = static_cast<size_t>(std::max(1, static_cast<int>(std::sqrt(static_cast<float>(framesInFile)))));
	for (int frame = 0; frame < bitmap.frames; ++frame)
	{
		const auto column = static_cast<size_t>(frame) % perRow;
		const auto row = static_cast<size_t>(frame) / perRow;
		for (size_t y = 0; y < side; ++y)
		{
			// One row of the frame, out of the file's row of frames
			const auto first = (((row * side + y) * perRow + column) * side) * texel;
			const auto last = std::min(first + side * texel, bytes.size());
			const auto count = first < last ? last - first : 0;
			bitmap.data.insert(bitmap.data.end(), bytes.begin() + static_cast<std::ptrdiff_t>(std::min(first, bytes.size())),
			                   bytes.begin() + static_cast<std::ptrdiff_t>(std::min(first, bytes.size()) + count));
			bitmap.data.resize(bitmap.data.size() + (side * texel - count), 0);
		}
	}
	return bitmap;
}

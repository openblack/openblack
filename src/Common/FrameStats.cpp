/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FrameStats.h"

#include <algorithm>
#include <numeric>

namespace openblack::frame_stats
{

Summary Summarise(std::span<const float> milliseconds)
{
	if (milliseconds.empty())
	{
		return {};
	}
	std::vector<float> sorted(milliseconds.begin(), milliseconds.end());
	std::ranges::sort(sorted);
	// The nearest rank: the smallest time that at least 95% of the frames are no longer than
	const auto rank = (sorted.size() * 95 + 99) / 100;
	const auto total = std::accumulate(sorted.begin(), sorted.end(), 0.0);
	return {
	    .count = sorted.size(),
	    .average = static_cast<float>(total / static_cast<double>(sorted.size())),
	    .p95 = sorted.at(std::max<size_t>(rank, 1) - 1),
	    .max = sorted.back(),
	};
}

Summary Window::Take()
{
	const auto summary = Summarise(_times);
	_times.clear();
	return summary;
}

} // namespace openblack::frame_stats

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>

#include <span>
#include <vector>

namespace openblack::frame_stats
{

/// How long a run of frames took, in milliseconds
struct Summary
{
	size_t count {0};
	float average {0.0f};
	/// 95 frames in 100 took this long or less
	float p95 {0.0f};
	float max {0.0f};
};

/// Sums up a run of frame times; an empty run gives an empty summary
[[nodiscard]] Summary Summarise(std::span<const float> milliseconds);

/// Collects the times of the frames since it was last summed up
class Window
{
public:
	void Add(float milliseconds) { _times.push_back(milliseconds); }
	[[nodiscard]] size_t Count() const { return _times.size(); }
	/// Sums up the frames collected so far and starts again
	[[nodiscard]] Summary Take();

private:
	std::vector<float> _times;
};

} // namespace openblack::frame_stats

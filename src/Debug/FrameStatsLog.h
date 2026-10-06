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
#include <map>
#include <string>

#include "Common/FrameStats.h"

#include "../Profiler.h"

namespace openblack::debug
{

/// Logs how long the frames take every so many frames: the average, 95th percentile and slowest frame, what the
/// renderer reports of the CPU and GPU, the average of each profiler stage and, when the renderer profiles its views,
/// the GPU time of each view
class FrameStatsLog
{
public:
	FrameStatsLog(uint32_t interval, bool views);

	/// After each frame, with how long the frame took and the profiler that timed its stages
	void Frame(float milliseconds, const openblack::Profiler& profiler);

private:
	void Report();

	uint32_t _interval;
	bool _views;
	frame_stats::Window _frames;
	frame_stats::Window _gpu;
	std::array<double, static_cast<size_t>(openblack::Profiler::Stage::_count)> _stageTotals {};
	std::array<double, static_cast<size_t>(openblack::Profiler::Stage::_count)> _stageMax {};
	std::map<std::string, double> _viewTotals;
	double _submitTotal {0.0};
	double _waitRenderTotal {0.0};
	uint64_t _drawCallTotal {0};
	uint32_t _reports {0};
};

} // namespace openblack::debug

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FrameStatsLog.h"

#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>

#include <bgfx/bgfx.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace openblack::debug
{

FrameStatsLog::FrameStatsLog(uint32_t interval, bool views)
    : _interval(std::max<uint32_t>(interval, 1))
    , _views(views)
{
}

void FrameStatsLog::Frame(float milliseconds, const openblack::Profiler& profiler)
{
	_frames.Add(milliseconds);

	// The stages timed in this frame: those that started since the frame did
	const auto& entry = profiler.GetEntries().at(profiler.GetEntryIndex(0));
	for (size_t i = 0; i < entry.stages.size(); ++i)
	{
		const auto& stage = entry.stages.at(i);
		if (stage.finalized && stage.start >= entry.frameStart)
		{
			const auto ms = std::chrono::duration<double, std::milli>(stage.end - stage.start).count();
			_stageTotals.at(i) += ms;
			_stageMax.at(i) = std::max(_stageMax.at(i), ms);
		}
	}

	const auto* stats = bgfx::getStats();
	const double toMsCpu = 1000.0 / static_cast<double>(stats->cpuTimerFreq);
	const double toMsGpu = 1000.0 / static_cast<double>(stats->gpuTimerFreq);
	_gpu.Add(static_cast<float>(static_cast<double>(stats->gpuTimeEnd - stats->gpuTimeBegin) * toMsGpu));
	_submitTotal += static_cast<double>(stats->cpuTimeEnd - stats->cpuTimeBegin) * toMsCpu;
	_waitRenderTotal += static_cast<double>(stats->waitRender) * toMsCpu;
	_drawCallTotal += stats->numDraw;
	if (_views)
	{
		for (uint16_t i = 0; i < stats->numViews; ++i)
		{
			const auto& view = stats->viewStats[i];
			_viewTotals[view.name] += static_cast<double>(view.gpuTimeEnd - view.gpuTimeBegin) * toMsGpu;
		}
	}

	if (_frames.Count() >= _interval)
	{
		Report();
	}
}

void FrameStatsLog::Report()
{
	const auto count = static_cast<double>(_frames.Count());
	const auto frames = _frames.Take();
	const auto gpu = _gpu.Take();
	auto logger = spdlog::get("game");
	++_reports;
	SPDLOG_LOGGER_INFO(logger,
	                   "Frame stats #{}: {} frames, frame avg {:.2f} ms p95 {:.2f} max {:.2f} ({:.1f} FPS); GPU avg {:.2f} "
	                   "p95 {:.2f}; render submit {:.2f} wait {:.2f}; {:.0f} draws",
	                   _reports, frames.count, frames.average, frames.p95, frames.max, 1000.0f / frames.average, gpu.average,
	                   gpu.p95, _submitTotal / count, _waitRenderTotal / count, static_cast<double>(_drawCallTotal) / count);

	// The stages that took the longest, nested ones among them
	std::vector<std::pair<double, size_t>> stages;
	for (size_t i = 0; i < _stageTotals.size(); ++i)
	{
		if (_stageTotals.at(i) > 0.0)
		{
			stages.emplace_back(_stageTotals.at(i) / count, i);
		}
	}
	std::ranges::sort(stages, std::greater {});
	std::string line;
	for (const auto& [ms, i] : stages)
	{
		if (ms >= 0.05)
		{
			line += fmt::format(" {}={:.2f}", openblack::Profiler::k_StageNames.at(i), ms);
		}
	}
	SPDLOG_LOGGER_INFO(logger, "Frame stats #{} stages (ms):{}", _reports, line);

	// The stages behind the slowest frames
	std::vector<std::pair<double, size_t>> slowest;
	for (size_t i = 0; i < _stageMax.size(); ++i)
	{
		if (_stageMax.at(i) >= 1.0)
		{
			slowest.emplace_back(_stageMax.at(i), i);
		}
	}
	std::ranges::sort(slowest, std::greater {});
	line.clear();
	for (const auto& [ms, i] : slowest)
	{
		line += fmt::format(" {}={:.2f}", openblack::Profiler::k_StageNames.at(i), ms);
	}
	SPDLOG_LOGGER_INFO(logger, "Frame stats #{} slowest stages (ms):{}", _reports, line);

	if (_views)
	{
		std::vector<std::pair<double, std::string>> views;
		for (const auto& [name, total] : _viewTotals)
		{
			views.emplace_back(total / count, name);
		}
		std::ranges::sort(views, std::greater {});
		line.clear();
		for (const auto& [ms, name] : views)
		{
			if (ms >= 0.02)
			{
				line += fmt::format(" [{}]={:.2f}", name, ms);
			}
		}
		SPDLOG_LOGGER_INFO(logger, "Frame stats #{} GPU views (ms):{}", _reports, line);
	}

	_stageTotals.fill(0.0);
	_stageMax.fill(0.0);
	_viewTotals.clear();
	_submitTotal = 0.0;
	_waitRenderTotal = 0.0;
	_drawCallTotal = 0;
}

} // namespace openblack::debug

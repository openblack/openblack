/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "BenchmarkRecorder.h"

#include <cmath>

#include <algorithm>
#include <iterator>

#include <fmt/format.h>

using namespace openblack;
using namespace openblack::benchmark;

namespace
{
/// A string as JSON writes it, quoted with its quotes, backslashes and control characters escaped
std::string Quoted(std::string_view text)
{
	std::string quoted = "\"";
	for (const auto c : text)
	{
		switch (c)
		{
		case '"':
			quoted += "\\\"";
			break;
		case '\\':
			quoted += "\\\\";
			break;
		case '\n':
			quoted += "\\n";
			break;
		default:
			if (static_cast<unsigned char>(c) < 0x20)
			{
				quoted += fmt::format("\\u{:04x}", static_cast<unsigned>(c));
			}
			else
			{
				quoted += c;
			}
		}
	}
	return quoted + "\"";
}

/// A field as CSV writes it, quoted when it holds a comma or a quote
std::string CsvField(std::string_view text)
{
	if (text.find_first_of(",\"\n") == std::string_view::npos)
	{
		return std::string(text);
	}
	std::string quoted = "\"";
	for (const auto c : text)
	{
		quoted += c == '"' ? std::string("\"\"") : std::string(1, c);
	}
	return quoted + "\"";
}

std::string SummaryJson(const frame_stats::Summary& summary)
{
	return fmt::format(R"({{"mean": {:.4f}, "p95": {:.4f}, "max": {:.4f}}})", summary.average, summary.p95, summary.max);
}
} // namespace

Recorder::Recorder(std::vector<StageInfo> stages, size_t capacity)
    : _stages(std::move(stages))
    , _capacity(std::max<size_t>(capacity, 1))
{
	_frames.reserve(_capacity);
	_updates.reserve(_capacity);
	_renders.reserve(_capacity);
	_draws.reserve(_capacity);
	_stageTimes.reserve(_capacity * _stages.size());
}

void Recorder::Add(float frameMs, float updateMs, float renderMs, std::span<const float> stageMs, float draws)
{
	const auto stages = _stages.size();
	if (_count < _capacity)
	{
		_frames.push_back(frameMs);
		_updates.push_back(updateMs);
		_renders.push_back(renderMs);
		_draws.push_back(draws);
		for (size_t i = 0; i < stages; ++i)
		{
			_stageTimes.push_back(i < stageMs.size() ? stageMs[i] : 0.0f);
		}
		++_count;
		return;
	}
	// Full: the oldest frame makes way
	_frames.at(_next) = frameMs;
	_updates.at(_next) = updateMs;
	_renders.at(_next) = renderMs;
	_draws.at(_next) = draws;
	for (size_t i = 0; i < stages; ++i)
	{
		_stageTimes.at((_next * stages) + i) = i < stageMs.size() ? stageMs[i] : 0.0f;
	}
	_next = (_next + 1) % _capacity;
}

void Recorder::Clear()
{
	_count = 0;
	_next = 0;
	_frames.clear();
	_updates.clear();
	_renders.clear();
	_draws.clear();
	_stageTimes.clear();
}

std::vector<float> Recorder::Column(const std::vector<float>& values, size_t stride, size_t offset) const
{
	std::vector<float> column;
	column.reserve(_count);
	for (size_t i = 0; i < _count; ++i)
	{
		const auto frame = (_next + i) % _count;
		column.push_back(values.at((frame * stride) + offset));
	}
	return column;
}

Results Recorder::Summarise() const
{
	Results results {
	    .frame = frame_stats::Summarise(_frames),
	    .update = frame_stats::Summarise(_updates),
	    .render = frame_stats::Summarise(_renders),
	    .draws = frame_stats::Summarise(_draws),
	};
	for (size_t stage = 0; stage < _stages.size(); ++stage)
	{
		const auto times = Column(_stageTimes, _stages.size(), stage);
		const auto framesRun = static_cast<size_t>(std::ranges::count_if(times, [](float ms) { return ms > 0.0f; }));
		if (framesRun == 0)
		{
			continue;
		}
		const auto summary = frame_stats::Summarise(times);
		results.stages.push_back({
		    .stage = stage,
		    .perFrame = summary,
		    .framesRun = framesRun,
		    .meanWhenRun = summary.average * static_cast<float>(summary.count) / static_cast<float>(framesRun),
		});
	}
	std::ranges::sort(results.stages,
	                  [](const StageResult& a, const StageResult& b) { return a.perFrame.average > b.perFrame.average; });
	return results;
}

std::string benchmark::ToJson(const RunInfo& run, const Results& results, std::span<const StageInfo> stages)
{
	std::string json = "{\n";
	json += fmt::format("  \"scenario\": {},\n  \"name\": {},\n  \"build\": {},\n", Quoted(run.scenarioId),
	                    Quoted(run.scenarioName), Quoted(run.build));
	json += fmt::format("  \"resolution\": [{}, {}],\n", run.width, run.height);
	json += fmt::format("  \"crowd\": {},\n  \"spawnMs\": {:.3f},\n  \"spawnFrames\": {},\n  \"warmUpFrames\": {},\n",
	                    run.crowd, run.spawnMs, run.spawnFrames, run.warmUpFrames);
	json += "  \"entities\": {";
	for (size_t i = 0; i < run.entityCounts.size(); ++i)
	{
		json += fmt::format("{}{}: {}", i == 0 ? "" : ", ", Quoted(run.entityCounts[i].first), run.entityCounts[i].second);
	}
	json += "},\n";
	json += fmt::format("  \"frames\": {},\n", results.frame.count);
	json += fmt::format("  \"frameMs\": {},\n", SummaryJson(results.frame));
	json += fmt::format("  \"updateMs\": {},\n", SummaryJson(results.update));
	json += fmt::format("  \"renderMs\": {},\n", SummaryJson(results.render));
	json += fmt::format("  \"drawCalls\": {},\n", SummaryJson(results.draws));
	json += "  \"stages\": [";
	for (size_t i = 0; i < results.stages.size(); ++i)
	{
		const auto& stage = results.stages[i];
		const auto& info = stages[stage.stage];
		json += fmt::format(
		    "{}\n    {{\"name\": {}, \"kind\": \"{}\", \"mean\": {:.4f}, \"p95\": {:.4f}, \"max\": {:.4f}, \"framesRun\": {}, "
		    "\"meanWhenRun\": {:.4f}}}",
		    i == 0 ? "" : ",", Quoted(info.name), info.render ? "render" : "update", stage.perFrame.average, stage.perFrame.p95,
		    stage.perFrame.max, stage.framesRun, stage.meanWhenRun);
	}
	json += "\n  ]\n}\n";
	return json;
}

std::string benchmark::ToCsv(const RunInfo& run, const Results& results, std::span<const StageInfo> stages)
{
	std::string csv = "scenario,build,crowd,metric,kind,mean_ms,p95_ms,max_ms,frames_run,mean_when_run_ms\n";
	const auto prefix = fmt::format("{},{},{}", CsvField(run.scenarioId), CsvField(run.build), run.crowd);
	const auto row = [&](std::string_view metric, std::string_view kind, const frame_stats::Summary& summary, size_t framesRun,
	                     float meanWhenRun) {
		csv += fmt::format("{},{},{},{:.4f},{:.4f},{:.4f},{},{:.4f}\n", prefix, CsvField(metric), kind, summary.average,
		                   summary.p95, summary.max, framesRun, meanWhenRun);
	};
	row("Frame", "total", results.frame, results.frame.count, results.frame.average);
	row("Update", "update", results.update, results.update.count, results.update.average);
	row("Render", "render", results.render, results.render.count, results.render.average);
	for (const auto& stage : results.stages)
	{
		const auto& info = stages[stage.stage];
		row(info.name, info.render ? "render" : "update", stage.perFrame, stage.framesRun, stage.meanWhenRun);
	}
	return csv;
}

float benchmark::ScalingExponent(double smallSize, double smallCost, double largeSize, double largeCost)
{
	if (smallSize <= 0.0 || largeSize <= 0.0 || smallCost <= 0.0 || largeCost <= 0.0 || smallSize == largeSize)
	{
		return 0.0f;
	}
	return static_cast<float>(std::log(largeCost / smallCost) / std::log(largeSize / smallSize));
}

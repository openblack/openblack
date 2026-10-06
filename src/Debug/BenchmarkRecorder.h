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
#include <cstdint>

#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Common/FrameStats.h"

/// The measurements of a benchmark: the time of each frame, of its update and its rendering, and of each profiler stage,
/// kept for the last so many frames and summed up as the mean, 95th percentile and slowest, and written out as JSON or
/// CSV to compare runs. Pure, so it is tested on its own.
namespace openblack::benchmark
{

/// A profiler stage as the results name it, and whether it is part of rendering the frame rather than updating it
struct StageInfo
{
	std::string_view name;
	bool render {false};
};

struct StageResult
{
	/// By its place in the stages given to the recorder
	size_t stage;
	/// Over every frame, those it didn't run in counting as nothing, so that a stage run once a game turn shows what it
	/// costs a frame on average, and its slowest frame
	frame_stats::Summary perFrame;
	/// The frames it ran in, and its mean over them: what a game turn of it costs
	size_t framesRun {0};
	float meanWhenRun {0.0f};
};

struct Results
{
	frame_stats::Summary frame;
	frame_stats::Summary update;
	frame_stats::Summary render;
	/// The draw calls the renderer made each frame
	frame_stats::Summary draws;
	/// The stages that ran, the costliest per frame first
	std::vector<StageResult> stages;
};

/// Keeps the last frames' times, up to its capacity
class Recorder
{
public:
	Recorder(std::vector<StageInfo> stages, size_t capacity);

	/// One frame: its time, the time until it started rendering and the time rendering it, each stage's time in it, in
	/// the order of the stages given, and the draw calls it made
	void Add(float frameMs, float updateMs, float renderMs, std::span<const float> stageMs, float draws = 0.0f);
	void Clear();
	[[nodiscard]] size_t Count() const { return _count; }
	[[nodiscard]] size_t Capacity() const { return _capacity; }
	[[nodiscard]] std::span<const StageInfo> Stages() const { return _stages; }
	[[nodiscard]] Results Summarise() const;

private:
	/// The frames in the order they came, the oldest first
	[[nodiscard]] std::vector<float> Column(const std::vector<float>& values, size_t stride, size_t offset) const;

	std::vector<StageInfo> _stages;
	size_t _capacity;
	size_t _count {0};
	/// Where the next frame goes once the buffers are full
	size_t _next {0};
	std::vector<float> _frames;
	std::vector<float> _updates;
	std::vector<float> _renders;
	std::vector<float> _draws;
	/// A row of the stages' times for each frame
	std::vector<float> _stageTimes;
};

/// What was run, to write with its results
struct RunInfo
{
	std::string scenarioId;
	std::string scenarioName;
	/// Debug, or an optimised build
	std::string build;
	uint32_t width {0};
	uint32_t height {0};
	/// The members of its crowd, and how long spawning them took over how many frames
	size_t crowd {0};
	double spawnMs {0.0};
	uint32_t spawnFrames {0};
	uint32_t warmUpFrames {0};
	/// How many entities there were of each kind, by name
	std::vector<std::pair<std::string, size_t>> entityCounts;
};

[[nodiscard]] std::string ToJson(const RunInfo& run, const Results& results, std::span<const StageInfo> stages);
/// One row for the frame, its update and its rendering, then one for each stage that ran
[[nodiscard]] std::string ToCsv(const RunInfo& run, const Results& results, std::span<const StageInfo> stages);

/// How a cost grows with the size of the crowd: the power of the size it goes with between two runs, 1 for linear and 2
/// for quadratic; 0 when either is empty or the sizes are the same
[[nodiscard]] float ScalingExponent(double smallSize, double smallCost, double largeSize, double largeCost);

} // namespace openblack::benchmark

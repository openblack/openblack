/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Profiler.h"

#include <cfloat>
#include <cstdio>

#include <algorithm>

#include <imgui_widget_flamegraph.h>

#include "EngineConfig.h"
#include "Graphics/RendererInterface.h"
#include "Locator.h"

#include "../Profiler.h"

using namespace openblack::debug::gui;

Profiler::Profiler() noexcept
    : Window("Profiler", ImVec2(650.0f, 800.0f))
{
}

void Profiler::Open() noexcept
{
	Window::Open();
	Locator::rendererInterface::value().SetProfile(true);
}

void Profiler::Close() noexcept
{
	Window::Close();
	Locator::rendererInterface::value().SetProfile(false);
}

void Profiler::Draw() noexcept
{
	auto& config = Locator::config::value();

	ImGui::Columns(5);
	ImGui::Checkbox("Sky", &config.drawSky);
	ImGui::NextColumn();
	ImGui::Checkbox("Water", &config.drawWater);
	ImGui::NextColumn();
	ImGui::Checkbox("Island", &config.drawIsland);
	ImGui::NextColumn();
	ImGui::Checkbox("Entities", &config.drawEntities);
	ImGui::NextColumn();
	ImGui::Checkbox("Sprites", &config.drawSprites);
	ImGui::Columns(1);

	const auto& profiler = Locator::profiler::value();
	const auto& entry = profiler.GetEntries().at(profiler.GetEntryIndex(-1));
	if (entry.frameStart == decltype(entry.frameStart) {} || entry.frameEnd <= entry.frameStart)
	{
		ImGui::TextUnformatted("Waiting for a completed frame...");
		return;
	}

	const std::chrono::duration<float, std::milli> frameDuration = entry.frameEnd - entry.frameStart;
	const float frameMs = frameDuration.count();
	_times.PushBack(frameMs);
	std::array<char, 256> frameTextOverlay {};
	std::snprintf(frameTextOverlay.data(), frameTextOverlay.size(), "%.3fms, %.1f FPS", _times.Back(), 1000.0f / _times.Back());

	const float width = std::max(1.0f, ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Frame").x -
	                                       ImGui::GetStyle().ItemInnerSpacing.x);
	ImGui::PlotHistogram("Frame", _times.values.data(), _times.count,
	                     _times.count == decltype(_times)::k_BufferSize ? _times.offset : 0, frameTextOverlay.data(), 0.0f,
	                     FLT_MAX, ImVec2(width, 45.0f));

	struct FrameData
	{
		const openblack::Profiler::Entry& entry;
		std::array<uint8_t, static_cast<size_t>(openblack::Profiler::Stage::_count)> stages {};
		int count = 0;
	};
	FrameData frame {entry};
	for (size_t i = 0; i < entry.stages.size(); ++i)
	{
		if (entry.stages.at(i).finalized)
		{
			frame.stages.at(frame.count++) = static_cast<uint8_t>(i);
		}
	}
	std::sort(frame.stages.begin(), frame.stages.begin() + frame.count,
	          [&entry](uint8_t a, uint8_t b) { return entry.stages.at(a).start < entry.stages.at(b).start; });

	ImGuiWidgetFlameGraph::PlotFlame(
	    "CPU",
	    [](float* startTimestamp, float* endTimestamp, ImU8* level, const char** caption, const void* data, int idx) {
		    const auto& frame = *static_cast<const FrameData*>(data);
		    const auto stageIndex = frame.stages.at(idx);
		    const auto& stage = frame.entry.stages.at(stageIndex);
		    if (startTimestamp != nullptr)
		    {
			    const std::chrono::duration<float, std::milli> start = stage.start - frame.entry.frameStart;
			    *startTimestamp = start.count();
		    }
		    if (endTimestamp != nullptr)
		    {
			    const std::chrono::duration<float, std::milli> end = stage.end - frame.entry.frameStart;
			    *endTimestamp = end.count();
		    }
		    if (level != nullptr)
		    {
			    *level = stage.level;
		    }
		    if (caption != nullptr)
		    {
			    *caption = openblack::Profiler::k_StageNames.at(stageIndex).data();
		    }
	    },
	    &frame, frame.count, 0, "Main Thread", 0.0f, frameMs, ImVec2(width, 0.0f));

	if (ImGui::CollapsingHeader("Details (CPU)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("Full Frame: %0.3f ms", frameMs);
		const float cursorX = ImGui::GetCursorPosX();
		const float indentSize = ImGui::CalcTextSize("    ").x;
		float unaccountedMs = frameMs;
		for (int i = 0; i < frame.count; ++i)
		{
			const auto stageIndex = frame.stages.at(i);
			const auto& stage = entry.stages.at(stageIndex);
			const std::chrono::duration<float, std::milli> duration = stage.end - stage.start;
			ImGui::SetCursorPosX(cursorX + indentSize * stage.level);
			ImGui::Text("    %s: %0.3f ms", openblack::Profiler::k_StageNames.at(stageIndex).data(), duration.count());
			if (stage.level == 0)
			{
				unaccountedMs -= duration.count();
			}
		}
		ImGui::SetCursorPosX(cursorX);
		ImGui::Text("    Unaccounted: %0.3f ms", std::max(0.0f, unaccountedMs));
	}
}

void Profiler::Update() noexcept {}

void Profiler::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) noexcept {}

void Profiler::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) noexcept {}

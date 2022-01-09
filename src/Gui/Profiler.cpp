/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Profiler.h"

#include <cinttypes>

#include <bgfx/bgfx.h>
#include <imgui_widget_flamegraph.h>

#include <ECS/Components/Transform.h>
#include <ECS/Components/Tree.h>
#include <ECS/Registry.h>
#include <Game.h>
#include <Profiler.h>

openblack::gui::Profiler::Profiler()
    : DebugWindow("Profiler", ImVec2(650.0f, 800.0f))
{
}

void openblack::gui::Profiler::Open()
{
	DebugWindow::Open();
	Game::instance()->GetConfig().bgfxProfile = true;
}

void openblack::gui::Profiler::Close()
{
	DebugWindow::Close();
	Game::instance()->GetConfig().bgfxProfile = false;
}

void openblack::gui::Profiler::Draw(Game& game)
{
	auto& config = game.GetConfig();

	using namespace ecs::components;

	const bgfx::Stats* stats = bgfx::getStats();
	const double toMsCpu = 1000.0 / stats->cpuTimerFreq;
	const double toMsGpu = 1000.0 / stats->gpuTimerFreq;
	const double frameMs = double(stats->cpuTimeFrame) * toMsCpu;
	_times.pushBack(static_cast<float>(frameMs));
	_fps.pushBack(static_cast<float>(1000.0 / frameMs));

	std::array<char, 256> frameTextOverlay;
	std::snprintf(frameTextOverlay.data(), frameTextOverlay.size(), "%.3fms, %.1f FPS", _times.back(), _fps.back());

	ImGui::Text("Submit CPU %0.3f, GPU %0.3f (Max GPU Latency: %d)", double(stats->cpuTimeEnd - stats->cpuTimeBegin) * toMsCpu,
	            double(stats->gpuTimeEnd - stats->gpuTimeBegin) * toMsGpu, stats->maxGpuLatency);
	ImGui::Text("Wait Submit %0.3f, Wait Render %0.3f", stats->waitSubmit * toMsCpu, stats->waitRender * toMsCpu);

	ImGui::Columns(5);
	ImGui::Checkbox("Sky", &config.drawSky);
	ImGui::NextColumn();
	ImGui::Checkbox("Water", &config.drawWater);
	ImGui::NextColumn();
	ImGui::Checkbox("Island", &config.drawIsland);
	ImGui::NextColumn();
	ImGui::Checkbox("Entities", &config.drawEntities);
	ImGui::NextColumn();
	ImGui::Checkbox("TestModel", &config.drawTestModel);
	ImGui::NextColumn();
	ImGui::Checkbox("Debug Cross", &config.drawDebugCross);
	ImGui::Columns(1);

	auto width = ImGui::GetColumnWidth() - ImGui::CalcTextSize("Frame").x;
	ImGui::PlotHistogram("Frame", _times._values.data(), decltype(_times)::_bufferSize, _times._offset, frameTextOverlay.data(),
	                     0.0f, FLT_MAX, ImVec2(width, 45.0f));

	ImGui::Text("Primitives Triangles %u, Triangle Strips %u, Lines %u "
	            "Line Strips %u, Points %u",
	            stats->numPrims[0], stats->numPrims[1], stats->numPrims[2], stats->numPrims[3], stats->numPrims[4]);
	ImGui::Columns(2);
	ImGui::Text("Num Entities %u, Trees %u", static_cast<uint32_t>(game.GetEntityRegistry().Size<Transform>()),
	            static_cast<uint32_t>(game.GetEntityRegistry().Size<Tree>()));
	ImGui::Text("Num Draw %u, Num Compute %u, Num Blit %u", stats->numDraw, stats->numCompute, stats->numBlit);
	ImGui::Text("Num Buffers Index %u, Vertex %u", stats->numIndexBuffers, stats->numVertexBuffers);
	ImGui::Text("Num Dynamic Buffers Index %u, Vertex %u", stats->numDynamicIndexBuffers, stats->numDynamicVertexBuffers);
	ImGui::Text("Num Transient Buffers Index %u, Vertex %u", stats->transientIbUsed, stats->transientVbUsed);
	ImGui::NextColumn();
	ImGui::Text("Num Vertex Layouts %u", stats->numVertexLayouts);
	ImGui::Text("Num Textures %u, FrameBuffers %u", stats->numTextures, stats->numFrameBuffers);
	ImGui::Text("Memory Texture %" PRId64 ", RenderTarget %" PRId64, stats->textureMemoryUsed, stats->rtMemoryUsed);
	ImGui::Text("Num Programs %u, Num Shaders %u, Uniforms %u", stats->numPrograms, stats->numShaders, stats->numUniforms);
	ImGui::Text("Num Occlusion Queries %u", stats->numOcclusionQueries);

	ImGui::Columns(1);

	auto& entry = game.GetProfiler()._entries[game.GetProfiler().GetEntryIndex(-1)];

	ImGuiWidgetFlameGraph::PlotFlame(
	    "CPU",
	    [](float* startTimestamp, float* endTimestamp, ImU8* level, const char** caption, const void* data, int idx) -> void {
		    auto entry = reinterpret_cast<const openblack::Profiler::Entry*>(data);
		    auto& stage = entry->_stages[idx];
		    if (startTimestamp)
		    {
			    std::chrono::duration<float, std::milli> fltStart = stage._start - entry->_frameStart;
			    *startTimestamp = fltStart.count();
		    }
		    if (endTimestamp)
		    {
			    *endTimestamp = stage._end.time_since_epoch().count() / 1e6f;

			    std::chrono::duration<float, std::milli> fltEnd = stage._end - entry->_frameStart;
			    *endTimestamp = fltEnd.count();
		    }
		    if (level)
		    {
			    *level = stage._level;
		    }
		    if (caption)
		    {
			    *caption = openblack::Profiler::stageNames[idx].data();
		    }
	    },
	    &entry, static_cast<uint8_t>(openblack::Profiler::Stage::_count), 0, "Main Thread", 0, FLT_MAX, ImVec2(width, 0));

	ImGuiWidgetFlameGraph::PlotFlame(
	    "GPU",
	    [](float* startTimestamp, float* endTimestamp, ImU8* level, const char** caption, const void* data, int idx) -> void {
		    auto stats = reinterpret_cast<const bgfx::Stats*>(data);
		    if (startTimestamp)
		    {
			    *startTimestamp = static_cast<float>(1000.0 * (stats->viewStats[idx].gpuTimeBegin - stats->gpuTimeBegin) /
			                                         (double)stats->gpuTimerFreq);
		    }
		    if (endTimestamp)
		    {
			    *endTimestamp = static_cast<float>(1000.0 * (stats->viewStats[idx].gpuTimeEnd - stats->gpuTimeBegin) /
			                                       (double)stats->gpuTimerFreq);
		    }
		    if (level)
		    {
			    *level = 0;
		    }
		    if (caption)
		    {
			    *caption = stats->viewStats[idx].name;
		    }
	    },
	    stats, stats->numViews, 0, "GPU Frame", 0,
	    static_cast<float>(1000.0 * (stats->gpuTimeEnd - stats->gpuTimeBegin) / (double)stats->gpuTimerFreq), ImVec2(width, 0));

	ImGui::Columns(2);
	if (ImGui::CollapsingHeader("Details (CPU)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		std::chrono::duration<float, std::milli> frameDuration = entry._frameEnd - entry._frameStart;
		ImGui::Text("Full Frame: %0.3f", frameDuration.count());
		auto cursorX = ImGui::GetCursorPosX();
		auto indentSize = ImGui::CalcTextSize("    ").x;

		for (uint8_t i = 0; i < static_cast<uint8_t>(openblack::Profiler::Stage::_count); ++i)
		{
			std::chrono::duration<float, std::milli> duration = entry._stages[i]._end - entry._stages[i]._start;
			ImGui::SetCursorPosX(cursorX + indentSize * entry._stages[i]._level);
			ImGui::Text("    %s: %0.3f", openblack::Profiler::stageNames[i].data(), duration.count());
			if (entry._stages[i]._level == 0)
				frameDuration -= duration;
		}
		ImGui::Text("    Unaccounted: %0.3f", frameDuration.count());
	}
	ImGui::NextColumn();
	if (ImGui::CollapsingHeader("Details (GPU)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		auto frameDuration = stats->gpuTimeEnd - stats->gpuTimeBegin;
		ImGui::Text("Full Frame: %0.3f", 1000.0f * frameDuration / (double)stats->gpuTimerFreq);

		for (uint8_t i = 0; i < stats->numViews; ++i)
		{
			auto const& viewStat = stats->viewStats[i];
			int64_t gpuTimeElapsed = viewStat.gpuTimeEnd - viewStat.gpuTimeBegin;

			ImGui::Text("    %s: %0.3f", viewStat.name, 1000.0f * gpuTimeElapsed / (double)stats->gpuTimerFreq);
			frameDuration -= gpuTimeElapsed;
		}
		ImGui::Text("    Unaccounted: %0.3f", 1000.0f * frameDuration / (double)stats->gpuTimerFreq);
	}
	ImGui::Columns(1);
}

void openblack::gui::Profiler::Update([[maybe_unused]] Game& game, [[maybe_unused]] const Renderer& renderer) {}

void openblack::gui::Profiler::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) {}

void openblack::gui::Profiler::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) {}

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
#include <chrono>
#include <map>
#include <string_view>

namespace openblack
{

class Profiler
{
public:
	enum class Stage : uint8_t
	{
		PhysicsUpdate,
		PathfindingUpdate,
		LivingActionUpdate,
		SdlInput,
		UpdateUniforms,
		UpdateEntities,
		UpdateAudio,
		GuiLoop,
		GameLogic,
		SceneDraw,
		FootprintPass,
		ReflectionPass,
		ReflectionDrawSky,
		ReflectionDrawWater,
		ReflectionDrawIsland,
		ReflectionDrawModels,
		ReflectionDrawSprites,
		MainPass,
		MainPassDrawSky,
		MainPassDrawWater,
		MainPassDrawIsland,
		MainPassDrawModels,
		MainPassDrawSprites,
		GuiDraw,
		RendererFrame,

		_count,
	};

	constexpr static std::array<std::string_view, static_cast<uint8_t>(Stage::_count)> k_StageNames = {
	    "Physics Update",       //
	    "Pathfinding Update",   //
	    "Living Action Update", //
	    "SDL Input",            //
	    "Update Uniforms",      //
	    "Entities",             //
	    "Audio",                //
	    "GUI Loop",             //
	    "Game Logic",           //
	    "Encode Draw Scene",    //
	    "Footprint Pass",       //
	    "Reflection Pass",      //
	    "Draw Sky",             //
	    "Draw Water",           //
	    "Draw Island",          //
	    "Draw Models",          //
	    "Draw Sprites",         //
	    "Main Pass",            //
	    "Draw Sky",             //
	    "Draw Water",           //
	    "Draw Island",          //
	    "Draw Models",          //
	    "Draw Sprites",         //
	    "Encode GUI Draw",      //
	    "Renderer Frame",       //
	};

private:
	struct ScopedSection
	{
		inline explicit ScopedSection(Profiler* profiler, Stage stage)
		    : profiler(profiler)
		    , stage(stage)
		{
			profiler->Begin(stage);
		}
		inline ~ScopedSection() { profiler->End(stage); }

		Profiler* const profiler;
		const Stage stage;
	};

public:
	struct Scope
	{
		uint8_t level;
		std::chrono::system_clock::time_point start;
		std::chrono::system_clock::time_point end;
		bool finalized = false;
	};

	struct Entry
	{
		std::chrono::system_clock::time_point frameStart;
		std::chrono::system_clock::time_point frameEnd;
		std::array<Scope, static_cast<uint8_t>(Stage::_count)> stages;
	};

	void Frame();
	void Begin(Stage stage);
	void End(Stage stage);
	inline ScopedSection BeginScoped(Stage stage) { return ScopedSection(this, stage); }

	[[nodiscard]] uint8_t GetEntryIndex(int8_t offset) const { return (_currentEntry + k_BufferSize + offset) % k_BufferSize; }

	constexpr static uint8_t k_BufferSize = 100;
	std::array<Entry, k_BufferSize>& GetEntries() { return _entries; }
	[[nodiscard]] const std::array<Entry, k_BufferSize>& GetEntries() const { return _entries; }

private:
	std::array<Entry, k_BufferSize> _entries;
	uint8_t _currentEntry = k_BufferSize - 1;
	uint8_t _currentLevel = 0;
};

} // namespace openblack

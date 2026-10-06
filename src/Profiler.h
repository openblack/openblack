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

#include <algorithm>
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
		CreatureMindUpdate,
		CreaturePlannerUpdate,
		CreatureLearningUpdate,
		CreaturePhysiologyUpdate,
		CreatureLocomotionUpdate,
		CreatureAnimationUpdate,
		CreatureHairUpdate,
		CreatureSkinUpdate,
		CreatureAudioUpdate,
		CreatureObjectActionUpdate,
		CreatureHandUpdate,
		CreatureFootprintsUpdate,
		CreatureLeashUpdate,
		CreatureCombatUpdate,
		VegetationUpdate,
		ParticlesUpdate,
		MagicUpdate,
		SdlInput,
		UpdateUniforms,
		UpdateEntities,
		UpdateEntitiesDescs,
		UpdateEntitiesUniforms,
		UpdateEntitiesTrees,
		UpdateAudio,
		GuiLoop,
		EditorUpdate,
		GameLogic,
		SceneDraw,
		FootprintPass,
		ObjectShadowPass,
		CreatureShadowPass,
		ParticlesGather,
		ReflectionPass,
		ReflectionDrawSky,
		ReflectionDrawWater,
		ReflectionDrawIsland,
		ReflectionDrawModels,
		ReflectionDrawVegetation,
		ReflectionDrawSprites,
		ReflectionDrawParticles,
		MainPass,
		MainPassDrawSky,
		MainPassDrawWater,
		MainPassDrawIsland,
		MainPassDrawModels,
		MainPassDrawVegetation,
		MainPassDrawSprites,
		MainPassDrawParticles,
		GuiDraw,
		RendererFrame,

		_count,
	};

	constexpr static std::array<std::string_view, static_cast<uint8_t>(Stage::_count)> k_StageNames = {
	    "Physics Update",       //
	    "Pathfinding Update",   //
	    "Living Action Update", //
	    "Creature Mind",        //
	    "Creature Planner",     //
	    "Creature Learning",    //
	    "Creature Physiology",  //
	    "Creature Locomotion",  //
	    "Creature Animation",   //
	    "Creature Hair",        //
	    "Creature Skin",        //
	    "Creature Audio",       //
	    "Creature Objects",     //
	    "Creature Hand",        //
	    "Creature Footprints",  //
	    "Creature Leash",       //
	    "Creature Combat",      //
	    "Vegetation Update",    //
	    "Particles",            //
	    "Magic",                //
	    "SDL Input",            //
	    "Update Uniforms",      //
	    "Entities",             //
	    "Entity Draw Lists",    //
	    "Entity Instances",     //
	    "Tree Instances",       //
	    "Audio",                //
	    "GUI Loop",             //
	    "Editor",               //
	    "Game Logic",           //
	    "Encode Draw Scene",    //
	    "Footprint Pass",       //
	    "Object Shadow Pass",   //
	    "Creature Shadow Pass", //
	    "Gather Particles",     //
	    "Reflection Pass",      //
	    "Draw Sky",             //
	    "Draw Water",           //
	    "Draw Island",          //
	    "Draw Models",          //
	    "Draw Vegetation",      //
	    "Draw Sprites",         //
	    "Draw Particles",       //
	    "Main Pass",            //
	    "Draw Sky",             //
	    "Draw Water",           //
	    "Draw Island",          //
	    "Draw Models",          //
	    "Draw Vegetation",      //
	    "Draw Sprites",         //
	    "Draw Particles",       //
	    "Encode GUI Draw",      //
	    "Renderer Frame",       //
	};
	// Every stage has a name: a short list would leave the last ones empty
	static_assert(std::ranges::none_of(k_StageNames, &std::string_view::empty));

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
		/// The time spent in the stage over the whole frame, and how often it ran: a stage may run more than once a
		/// frame, as in a game turn and again for the frame, where start and end only hold the last run
		std::chrono::system_clock::duration total {};
		uint16_t calls = 0;
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

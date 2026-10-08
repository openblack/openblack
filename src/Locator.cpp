/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Locator.h"

#define LOCATOR_IMPLEMENTATIONS

#include <spdlog/spdlog.h>

#include "3D/Implementations/LandIsland.h"
#include "3D/Implementations/Ocean.h"
#include "3D/Implementations/Sky.h"
#include "3D/Implementations/TempleInterior.h"
#include "3D/Implementations/UnloadedIsland.h"
#include "3D/LandData.h"
#include "Audio/AudioManager.h"
#include "Audio/AudioManagerNoOp.h"
#include "CHLApi.h"
#include "Common/EventManager.h"
#include "Common/GameRandomProduction.h"
#include "Common/RandomNumberManagerProduction.h"
#include "Debug/DebugGuiInterface.h"
#include "ECS/Archetypes/PlayerArchetype.h"
#include "ECS/MapProduction.h"
#include "ECS/Registry.h"
#include "ECS/Systems/Implementations/AlignmentSystem.h"
#include "ECS/Systems/Implementations/CameraBookmarkSystem.h"
#include "ECS/Systems/Implementations/CameraHelpSystem.h"
#include "ECS/Systems/Implementations/CameraPathSystem.h"
#include "ECS/Systems/Implementations/ChimneySmokeSystem.h"
#include "ECS/Systems/Implementations/CinematicDirectorSystem.h"
#include "ECS/Systems/Implementations/CloudSystem.h"
#include "ECS/Systems/Implementations/CreatureAnimationSystem.h"
#include "ECS/Systems/Implementations/CreatureAudioSystem.h"
#include "ECS/Systems/Implementations/CreatureCaveSystem.h"
#include "ECS/Systems/Implementations/CreatureFightSystem.h"
#include "ECS/Systems/Implementations/CreatureHairSystem.h"
#include "ECS/Systems/Implementations/CreatureHandSystem.h"
#include "ECS/Systems/Implementations/CreatureLocomotionSystem.h"
#include "ECS/Systems/Implementations/CreatureMindSystem.h"
#include "ECS/Systems/Implementations/CreatureModeSystem.h"
#include "ECS/Systems/Implementations/CreatureObjectActionSystem.h"
#include "ECS/Systems/Implementations/CreaturePhysiologySystem.h"
#include "ECS/Systems/Implementations/CreatureSkinSystem.h"
#include "ECS/Systems/Implementations/DynamicsSystem.h"
#include "ECS/Systems/Implementations/EditorSystem.h"
#include "ECS/Systems/Implementations/FieldSystem.h"
#include "ECS/Systems/Implementations/FireSystem.h"
#include "ECS/Systems/Implementations/FootprintSystem.h"
#include "ECS/Systems/Implementations/GestureSystem.h"
#include "ECS/Systems/Implementations/HandSystem.h"
#include "ECS/Systems/Implementations/InfluenceSystem.h"
#include "ECS/Systems/Implementations/LeashSystem.h"
#include "ECS/Systems/Implementations/LivingActionSystem.h"
#include "ECS/Systems/Implementations/MagicSystem.h"
#include "ECS/Systems/Implementations/MistSystem.h"
#include "ECS/Systems/Implementations/ParticleSystem.h"
#include "ECS/Systems/Implementations/PathfindingSystem.h"
#include "ECS/Systems/Implementations/PlayerSystem.h"
#include "ECS/Systems/Implementations/RainSystem.h"
#include "ECS/Systems/Implementations/RenderingSystem.h"
#include "ECS/Systems/Implementations/SnowSystem.h"
#include "ECS/Systems/Implementations/SnowfallSystem.h"
#include "ECS/Systems/Implementations/SoundTagSystem.h"
#include "ECS/Systems/Implementations/TempleExteriorSystem.h"
#include "ECS/Systems/Implementations/TimeSystem.h"
#include "ECS/Systems/Implementations/TownDesireSystem.h"
#include "ECS/Systems/Implementations/TownSystem.h"
#include "ECS/Systems/Implementations/VegetationSystem.h"
#include "ECS/Systems/Implementations/VillageLightSystem.h"
#include "ECS/Systems/Implementations/WaterRingSystem.h"
#include "ECS/Systems/Implementations/WeatherSystem.h"
#include "Graphics/RendererInterface.h"
#include "Input/GameActionMap.h"
#include "LHVM.h"
#include "Profiler.h"
#include "Resources/Resources.h"
#include "Windowing/Sdl2WindowingSystem.h"
#if __ANDROID__
#include "FileSystem/AndroidFileSystem.h"
#else
#include "FileSystem/DefaultFileSystem.h"
#endif

using namespace openblack::audio;
using namespace openblack::filesystem;
using openblack::GameRandomProduction;
using openblack::LandIsland;
using openblack::RandomNumberManagerProduction;
using openblack::TempleInterior;
using openblack::UnloadedIsland;
using openblack::chlapi::CHLApi;
using openblack::debug::gui::DebugGuiInterface;
using openblack::ecs::MapProduction;
using openblack::ecs::Registry;
using openblack::ecs::systems::AlignmentSystem;
using openblack::ecs::systems::CameraBookmarkSystem;
using openblack::ecs::systems::CameraHelpSystem;
using openblack::ecs::systems::CameraPathSystem;
using openblack::ecs::systems::ChimneySmokeSystem;
using openblack::ecs::systems::CinematicDirectorSystem;
using openblack::ecs::systems::CloudSystem;
using openblack::ecs::systems::CreatureAnimationSystem;
using openblack::ecs::systems::CreatureAudioSystem;
using openblack::ecs::systems::CreatureCaveSystem;
using openblack::ecs::systems::CreatureFightSystem;
using openblack::ecs::systems::CreatureHairSystem;
using openblack::ecs::systems::CreatureHandSystem;
using openblack::ecs::systems::CreatureLocomotionSystem;
using openblack::ecs::systems::CreatureMindSystem;
using openblack::ecs::systems::CreatureModeSystem;
using openblack::ecs::systems::CreatureObjectActionSystem;
using openblack::ecs::systems::CreaturePhysiologySystem;
using openblack::ecs::systems::CreatureSkinSystem;
using openblack::ecs::systems::DynamicsSystem;
using openblack::ecs::systems::EditorSystem;
using openblack::ecs::systems::FieldSystem;
using openblack::ecs::systems::FootprintSystem;
using openblack::ecs::systems::GestureEventsInterface;
using openblack::ecs::systems::GestureSystem;
using openblack::ecs::systems::HandSystem;
using openblack::ecs::systems::InfluenceSystem;
using openblack::ecs::systems::LeashSystem;
using openblack::ecs::systems::LivingActionSystem;
using openblack::ecs::systems::MagicSystem;
using openblack::ecs::systems::MistSystem;
using openblack::ecs::systems::ParticleSystem;
using openblack::ecs::systems::PathfindingSystem;
using openblack::ecs::systems::PlayerSystem;
using openblack::ecs::systems::RainSystem;
using openblack::ecs::systems::RenderingSystem;
using openblack::ecs::systems::SnowfallSystem;
using openblack::ecs::systems::SnowSystem;
using openblack::ecs::systems::SoundTagSystem;
using openblack::ecs::systems::TempleExteriorSystem;
using openblack::ecs::systems::TimeSystem;
using openblack::ecs::systems::TownDesireSystem;
using openblack::ecs::systems::TownSystem;
using openblack::ecs::systems::VegetationSystem;
using openblack::ecs::systems::VillageLightSystem;
using openblack::ecs::systems::WaterRingSystem;
using openblack::ecs::systems::WeatherSystem;
using openblack::graphics::RendererInterface;
using openblack::input::GameActionMap;
using openblack::lhvm::LHVM;
using openblack::resources::Resources;
using openblack::windowing::DisplayMode;
using openblack::windowing::Sdl2WindowingSystem;

void openblack::InitializeWindow(const std::string& title, int width, int height, DisplayMode displayMode, uint32_t extraFlags)
{
	Locator::windowing::emplace<Sdl2WindowingSystem>(title, width, height, displayMode, extraFlags);
}

bool openblack::InitializeEngine(GraphicsBackend backend, bool vsync) noexcept
{
	SPDLOG_LOGGER_INFO(spdlog::get("game"), "EnTT version: {}", ENTT_VERSION);
	SPDLOG_LOGGER_INFO(spdlog::get("game"), GLM_VERSION_COMPLETE);

	Locator::profiler::emplace();

	Locator::rendererInterface::reset(RendererInterface::Create(backend, vsync).release());
	if (!Locator::rendererInterface::has_value())
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("graphics"), "Failed to create renderer");
		return false;
	}
	Locator::debugGui::reset(DebugGuiInterface::Create(graphics::RenderPass::ImGui).release());
	Locator::events::emplace<EventManager>();

#if __ANDROID__
	Locator::filesystem::emplace<AndroidFileSystem>();
#else
	Locator::filesystem::emplace<DefaultFileSystem>();
#endif
	Locator::rng::emplace<RandomNumberManagerProduction>();
	Locator::gameRandom::emplace<GameRandomProduction>();
	try
	{
		Locator::audio::emplace<AudioManager>();
	}
	catch (std::runtime_error& error)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("audio"), "Falling back to no-op audio: {}", error.what());
		Locator::audio::emplace<AudioManagerNoOp>();
	}

	Locator::chlapi::emplace<CHLApi>();
	Locator::vm::emplace<LHVM>();
	return true;
}

bool openblack::InitializeGame() noexcept
{
	Locator::terrainSystem::emplace<UnloadedIsland>();
	Locator::resources::emplace<Resources>();
	Locator::playerSystem::emplace<PlayerSystem>();
	Locator::gameActionSystem::emplace<GameActionMap>();
	Locator::rendereringSystem::emplace<RenderingSystem>();
	Locator::entitiesRegistry::emplace<Registry>();
	Locator::handSystem::emplace<HandSystem>();
	Locator::temple::emplace<TempleInterior>();
	Locator::oceanSystem::emplace<Ocean>();
	Locator::skySystem::emplace<Sky>();
	Locator::alignmentSystem::emplace<AlignmentSystem>();
	Locator::cameraHelpSystem::emplace<CameraHelpSystem>();
	Locator::templeExteriorSystem::emplace<TempleExteriorSystem>();
	Locator::time::emplace<TimeSystem>();
	Locator::vegetation::emplace<VegetationSystem>();
	Locator::mistSystem::emplace<MistSystem>();
	Locator::cloudSystem::emplace<CloudSystem>();
	Locator::villageLightSystem::emplace<VillageLightSystem>();
	Locator::fieldSystem::emplace<FieldSystem>();
	Locator::snowSystem::emplace<SnowSystem>();
	Locator::snowfallSystem::emplace<SnowfallSystem>();
	Locator::waterRingSystem::emplace<WaterRingSystem>();
	Locator::creatureAnimationSystem::emplace<CreatureAnimationSystem>();
	Locator::creatureMindSystem::emplace<CreatureMindSystem>();
	Locator::creaturePhysiologySystem::emplace<CreaturePhysiologySystem>();
	Locator::creatureHairSystem::emplace<CreatureHairSystem>();
	Locator::creatureAudioSystem::emplace<CreatureAudioSystem>();
	Locator::creatureObjectActionSystem::emplace<CreatureObjectActionSystem>();
	Locator::creatureHandSystem::emplace<CreatureHandSystem>();
	Locator::footprintSystem::emplace<FootprintSystem>();
	Locator::editorSystem::emplace<EditorSystem>();
	Locator::creatureSkinSystem::emplace<CreatureSkinSystem>();
	Locator::leashSystem::emplace<LeashSystem>();
	Locator::creatureFightSystem::emplace<CreatureFightSystem>();
	Locator::creatureModeSystem::emplace<CreatureModeSystem>();
	Locator::creatureCaveSystem::emplace<CreatureCaveSystem>();
	Locator::cinematicDirectorSystem::emplace<CinematicDirectorSystem>();
	Locator::soundTagSystem::emplace<SoundTagSystem>();
	Locator::rainSystem::emplace<RainSystem>();
	Locator::chimneySmokeSystem::emplace<ChimneySmokeSystem>();
	Locator::influenceSystem::emplace<InfluenceSystem>();
	Locator::townDesireSystem::emplace<TownDesireSystem>();
	Locator::particleSystem::emplace<ParticleSystem>();
	Locator::magicSystem::emplace<MagicSystem>();
	Locator::fireSystem::emplace<ecs::systems::FireSystem>();
	return true;
}

namespace openblack
{
namespace
{
template <typename LandSource>
void InitializeLevelWith(const LandSource& land)
{
	// Both seeds go to 0 with every map, as the game clears them
	Locator::gameRandom::value().SetSeeds({0, 0});
	Locator::entitiesMap::emplace<MapProduction>();
	Locator::dynamicsSystem::emplace<DynamicsSystem>();
	Locator::livingActionSystem::emplace<LivingActionSystem>();
	Locator::townSystem::emplace<TownSystem>();
	Locator::weatherSystem::emplace<WeatherSystem>();
	Locator::pathfindingSystem::emplace<PathfindingSystem>();
	// Where creatures can walk is sorted anew for each land
	Locator::creatureLocomotionSystem::emplace<CreatureLocomotionSystem>();
	Locator::cameraBookmarkSystem::emplace<CameraBookmarkSystem>();
	Locator::terrainSystem::emplace<LandIsland>(land);
	Locator::cameraPathSystem::emplace<CameraPathSystem>();
}
} // namespace
} // namespace openblack

void openblack::InitializeLevel(const std::filesystem::path& path)
{
	InitializeLevelWith(path);
}

void openblack::InitializeLevel(const LandData& land)
{
	InitializeLevelWith(land);
}

void openblack::ShutDownServices()
{
	// Stop all sounds
	if (Locator::audio::has_value())
	{
		Locator::audio::value().Stop();
	}

	// Manually delete the assets here before BGFX renderer clears its buffers resulting in invalid handles in our assets
	if (Locator::resources::has_value())
	{
		auto& resources = Locator::resources::value();
		resources.GetMeshes().Clear();
		resources.GetTextures().Clear();
		resources.GetAnimations().Clear();
		resources.GetSounds().Clear();
	}

	// The audio resources have been cleared and all sounds have been stopped. It is now safe to reset audio
	if (Locator::audio::has_value())
	{
		Locator::audio::reset();
	}

	Locator::rendereringSystem::reset();
	Locator::dynamicsSystem::reset();
	Locator::editorSystem::reset();
	Locator::cameraBookmarkSystem::reset();
	Locator::livingActionSystem::reset();
	Locator::townSystem::reset();
	Locator::weatherSystem::reset();
	Locator::handSystem::reset();
	Locator::pathfindingSystem::reset();
	Locator::creatureLocomotionSystem::reset();
	Locator::cinematicDirectorSystem::reset();
	Locator::influenceSystem::reset();
	Locator::chimneySmokeSystem::reset();
	Locator::rainSystem::reset();
	Locator::snowfallSystem::reset();
	Locator::waterRingSystem::reset();
	Locator::creatureCaveSystem::reset();
	Locator::creatureModeSystem::reset();
	Locator::creatureFightSystem::reset();
	Locator::leashSystem::reset();
	Locator::creatureMindSystem::reset();
	Locator::creaturePhysiologySystem::reset();
	Locator::creatureSkinSystem::reset();
	Locator::creatureHairSystem::reset();
	Locator::footprintSystem::reset();
	Locator::creatureAudioSystem::reset();
	Locator::creatureObjectActionSystem::reset();
	Locator::creatureHandSystem::reset();
	Locator::creatureAnimationSystem::reset();
	Locator::snowSystem::reset();
	Locator::fieldSystem::reset();
	Locator::animalSystem::reset();
	Locator::soundTagSystem::reset();
	Locator::townDesireSystem::reset();
	Locator::miracleFxSystem::reset();
	Locator::fireSystem::reset();
	Locator::explosionSystem::reset();
	Locator::magicSystem::reset();
	Locator::gestureEvents::reset();
	Locator::reactionSystem::reset();
	Locator::teleportSystem::reset();
	Locator::tornadoSystem::reset();
	Locator::magicShieldSystem::reset();
	Locator::forestSystem::reset();
	Locator::gestureSystem::reset();
	Locator::particleSystem::reset();
	Locator::terrainSystem::reset();
	Locator::filesystem::reset();
	Locator::gameActionSystem::reset();

	Locator::oceanSystem::reset();
	Locator::skySystem ::reset();
	Locator::debugGui::reset();
	Locator::entitiesRegistry::reset();
	Locator::rendererInterface::reset();
	Locator::windowing::reset();
	Locator::events::reset();
	Locator::camera::reset();
	Locator::config::reset();
	Locator::infoConstants::reset();
	Locator::profiler::reset();

	Locator::vm::reset();
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Game.h"

#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include <LHVM.h>
#include <LNDFile.h>
#include <MorphFile.h>
#include <PackFile.h>
#include <SDL.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "3D/CreatureCaveTrophies.h"
#include "3D/DayNightClock.h"
#include "3D/FlatLand.h"
#include "3D/GripLandscapeEffect.h"
#include "3D/HandAnimation.h"
#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/LandLightFrame.h"
#include "3D/LandLightTable.h"
#include "3D/OceanInterface.h"
#include "3D/SkyInterface.h"
#include "3D/SnowCover.h"
#include "3D/TempleInteriorInterface.h"
#include "3D/WaterRings.h"
#include "Audio/AtmosAudio.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/GameMusic.h"
#include "CHLApi.h"
#include "Camera/Camera.h"
#include "Camera/NearClipping.h"
#include "Common/EventManager.h"
#include "Common/GameRandom.h"
#include "Common/RandomNumberManager.h"
#include "Common/StringUtils.h"
#include "Creature/CreatureHandRules.h"
#include "Debug/DebugGuiInterface.h"
#include "Debug/FrameStatsLog.h"
#include "Debug/TestbedDispenserGrid.h"
#include "ECS/Archetypes/PlayerArchetype.h"
#include "ECS/Components/CameraBookmark.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureHair.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/Hand.h"
#include "ECS/Components/Mist.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/CameraBookmarkSystemInterface.h"
#include "ECS/Systems/CameraPathSystemInterface.h"
#include "ECS/Systems/ChimneySmokeSystemInterface.h"
#include "ECS/Systems/CinematicDirectorSystemInterface.h"
#include "ECS/Systems/CloudSystemInterface.h"
#include "ECS/Systems/CreatureAnimationSystemInterface.h"
#include "ECS/Systems/CreatureAudioSystemInterface.h"
#include "ECS/Systems/CreatureFightSystemInterface.h"
#include "ECS/Systems/CreatureHairSystemInterface.h"
#include "ECS/Systems/CreatureHandSystemInterface.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/CreatureModeSystemInterface.h"
#include "ECS/Systems/CreatureObjectActionSystemInterface.h"
#include "ECS/Systems/CreaturePhysiologySystemInterface.h"
#include "ECS/Systems/CreatureSkinSystemInterface.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/EditorSystemInterface.h"
#include "ECS/Systems/FieldSystemInterface.h"
#include "ECS/Systems/FootprintSystemInterface.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/InfluenceSystemInterface.h"
#include "ECS/Systems/LeashSystemInterface.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/MistSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/PathfindingSystemInterface.h"
#include "ECS/Systems/PlayerSystemInterface.h"
#include "ECS/Systems/RainSystemInterface.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "ECS/Systems/SnowSystemInterface.h"
#include "ECS/Systems/SnowfallSystemInterface.h"
#include "ECS/Systems/SoundTagSystemInterface.h"
#include "ECS/Systems/TempleExteriorSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/Systems/TownDesireSystemInterface.h"
#include "ECS/Systems/VegetationInterface.h"
#include "ECS/Systems/VillageLightSystemInterface.h"
#include "ECS/Systems/WaterRingSystemInterface.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "EngineConfig.h"
#include "FileSystem/FileSystemInterface.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/RendererInterface.h"
#include "Gui/GameInterface.h"
#include "Input/GameActionMapInterface.h"
#include "LHScriptX/Script.h"
#include "Locator.h"
#include "Parsers/InfoFile.h"
#include "Profiler.h"
#include "Resources/Loaders.h"
#include "Resources/ResourcesInterface.h"
#include "Serializer/FotFile.h"

#ifdef __ANDROID__
#include <spdlog/sinks/android_sink.h>
#endif

using namespace openblack;
using namespace openblack::lhscriptx;
using namespace std::chrono_literals;

namespace
{
// Where the camera starts on the testbed: above and behind the middle of the map
constexpr float k_TestbedCameraHeight = 60.0f;
constexpr float k_TestbedCameraBack = 120.0f;
} // namespace

const std::string k_WindowTitle = "openblack";

Game* Game::sInstance = nullptr;

Game::Game(Arguments&& args) noexcept
    : _gamePath(args.gamePath)
    , _startMap(args.startLevel)
    , _startTestbed(args.startTestbed || args.scenario.has_value())
    , _scenarioRequest(args.scenario)
    , _requestScreenshot(args.requestScreenshot)
{
	Locator::camera::emplace(glm::zero<glm::vec3>());
	std::function<std::shared_ptr<spdlog::logger>(const std::string&)> createLogger;
#ifdef __ANDROID__
	if (!args.logFile.empty() && args.logFile == "logcat")
	{
		createLogger = [](const std::string& name) { return spdlog::android_logger_mt(name, "spdlog-android"); };
	}
	else
#endif // __ANDROID__
	{
		if (!args.logFile.empty() && args.logFile != "stdout")
		{
			// Every subsystem's logger writes through the one file: each opening the file for itself, their writes
			// overwrote each other's and lines went missing
			auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(args.logFile);
			createLogger = [fileSink](const std::string& name) {
				auto logger = std::make_shared<spdlog::logger>(name, fileSink);
				spdlog::register_logger(logger);
				return logger;
			};
		}
		else
		{
			createLogger = [](const std::string& name) { return spdlog::stdout_color_mt(name); };
		}
	}
	// TODO (#749) use std::views::enumerate
	for (size_t i = 0; const auto& subsystem : k_LoggingSubsystemStrs)
	{
		auto logger = createLogger(subsystem.data());
		logger->set_level(args.logLevels.at(i));
		++i;
	}
	sInstance = this;

	auto& config = Locator::config::emplace();
	config.numFramesToSimulate = args.numFramesToSimulate;
	config.frameStatsInterval = args.frameStatsInterval;
	config.frameStatsViews = args.frameStatsViews;
	config.resolution = {args.windowWidth, args.windowHeight};
	config.displayMode = args.displayMode;
	config.graphicsBackend = args.graphicsBackend;
	config.vsync = args.vsync;
	config.detailLevel = args.detailLevel;
	config.guiScale = args.guiScale;
}

Game::~Game() noexcept
{
	// Its textures go before the renderer, and the temple's scrolls are written with its text
	if (Locator::temple::has_value())
	{
		Locator::temple::value().SetInterface(nullptr);
	}
	_interface.reset();
	ShutDownServices();
	SDL_Quit(); // todo: move to GameWindow
	spdlog::shutdown();
}

bool Game::ProcessEvents(const SDL_Event& event) noexcept
{
	static bool leftMouseButton = false;
	static bool middleMouseButton = false;
	static bool rightMouseButton = false;

	// Pressed and let go, or as the mouse moving finds them: a press or a let go the menu or the debug windows took would
	// otherwise leave the hand gripping, as on leaving the temple
	if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == SDL_BUTTON_LEFT)
	{
		leftMouseButton = event.type == SDL_MOUSEBUTTONDOWN;
	}
	if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == SDL_BUTTON_MIDDLE)
	{
		middleMouseButton = event.type == SDL_MOUSEBUTTONDOWN;
	}
	const bool rightLetGo =
	    rightMouseButton && ((event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT) ||
	                         (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON_RMASK) == 0));
	if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == SDL_BUTTON_RIGHT)
	{
		rightMouseButton = event.type == SDL_MOUSEBUTTONDOWN;
	}
	if (event.type == SDL_MOUSEMOTION)
	{
		leftMouseButton = (event.motion.state & SDL_BUTTON_LMASK) != 0;
		middleMouseButton = (event.motion.state & SDL_BUTTON_MMASK) != 0;
		rightMouseButton = (event.motion.state & SDL_BUTTON_RMASK) != 0;
	}

	// The hand grips the land, which the temple has none of: its camera takes the clicks
	const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
	// Holding the right button on a creature holds the hand to it to stroke or slap it; clicking it puts the leash on.
	// While the player's creature fights, clicks on the creatures and the arena's ground direct the fight instead.
	auto& creatureHand = Locator::creatureHandSystem::value();
	auto& fights = Locator::creatureFightSystem::value();
	auto& magic = Locator::magicSystem::value();
	// A miracle in the hand, or a one-shot bubble under it, takes the press before the creatures and the land
	bool magicTookPress = false;
	if (!inTemple && event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && !middleMouseButton &&
	    !Locator::debugGui::value().IsMouseOverWindow())
	{
		magicTookPress = magic.PressAction();
	}
	if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
	{
		magic.ReleaseAction();
	}
	// The other button lets go of the miracle in the hand, or else takes hold of the creature under the hand. With the
	// leash on, pressed on another creature it ties the leash to that one instead (see the leash's input).
	if (!inTemple && event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT)
	{
		_actionPressTaken = magic.GetHeldSeed().has_value();
		magic.DiscardHeldSeed();
		if (!_actionPressTaken)
		{
			const auto screenSize = Locator::windowing::value().GetSize();
			glm::vec3 rayOrigin;
			glm::vec3 rayDirection;
			Locator::camera::value().DeprojectScreenToWorld(glm::vec2(event.button.x, event.button.y) /
			                                                    static_cast<glm::vec2>(glm::max(screenSize, glm::ivec2(1))),
			                                                rayOrigin, rayDirection);
			const auto& leashes = Locator::leashSystem::value();
			const auto own = leashes.PlayersCreature(PlayerNames::PLAYER_ONE);
			const auto under = creatureHand.CreatureAlong(rayOrigin, rayDirection);
			const bool tying = under.has_value() && own.has_value() && *under != *own && leashes.IsLeashed(*own);
			if (under.has_value() && !tying)
			{
				_actionPressTaken = true;
				if (!creatureHand.Grab(rayOrigin, rayDirection))
				{
					// A creature the hand may not hold: a click on it still asks the leash, which says why not
					Locator::leashSystem::value().TapCreature(PlayerNames::PLAYER_ONE, *under);
				}
			}
		}
	}
	if (!magicTookPress && !magic.IsHandBusy() && !inTemple && event.type == SDL_MOUSEBUTTONDOWN &&
	    event.button.button == SDL_BUTTON_LEFT && !middleMouseButton)
	{
		const auto screenSize = Locator::windowing::value().GetSize();
		glm::vec3 rayOrigin;
		glm::vec3 rayDirection;
		Locator::camera::value().DeprojectScreenToWorld(glm::vec2(event.button.x, event.button.y) /
		                                                    static_cast<glm::vec2>(glm::max(screenSize, glm::ivec2(1))),
		                                                rayOrigin, rayDirection);
		// The second press of a double click on a creature, anyone's, locks the camera onto it
		const bool doubleClicked =
		    Locator::creatureModeSystem::has_value() &&
		    Locator::creatureModeSystem::value().Press({.milliseconds = event.button.timestamp,
		                                                .screen = glm::vec2(event.button.x, event.button.y),
		                                                .creature = creatureHand.CreatureAlong(rayOrigin, rayDirection)});
		if (!doubleClicked && !fights.Press(rayOrigin, rayDirection))
		{
			PlayHandGrabSound();
		}
	}
	if (!leftMouseButton && fights.IsPressed())
	{
		fights.Release();
	}
	// Letting go of the right button lets go of the creature. Let go quickly, having neither stroked nor slapped it, the
	// press was a click, which puts the leash on the player's creature.
	if (rightLetGo && creatureHand.GetCreature().has_value() && !creatureHand.IsHeldByCommand())
	{
		const auto creature = *creatureHand.GetCreature();
		const bool click = creatureHand.IsClick();
		creatureHand.Release();
		if (click)
		{
			Locator::leashSystem::value().TapCreature(PlayerNames::PLAYER_ONE, creature);
		}
	}
	const bool onCreature = creatureHand.GetCreature().has_value();

	_handGripping =
	    !inTemple && (middleMouseButton || (leftMouseButton && !onCreature && !fights.IsPressed() && !magic.IsHandBusy()));
	_handRotating = !inTemple && middleMouseButton;

	auto& window = Locator::windowing::value();
	auto& camera = Locator::camera::value();

	switch (event.type)
	{
	case SDL_QUIT:
		return false;
	case SDL_WINDOWEVENT:
		if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == window.GetID())
		{
			return false;
		}
		else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
		{
			const auto resolution = glm::u16vec2(event.window.data1, event.window.data2);
			Locator::rendererInterface::value().Reset(resolution);
			Locator::rendererInterface::value().ConfigureView(graphics::RenderPass::Main, resolution, 0x274659ff);
			Locator::oceanSystem::value().ResizeReflectionFramebuffer(resolution);
			Locator::rendererInterface::value().ConfigureView(graphics::RenderPass::Reflection, resolution, 0x274659ff);

			auto aspect = window.GetAspectRatio();
			const auto& config = Locator::config::value();
			// A camera model, as the temple's, can look through its own lens
			const auto lens = camera.GetModel().GetLens();
			camera.SetProjectionMatrixPerspective(lens.has_value() ? lens->horizontalFieldOfView : config.cameraXFov, aspect,
			                                      lens.has_value() ? lens->nearClip : config.cameraNearClip,
			                                      config.cameraFarClip);
		}
		break;
	case SDL_KEYDOWN:
		switch (event.key.keysym.sym)
		{
		case SDLK_ESCAPE:
			// Not while a script has the cinema bars in for its scene
			if (!Locator::cinematicDirectorSystem::value().IsInterfaceActive())
			{
				break;
			}
			return false;
		case SDLK_f:
			window.SetDisplayMode(windowing::DisplayMode::Fullscreen);
			break;
		case SDLK_p:
			Locator::time::value().SetPaused(!IsPaused());
			break;
		// F1 is the game's Help key, so the renderer's statistics are on F11
		case SDLK_F11:
			Locator::rendererInterface::value().SetDebug(!Locator::rendererInterface::value().GetDebug());
			break;
		case SDLK_1:
		case SDLK_2:
		case SDLK_3:
		case SDLK_4:
		case SDLK_5:
		case SDLK_6:
		case SDLK_7:
		case SDLK_8:
			// The camera's bookmarks aren't for the player while a script has the cinema bars in
			if (!Locator::cinematicDirectorSystem::value().IsInterfaceActive())
			{
				break;
			}
			if ((event.key.keysym.mod & KMOD_CTRL) != 0)
			{
				const auto index = static_cast<uint8_t>(event.key.keysym.sym - SDLK_1);
				const auto positions = Locator::handSystem::value().GetPlayerHandPositions();
				if (positions[static_cast<size_t>(ecs::systems::HandSystemInterface::Side::Left)] ||
				    positions[static_cast<size_t>(ecs::systems::HandSystemInterface::Side::Right)])
				{
					const auto handPosition =
					    positions[static_cast<size_t>(ecs::systems::HandSystemInterface::Side::Left)].value_or(
					        positions[static_cast<size_t>(ecs::systems::HandSystemInterface::Side::Right)].value_or(
					            glm::zero<glm::vec3>()));
					Locator::cameraBookmarkSystem::value().SetBookmark(index, handPosition, camera.GetOrigin());
				}
			}
			else
			{
				const auto& entitiesRegistry = Locator::entitiesRegistry::value();
				const size_t index = event.key.keysym.sym - SDLK_1;
				const auto& bookmarkEntities = Locator::cameraBookmarkSystem::value().GetBookmarks();
				const auto entity = bookmarkEntities.at(index);
				const auto [transform, bookmark] =
				    entitiesRegistry.TryGet<ecs::components::Transform, ecs::components::CameraBookmark>(entity);
				if (transform != nullptr && bookmark != nullptr)
				{
					camera.GetModel().SetFlight(bookmark->savedOrigin, transform->position);
				}
			}
			break;
		}
		break;
	case SDL_MOUSEMOTION:
	{
		SDL_GetMouseState(&_mousePosition.x, &_mousePosition.y);
		break;
	}
	case SDL_MOUSEBUTTONUP:
		switch (event.button.button)
		{
		case SDL_BUTTON_MIDDLE:
		{
			const glm::ivec2 screenSize = window.GetSize();
			SDL_SetRelativeMouseMode((event.type == SDL_MOUSEBUTTONDOWN) ? SDL_TRUE : SDL_FALSE);
			SDL_WarpMouseInWindow(static_cast<SDL_Window*>(window.GetHandle()), screenSize.x / 2, screenSize.y / 2);
		}
		break;
		}
		break;
	}

	return true;
}

void Game::SetGameSpeed(float multiplier)
{
	Locator::time::value().SetSpeed(1.0f / multiplier);
}

float Game::GetGameSpeed() const
{
	return 1.0f / Locator::time::value().GetSpeed();
}

uint32_t Game::GetTurn() const
{
	return Locator::time::value().GetTurn();
}

bool Game::IsPaused() const
{
	return Locator::time::value().IsPaused();
}

void Game::UpdateHandInterface()
{
	if (!_interface)
	{
		return;
	}
	_interface->SetCreaturePanel(std::nullopt);
	// The temple places the hand's tooltip itself
	const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
	// The fighters' health and stamina, while a fight is on
	_interface->SetFightPanel(inTemple ? std::nullopt : Locator::creatureFightSystem::value().GetPanel());
	if (inTemple)
	{
		return;
	}
	// Outside it, the tooltip is drawn by the hand, which is at the cursor
	_interface->SetHandOnScreen(Locator::debugGui::value().IsMouseOverWindow() ? std::nullopt
	                                                                           : std::optional(glm::vec2(_mousePosition)));
	// Nor is the panel shown while a script has the cinema bars in
	if (!Locator::cinematicDirectorSystem::value().IsInterfaceActive())
	{
		return;
	}
	// The creature the hand is held to, or else the one it is over, any player's
	const auto& creatureHand = Locator::creatureHandSystem::value();
	auto creature = creatureHand.GetCreature();
	const bool rightButtonHeld = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK) != 0;
	if (!creature.has_value() && creature_panel::Triggered(rightButtonHeld))
	{
		creature = _creatureUnderHand;
	}
	// Without one, the creature Creature Mode follows shows near the top of the screen, without the reward
	std::optional<float> reward = creatureHand.GetLastFeedbackSum();
	if (!creature.has_value() && Locator::creatureModeSystem::has_value())
	{
		creature = Locator::creatureModeSystem::value().GetCreature();
		reward.reset();
	}
	if (!creature.has_value())
	{
		return;
	}
	const auto* needs = Locator::entitiesRegistry::value().TryGet<const ecs::components::CreatureNeeds>(*creature);
	if (needs != nullptr)
	{
		// The reward is the hand's, which it keeps showing after letting go until it takes hold again
		_interface->SetCreaturePanel(creature_panel::FromNeeds(needs->needs, reward));
	}
}

void Game::ProcessHandToolTipTurn()
{
	if (!_interface || (Locator::temple::has_value() && Locator::temple::value().Active()))
	{
		return;
	}
	auto& toolTips = _interface->GetToolTips();
	// Over the player's own creature, the hand shows that it can take hold of it to stroke or slap it. It can hold other
	// players' creatures too, but the game only offers it for the player's own.
	const auto over = _creatureUnderHand.has_value() ? _creatureUnderHand : Locator::creatureHandSystem::value().GetCreature();
	if (over.has_value() && !_interface->GetMenu().IsOpen() && Locator::cinematicDirectorSystem::value().IsInterfaceActive())
	{
		const auto* creature = Locator::entitiesRegistry::value().TryGet<const ecs::components::Creature>(*over);
		const auto* mind = Locator::entitiesRegistry::value().TryGet<const ecs::components::CreatureMindState>(*over);
		const bool asleep = mind != nullptr && creature_mind::IsAsleep(mind->idle);
		if (creature != nullptr && creature_hand::ShowsInteractTip(PlayerNames::PLAYER_ONE, creature->owner, asleep))
		{
			toolTips.Submit(creature_panel::k_InteractToolTip, gui::ToolTipAction::Select, gui::ToolTipArrows::k_None);
		}
	}
	toolTips.ProcessTurn();
}

bool Game::GameLogicLoop() noexcept
{
	using namespace ecs::components;
	using namespace ecs::systems;

	const auto currentTime = std::chrono::steady_clock::now();
	const auto delta = currentTime - _lastGameLoopTime;
	auto& clock = Locator::time::value();

	// The game pauses the world while the player is in the temple, whose own turns keep the audio going
	if (Locator::temple::has_value() && Locator::temple::value().Active())
	{
		// NOLINTNEXTLINE(modernize-use-nullptr): clang-tidy bug
		if (delta >= k_TurnDuration * GetGameSpeed())
		{
			ProcessTempleAudioTurn();
			_lastGameLoopTime = currentTime;
		}
		return false;
	}

	if (clock.IsPaused())
	{
		// The ambience is silent while the game is paused
		Locator::audio::value().AtmosProcess(false);
		return false;
	}

	if (!clock.IsTurnDue())
	{
		return false;
	}
	clock.StartTurn();
	ProcessHandToolTipTurn();

	// Build Map Grid Acceleration Structure
	Locator::entitiesMap::value().Rebuild();

	auto& profiler = Locator::profiler::value();

	{
		auto pathfinding = profiler.BeginScoped(Profiler::Stage::PathfindingUpdate);
		Locator::pathfindingSystem::value().Update();
	}
	// The towns work out what they want, then their villagers act on it
	Locator::townDesireSystem::value().ProcessTurn();
	Locator::chimneySmokeSystem::value().ProcessTurn();
	// How far the players' influence reaches, and its border
	Locator::influenceSystem::value().ProcessTurn(Locator::time::value().GetTurn());
	// The crops in the fields grow
	Locator::fieldSystem::value().ProcessTurn(Locator::time::value().GetTurn());
	{
		// The creatures age, grow, get hungry, tired and thirsty, and heal while they sleep
		auto creaturePhysiology = profiler.BeginScoped(Profiler::Stage::CreaturePhysiologyUpdate);
		Locator::creaturePhysiologySystem::value().ProcessTurn();
	}
	// The creatures' bodies follow their fatness, and their marks heal
	Locator::creatureAnimationSystem::value().ProcessTurn();
	Locator::creatureSkinSystem::value().ProcessTurn();
	{
		// A taut leash pulls its creature to the hand before the creature's mind thinks
		auto creatureLeash = profiler.BeginScoped(Profiler::Stage::CreatureLeashUpdate);
		Locator::leashSystem::value().ProcessTurn();
	}
	{
		// The creatures want things, and decide what to do while idle
		auto creatureMind = profiler.BeginScoped(Profiler::Stage::CreatureMindUpdate);
		Locator::creatureMindSystem::value().ProcessTurn();
	}
	{
		// They weigh their desires against what they could do about them, and change what they do for a pressing plan
		auto creaturePlanner = profiler.BeginScoped(Profiler::Stage::CreaturePlannerUpdate);
		Locator::creatureMindSystem::value().PlanTurn();
	}
	{
		// They learn from what the leash shows them, and copy the player
		auto creatureLearning = profiler.BeginScoped(Profiler::Stage::CreatureLearningUpdate);
		Locator::creatureMindSystem::value().LearnTurn();
	}
	{
		// They plan their routes and walk, run and turn
		auto creatureLocomotion = profiler.BeginScoped(Profiler::Stage::CreatureLocomotionUpdate);
		Locator::creatureLocomotionSystem::value().ProcessTurn();
	}
	{
		// They walk up to the things they act on, and what they carry makes them stronger
		auto creatureObjectActions = profiler.BeginScoped(Profiler::Stage::CreatureObjectActionUpdate);
		Locator::creatureObjectActionSystem::value().ProcessTurn();
	}
	{
		// Fights start and end, the fighters choose their moves, and creatures knocked out come round
		auto creatureCombat = profiler.BeginScoped(Profiler::Stage::CreatureCombatUpdate);
		Locator::creatureFightSystem::value().ProcessTurn();
	}
	{
		auto actions = profiler.BeginScoped(Profiler::Stage::LivingActionUpdate);
		Locator::livingActionSystem::value().Update();
	}

	auto& lhvm = Locator::vm::value();
	lhvm.LookIn(lhvm::ScriptType::All);
	// The scripts' fade moves on with their turn
	Locator::cinematicDirectorSystem::value().ProcessTurn();

	// The time of day moves on
	Locator::skySystem::value().GetClock().ProcessTurn();

	// The weather moves on, then the ambience follows the weather at the camera
	const auto cameraPosition = Locator::camera::value().GetOrigin();
	ecs::components::WeatherInfo weather {};
	if (Locator::weatherSystem::has_value())
	{
		auto& weatherSystem = Locator::weatherSystem::value();
		weatherSystem.Update(clock.GetTurn());
		// The storms that snow lay it on the land, and it melts
		Locator::snowSystem::value().ProcessTurn(weatherSystem.GetActiveStorms());
		weather = weatherSystem.GetWeatherSmooth(cameraPosition);
	}

	// The objects' looping sounds start again where they have stopped
	Locator::soundTagSystem::value().ProcessTurn(cameraPosition);
	{
		// The dispensers, then each miracle's upkeep, its own particle effect and what that effect tells it
		auto magic = profiler.BeginScoped(Profiler::Stage::MagicUpdate);
		Locator::magicSystem::value().ProcessTurn();
	}
	{
		// The particle effects not owned by a miracle step, and the spot visuals count down
		auto particles = profiler.BeginScoped(Profiler::Stage::ParticlesUpdate);
		Locator::particleSystem::value().ProcessTurn();
	}

	// Each turn ends with the camera taking the alignment of the player of most influence where it is
	Locator::alignmentSystem::value().UpdateTurn();
	// The temples' outsides follow their players' alignments
	Locator::templeExteriorSystem::value().UpdateTurn();

	if (_atmosAudio)
	{
		_atmosAudio->SetAlignment(Locator::alignmentSystem::value().GetCameraAlignment());
		_atmosAudio->EndTurn({
		    .camera = cameraPosition,
		    .weather =
		        {
		            .rain = weather.rain,
		            .snow = weather.snow,
		            .windX = weather.windX,
		            .windZ = weather.windZ,
		        },
		    .paused = false,
		    .turn = clock.GetTurn(),
		    .inCitadel = false,
		    .videoPlaying = false,
		});
	}

	ProcessMusicTurn(cameraPosition, false);

	_lastGameLoopTime = currentTime;
	_turnDeltaTime = delta;

	return false;
}

void Game::ProcessMusicTurn(glm::vec3 cameraPosition, bool inCitadel)
{
	// The game picks the music for the turn
	if (!_gameMusic)
	{
		return;
	}
	audio::GameMusic::TurnInputs music {
	    .turn = GetTurn(),
	    .camera = cameraPosition,
	    .groundHeight = Locator::terrainSystem::value().GetHeightAt(glm::xz(cameraPosition)),
	    .inCitadel = inCitadel,
	    // TODO(raffclar): the player's alignment once it is simulated
	    .alignment = 0.0f,
	    .towns = {},
	};
	Locator::entitiesRegistry::value().Each<const ecs::components::Town, const Tribe, const ecs::components::Transform>(
	    [&music](const ecs::components::Town& town, const Tribe tribe, const ecs::components::Transform& transform) {
		    music.towns.push_back({
		        .position = transform.position,
		        .tribe = static_cast<int32_t>(tribe),
		        .id = town.id,
		    });
	    });
	_gameMusic->ProcessTurn(music);
}

void Game::ProcessTempleRoomKeys()
{
	if (!Locator::temple::has_value())
	{
		return;
	}
	// The keys for the temple's rooms take the player into the temple at that room's path in, and inside the temple
	// they cut to the room
	constexpr std::array<std::pair<input::BindableActionMap, TempleRoom>, 6> k_RoomKeys {{
	    {input::BindableActionMap::ZOOM_TO_INSIDE_TEMPLE, TempleRoom::Main},
	    {input::BindableActionMap::ZOOM_TO_CREATURE_ROOM, TempleRoom::CreatureCave},
	    {input::BindableActionMap::ZOOM_TO_CHALLENGE_ROOM, TempleRoom::Challenge},
	    {input::BindableActionMap::ZOOM_TO_SAVE_GAME_ROOM, TempleRoom::SaveGame},
	    {input::BindableActionMap::ZOOM_TO_OPTIONS_ROOM, TempleRoom::Options},
	    {input::BindableActionMap::ZOOM_TO_LIBRARY, TempleRoom::Credits},
	}};
	const auto& actions = Locator::gameActionSystem::value();
	auto& temple = Locator::temple::value();
	for (const auto& [action, room] : k_RoomKeys)
	{
		if (!actions.Get(action) || !actions.GetChanged(action))
		{
			continue;
		}
		if (temple.Active())
		{
			temple.GoToRoom(room);
		}
		else
		{
			temple.Activate(room);
		}
		return;
	}
}

void Game::ProcessTempleAudioTurn()
{
	// The temple's turns play the citadel's music, while the land's ambience fades out
	ProcessMusicTurn(Locator::camera::value().GetOrigin(), true);
	if (_atmosAudio)
	{
		_atmosAudio->ContinueTurn({.paused = false, .turn = GetTurn(), .inCitadel = true, .videoPlaying = false});
	}
}

bool Game::Update() noexcept
{
	auto& profiler = Locator::profiler::value();

	profiler.Frame();

	auto& camera = Locator::camera::value();
	auto& config = Locator::config::value();

	auto previous = profiler.GetEntries().at(profiler.GetEntryIndex(-1)).frameStart;
	auto current = profiler.GetEntries().at(profiler.GetEntryIndex(0)).frameStart;
	// Prevent spike at first frame
	if (previous.time_since_epoch().count() == 0)
	{
		current = previous;
	}
	auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(current - previous);

	Locator::debugGui::value().SetScale(config.guiScale);
	Locator::time::value().Update();

	// The physics world isn't stepped: the game's objects don't move as rigid bodies, and the world only answers the
	// rays cast for the hand, the camera and the like. Stepping it let the features fall and lose their turn.

	// Input events
	{
		auto sdlInput = profiler.BeginScoped(Profiler::Stage::SdlInput);
		// The debug windows' presses of the options screen's actions are made by the action map's frame
		if (!Locator::debugGui::value().StealsFocus() || Locator::gameActionSystem::value().HasQueuedPresses())
		{
			Locator::gameActionSystem::value().Frame();
		}
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0)
		{
			Locator::events::value().Create<SDL_Event>(e);
		}
		camera.HandleActions(deltaTime);
		ProcessTempleRoomKeys();
		_shortcutKeys.Update();
	}
	Locator::cameraPathSystem::value().Update(deltaTime);

	if (!config.running || _quitRequested)
	{
		return false;
	}

	// ImGui events + prepare
	{
		auto guiLoop = profiler.BeginScoped(Profiler::Stage::GuiLoop);
		// The debug menu bar comes up with the game's menu, or always without it
		Locator::debugGui::value().SetMenuBarVisible(!_interface || _interface->GetMenu().IsOpen());
		if (Locator::debugGui::value().Loop())
		{
			return false; // Quit event
		}
	}
	// The in-game editor keeps its camera on what it has picked
	if (Locator::editorSystem::has_value())
	{
		auto editor = profiler.BeginScoped(Profiler::Stage::EditorUpdate);
		Locator::editorSystem::value().Update(deltaTime);
	}

	// Creature Mode keeps the camera on its creature, and the cave follows the temple's creature room
	if (Locator::creatureModeSystem::has_value())
	{
		auto creatureMode = profiler.BeginScoped(Profiler::Stage::CreatureModeUpdate);
		Locator::creatureModeSystem::value().Update(deltaTime, {.handGripping = _handGripping});
	}

	camera.Update(deltaTime);
	// Outside a camera with a lens of its own, the near plane follows the camera's height over the land
	if (!camera.GetModel().GetLens().has_value() && Locator::terrainSystem::has_value())
	{
		const auto origin = camera.GetOrigin();
		const float height = origin.y - Locator::terrainSystem::value().GetHeightAt(glm::vec2(origin.x, origin.z));
		const float nearClip = near_clipping::NearPlane(height, Locator::cinematicDirectorSystem::value().IsCloseClipping());
		if (nearClip != camera.GetNearClip())
		{
			camera.SetNearClip(nearClip);
		}
	}
	// The temple's camera may have taken the player out of the temple
	if (Locator::temple::has_value())
	{
		Locator::temple::value().Update(deltaTime);
	}
	Locator::cameraBookmarkSystem::value().Update(deltaTime);
	if (_interface)
	{
		_interface->Update(std::chrono::duration_cast<std::chrono::duration<float>>(deltaTime).count());
		HandleInterfaceAction();
	}
	GripLandscapeEffect::Update(std::chrono::duration_cast<std::chrono::duration<float>>(deltaTime).count());

	// Update Game Logic in Registry
	{
		auto gameLogic = profiler.BeginScoped(Profiler::Stage::GameLogic);
		if (GameLogicLoop())
		{
			return false; // Quit event
		}
	}

	// The frame's game time: none while paused, quicker or slower with the game speed
	auto& clock = Locator::time::value();
	clock.UpdateFrame();
	const auto gameTime = std::chrono::duration<float, std::milli>(clock.GetFrameGameTime());
	Locator::alignmentSystem::value().Update(gameTime);
	{
		auto actions = profiler.BeginScoped(Profiler::Stage::VegetationUpdate);
		Locator::vegetation::value().Update(gameTime);
	}
	// The clouds drift with the wind, then every mist and cloud animates
	Locator::cloudSystem::value().Update(gameTime);
	// The rain falls as the storm nearest the camera has it
	Locator::rainSystem::value().Update(std::chrono::duration<float>(gameTime).count(), camera.GetOrigin());
	// The rings on the water grow and fade
	Locator::waterRingSystem::value().Update(gameTime);
	{
		// The fight animations play, blows land and the fighters move as their animations carry them
		auto creatureCombat = profiler.BeginScoped(Profiler::Stage::CreatureCombatUpdate);
		Locator::creatureFightSystem::value().Update(gameTime);
	}
	{
		// The creatures are drawn moving between the last two turns
		auto creatureLocomotion = profiler.BeginScoped(Profiler::Stage::CreatureLocomotionUpdate);
		Locator::creatureLocomotionSystem::value().Update(clock.GetTurnFraction());
	}
	{
		// Drops of creatures' sick fly and fall
		auto creaturePhysiology = profiler.BeginScoped(Profiler::Stage::CreaturePhysiologyUpdate);
		Locator::creaturePhysiologySystem::value().Update(std::chrono::duration<float>(gameTime).count());
	}
	{
		// What they do with things plays on their bodies
		auto creatureObjectActions = profiler.BeginScoped(Profiler::Stage::CreatureObjectActionUpdate);
		Locator::creatureObjectActionSystem::value().Update(gameTime);
	}
	{
		// The creatures breathe, act, pull faces and look about
		auto creatureAnimation = profiler.BeginScoped(Profiler::Stage::CreatureAnimationUpdate);
		Locator::creatureAnimationSystem::value().Update(gameTime);
	}
	{
		// They take hold of and let go of things with their hands as posed, and what they let go of flies
		auto creatureObjectActions = profiler.BeginScoped(Profiler::Stage::CreatureObjectActionUpdate);
		Locator::creatureObjectActionSystem::value().LateUpdate(gameTime);
	}
	{
		// Their hair swings from the posed bodies
		auto creatureHair = profiler.BeginScoped(Profiler::Stage::CreatureHairUpdate);
		Locator::creatureHairSystem::value().Update(gameTime);
	}
	{
		// They make the sounds of the moments their animations have played past
		auto creatureAudio = profiler.BeginScoped(Profiler::Stage::CreatureAudioUpdate);
		Locator::creatureAudioSystem::value().Update(gameTime);
	}
	{
		// Their footprints fade away
		auto creatureFootprints = profiler.BeginScoped(Profiler::Stage::CreatureFootprintsUpdate);
		Locator::footprintSystem::value().Update(gameTime);
	}
	{
		// Their skins are painted again where their alignment, tattoos or marks have changed
		auto creatureSkin = profiler.BeginScoped(Profiler::Stage::CreatureSkinUpdate);
		Locator::creatureSkinSystem::value().Update();
	}
	// The snow falls as the rain does
	Locator::snowfallSystem::value().Update(std::chrono::duration<float>(gameTime).count(),
	                                        Locator::rainSystem::value().GetFall());
	// The homes' smoke rises while someone is in
	Locator::chimneySmokeSystem::value().Update(gameTime);
	Locator::influenceSystem::value().Update(gameTime);
	Locator::mistSystem::value().Update(gameTime);
	Locator::villageLightSystem::value().Update(gameTime);
	Locator::fieldSystem::value().Update(gameTime);
	Locator::cinematicDirectorSystem::value().Update(gameTime);
	// The cinema bars coming in hide the game's dialogs
	if (Locator::cinematicDirectorSystem::value().TakeHideDialogs() && _interface && _interface->GetMenu().IsOpen())
	{
		_interface->GetMenu().Close();
	}

	// Update Uniforms
	{
		auto profilerScopedUpdateUniforms = profiler.BeginScoped(Profiler::Stage::UpdateUniforms);

		// Update Hand and intersection point
		ecs::components::Transform intersectionTransform {};
		bool enterTemple = false;
		{
			const auto screenSize =
			    Locator::windowing::has_value() ? Locator::windowing::value().GetSize() : glm::zero<glm::ivec2>();
			const auto scale = glm::vec3(50.0f, 50.0f, 50.0f);
			if (screenSize.x > 0 && screenSize.y > 0)
			{
				glm::vec3 rayOrigin;
				glm::vec3 rayDirection;
				camera.DeprojectScreenToWorld(static_cast<glm::vec2>(_mousePosition) / static_cast<glm::vec2>(screenSize),
				                              rayOrigin, rayDirection);
				auto& dynamicsSystem = Locator::dynamicsSystem::value();

				_cursorWorldPosition.reset();
				if (Locator::temple::has_value() && Locator::temple::value().Active())
				{
					// In the temple, the hand goes where the cursor meets the room, turning to its surface
					if (const auto hit = Locator::temple::value().GetCursorHit())
					{
						const auto seconds = std::chrono::duration_cast<std::chrono::duration<float>>(deltaTime).count();
						_handTempleNormal.SetDestination(hit->normal, k_HandTempleTurnTime);
						_handTempleNormal.Update(seconds);
						const auto normal = _handTempleNormal.GetValue();
						_cursorWorldPosition = hit->point;
						intersectionTransform.position = hit->point;
						intersectionTransform.rotation =
						    glm::length(normal) > 0.0f
						        ? glm::mat3_cast(glm::rotation(glm::vec3(0.0f, 1.0f, 0.0f), glm::normalize(normal)))
						        : glm::mat3(1.0f);
					}
				}
				else if (!glm::any(glm::isnan(rayOrigin) || glm::isnan(rayDirection)))
				{
					// The Action button on the player's own temple's entrance takes them inside
					// TODO(raffclar): in a game of one player, only once a script lets the player use the temple
					const auto& actions = Locator::gameActionSystem::value();
					if (Locator::cinematicDirectorSystem::value().IsInterfaceActive() &&
					    actions.GetChanged(input::BindableActionMap::ACTION) && actions.Get(input::BindableActionMap::ACTION))
					{
						enterTemple = Locator::templeExteriorSystem::value().EntranceAt(rayOrigin, rayDirection) ==
						              PlayerNames::PLAYER_ONE;
					}
					// The leash keys, and the Action button tapping leash posts, creatures and things to tie the leash to.
					// Not while the debug windows have the keyboard or mouse, as when typing in a text field: the controls
					// aren't updated then, so a key just pressed would read as pressed again every frame.
					if (!Locator::debugGui::value().StealsFocus())
					{
						auto& leashes = Locator::leashSystem::value();
						leashes.HandleInput(rayOrigin, rayDirection, _actionPressTaken);
						_actionPressTaken = false;
						// Shaking the free hand takes off the leash held in it
						const auto seconds = std::chrono::duration_cast<std::chrono::duration<float>>(deltaTime).count();
						const bool handFree = !_handGripping && !Locator::creatureHandSystem::value().GetCreature() &&
						                      !Locator::magicSystem::value().GetHeldSeed().has_value();
						leashes.TrackHand(PlayerNames::PLAYER_ONE,
						                  static_cast<glm::vec2>(_mousePosition) / static_cast<float>(screenSize.y), seconds,
						                  handFree);
					}
					if (auto hit = dynamicsSystem.RayCastClosestHit(rayOrigin, rayDirection, 1e10f))
					{
						intersectionTransform = hit->first;
						_cursorWorldPosition = intersectionTransform.position;
					}
					else // For the water
					{
						float intersectDistance = 0.0f;
						const auto planeOrigin = glm::vec3(0.0f, 0.0f, 0.0f);
						const auto planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
						if (glm::intersectRayPlane(rayOrigin, rayDirection, planeOrigin, planeNormal, intersectDistance))
						{
							intersectionTransform.position = rayOrigin + rayDirection * intersectDistance;
							intersectionTransform.rotation = glm::mat3(1.0f);
							_cursorWorldPosition = intersectionTransform.position;
						}
					}
				}
				intersectionTransform.scale = scale;
			}
		}

		if (enterTemple && Locator::temple::has_value())
		{
			Locator::temple::value().Activate();
		}

		// Update Hand
		{
			const glm::mat4 modelRotationCorrection = glm::eulerAngleX(glm::radians(90.0f));

			const auto handEntity = Locator::handSystem::value()
			                            .GetPlayerHands()[static_cast<size_t>(ecs::systems::HandSystemInterface::Side::Left)];
			auto& handTransform = Locator::entitiesRegistry::value().Get<ecs::components::Transform>(handEntity);
			if (!_handGripping)
			{
				// TODO(#480): move using velocity rather than snapping hand to intersectionTransform
				handTransform.rotation = glm::eulerAngleY(camera.GetRotation().y) * modelRotationCorrection;
				handTransform.rotation = intersectionTransform.rotation * handTransform.rotation;
			}
			PlaceHand(handTransform, std::chrono::duration_cast<std::chrono::duration<float>>(deltaTime).count());
			// Held to a creature, the hand rests on its body under the cursor, stroking and slapping it
			{
				auto creatureHand = profiler.BeginScoped(Profiler::Stage::CreatureHandUpdate);
				const auto screenSize =
				    Locator::windowing::has_value() ? Locator::windowing::value().GetSize() : glm::zero<glm::ivec2>();
				_handOnCreature.reset();
				_creatureUnderHand.reset();
				if (screenSize.x > 0 && screenSize.y > 0)
				{
					auto& hands = Locator::creatureHandSystem::value();
					glm::vec3 rayOrigin;
					glm::vec3 rayDirection;
					camera.DeprojectScreenToWorld(static_cast<glm::vec2>(_mousePosition) / static_cast<glm::vec2>(screenSize),
					                              rayOrigin, rayDirection);
					// The hand isn't over the world while it is over a debug window, or in the temple
					const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
					if (!inTemple && !Locator::debugGui::value().IsMouseOverWindow())
					{
						_creatureUnderHand = hands.CreatureAlong(rayOrigin, rayDirection);
					}
					if (hands.GetCreature().has_value())
					{
						_handOnCreature =
						    hands.Update(rayOrigin, rayDirection, static_cast<glm::vec2>(_mousePosition),
						                 std::chrono::duration_cast<std::chrono::duration<float>>(deltaTime).count());
					}
					if (_handOnCreature.has_value())
					{
						handTransform.position = _handOnCreature->position;
					}
				}
				UpdateHandInterface();
			}
			{
				auto magic = profiler.BeginScoped(Profiler::Stage::MagicUpdate);
				UpdateMagicHand(handTransform.position,
				                std::chrono::duration_cast<std::chrono::duration<float>>(deltaTime).count());
			}
			Locator::entitiesRegistry::value().SetDirty();
		}

		// The leashes' ropes swing from where the hand now is
		{
			auto creatureLeash = profiler.BeginScoped(Profiler::Stage::CreatureLeashUpdate);
			Locator::leashSystem::value().Update(std::chrono::duration<float>(gameTime).count());
		}

		// Animate the hand: gripping while it drags the land, otherwise its normal pose. Turning the camera with the
		// middle button leaves the hand idle.
		if (_handAnimation)
		{
			using HandState = HandAnimation::State;
			using HandCycle = HandAnimation::Cycle;
			const bool dragging = _handGripping && !_handRotating;
			const auto state = dragging ? HandState::Camera : HandState::Normal;
			auto cycle = dragging ? HandCycle::Grip : HandCycle::Wiggle;
			// On a creature, it strokes it or shows its slap
			if (_handOnCreature.has_value())
			{
				cycle = _handOnCreature->slapping ? HandCycle::Slap
				        : _handOnCreature->onBody ? HandCycle::Stroke
				                                  : HandCycle::Wiggle;
			}
			_handAnimation->Update(deltaTime, state, cycle, _mousePosition);

			const auto handEntity = Locator::handSystem::value()
			                            .GetPlayerHands()[static_cast<size_t>(ecs::systems::HandSystemInterface::Side::Left)];
			auto& registry = Locator::entitiesRegistry::value();
			if (auto* hand = registry.TryGet<ecs::components::Hand>(handEntity))
			{
				hand->boneMatrices = _handAnimation->GetBoneMatrices();
			}
			// The hand keeps about the same size on screen however far away it is
			if (auto* handTransform = registry.TryGet<ecs::components::Transform>(handEntity))
			{
				const auto distance = glm::distance(camera.GetOrigin(), handTransform->position);
				// The game mirrors the mesh's left hand along its x axis to make a right hand
				const auto scale = _handAnimation->ScaleAtDistance(distance);
				handTransform->scale = glm::vec3(config.rightHandedHand ? -scale : scale, scale, scale);
			}
		}

		// The trees bend away from where the hand now is, and rustle
		{
			auto actions = profiler.BeginScoped(Profiler::Stage::VegetationUpdate);
			auto& vegetation = Locator::vegetation::value();
			vegetation.UpdateBendPoints();
			vegetation.Rustle(gameTime);
		}

		// Update Entities
		{
			auto updateEntities = profiler.BeginScoped(Profiler::Stage::UpdateEntities);
			if (config.drawEntities)
			{
				Locator::rendereringSystem::value().PrepareDraw(config.drawBoundingBoxes, config.drawFootpaths,
				                                                config.drawStreams);
			}
		}
	} // Update Uniforms

	// Update Audio
	{
		auto updateAudio = profiler.BeginScoped(Profiler::Stage::UpdateAudio);
		Locator::audio::value().Update();
	} // Update Audio

	return config.numFramesToSimulate == 0 || _frameCount < config.numFramesToSimulate;
}

bool Game::Initialize() noexcept
{
	auto& config = Locator::config::value();

	if (config.graphicsBackend != GraphicsBackend::Noop)
	{
		uint32_t extraFlags = 0;
		if (config.graphicsBackend == GraphicsBackend::Metal)
		{
			extraFlags |= SDL_WINDOW_METAL;
		}
		openblack::InitializeWindow(k_WindowTitle, config.resolution.x, config.resolution.y, config.displayMode, extraFlags);
	}

	using filesystem::Path;
	if (!InitializeEngine(config.graphicsBackend, config.vsync))
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("game"), "Failed to initialize engine services.");
		return false;
	}
	auto& fileSystem = Locator::filesystem::value();
	auto& events = Locator::events::value();

	events.AddHandler(std::function([this, &config](const SDL_Event& event) {
		// If gui captures this input, do not propagate
		if (!Locator::debugGui::value().ProcessEvents(event))
		{
			// Inside the temple, Escape goes back to its main room and out, as the temple's keys do
			if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE && event.key.repeat == 0 &&
			    Locator::temple::has_value() && Locator::temple::value().Active())
			{
				Locator::temple::value().Escape();
				return;
			}
			// The game's menu takes Escape, and the keyboard and mouse while it is open
			if (_interface && Locator::windowing::has_value() &&
			    _interface->ProcessEvent(event, static_cast<glm::u16vec2>(Locator::windowing::value().GetSize())))
			{
				HandleInterfaceAction();
				return;
			}
			config.running = this->ProcessEvents(event);
			Locator::gameActionSystem::value().ProcessEvent(event);
		}
	}));

	if (!fileSystem.IsPathValid(_gamePath))
	{
		// no key, don't guess, let the user know to set the command param
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Game Path missing",
		                         "Game path was not supplied, use the -g "
		                         "command parameter to set it.",
		                         nullptr);
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to find the GameDir.");
		return false;
	}

	fileSystem.SetGamePath(_gamePath);

	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "The GamePath is \"{}\".", fileSystem.GetGamePath().generic_string());

	if (std::filesystem::path(_startMap).is_absolute())
	{
		if (std::find(_startMap.begin(), _startMap.end(), "Scripts") != _startMap.end())
		{
			auto p = _startMap;
			while (p.filename() != "Scripts" && p != p.parent_path())
			{
				p = p.parent_path();
			}
			fileSystem.AddAdditionalPath(p.parent_path());
		}
		else
		{
			fileSystem.AddAdditionalPath(_startMap.parent_path());
		}
	}
	else
	{
		_startMap = fileSystem.GetPath<Path::Scripts>() / _startMap;
	}

	if (!InitializeGame())
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("game"), "Failed to initialize game services.");
		return false;
	}

	auto& resources = Locator::resources::value();
	auto& meshManager = resources.GetMeshes();
	auto& textureManager = resources.GetTextures();
	auto& animationManager = resources.GetAnimations();
	auto& levelManager = resources.GetLevels();
	auto& soundManager = resources.GetSounds();
	auto& glowManager = resources.GetGlows();
	auto& camPathManager = resources.GetCameraPaths();

	// The main room's markers of the temples, creatures and challenges on the map, and the creature's room's belts and
	// medals
	std::vector<std::string> icons {"I_citadel_on_map", "I_creature_on_map", "I_challenge_on_map"};
	for (uint32_t i = 0; i < CreatureCaveTrophies::k_IconCount; ++i)
	{
		icons.push_back(CreatureCaveTrophies::IconName(i));
	}
	for (const auto& icon : icons)
	{
		const auto path = fileSystem.GetPath<Path::Citadel>() / "icons" / fmt::format("{}.l3d", icon);
		try
		{
			meshManager.Load(fmt::format("temple/icons/{}", icon), resources::L3DLoader::FromDiskTag {}, path);
		}
		catch (std::runtime_error& err)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
		}
	}

	fileSystem.Iterate(fileSystem.GetPath<Path::Citadel>() / "OutsideMeshes", false,
	                   [&meshManager, &resources](const std::filesystem::path& f) {
		                   const auto extension = string_utils::LowerCase(f.extension().string());
		                   const auto name = fmt::format("temple/{}", string_utils::LowerCase(f.stem().string()));
		                   try
		                   {
			                   if (extension == ".zzz")
			                   {
				                   SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading temple mesh: {}", f.stem().string());
				                   meshManager.Load(name, resources::L3DLoader::FromDiskTag {}, f);
				                   // The temple's outside is blended from the temple meshes, into the first temple's,
				                   // and its entrance is picked under the cursor
				                   if (name.starts_with("temple/b_temple") || name.starts_with("temple/b_first_temple") ||
				                       name == "temple/entrance_l3d")
				                   {
					                   resources.GetL3DFiles().Load(name, resources::L3DFileLoader::FromDiskTag {}, f);
				                   }
			                   }
			                   else if (extension == ".16b")
			                   {
				                   // And its texture from these, from evil to neutral to good
				                   resources.GetBitmaps().Load(name, resources::Bitmap16BLoader::FromDiskTag {}, f);
			                   }
		                   }
		                   catch (std::runtime_error& err)
		                   {
			                   SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
		                   }
	                   });

	// The land's light is built from this every frame
	if (const auto palette = fileSystem.GetPath<filesystem::Path::WeatherSystem>() / "palette.raw"; fileSystem.Exists(palette))
	{
		resources.GetLandLightPalettes().Load(LandLightPalette::k_Id.value(), resources::LandLightPaletteLoader::FromDiskTag {},
		                                      palette);
	}

	fileSystem.Iterate( //
	    fileSystem.GetPath<filesystem::Path::Citadel>() / "engine", false,
	    [&meshManager, &glowManager](const std::filesystem::path& f) {
		    if (f.extension() == ".zzz")
		    {
			    if (f.stem().string().ends_with("lo_l3d"))
			    {
				    SPDLOG_LOGGER_WARN(
				        spdlog::get("game"),
				        "Skipping lo duplicate lo meshes. See https://github.com/openblack/openblack/issues/727");
				    return;
			    }
			    SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading interior temple mesh: {}", f.stem().string());
			    try
			    {
				    meshManager.Load(fmt::format("temple/interior/{}", f.stem().string()), resources::L3DLoader::FromDiskTag {},
				                     f);
			    }
			    catch (std::runtime_error& err)
			    {
				    SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
			    }
		    }
		    else if (f.extension() == ".glw")
		    {
			    SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading interior temple glows: {}", f.stem().string());
			    try
			    {
				    glowManager.Load(fmt::format("temple/interior/glow/{}", f.stem().string()),
				                     resources::LightLoader::FromDiskTag {}, f);
			    }
			    catch (std::runtime_error& err)
			    {
				    SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
			    }
		    }
	    });

	pack::PackFile pack;

	auto packResult = pack.ReadFile(*fileSystem.GetData(fileSystem.GetPath<Path::Data>() / "AllMeshes.g3d"));
	if (packResult != pack::PackResult::Success)
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("game"), "Unable to load AllMeshes.g3d: {}", pack::ResultToStr(packResult));
		return false;
	}

	const auto& meshes = pack.GetMeshes();
	// TODO (#749) use std::views::enumerate
	for (size_t i = 0; const auto& mesh : meshes)
	{
		const auto meshId = static_cast<MeshId>(i);
		meshManager.Load(meshId, resources::L3DLoader::FromBufferTag {}, k_MeshNames.at(i), mesh);
		++i;
	}

	const auto& textures = pack.GetTextures();
	for (auto const& [name, g3dTexture] : textures)
	{
		textureManager.Load(g3dTexture.header.id, resources::Texture2DLoader::FromPackTag {}, name, g3dTexture);
	}

	pack::PackFile animationPack;
	packResult = animationPack.ReadFile(*fileSystem.GetData(fileSystem.GetPath<Path::Data>() / "AllAnims.anm"));
	if (packResult != pack::PackResult::Success)
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("game"), "Unable to load AllAnims.anm: {}", pack::ResultToStr(packResult));
		return false;
	}

	const auto& animations = animationPack.GetAnimations();
	// TODO (#749) use std::views::enumerate
	for (size_t i = 0; i < animations.size(); i++)
	{
		animationManager.Load(i, resources::L3DAnimLoader::FromBufferTag {}, animations[i]);
	}

	fileSystem.Iterate(fileSystem.GetPath<Path::CreatureMesh>(), false, [&meshManager](const std::filesystem::path& f) {
		const auto& fileName = f.stem().string();
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading creature mesh: {}", fileName);
		try
		{
			if (string_utils::BeginsWith(fileName, "Hand"))
			{
				return;
			}

			const auto meshId = creature::GetIdFromMeshName(fileName);
			meshManager.Load(meshId, resources::L3DLoader::FromDiskTag {}, f);
		}
		catch (std::runtime_error& err)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
		}
	});

	fileSystem.Iterate(fileSystem.GetPath<Path::Symbols>(), false, [&camPathManager](const std::filesystem::path& f) {
		if (f.extension() != ".cam")
		{
			return;
		}
		const auto& fileName = f.stem().string();
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading symbol cam: {}", fileName);
		const auto pathId = fmt::format("symbol/{}", fileName);
		try
		{
			camPathManager.Load(pathId, resources::CameraPathLoader::FromDiskTag {}, f);
		}
		catch (std::runtime_error& err)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
		}
	});

	fileSystem.Iterate(fileSystem.GetPath<Path::CitadelEngine>(), false, [&camPathManager](const std::filesystem::path& f) {
		if (f.extension() != ".cam")
		{
			return;
		}
		const auto& fileName = f.stem().string();
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading interior temple cam: {}", fileName);
		const auto pathId = fmt::format("temple/{}", fileName);
		try
		{
			camPathManager.Load(pathId, resources::CameraPathLoader::FromDiskTag {}, f);
		}
		catch (std::runtime_error& err)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
		}
	});

	// Load loose one-off assets
	{
		using AFromDiskTag = resources::L3DAnimLoader::FromDiskTag;
		animationManager.Load("coffre", AFromDiskTag {}, fileSystem.GetPath<Path::Misc>() / "coffre.anm");

		using LFromDiskTag = resources::L3DLoader::FromDiskTag;
		meshManager.Load("hand", LFromDiskTag {}, fileSystem.GetPath<Path::CreatureMesh>() / "Hand_Boned_Base2.l3d");
		LoadHandAnimation();
		LoadCreatureRigs();
		meshManager.Load("coffre", LFromDiskTag {}, fileSystem.GetPath<Path::Misc>() / "coffre.l3d");
		// The collar the citadel's leash posts are drawn with
		if (const auto path = fileSystem.GetPath<Path::Misc>() / "leash.l3d"; fileSystem.Exists(path))
		{
			meshManager.Load("misc/leash", LFromDiskTag {}, path);
		}
		// The eyes every creature is drawn with
		for (const auto& [id, file] : {std::pair {ecs::components::CreatureEyes::k_EyeballMeshId, "Eyeball.l3d"},
		                               std::pair {ecs::components::CreatureEyes::k_EyelidMeshId, "Eyelid.l3d"}})
		{
			if (const auto path = fileSystem.GetPath<Path::Data>() / file; fileSystem.Exists(path))
			{
				meshManager.Load(id, LFromDiskTag {}, path);
			}
		}
		meshManager.Load("cone", LFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "cone.l3d");
		meshManager.Load("marker", LFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "marker.l3d");
		meshManager.Load("river", LFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "river.l3d");
		meshManager.Load("river2", LFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "river2.l3d");
		meshManager.Load("metre_sphere", LFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "metre_sphere.l3d");
		meshManager.Load(SkyInterface::k_SunMeshId.value(), LFromDiskTag {},
		                 fileSystem.GetPath<Path::WeatherSystem>() / "sun.l3d");
		meshManager.Load(SkyInterface::k_MoonMeshId.value(), LFromDiskTag {},
		                 fileSystem.GetPath<Path::WeatherSystem>() / "moon.l3d");
		meshManager.Load(ecs::components::Mist::k_MeshId, LFromDiskTag {}, fileSystem.GetPath<Path::Landscape>() / "mist.l3d");

		using CFromDiskTag = resources::CameraPathLoader::FromDiskTag;
		camPathManager.Load("cam", CFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "cam.cam");
		camPathManager.Load("flying", CFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "flying.cam");
	}

	// The game's menu, which greets the player by their profile's name: openblack has no profiles, so by the name
	// they log in with
	{
		const auto* user = std::getenv("USERNAME");
		user = user != nullptr ? user : std::getenv("USER");
		// The settings the menu starts with are the game's
		gui::MenuSettings settings;
		auto& audio = Locator::audio::value();
		settings.sfxVolume = audio.GetSfxVolume();
		settings.musicVolume = audio.GetMusicVolume();
		settings.leftHandedHand = !Locator::config::value().rightHandedHand;
		_interface = gui::GameInterface::Create(gui::ToUtf16(user != nullptr ? user : "Player"), std::move(settings));
		if (!_interface)
		{
			SPDLOG_LOGGER_WARN(spdlog::get("game"), "The game's menu is not available, Escape quits");
		}
		else
		{
			if (Locator::temple::has_value())
			{
				Locator::temple::value().SetInterface(_interface.get());
			}
		}
	}

	// TODO(raffclar): #400: Parse level files within the resource loader
	// TODO(raffclar): #405: Determine campaign levels from the challenge script file
	// Load the campaign levels
	fileSystem.Iterate(fileSystem.GetPath<Path::Scripts>(), false, [&levelManager](const std::filesystem::path& f) {
		const auto& name = f.stem().string();
		if (f.extension() != ".txt" || name.rfind("InfoScript", 0) != std::string::npos)
		{
			return;
		}
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading campaign level: {}", f.stem().string());
		try
		{
			if (Level::IsLevelFile(f))
			{
				levelManager.Load(fmt::format("campaign/{}", name), resources::LevelLoader::FromDiskTag {}, f,
				                  Level::LandType::Campaign);
			}
		}
		catch (std::runtime_error& err)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
		}
	});
	// Load Playgrounds
	// Attempt to load additional levels as playgrounds
	fileSystem.Iterate(fileSystem.GetPath<Path::Playgrounds>(), false, [&levelManager](const std::filesystem::path& f) {
		if (f.extension() != ".txt")
		{
			return;
		}
		const auto& name = f.stem().string();
		if (levelManager.Contains(fmt::format("playgrounds/{}", name)))
		{
			// Already added
			return;
		}

		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading custom level: {}", f.stem().string());
		try
		{
			if (Level::IsLevelFile(f))
			{
				levelManager.Load(fmt::format("playgrounds/{}", name), resources::LevelLoader::FromDiskTag {}, f,
				                  Level::LandType::Skirmish);
			}
		}
		catch (std::runtime_error& err)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
		}
	});

	// Load all sound packs in the Audio directory
	auto& audioManager = Locator::audio::value();
	fileSystem.Iterate(
	    fileSystem.GetPath<Path::Audio>(), true, [&audioManager, &soundManager, &fileSystem](const std::filesystem::path& f) {
		    if (f.extension() != ".sad")
		    {
			    return;
		    }

		    pack::PackFile soundPack;
		    SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Opening sound pack {}", f.filename().string());
		    const auto result = soundPack.ReadFile(*fileSystem.GetData(f));
		    if (result != pack::PackResult::Success)
		    {
			    SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Unable to load sound pack {}: {}", f.filename().string(),
			                        pack::ResultToStr(result));
			    return;
		    }
		    const auto& audioHeaders = soundPack.GetAudioSampleHeaders();
		    const auto& audioData = soundPack.GetAudioSamplesData();
		    auto soundName = std::filesystem::path(audioHeaders[0].name.data());

		    if (audioHeaders.empty())
		    {
			    SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Empty sound pack found for {}. Skipping", f.filename().string());
			    return;
		    }

		    auto groupName = f.filename().string();

		    // A hacky way of detecting if the sound is music as all music sounds end with "mpg"
		    if (soundName.extension() == ".mpg")
		    {
			    auto buffers = std::queue<std::vector<uint8_t>>();
			    auto packName = f.string();
			    audioManager.AddMusicEntry(packName);
		    }
		    else
		    {
			    audioManager.CreateSoundGroup(groupName);
			    for (size_t i = 0; i < audioHeaders.size(); i++)
			    {
				    soundName = std::filesystem::path(audioHeaders[i].name.data());
				    // Banks have gaps between their samples, which are skipped without skipping the samples after them
				    if (audioData[i].empty())
				    {
					    SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Empty sound buffer found for {}/{}. Skipping", groupName,
					                        audioHeaders[i].id);
					    continue;
				    }

				    const auto stringId = fmt::format("{}/{}", groupName, audioHeaders[i].id);
				    const entt::id_type id = entt::hashed_string(stringId.c_str());
				    const std::vector<std::vector<uint8_t>> buffer = {audioData[i]};
				    SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Loading sound {}: {}", stringId, audioHeaders[i].name.data());
				    soundManager.Load(id, resources::SoundLoader::FromBufferTag {}, audioHeaders[i], buffer);
				    audioManager.AddToSoundGroup(groupName, id);
			    }

			    // What the bank plays for things happening in the game, such as trees rustling
			    if (soundPack.HasBlock("LHAudioAnimArrayTable") && soundPack.HasBlock("LHAudioWaveNumTable"))
			    {
				    if (auto effects = audio::AnimEffectTable::Parse(soundPack.GetBlock("LHAudioAnimArrayTable"),
				                                                     soundPack.GetBlock("LHAudioWaveNumTable")))
				    {
					    audioManager.AddAnimEffects(groupName, std::move(*effects));
				    }
				    else
				    {
					    SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Malformed animation effects in sound pack {}",
					                       f.filename().string());
				    }
			    }
		    }
	    });

	{
		InfoFile infoFile;
		auto result = infoFile.LoadFromFile(Locator::filesystem::value().GetPath<filesystem::Path::Scripts>() / "info.dat");
		if (!result)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to load game info data.");
			return false;
		}
		Locator::infoConstants::reset(result.release());
	}

	fileSystem.Iterate(fileSystem.GetPath<Path::Textures>(), false, [&textureManager](const std::filesystem::path& f) {
		if (string_utils::LowerCase(f.extension().string()) == ".raw")
		{
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading raw texture: {}", f.stem().string());
			try
			{
				textureManager.Load(fmt::format("raw/{}", f.stem().string()), resources::Texture2DLoader::FromDiskTag {}, f);
			}
			catch (std::runtime_error& err)
			{
				SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
			}
		}
	});

	// The texture every creature's hair is drawn with, and its alpha beside it
	for (const auto& [id, name] : {std::pair {ecs::components::CreatureHair::k_TextureId, "C_Ape_Hair.raw"},
	                               std::pair {ecs::components::CreatureHair::k_AlphaTextureId, "C_Ape_Haira.raw"}})
	{
		const auto path = fileSystem.GetPath<Path::Data>() / name;
		if (!fileSystem.Exists(path))
		{
			continue;
		}
		try
		{
			textureManager.Load(id, resources::Texture2DLoader::FromDiskTag {}, path);
		}
		catch (std::runtime_error& err)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
		}
	}

	// What creatures' tattoos and marks are painted with
	try
	{
		const auto data = fileSystem.GetPath<Path::Data>();
		Locator::resources::value().GetCreatureSkinArt().Load(
		    creature_skin::k_ArtId, resources::CreatureSkinArtLoader::FromDiskTag {},
		    resources::CreatureSkinArtLoader::Paths {
		        .symbols = fileSystem.GetPath<Path::Textures>() / "PlayersSymbols.raw",
		        .defaultSymbols = fileSystem.GetPath<Path::Textures>() / "I_PLAYER_SYMBOLS_.raw",
		        .freshDamage = data / "damage_new256.raw",
		        .freshDamageAlpha = data / "damage_new256A.raw",
		        .oldDamage = data / "damage_old256.raw",
		        .oldDamageAlpha = data / "damage_old256A.raw",
		        .palette = data / "tattoocols.raw",
		    });
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
	}

	// The noise that makes the snow's edges on the land ragged
	try
	{
		textureManager.Load(snow_cover::k_NoiseTextureId.value(), resources::Texture2DLoader::FromDiskTag {},
		                    fileSystem.GetPath<Path::WeatherSystem>() / "snowmap.raw");
	}
	catch (std::runtime_error& err)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
	}

	return true;
}

bool Game::Run() noexcept
{
	auto& config = Locator::config::value();

	if (_startTestbed)
	{
		LoadTestbed();
	}
	else if (!LoadMap(_startMap))
	{
		return false;
	}

	Locator::dynamicsSystem::value().RegisterRigidBodies();

	auto& fileSystem = Locator::filesystem::value();

	auto challengePath = fileSystem.GetPath<filesystem::Path::Quests>() / "challenge.chl";
	if (fileSystem.Exists(challengePath))
	{
		auto& chlapi = Locator::chlapi::value();
		auto& lhvm = Locator::vm::value();
		// The virtual machine's errors go to the scripting log, where the editor's Scripts panel shows them
		lhvm.Initialise(
		    &chlapi.GetFunctionsTable(), nullptr, nullptr, nullptr,
		    [](lhvm::ErrorCode code, const std::string& text, uint32_t number) {
			    SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "Script error: {} ({} {})",
			                        lhvm::k_ErrorMsg.at(static_cast<size_t>(code)), text, number);
		    },
		    nullptr, nullptr);
		try
		{
			lhvm.LoadBinary(fileSystem.ReadAll(challengePath));
			// The story's scripts run the first land; on the testbed they would set its time of day and stop its clock
			if (!_startTestbed)
			{
				lhvm.StartScript("LandControlAll", lhvm::ScriptType::All);
			}
		}
		catch (const std::runtime_error& err)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to read challenge file at {}: {}",
			                    (fileSystem.GetGamePath() / challengePath).generic_string(), err.what());
		}
	}
	else
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Challenge file not found at {}",
		                    (fileSystem.GetGamePath() / challengePath).generic_string());
		return false;
	}

	// Initialize the Acceleration Structure
	Locator::entitiesMap::value().Rebuild();

	if (Locator::windowing::has_value())
	{
		const auto size = static_cast<glm::u16vec2>(Locator::windowing::value().GetSize());
		Locator::rendererInterface::value().ConfigureView(graphics::RenderPass::Main, size, 0x274659ff);
		Locator::oceanSystem::value().ResizeReflectionFramebuffer(size);
	}

	{
		uint16_t width;
		uint16_t height;
		Locator::oceanSystem::value().GetReflectionFramebuffer().GetSize(width, height);
		Locator::rendererInterface::value().ConfigureView(graphics::RenderPass::Reflection, {width, height}, 0x274659ff);
	}

	Game::SetTime(config.timeOfDay);
	Locator::time::value().Start();

	_frameCount = 0;
	auto lastTime = std::chrono::high_resolution_clock::now();
	auto& profiler = Locator::profiler::value();
	std::optional<debug::FrameStatsLog> frameStats;
	if (config.frameStatsInterval > 0)
	{
		frameStats.emplace(config.frameStatsInterval, config.frameStatsViews);
		if (config.frameStatsViews)
		{
			Locator::rendererInterface::value().SetProfile(true);
		}
	}
	auto frameStart = std::chrono::steady_clock::now();
	while (Update())
	{
		auto duration = std::chrono::high_resolution_clock::now() - lastTime;
		auto milliseconds = std::chrono::duration_cast<std::chrono::duration<uint32_t, std::milli>>(duration);
		{
			auto section = profiler.BeginScoped(Profiler::Stage::SceneDraw);

			const graphics::RendererInterface::DrawSceneDesc drawDesc {
			    .camera = &Locator::camera::value(),
			    .frameBuffer = nullptr,
			    .entities = Locator::entitiesRegistry::value(),
			    .time = milliseconds.count(), // TODO(#481): get actual time
			    .timeOfDay = config.timeOfDay,
			    .smallBumpMapStrength = config.smallBumpMapStrength,
			    .viewId = graphics::RenderPass::Main,
			    .drawSky = config.drawSky,
			    .drawWater = config.drawWater,
			    .drawIsland = config.drawIsland,
			    .drawEntities = config.drawEntities,
			    .drawSprites = config.drawSprites,
			    .drawVegetation = config.drawVegetation,
			    .drawBoundingBoxes = config.drawBoundingBoxes,
			    .cullBack = false,
			    .wireframe = config.wireframe,
			    .drawHand = (!_interface || !_interface->GetMenu().IsOpen()) &&
			                Locator::cinematicDirectorSystem::value().IsInterfaceActive(),
			};
			Locator::rendererInterface::value().DrawScene(drawDesc);
		}

		// The game's interface over the scene
		if (_interface && Locator::windowing::has_value())
		{
			glm::ivec2 mouse;
			SDL_GetMouseState(&mouse.x, &mouse.y);
			_interface->Draw(static_cast<glm::u16vec2>(Locator::windowing::value().GetSize()), mouse, SDL_GetTicks(),
			                 Locator::debugGui::value().IsMouseOverWindow());
		}

		{
			auto section = profiler.BeginScoped(Profiler::Stage::GuiDraw);
			const bool screenshotThisFrame = _requestScreenshot.has_value() && _requestScreenshot->first == _frameCount;
			if (screenshotThisFrame)
			{
				Locator::rendererInterface::value().RequestScreenshot(_requestScreenshot->second);
			}
			Locator::debugGui::value().Draw();
		}

		{
			auto section = profiler.BeginScoped(Profiler::Stage::RendererFrame);
			Locator::rendererInterface::value().Frame();
		}

		// Clear the stale screenshot request
		if (_requestScreenshot.has_value())
		{
			if (_requestScreenshot->first <= _frameCount)
			{
				_requestScreenshot = std::nullopt;
			}
		}

		_frameCount++;

		const auto frameEnd = std::chrono::steady_clock::now();
		if (frameStats.has_value())
		{
			frameStats->Frame(std::chrono::duration<float, std::milli>(frameEnd - frameStart).count(), profiler);
		}
		frameStart = frameEnd;
	}

	return true;
}

bool Game::LoadMap(const std::filesystem::path& path) noexcept
{
	auto& fileSystem = Locator::filesystem::value();

	if (!fileSystem.Exists(path))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Could not find script {}", path.generic_string());
		return false;
	}

	const auto data = fileSystem.ReadAll(path);
	const auto source = std::string(reinterpret_cast<const char*>(data.data()), data.size());

	PrepareNewLand();

	Script script;
	try
	{
		script.Load(source);
	}
	catch (const std::exception& e)
	{
		// A script that can't be read leaves the land as far as it got, rather than ending the game
		SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "Error in the map script {}: {}", path.generic_string(), e.what());
	}

	// Each released map comes with an optional .fot file which contains the footpath information for the map
	const auto stem = string_utils::LowerCase(path.stem().generic_string());
	const auto fotPath = fileSystem.GetPath<filesystem::Path::Landscape>() / fmt::format("{}.fot", stem);

	if (fileSystem.Exists(fotPath))
	{
		FotFile fotFile(*this);
		fotFile.Load(fotPath);
	}
	else
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "The map at {} does not come with a footpath file. Expected {}",
		                   path.generic_string(), fotPath.generic_string());
	}

	StartNewLand();
	return true;
}

void Game::LoadTestbed() noexcept
{
	// No script runs on the testbed: the story's would set its time of day and stop its clock a few turns in
	if (Locator::vm::has_value())
	{
		Locator::vm::value().StopAllTasks();
	}
	PrepareNewLand();
	InitializeLevel(flat_land::Build());
	SetUpLandscape();

	// Looking down over the middle of the map, from the south
	const auto& land = Locator::terrainSystem::value();
	const auto middle = (land.GetExtent().minimum + land.GetExtent().maximum) * 0.5f;
	const auto ground = land.GetHeightAt(middle);
	Locator::camera::value()
	    .SetOrigin({middle.x, ground + k_TestbedCameraHeight, middle.y - k_TestbedCameraBack})
	    .SetFocus({middle.x, ground, middle.y});

	StartNewLand();

	// The testbed has no temple to give its player influence: its miracles may be cast anywhere. A dispenser of every
	// miracle stands in a grid in front of the camera.
	Locator::magicSystem::value().SetIgnoreInfluence(true);
	testbed_dispensers::PlaceGrid(middle);

	// The testbed comes with its window of scenarios to try out on it
	if (Locator::debugGui::has_value())
	{
		Locator::debugGui::value().OpenWindow(debug::gui::k_TestbedScenariosWindow);
	}
}

void Game::PrepareNewLand()
{
	// A new land has no weather of the last one, and none of its script's fades, cinema bars or clipping
	if (Locator::weatherSystem::has_value())
	{
		Locator::weatherSystem::value().Reset();
		Locator::snowSystem::value().Reset();
		Locator::waterRingSystem::value().Reset();
	}
	Locator::cinematicDirectorSystem::value().Reset();
	Locator::influenceSystem::value().Reset();
	// Nor its creatures' footprints
	Locator::footprintSystem::value().Reset();
	// Nor its miracles, nor their particle effects
	Locator::magicSystem::value().Reset();
	Locator::magicSystem::value().SetIgnoreInfluence(false);
	Locator::particleSystem::value().Reset();

	// Reset everything. Deletes all entities and their components
	Locator::entitiesRegistry::value().Reset();
	// TODO(#661): split entities that are permanent from map entities and move hand and camera to init
	// We need a hand for the player
	Locator::handSystem::value().Initialize();

	// create our camera
	auto& config = Locator::config::value();
	const auto aspect = Locator::windowing::has_value() ? Locator::windowing::value().GetAspectRatio() : 1.0f;
	Locator::camera::value().SetProjectionMatrixPerspective(config.cameraXFov, aspect, config.cameraNearClip,
	                                                        config.cameraFarClip);
}

void Game::StartNewLand()
{
	_lastGameLoopTime = std::chrono::steady_clock::now();
	_turnDeltaTime = 0ns;
	// The game starts running, as Black & White does
	Locator::time::value().StartGameClock(false);
	SetGameSpeed(Game::k_TurnDurationMultiplierNormal);

	if (!_atmosAudio)
	{
		_atmosAudio = std::make_unique<audio::AtmosAudio>();
	}
	_atmosAudio->Init();
	if (!_gameMusic)
	{
		_gameMusic = std::make_unique<audio::GameMusic>();
	}
	_gameMusic->Reset();
}

void Game::HandleInterfaceAction()
{
	using Action = gui::GameMenu::Action;
	const auto action = _interface->TakeAction();

	// The settings the player changes take effect at once
	if (_interface->TakeSettingsChanged())
	{
		const auto& settings = _interface->GetMenu().GetSettings();
		auto& audio = Locator::audio::value();
		audio.SetSfxVolume(settings.sfxVolume);
		audio.SetMusicVolume(settings.musicVolume);
		Locator::config::value().rightHandedHand = !settings.leftHandedHand;
	}

	// The menu pauses the game while it is open
	const auto open = _interface->GetMenu().IsOpen();
	if (open && !_menuWasOpen)
	{
		_pausedBeforeMenu = IsPaused();
		Locator::time::value().SetPaused(true);
	}
	else if (!open && _menuWasOpen)
	{
		Locator::time::value().SetPaused(_pausedBeforeMenu);
	}
	_menuWasOpen = open;

	switch (action)
	{
	case Action::Quit:
		RequestQuit();
		break;
	case Action::StartSkirmish:
	case Action::JoinOnline:
	case Action::Statistics:
	case Action::CreatePlayer:
	case Action::DeletePlayer:
	case Action::EditTattoo:
	case Action::StartNewGame:
	case Action::RedefineControl:
		SPDLOG_LOGGER_INFO(spdlog::get("game"), "This part of the menu is not available yet");
		break;
	case Action::Continue:
	case Action::None:
		break;
	}
}

void Game::LoadLandscape(const std::filesystem::path& path)
{
	auto& fileSystem = Locator::filesystem::value();

	auto fixedName = fileSystem.FindPath(filesystem::FileSystemInterface::FixPath(path));

	if (!fileSystem.Exists(fixedName))
	{
		throw std::runtime_error("Could not find landscape " + path.generic_string());
	}
	InitializeLevel(fixedName);
	SetUpLandscape();
}

void Game::SetUpLandscape()
{
	// A land starts at noon on the game's cycle of day and night, which its script may change, under new clouds
	auto& sky = Locator::skySystem::value();
	sky.GetClock().Reset();
	sky.SetTime(sky.GetClock().GetScriptTime());
	Locator::cloudSystem::value().Reset();

	// There is always a player active
	Locator::playerSystem::value().AddPlayer(ecs::archetypes::PlayerArchetype::Create(PlayerNames::PLAYER_ONE));

	// There is always at least one player active.
	ecs::archetypes::PlayerArchetype::Create(PlayerNames::PLAYER_ONE);

	Locator::cameraBookmarkSystem::value().Initialize();
	Locator::dynamicsSystem::value().RegisterIslandRigidBodies(Locator::terrainSystem::value());
	Locator::playerSystem::value().RegisterPlayers();
	if (Locator::cameraPathSystem::value().IsPathing())
	{
		Locator::cameraPathSystem::value().Stop();
	}
}

void Game::SetTime(float time) noexcept
{
	Locator::skySystem::value().SetTime(time);
}

void Game::RequestScreenshot(const std::filesystem::path& path) noexcept
{
	_requestScreenshot = std::make_pair(_frameCount, path);
}

void Game::LoadHandAnimation()
{
	auto& fileSystem = Locator::filesystem::value();
	const auto path = fileSystem.GetPath<filesystem::Path::Data>() / "CTR" / "hh.hbn";
	const auto specPath = fileSystem.GetPath<filesystem::Path::Data>() / "hndspec5.txt";
	if (!fileSystem.Exists(path) || !fileSystem.Exists(specPath))
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "The hand is not animated: {} or {} is missing", path.string(),
		                   specPath.string());
		return;
	}

	pack::PackFile pack;
	const auto packResult = pack.ReadFile(*fileSystem.GetData(path));
	if (packResult != pack::PackResult::Success || !pack.HasBlock("Hand"))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Unable to read the Hand block of {}: {}", path.string(),
		                    pack::ResultToStr(packResult));
		return;
	}
	morph::MorphFile morphFile;
	const auto morphResult = morphFile.Open(pack.GetBlock("Hand"), fileSystem.FindPath(specPath).parent_path());
	if (morphResult != morph::MorphResult::Success)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Unable to read the hand animations of {}: {}", path.string(),
		                    morph::ResultToStr(morphResult));
		return;
	}

	const auto mesh = Locator::resources::value().GetMeshes().Handle(entt::hashed_string("hand"));
	auto animation = std::make_unique<HandAnimation>();
	if (!mesh || !animation->Load(morphFile, mesh->GetBoneParents(), mesh->GetBoneMatrices()))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "The hand animations of {} do not fit the hand mesh", path.string());
		return;
	}
	_handAnimation = std::move(animation);
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loaded the hand animations of {}", path.string());
}

void Game::LoadCreatureRigs()
{
	auto& fileSystem = Locator::filesystem::value();
	const auto directory = fileSystem.GetPath<filesystem::Path::Data>() / "CTR";
	const auto specPath = fileSystem.GetPath<filesystem::Path::Data>() / "ctrspec27.txt";
	if (!fileSystem.Exists(specPath))
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "The creatures are not animated: {} is missing", specPath.string());
		return;
	}
	const auto specDirectory = fileSystem.FindPath(specPath).parent_path();
	const auto meshDirectory = fileSystem.GetPath<filesystem::Path::CreatureMesh>();
	auto& rigs = Locator::resources::value().GetCreatureRigs();
	fileSystem.Iterate(directory, false, [&](const std::filesystem::path& path) {
		if (string_utils::LowerCase(path.extension().string()) != ".cbn")
		{
			return;
		}
		pack::PackFile pack;
		if (pack.ReadFile(*fileSystem.GetData(path)) != pack::PackResult::Success || !pack.HasBlock("Creature"))
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Unable to read the Creature block of {}", path.string());
			return;
		}
		const auto& block = pack.GetBlock("Creature");
		if (block.size() < sizeof(morph::MorphHeader))
		{
			return;
		}
		// The species is the one the base mesh named in the header is of
		morph::MorphHeader header {};
		std::memcpy(&header, block.data(), sizeof(header));
		const auto species = creature::GetSpeciesFromMeshName(header.baseMeshName.data());
		if (species == CreatureType::Unknown)
		{
			return;
		}
		try
		{
			rigs.Load(creature::GetRigId(species), resources::CreatureRigLoader::FromBufferTag {}, block, specDirectory,
			          meshDirectory);
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loaded the creature animations of {}", path.string());
		}
		catch (std::runtime_error& err)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}: {}", path.string(), err.what());
		}
	});
}

void Game::PlaceHand(ecs::components::Transform& handTransform, float deltaSeconds)
{
	const auto& camera = Locator::camera::value();
	const auto eye = camera.GetOrigin();
	const bool dragging = _handGripping && !_handRotating;
	const bool rotating = _handGripping && _handRotating;

	// In the temple the hand hangs on the line of sight through the cursor, a little short of where it
	// meets the room, slowly away from the camera and quickly towards it
	if (Locator::temple::has_value() && Locator::temple::value().Active())
	{
		if (_cursorWorldPosition)
		{
			const auto toRoom = *_cursorWorldPosition - eye;
			const auto roomDistance = glm::length(toRoom);
			if (roomDistance > 0.0f)
			{
				_handRayDirection = toRoom / roomDistance;
			}
			const auto target =
			    glm::clamp(glm::max(roomDistance - k_HandTempleGap, 1.0f), k_HandTempleMinDistance, k_HandTempleMaxDistance);
			const auto easeTime = _handHoverZoomer.GetDestination() <= target ? 0.4f : 0.2f;
			_handHoverZoomer.SetDestination(target, easeTime);
			_handHoverZoomer.Update(deltaSeconds);
			// The hand's distance from the camera
			_handDistance = _handHoverZoomer.GetValue();
		}
		handTransform.position = eye + _handRayDirection * _handDistance;
		return;
	}

	// Turning the camera, the hand holds on to the land under the cursor like it does dragging it, though it keeps
	// its idle pose, so it hovers its height above the land as it does over it
	if (rotating)
	{
		if (!_handWasRotating)
		{
			auto hold = handTransform.position;
			if (_cursorWorldPosition)
			{
				const auto overSea =
				    Locator::terrainSystem::value().GetHeightAt(glm::xz(*_cursorWorldPosition)) < k_HandSeaAltitude;
				const auto towardsLand = glm::normalize(*_cursorWorldPosition - eye);
				const auto handHeight = k_HandHeight * HandAnimation::SizeAtDistance(_handDistance);
				hold = overSea ? *_cursorWorldPosition : *_cursorWorldPosition - towardsLand * handHeight;
			}
			_handHoldPoint = hold;
			_handGripFrom = handTransform.position;
			_handGripBlend.Reset(0.0f);
			_handGripBlend.SetDestination(1.0f, k_HandGripSettleTime);
		}
		_handGripBlend.Update(deltaSeconds);
		handTransform.position = glm::mix(_handGripFrom, _handHoldPoint, _handGripBlend.GetValue());
		_handDistance = glm::clamp(glm::distance(eye, handTransform.position), k_HandMinDistance, k_HandMaxDistance);
		_handHoverZoomer.Reset(_handDistance);
		_handWasRotating = true;
		_handWasDragging = false;
		return;
	}
	_handWasRotating = false;

	// Dragging the land, the camera keeps the land the hand gripped under the cursor, and the hand is kept under the
	// cursor, so the hand holds on to that land and moves with it. It settles onto it as quickly as it changes pose.
	if (dragging)
	{
		if (!_handWasDragging)
		{
			_handGripPoint = _cursorWorldPosition.value_or(handTransform.position);
			_handGripFrom = handTransform.position;
			_handGripBlend.Reset(0.0f);
			_handGripBlend.SetDestination(1.0f, k_HandGripSettleTime);
		}
		_handGripBlend.Update(deltaSeconds);
		handTransform.position = glm::mix(_handGripFrom, _handGripPoint, _handGripBlend.GetValue());
		_handDistance = glm::clamp(glm::distance(eye, handTransform.position), k_HandMinDistance, k_HandMaxDistance);
		_handHoverZoomer.Reset(_handDistance);
		_handWasDragging = true;
		return;
	}
	_handWasDragging = false;

	// The game puts the origin of the hand, by its fingertips, on the line of sight through the cursor, so the hand is
	// always under the cursor on screen. How far along it depends on the land the cursor is over.
	if (_cursorWorldPosition)
	{
		const auto toLand = *_cursorWorldPosition - eye;
		const auto landDistance = glm::length(toLand);
		if (landDistance > 0.0f)
		{
			_handRayDirection = toLand / landDistance;
		}

		// The hand is pulled back from the land towards the camera by its height, so its fingers hang
		// down to the land. Over the sea it rests on the water.
		const auto overSea = Locator::terrainSystem::value().GetHeightAt(glm::xz(*_cursorWorldPosition)) < k_HandSeaAltitude;
		const auto handHeight = k_HandHeight * HandAnimation::SizeAtDistance(_handDistance);
		const auto nearest =
		    glm::clamp(overSea ? landDistance : landDistance - handHeight, k_HandMinDistance, k_HandMaxDistance);

		// The hand eases out to the land, slowly away from the camera and quickly towards it
		const auto target = glm::max(landDistance, 1.0f);
		const auto easeTime = _handHoverZoomer.GetValue() <= target ? k_HandEaseOutTime : k_HandEaseInTime;
		_handHoverZoomer.SetDestination(target, easeTime);
		_handHoverZoomer.Update(deltaSeconds);
		if (_handHoverZoomer.GetValue() < 1.0f)
		{
			_handHoverZoomer.Reset(1.0f);
		}
		// The hand's distance from the camera, no further out than the land less the hand's height
		_handDistance = glm::clamp(glm::min(_handHoverZoomer.GetValue(), nearest), k_HandMinDistance, k_HandMaxDistance);
	}
	handTransform.position = eye + _handRayDirection * _handDistance;
}

void Game::UpdateMagicHand(const glm::vec3& handPosition, float deltaSeconds)
{
	const auto screenSize = Locator::windowing::has_value() ? Locator::windowing::value().GetSize() : glm::zero<glm::ivec2>();
	ecs::systems::MagicSystemInterface::HandFrame frame {.handPosition = handPosition, .point = _cursorWorldPosition};
	if (screenSize.x > 0 && screenSize.y > 0)
	{
		Locator::camera::value().DeprojectScreenToWorld(
		    static_cast<glm::vec2>(_mousePosition) / static_cast<glm::vec2>(screenSize), frame.rayOrigin, frame.rayDirection);
	}
	frame.cameraForward = Locator::camera::value().GetForward();
	const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
	frame.overWorld = !inTemple && !Locator::debugGui::value().IsMouseOverWindow();
	auto& magic = Locator::magicSystem::value();
	magic.UpdateHand(frame, deltaSeconds);
	magic.Update(deltaSeconds);
}

void Game::PlayHandGrabSound()
{
	if (!_cursorWorldPosition || !Locator::terrainSystem::has_value())
	{
		return;
	}
	const auto position = *_cursorWorldPosition;

	// Land is a cell of the landscape without water
	bool isLand = false;
	const auto cell = glm::floor(glm::vec2(position.x, position.z) / 10.0f);
	if (cell.x >= 0.0f && cell.y >= 0.0f && cell.x < 512.0f && cell.y < 512.0f)
	{
		const auto* landCell = Locator::terrainSystem::value().FindCell(glm::u16vec2(cell));
		isLand = landCell != nullptr && landCell->properties.hasWater == 0;
	}

	auto& audio = Locator::audio::value();
	if (isLand)
	{
		// The game throws up a spot visual where the hand grips the land, as it plays the sound
		GripLandscapeEffect::Spawn(
		    glm::vec3(position.x, Locator::terrainSystem::value().GetHeightAt(glm::xz(position)), position.z));
	}
	if (isLand)
	{
		// One of G_HandGrabLand_01 to _06, centred on the listener
		const auto sample = 4 + Locator::rng::value().NextValue(0, 5);
		const auto id = fmt::format("InGame.sad/{}", sample);
		audio.PlaySoundEffect(entt::hashed_string(id.c_str()), std::nullopt);
	}
	else
	{
		// While the game runs, the hand splashes where it goes in: a ring on the water, in the land's brightest light
		if (!Locator::time::value().IsPaused())
		{
			const auto angle = Locator::gameRandom::value().CrtRandom(0.0f, glm::two_pi<float>());
			Locator::waterRingSystem::value().Add(
			    water_rings::HandSplash(glm::vec2(position.x, position.z), angle, FrameLandLight(255)));
		}
		// G_HandInWater_01 to _10 in turn, on the water's surface where the hand went in
		const auto id = fmt::format("InGame.sad/{}", 99 + _handInWaterSample);
		_handInWaterSample = (_handInWaterSample + 1) % 10;
		audio.PlaySoundEffect(entt::hashed_string(id.c_str()), glm::vec3(position.x, 0.2f, position.z));
	}
}

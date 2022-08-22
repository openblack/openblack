/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Game.h"

#include <string>

#include <LHVM.h>
#include <SDL.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/OceanInterface.h"
#include "3D/SkyInterface.h"
#include "3D/TempleInteriorInterface.h"
#include "Audio/AudioManagerInterface.h"
#include "CHLApi.h"
#include "Camera/Camera.h"
#include "Common/EventManager.h"
#include "Common/StringUtils.h"
#include "Debug/DebugGuiInterface.h"
#include "ECS/Archetypes/PlayerArchetype.h"
#include "ECS/Components/CameraBookmark.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CameraBookmarkSystemInterface.h"
#include "ECS/Systems/CameraPathSystemInterface.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/PathfindingSystemInterface.h"
#include "ECS/Systems/PlayerSystemInterface.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "EngineConfig.h"
#include "FileSystem/FileSystemInterface.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/RendererInterface.h"
#include "Input/GameActionMapInterface.h"
#include "LHScriptX/Script.h"
#include "Locator.h"
#include "Parsers/InfoFile.h"
#include "Profiler.h"
#include "Resources/Loaders.h"
#include "Resources/MeshId.h"
#include "Resources/ResourcesInterface.h"
#include "Serializer/FotFile.h"

#ifdef __ANDROID__
#include <spdlog/sinks/android_sink.h>
#endif

using namespace openblack;
using namespace openblack::lhscriptx;
using namespace std::chrono_literals;

const std::string k_WindowTitle = "openblack";

Game* Game::sInstance = nullptr;

Game::Game(Arguments&& args) noexcept
    : _gamePath(args.gamePath)
    , _startMap(args.startLevel)
    , _handPose(glm::identity<glm::mat4>())
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
			createLogger = [&args](const std::string& name) { return spdlog::basic_logger_mt(name, args.logFile); };
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
	config.numFramesToSimulate = args.numFramesToSimulate;
	config.numFramesToSimulate = args.numFramesToSimulate;
	config.rendererType = args.rendererType;
	config.resolution = {args.windowWidth, args.windowHeight};
	config.displayMode = args.displayMode;
	config.rendererType = args.rendererType;
	config.vsync = args.vsync;
	config.guiScale = args.guiScale;
}

Game::~Game() noexcept
{
	ShutDownServices();
	SDL_Quit(); // todo: move to GameWindow
	spdlog::shutdown();
}

bool Game::ProcessEvents(const SDL_Event& event) noexcept
{
	static bool leftMouseButton = false;
	static bool middleMouseButton = false;

	if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == SDL_BUTTON_LEFT)
	{
		leftMouseButton = !leftMouseButton;
	}
	if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == SDL_BUTTON_MIDDLE)
	{
		middleMouseButton = !middleMouseButton;
	}

	_handGripping = middleMouseButton || leftMouseButton;

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

			auto aspect = window.GetAspectRatio();
			const auto& config = Locator::config::value();
			camera.SetProjectionMatrixPerspective(config.cameraXFov, aspect, config.cameraNearClip, config.cameraFarClip);
		}
		break;
	case SDL_KEYDOWN:
		switch (event.key.keysym.sym)
		{
		case SDLK_ESCAPE:
			return false;
		case SDLK_f:
			window.SetDisplayMode(windowing::DisplayMode::Fullscreen);
			break;
		case SDLK_p:
			_paused = !_paused;
			break;
		case SDLK_F1:
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
			if ((event.key.keysym.mod & KMOD_CTRL) != 0)
			{
				const auto handPosition = _handPose * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				const auto index = static_cast<uint8_t>(event.key.keysym.sym - SDLK_1);
				Locator::cameraBookmarkSystem::value().SetBookmark(index, handPosition, camera.GetOrigin());
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

bool Game::GameLogicLoop() noexcept
{
	using namespace ecs::components;
	using namespace ecs::systems;

	if (_paused)
	{
		return false;
	}

	const auto currentTime = std::chrono::steady_clock::now();
	const auto delta = currentTime - _lastGameLoopTime;
	const auto turnDuration = k_TurnDuration * _gameSpeedMultiplier;
	// NOLINTNEXTLINE(modernize-use-nullptr): clang-tidy bug
	if (delta < turnDuration)
	{
		return false;
	}

	// Build Map Grid Acceleration Structure
	Locator::entitiesMap::value().Rebuild();

	auto& profiler = Locator::profiler::value();

	{
		auto pathfinding = profiler.BeginScoped(Profiler::Stage::PathfindingUpdate);
		Locator::pathfindingSystem::value().Update();
	}
	{
		auto actions = profiler.BeginScoped(Profiler::Stage::LivingActionUpdate);
		Locator::livingActionSystem::value().Update();
	}

	auto& lhvm = Locator::vm::value();
	lhvm.LookIn(lhvm::ScriptType::All);

	_lastGameLoopTime = currentTime;
	_turnDeltaTime = delta;
	++_turnCount;

	return false;
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

	// Physics
	{
		auto physics = profiler.BeginScoped(Profiler::Stage::PhysicsUpdate);
		if (_frameCount > 0)
		{
			auto& dynamicsSystem = Locator::dynamicsSystem::value();
			dynamicsSystem.Update(deltaTime);
			dynamicsSystem.UpdatePhysicsTransforms();
		}
	}

	// Input events
	{
		auto sdlInput = profiler.BeginScoped(Profiler::Stage::SdlInput);
		if (!Locator::debugGui::value().StealsFocus())
		{
			Locator::gameActionSystem::value().Frame();
		}
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0)
		{
			Locator::events::value().Create<SDL_Event>(e);
		}
		camera.HandleActions(deltaTime);
	}
	Locator::cameraPathSystem::value().Update(deltaTime);

	if (!config.running)
	{
		return false;
	}

	// ImGui events + prepare
	{
		auto guiLoop = profiler.BeginScoped(Profiler::Stage::GuiLoop);
		if (Locator::debugGui::value().Loop())
		{
			return false; // Quit event
		}
	}

	camera.Update(deltaTime);
	Locator::cameraBookmarkSystem::value().Update(deltaTime);

	// Update Game Logic in Registry
	{
		auto gameLogic = profiler.BeginScoped(Profiler::Stage::GameLogic);
		if (GameLogicLoop())
		{
			return false; // Quit event
		}
	}

	// Update Uniforms
	{
		auto profilerScopedUpdateUniforms = profiler.BeginScoped(Profiler::Stage::UpdateUniforms);

		// Update Debug Cross
		ecs::components::Transform intersectionTransform {};
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

				if (!glm::any(glm::isnan(rayOrigin) || glm::isnan(rayDirection)))
				{
					if (auto hit = dynamicsSystem.RayCastClosestHit(rayOrigin, rayDirection, 1e10f))
					{
						intersectionTransform = hit->first;
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
						}
					}
				}
				intersectionTransform.scale = scale;
				_handPose = glm::mat4(1.0f);
				_handPose = glm::translate(_handPose, intersectionTransform.position);
				_handPose *= glm::mat4(intersectionTransform.rotation);
				_handPose = glm::scale(_handPose, intersectionTransform.scale);
				Locator::rendererInterface::value().UpdateDebugCrossUniforms(
				    glm::translate(camera.GetFocus(Camera::Interpolation::Target)));
			}
		}

		// Update Hand
		if (!_handGripping)
		{
			const glm::vec3 handOffset(0, 1.5f, 0);
			const glm::mat4 modelRotationCorrection = glm::eulerAngleX(glm::radians(90.0f));

			const auto handEntity = Locator::handSystem::value()
			                            .GetPlayerHands()[static_cast<size_t>(ecs::systems::HandSystemInterface::Side::Left)];
			auto& handTransform = Locator::entitiesRegistry::value().Get<ecs::components::Transform>(handEntity);
			// TODO(#480): move using velocity rather than snapping hand to intersectionTransform
			handTransform.position = intersectionTransform.position;
			handTransform.rotation = glm::eulerAngleY(camera.GetRotation().y) * modelRotationCorrection;
			handTransform.rotation = intersectionTransform.rotation * handTransform.rotation;
			handTransform.position += intersectionTransform.rotation * handOffset;
			Locator::entitiesRegistry::value().SetDirty();
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

	if (config.rendererType != bgfx::RendererType::Noop)
	{
		uint32_t extraFlags = 0;
		if (config.rendererType == bgfx::RendererType::Enum::Metal)
		{
			extraFlags |= SDL_WINDOW_METAL;
		}
		openblack::InitializeWindow(k_WindowTitle, config.resolution.x, config.resolution.y, config.displayMode, extraFlags);
	}

	using filesystem::Path;
	if (!InitializeEngine(static_cast<uint8_t>(config.rendererType), config.vsync))
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

	fileSystem.Iterate(
	    fileSystem.GetPath<Path::Citadel>() / "OutsideMeshes", false, [&meshManager](const std::filesystem::path& f) {
		    if (f.extension() == ".zzz")
		    {
			    SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading temple mesh: {}", f.stem().string());
			    try
			    {
				    meshManager.Load(fmt::format("temple/{}", f.stem().string()), resources::L3DLoader::FromDiskTag {}, f);
			    }
			    catch (std::runtime_error& err)
			    {
				    SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
			    }
		    }
	    });

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
		meshManager.Load("coffre", LFromDiskTag {}, fileSystem.GetPath<Path::Misc>() / "coffre.l3d");
		meshManager.Load("cone", LFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "cone.l3d");
		meshManager.Load("marker", LFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "marker.l3d");
		meshManager.Load("river", LFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "river.l3d");
		meshManager.Load("river2", LFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "river2.l3d");
		meshManager.Load("metre_sphere", LFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "metre_sphere.l3d");

		using CFromDiskTag = resources::CameraPathLoader::FromDiskTag;
		camPathManager.Load("cam", CFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "cam.cam");
		camPathManager.Load("flying", CFromDiskTag {}, fileSystem.GetPath<Path::Data>() / "flying.cam");
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
				    if (audioData[i].empty())
				    {
					    SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Empty sound buffer found for {}. Skipping",
					                       soundName.string());
					    return;
				    }

				    const auto stringId = fmt::format("{}/{}", groupName, audioHeaders[i].id);
				    const entt::id_type id = entt::hashed_string(stringId.c_str());
				    const std::vector<std::vector<uint8_t>> buffer = {audioData[i]};
				    SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Loading sound {}: {}", stringId, audioHeaders[i].name.data());
				    soundManager.Load(id, resources::SoundLoader::FromBufferTag {}, audioHeaders[i], buffer);
				    audioManager.AddToSoundGroup(groupName, id);
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

	return true;
}

bool Game::Run() noexcept
{
	auto& config = Locator::config::value();

	if (!LoadMap(_startMap))
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
		lhvm.Initialise(&chlapi.GetFunctionsTable(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		try
		{
			lhvm.LoadBinary(fileSystem.ReadAll(challengePath));
			lhvm.StartScript("LandControlAll", lhvm::ScriptType::All);
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
	}

	{
		uint16_t width;
		uint16_t height;
		Locator::oceanSystem::value().GetReflectionFramebuffer().GetSize(width, height);
		Locator::rendererInterface::value().ConfigureView(graphics::RenderPass::Reflection, {width, height}, 0x274659ff);
	}

	if (config.drawIsland)
	{
		uint16_t width;
		uint16_t height;
		Locator::terrainSystem::value().GetFootprintFramebuffer().GetSize(width, height);
		Locator::rendererInterface::value().ConfigureView(graphics::RenderPass::Footprint, {width, height}, 0x00000000);
	}

	Game::SetTime(config.timeOfDay);

	_frameCount = 0;
	auto lastTime = std::chrono::high_resolution_clock::now();
	auto& profiler = Locator::profiler::value();
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
			    .bumpMapStrength = config.bumpMapStrength,
			    .smallBumpMapStrength = config.smallBumpMapStrength,
			    .viewId = graphics::RenderPass::Main,
			    .drawSky = config.drawSky,
			    .drawWater = config.drawWater,
			    .drawIsland = config.drawIsland,
			    .drawEntities = config.drawEntities,
			    .drawSprites = config.drawSprites,
			    .drawTestModel = config.drawTestModel,
			    .drawDebugCross = config.drawDebugCross,
			    .drawBoundingBoxes = config.drawBoundingBoxes,
			    .cullBack = false,
			    .wireframe = config.wireframe,
			};
			Locator::rendererInterface::value().DrawScene(drawDesc);
		}

		{
			auto section = profiler.BeginScoped(Profiler::Stage::GuiDraw);
			const bool screenshotThisFrame = _requestScreenshot.has_value() && _requestScreenshot->first == _frameCount;
			// Skip drawing Debug UI for screenshots
			if (screenshotThisFrame)
			{
				SPDLOG_LOGGER_INFO(spdlog::get("game"), "Requesting a screenshot at frame {}...", _frameCount);
				Locator::rendererInterface::value().RequestScreenshot(_requestScreenshot->second);
			}
			else
			{
				Locator::debugGui::value().Draw();
			}
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

	Script script;
	script.Load(source);

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

	_lastGameLoopTime = std::chrono::steady_clock::now();
	_turnDeltaTime = 0ns;
	SetGameSpeed(Game::k_TurnDurationMultiplierNormal);
	_turnCount = 0;
	_paused = true;

	return true;
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

	// There is always a player active
	Locator::playerSystem::value().AddPlayer(ecs::archetypes::PlayerArchetype::Create(PlayerNames::PLAYER_ONE));

	// There is always at least one player active.
	ecs::archetypes::PlayerArchetype::Create(PlayerNames::PLAYER_ONE);

	Locator::cameraBookmarkSystem::value().Initialize();
	Locator::dynamicsSystem::value().RegisterIslandRigidBodies(Locator::terrainSystem::value());
	Locator::playerSystem::value().RegisterPlayers();
}

void Game::SetTime(float time) noexcept
{
	Locator::skySystem::value().SetTime(time);
}

void Game::RequestScreenshot(const std::filesystem::path& path) noexcept
{
	_requestScreenshot = std::make_pair(_frameCount, path);
}

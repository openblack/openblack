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

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "3D/LandIslandInterface.h"
#include "3D/Sky.h"
#include "3D/TempleInteriorInterface.h"
#include "3D/Water.h"
#include "Audio/AudioManagerInterface.h"
#include "Camera/Camera.h"
#include "Common/EventManager.h"
#include "Common/RandomNumberManager.h"
#include "Common/StringUtils.h"
#include "Debug/Gui.h"
#include "ECS/Archetypes/HandArchetype.h"
#include "ECS/Archetypes/MobileStaticArchetype.h"
#include "ECS/Archetypes/PlayerArchetype.h"
#include "ECS/Components/CameraBookmark.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Transform.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CameraBookmarkSystemInterface.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/PathfindingSystemInterface.h"
#include "ECS/Systems/PlayerSystemInterface.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "ECS/Systems/TownSystemInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/Texture2D.h"
#include "Input/GameActionMapInterface.h"
#include "LHScriptX/Script.h"
#include "LHVM.h"
#include "Locator.h"
#include "PackFile.h"
#include "Parsers/InfoFile.h"
#include "Profiler.h"
#include "ReferenceManager.h"
#include "Renderer.h"
#include "Resources/Loaders.h"
#include "Resources/ResourcesInterface.h"
#include "ScriptHeaders/ScriptEnums.h"
#include "Serializer/FotFile.h"

#ifdef __ANDROID__
#include <spdlog/sinks/android_sink.h>
#endif

using namespace openblack;
using namespace openblack::lhscriptx;
using namespace openblack::ecs::archetypes;
using namespace std::chrono_literals;

const std::string k_WindowTitle = "openblack";

Game* Game::sInstance = nullptr;

std::unordered_set<std::string> GetUniqueWords(const std::string& strings)
{
	std::unordered_set<std::string> result;
	std::istringstream iss(strings);
	std::string word;
	while (std::getline(iss, word, ' '))
	{
		result.insert(word);
	}
	return result;
}

Game::Game(Arguments&& args)
    : _gamePath(args.gamePath)
    , _camera(std::make_unique<Camera>(glm::zero<glm::vec3>()))
    , _eventManager(std::make_unique<EventManager>())
    , _startMap(args.startLevel)
    , _handPose(glm::identity<glm::mat4>())
    , _requestScreenshot(args.requestScreenshot)
{
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

	std::string binaryPath = std::filesystem::path {args.executablePath}.parent_path().generic_string();
	_config.numFramesToSimulate = args.numFramesToSimulate;
	SPDLOG_LOGGER_INFO(spdlog::get("game"), "current binary path: {}", binaryPath);
	if (args.rendererType != bgfx::RendererType::Noop)
	{
		uint32_t extraFlags = 0;
		if (args.rendererType == bgfx::RendererType::Enum::Metal)
		{
			extraFlags |= SDL_WINDOW_METAL;
		}
		openblack::InitializeWindow(k_WindowTitle, args.windowWidth, args.windowHeight, args.displayMode, extraFlags);
	}
	try
	{
		_renderer = std::make_unique<Renderer>(args.rendererType, args.vsync);
	}
	catch (std::runtime_error& exception)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to create renderer", exception.what(), nullptr);
		throw exception;
	}

	_gui = debug::gui::Gui::Create(graphics::RenderPass::ImGui, args.scale);

	_eventManager->AddHandler(std::function([this](const SDL_Event& event) {
		// If gui captures this input, do not propagate
		if (!this->_gui->ProcessEvents(event))
		{
			this->_config.running = this->ProcessEvents(event);
			Locator::gameActionSystem::value().ProcessEvent(event);
		}
	}));
}

Game::~Game()
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
	Locator::cameraBookmarkSystem::reset();
	Locator::livingActionSystem::reset();
	Locator::townSystem::reset();
	Locator::pathfindingSystem::reset();
	Locator::terrainSystem::reset();
	Locator::filesystem::reset();
	Locator::gameActionSystem::reset();

	_water.reset();
	_sky.reset();
	_gui.reset();
	_renderer.reset();
	Locator::windowing::reset();
	_eventManager.reset();
	SDL_Quit(); // todo: move to GameWindow
	spdlog::shutdown();
}

entt::entity Game::GetHand() const
{
	return _handEntity;
}

bool Game::ProcessEvents(const SDL_Event& event)
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
			const auto width = static_cast<uint16_t>(event.window.data1);
			const auto height = static_cast<uint16_t>(event.window.data2);
			_renderer->Reset(width, height);
			_renderer->ConfigureView(graphics::RenderPass::Main, width, height);

			auto aspect = window.GetAspectRatio();
			_camera->SetProjectionMatrixPerspective(_config.cameraXFov, aspect, _config.cameraNearClip, _config.cameraFarClip);
		}
		break;
	case SDL_KEYDOWN:
		_keysdown.insert(event.key.keysym.scancode);
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
			_config.bgfxDebug = !_config.bgfxDebug;
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
				Locator::cameraBookmarkSystem::value().SetBookmark(index, handPosition, _camera->GetOrigin());
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
					_camera->GetModel().SetFlight(bookmark->savedOrigin, transform->position);
				}
			}
			break;
		}
		break;
	case SDL_KEYUP:
		_keysdown.erase(event.key.keysym.scancode);
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

bool Game::GameLogicLoop()
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

	{
		auto pathfinding = _profiler->BeginScoped(Profiler::Stage::PathfindingUpdate);
		Locator::pathfindingSystem::value().Update();
	}
	{
		auto actions = _profiler->BeginScoped(Profiler::Stage::LivingActionUpdate);
		Locator::livingActionSystem::value().Update();
	}

	_lhvm->LookIn(LHVM::ScriptType::ALL);
	_refManager.GC();

	_lastGameLoopTime = currentTime;
	_turnDeltaTime = delta;
	++_turnCount;

	return false;
}

bool Game::Update()
{
	_profiler->Frame();
	auto previous = _profiler->GetEntries().at(_profiler->GetEntryIndex(-1)).frameStart;
	auto current = _profiler->GetEntries().at(_profiler->GetEntryIndex(0)).frameStart;
	// Prevent spike at first frame
	if (previous.time_since_epoch().count() == 0)
	{
		current = previous;
	}
	auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(current - previous);

	// Physics
	{
		auto physics = _profiler->BeginScoped(Profiler::Stage::PhysicsUpdate);
		if (_frameCount > 0)
		{
			auto& dynamicsSystem = Locator::dynamicsSystem::value();
			dynamicsSystem.Update(deltaTime);
			dynamicsSystem.UpdatePhysicsTransforms();
		}
	}

	// Input events
	{
		auto sdlInput = _profiler->BeginScoped(Profiler::Stage::SdlInput);
		if (!this->_gui->StealsFocus())
		{
			Locator::gameActionSystem::value().Frame();
		}
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0)
		{
			_eventManager->Create<SDL_Event>(e);
		}
		_camera->HandleActions(deltaTime);
	}

	if (!this->_config.running)
	{
		return false;
	}

	// ImGui events + prepare
	{
		auto guiLoop = _profiler->BeginScoped(Profiler::Stage::GuiLoop);
		if (_gui->Loop(*this, *_renderer))
		{
			return false; // Quit event
		}
	}

	_camera->Update(deltaTime);
	Locator::cameraBookmarkSystem::value().Update(deltaTime);

	// Update Game Logic in Registry
	{
		auto gameLogic = _profiler->BeginScoped(Profiler::Stage::GameLogic);
		if (GameLogicLoop())
		{
			return false; // Quit event
		}
	}

	// Update Uniforms
	{
		auto profilerScopedUpdateUniforms = _profiler->BeginScoped(Profiler::Stage::UpdateUniforms);

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
				_camera->DeprojectScreenToWorld(static_cast<glm::vec2>(_mousePosition) / static_cast<glm::vec2>(screenSize),
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
				_renderer->UpdateDebugCrossUniforms(glm::translate(_camera->GetFocus(Camera::Interpolation::Target)));
			}
		}

		// Update Hand
		if (!_handGripping)
		{
			const glm::vec3 handOffset(0, 1.5f, 0);
			const glm::mat4 modelRotationCorrection = glm::eulerAngleX(glm::radians(90.0f));
			auto& handTransform = Locator::entitiesRegistry::value().Get<ecs::components::Transform>(_handEntity);
			// TODO(#480): move using velocity rather than snapping hand to intersectionTransform
			handTransform.position = intersectionTransform.position;
			handTransform.rotation = glm::eulerAngleY(_camera->GetRotation().y) * modelRotationCorrection;
			handTransform.rotation = intersectionTransform.rotation * handTransform.rotation;
			handTransform.position += intersectionTransform.rotation * handOffset;
			Locator::entitiesRegistry::value().SetDirty();
		}

		// Update Entities
		{
			auto updateEntities = _profiler->BeginScoped(Profiler::Stage::UpdateEntities);
			if (_config.drawEntities)
			{
				Locator::rendereringSystem::value().PrepareDraw(_config.drawBoundingBoxes, _config.drawFootpaths,
				                                                _config.drawStreams);
			}
		}
	} // Update Uniforms

	// Update Audio
	{
		auto updateAudio = _profiler->BeginScoped(Profiler::Stage::UpdateAudio);
		Locator::audio::value().Update(*this);
	} // Update Audio

	return _config.numFramesToSimulate == 0 || _frameCount < _config.numFramesToSimulate;
}

bool Game::Initialize()
{
	using filesystem::Path;
	InitializeGame();
	auto& fileSystem = Locator::filesystem::value();
	auto& resources = Locator::resources::value();
	auto& meshManager = resources.GetMeshes();
	auto& textureManager = resources.GetTextures();
	auto& animationManager = resources.GetAnimations();
	auto& levelManager = resources.GetLevels();
	auto& soundManager = resources.GetSounds();
	auto& glowManager = resources.GetGlows();

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

	pack.ReadFile(*fileSystem.GetData(fileSystem.GetPath<Path::Data>() / "AllMeshes.g3d"));

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
	animationPack.ReadFile(*fileSystem.GetData(fileSystem.GetPath<Path::Data>() / "AllAnims.anm"));

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

	// Create profiler
	_profiler = std::make_unique<Profiler>();

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
		    soundPack.ReadFile(*fileSystem.GetData(f));
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

	if (!LoadVariables())
	{
		return false;
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

	_sky = std::make_unique<Sky>();
	_water = std::make_unique<Water>();

	InitFunctionsTable();

	return true;
}

void Game::NativeCallEnterCallback([[maybe_unused]] const uint32_t func)
{
	// SPDLOG_LOGGER_DEBUG(spdlog::get("lhvm"), "enter {}()", _functionsTable[func].name);
}

void Game::NativeCallExitCallback([[maybe_unused]] const uint32_t func)
{
	// SPDLOG_LOGGER_DEBUG(spdlog::get("lhvm"), "exit {}()", _functionsTable[func].name);
}

void Game::StopTaskCallback([[maybe_unused]] const uint32_t taskNumber)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("lhvm"), "Task {} stopped", taskNumber);
}

void Game::LhvmErrorCallback([[maybe_unused]] const LHVM::ErrorCode code, [[maybe_unused]] const std::string v0,
                             [[maybe_unused]] const uint32_t v1)
{
	const auto& msg = LHVM::k_ErrorMsg.at(static_cast<int>(code));
	SPDLOG_LOGGER_ERROR(spdlog::get("lhvm"), "{}", msg);
}

void Game::AddObjectReference(const uint32_t objId)
{
	_refManager.AddReference(objId);
}

void Game::RemoveObjectReference(const uint32_t objId)
{
	_refManager.RemoveReference(objId);
}

glm::vec3 Game::PopVec()
{
	const auto z = _lhvm->Popf();
	const auto y = _lhvm->Popf();
	const auto x = _lhvm->Popf();
	return {x, y, z};
}

void Game::PushVec(const glm::vec3& vec)
{
	_lhvm->Pushv(vec.x);
	_lhvm->Pushv(vec.y);
	_lhvm->Pushv(vec.z);
}

std::string Game::PopString()
{
	return _lhvm->GetString(_lhvm->Pop().intVal);
}

std::vector<float> Game::PopVarArg(const int32_t argc)
{
	std::vector<float> vals;
	vals.resize(argc);
	for (int i = argc - 1; i >= 0; i--)
	{
		vals[i] = _lhvm->Popf();
	}
	return vals;
}

entt::entity Game::CreateScriptObject(const script::ObjectType type, uint32_t subtype, const glm::vec3& position,
                                      float altitude, float xAngleRadians, float yAngleRadians, const float zAngleRadians,
                                      const float scale)
{
	// TODO(Daniels118): handle all types
	switch (type)
	{
	case script::ObjectType::MobileStatic:
	case script::ObjectType::Rock: // TODO(Daniels118): add a Rock archetype
		return MobileStaticArchetype::Create(position, static_cast<MobileStaticInfo>(subtype), altitude, xAngleRadians,
		                                     yAngleRadians, zAngleRadians, scale);
	default:
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "CreateScriptObject not implemented for type {}", static_cast<int>(type));
	}
	return static_cast<entt::entity>(0);
}

bool Game::Run()
{
	LoadMap(_startMap);
	Locator::dynamicsSystem::value().RegisterRigidBodies();

	auto& fileSystem = Locator::filesystem::value();

	auto challengePath = fileSystem.GetPath<filesystem::Path::Quests>() / "challenge.chl";
	if (fileSystem.Exists(challengePath))
	{
		_lhvm = std::make_unique<LHVM::LHVM>();

		_nativeCallEnterCallback = [this](auto&& PH1) { NativeCallEnterCallback(std::forward<decltype(PH1)>(PH1)); };
		_nativeCallExitCallback = [this](auto&& PH1) { NativeCallExitCallback(std::forward<decltype(PH1)>(PH1)); };
		_stopTaskCallback = [this](auto&& PH1) { StopTaskCallback(std::forward<decltype(PH1)>(PH1)); };
		_lhvmErrorCallback = [this](auto&& PH1, auto&& PH2, auto&& PH3) {
			LhvmErrorCallback(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
			                  std::forward<decltype(PH3)>(PH3));
		};
		_addObjectReference = [this](auto&& PH1) { AddObjectReference(std::forward<decltype(PH1)>(PH1)); };
		_removeObjectReference = [this](auto&& PH1) { RemoveObjectReference(std::forward<decltype(PH1)>(PH1)); };

		_lhvm->Initialise(&_functionsTable, _nativeCallEnterCallback, _nativeCallExitCallback, _stopTaskCallback,
		                  _lhvmErrorCallback, _addObjectReference, _removeObjectReference);

		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading LHVM from: {}", challengePath.generic_string());
#if __ANDROID__
		//  Android has a complicated permissions API, must call java code to read contents.
		_lhvm->LoadBinary(fileSystem.ReadAll(fileSystem.FindPath(challengePath)));
#else
		_lhvm->LoadBinary(fileSystem.FindPath(challengePath));
#endif

		_lhvm->StartScript("LandControlAll", LHVM::ScriptType::ALL);
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
		_renderer->ConfigureView(graphics::RenderPass::Main, size.x, size.y);
	}

	{
		uint16_t width;
		uint16_t height;
		_water->GetFrameBuffer().GetSize(width, height);
		_renderer->ConfigureView(graphics::RenderPass::Reflection, width, height);
	}

	if (_config.drawIsland)
	{
		uint16_t width;
		uint16_t height;
		Locator::terrainSystem::value().GetFootprintFramebuffer().GetSize(width, height);
		_renderer->ConfigureView(graphics::RenderPass::Footprint, width, height, 0x00000000);
	}

	Game::SetTime(_config.timeOfDay);

	_frameCount = 0;
	auto lastTime = std::chrono::high_resolution_clock::now();
	while (Update())
	{
		auto duration = std::chrono::high_resolution_clock::now() - lastTime;
		auto milliseconds = std::chrono::duration_cast<std::chrono::duration<uint32_t, std::milli>>(duration);
		{
			auto section = _profiler->BeginScoped(Profiler::Stage::SceneDraw);

			Renderer::DrawSceneDesc drawDesc {
			    /*profiler =*/*_profiler,
			    /*camera =*/_camera.get(),
			    /*frameBuffer =*/nullptr,
			    /*sky =*/*_sky,
			    /*water =*/*_water,
			    /*entities =*/Locator::entitiesRegistry::value(),
			    /*time =*/milliseconds.count(), // TODO(#481): get actual time
			    /*timeOfDay =*/_config.timeOfDay,
			    /*bumpMapStrength =*/_config.bumpMapStrength,
			    /*smallBumpMapStrength =*/_config.smallBumpMapStrength,
			    /*viewId =*/graphics::RenderPass::Main,
			    /*drawSky =*/_config.drawSky,
			    /*drawWater =*/_config.drawWater,
			    /*drawIsland =*/_config.drawIsland,
			    /*drawEntities =*/_config.drawEntities,
			    /*drawSprites =*/_config.drawSprites,
			    /*drawTestModel =*/_config.drawTestModel,
			    /*drawDebugCross =*/_config.drawDebugCross,
			    /*drawBoundingBoxes =*/_config.drawBoundingBoxes,
			    /*cullBack =*/false,
			    /*bgfxDebug =*/_config.bgfxDebug,
			    /*bgfxProfile =*/_config.bgfxProfile,
			    /*wireframe =*/_config.wireframe,
			};

			_renderer->DrawScene(drawDesc);
		}

		{
			auto section = _profiler->BeginScoped(Profiler::Stage::GuiDraw);
			_gui->Draw();
		}

		{
			auto section = _profiler->BeginScoped(Profiler::Stage::RendererFrame);
			_renderer->Frame();
		}

		if (_requestScreenshot.has_value())
		{
			if (_requestScreenshot->first == _frameCount)
			{
				SPDLOG_LOGGER_INFO(spdlog::get("game"), "Requesting a screenshot at frame {}...", _frameCount);
				_renderer->RequestScreenshot(_requestScreenshot->second);
			}
			if (_requestScreenshot->first <= _frameCount)
			{
				_requestScreenshot = std::nullopt;
			}
		}

		_frameCount++;
	}

	return true;
}

void Game::LoadMap(const std::filesystem::path& path)
{
	auto& fileSystem = Locator::filesystem::value();

	if (!fileSystem.Exists(path))
	{
		throw std::runtime_error("Could not find script " + path.generic_string());
	}

	const auto data = fileSystem.ReadAll(path);
	const auto source = std::string(reinterpret_cast<const char*>(data.data()), data.size());

	// Reset everything. Deletes all entities and their components
	Locator::entitiesRegistry::value().Reset();

	// TODO(#661): split entities that are permanent from map entities and move hand and camera to init
	// We need a hand for the player
	_handEntity = ecs::archetypes::HandArchetype::Create(glm::vec3(0.0f), glm::half_pi<float>(), 0.0f, glm::half_pi<float>(),
	                                                     0.01f, false);

	// create our camera
	const auto aspect = Locator::windowing::has_value() ? Locator::windowing::value().GetAspectRatio() : 1.0f;
	_camera->SetProjectionMatrixPerspective(_config.cameraXFov, aspect, _config.cameraNearClip, _config.cameraFarClip);

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

bool Game::LoadVariables()
{
	InfoFile infoFile;
	return infoFile.LoadFromFile(Locator::filesystem::value().GetPath<filesystem::Path::Scripts>() / "info.dat",
	                             _infoConstants);
}

void Game::SetTime(float time)
{
	GetSky().SetTime(time);
}

void Game::RequestScreenshot(const std::filesystem::path& path)
{
	_requestScreenshot = std::make_pair(_frameCount, path);
}

void Game::InitFunctionsTable()
{
	_functionsTable.reserve(464);
	InitFunctionsTable1();
	InitFunctionsTable2();
}

void Game::InitFunctionsTable1()
{
	CREATE_FUNCTION_BINDING("NONE", 0, 0, VmNone);
	CREATE_FUNCTION_BINDING("SET_CAMERA_POSITION", 3, 0, VmSetCameraPosition);
	CREATE_FUNCTION_BINDING("SET_CAMERA_FOCUS", 3, 0, VmSetCameraFocus);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_POSITION", 4, 0, VmMoveCameraPosition);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_FOCUS", 4, 0, VmMoveCameraFocus);
	CREATE_FUNCTION_BINDING("GET_CAMERA_POSITION", 0, 3, VmGetCameraPosition);
	CREATE_FUNCTION_BINDING("GET_CAMERA_FOCUS", 0, 3, VmGetCameraFocus);
	CREATE_FUNCTION_BINDING("SPIRIT_EJECT", 1, 0, VmSpiritEject);
	CREATE_FUNCTION_BINDING("SPIRIT_HOME", 1, 0, VmSpiritHome);
	CREATE_FUNCTION_BINDING("SPIRIT_POINT_POS", 5, 0, VmSpiritPointPos);
	CREATE_FUNCTION_BINDING("SPIRIT_POINT_GAME_THING", 3, 0, VmSpiritPointGameThing);
	CREATE_FUNCTION_BINDING("GAME_THING_FIELD_OF_VIEW", 1, 1, VmGameThingFieldOfView);
	CREATE_FUNCTION_BINDING("POS_FIELD_OF_VIEW", 3, 1, VmPosFieldOfView);
	CREATE_FUNCTION_BINDING("RUN_TEXT", 3, 0, VmRunText);
	CREATE_FUNCTION_BINDING("TEMP_TEXT", 3, 0, VmTempText);
	CREATE_FUNCTION_BINDING("TEXT_READ", 0, 1, VmTextRead);
	CREATE_FUNCTION_BINDING("GAME_THING_CLICKED", 1, 1, VmGameThingClicked);
	CREATE_FUNCTION_BINDING("SET_SCRIPT_STATE", 2, 0, VmSetScriptState);
	CREATE_FUNCTION_BINDING("SET_SCRIPT_STATE_POS", 4, 0, VmSetScriptStatePos);
	CREATE_FUNCTION_BINDING("SET_SCRIPT_FLOAT", 2, 0, VmSetScriptFloat);
	CREATE_FUNCTION_BINDING("SET_SCRIPT_ULONG", 3, 0, VmSetScriptUlong);
	CREATE_FUNCTION_BINDING("GET_PROPERTY", 2, 1, VmGetProperty);
	CREATE_FUNCTION_BINDING("SET_PROPERTY", 3, 0, VmSetProperty);
	CREATE_FUNCTION_BINDING("GET_POSITION", 1, 3, VmGetPosition);
	CREATE_FUNCTION_BINDING("SET_POSITION", 4, 0, VmSetPosition);
	CREATE_FUNCTION_BINDING("GET_DISTANCE", 6, 1, VmGetDistance);
	CREATE_FUNCTION_BINDING("CALL", 6, 1, VmCall);
	CREATE_FUNCTION_BINDING("CREATE", 5, 1, VmCreate);
	CREATE_FUNCTION_BINDING("RANDOM", 2, 1, VmRandom);
	CREATE_FUNCTION_BINDING("DLL_GETTIME", 0, 1, VmDllGettime);
	CREATE_FUNCTION_BINDING("START_CAMERA_CONTROL", 0, 1, VmStartCameraControl);
	CREATE_FUNCTION_BINDING("END_CAMERA_CONTROL", 0, 0, VmEndCameraControl);
	CREATE_FUNCTION_BINDING("SET_WIDESCREEN", 1, 0, VmSetWidescreen);
	CREATE_FUNCTION_BINDING("MOVE_GAME_THING", 5, 0, VmMoveGameThing);
	CREATE_FUNCTION_BINDING("SET_FOCUS", 4, 0, VmSetFocus);
	CREATE_FUNCTION_BINDING("HAS_CAMERA_ARRIVED", 0, 1, VmHasCameraArrived);
	CREATE_FUNCTION_BINDING("FLOCK_CREATE", 3, 1, VmFlockCreate);
	CREATE_FUNCTION_BINDING("FLOCK_ATTACH", 3, 1, VmFlockAttach);
	CREATE_FUNCTION_BINDING("FLOCK_DETACH", 2, 1, VmFlockDetach);
	CREATE_FUNCTION_BINDING("FLOCK_DISBAND", 1, 0, VmFlockDisband);
	CREATE_FUNCTION_BINDING("ID_SIZE", 1, 1, VmIdSize);
	CREATE_FUNCTION_BINDING("FLOCK_MEMBER", 2, 1, VmFlockMember);
	CREATE_FUNCTION_BINDING("GET_HAND_POSITION", 0, 3, VmGetHandPosition);
	CREATE_FUNCTION_BINDING("PLAY_SOUND_EFFECT", 6, 0, VmPlaySoundEffect);
	CREATE_FUNCTION_BINDING("START_MUSIC", 1, 0, VmStartMusic);
	CREATE_FUNCTION_BINDING("STOP_MUSIC", 0, 0, VmStopMusic);
	CREATE_FUNCTION_BINDING("ATTACH_MUSIC", 2, 0, VmAttachMusic);
	CREATE_FUNCTION_BINDING("DETACH_MUSIC", 1, 0, VmDetachMusic);
	CREATE_FUNCTION_BINDING("OBJECT_DELETE", 2, 0, VmObjectDelete);
	CREATE_FUNCTION_BINDING("FOCUS_FOLLOW", 1, 0, VmFocusFollow);
	CREATE_FUNCTION_BINDING("POSITION_FOLLOW", 1, 0, VmPositionFollow);
	CREATE_FUNCTION_BINDING("CALL_NEAR", 7, 1, VmCallNear);
	CREATE_FUNCTION_BINDING("SPECIAL_EFFECT_POSITION", 5, 1, VmSpecialEffectPosition);
	CREATE_FUNCTION_BINDING("SPECIAL_EFFECT_OBJECT", 3, 1, VmSpecialEffectObject);
	CREATE_FUNCTION_BINDING("DANCE_CREATE", 6, 1, VmDanceCreate);
	CREATE_FUNCTION_BINDING("CALL_IN", 4, 1, VmCallIn);
	CREATE_FUNCTION_BINDING("CHANGE_INNER_OUTER_PROPERTIES", 4, 0, VmChangeInnerOuterProperties);
	CREATE_FUNCTION_BINDING("SNAPSHOT", -1, 0, VmSnapshot);
	CREATE_FUNCTION_BINDING("GET_ALIGNMENT", 1, 1, VmGetAlignment);
	CREATE_FUNCTION_BINDING("SET_ALIGNMENT", 2, 0, VmSetAlignment);
	CREATE_FUNCTION_BINDING("INFLUENCE_OBJECT", 4, 1, VmInfluenceObject);
	CREATE_FUNCTION_BINDING("INFLUENCE_POSITION", 6, 1, VmInfluencePosition);
	CREATE_FUNCTION_BINDING("GET_INFLUENCE", 5, 1, VmGetInfluence);
	CREATE_FUNCTION_BINDING("SET_INTERFACE_INTERACTION", 1, 0, VmSetInterfaceInteraction);
	CREATE_FUNCTION_BINDING("PLAYED", 1, 1, VmPlayed);
	CREATE_FUNCTION_BINDING("RANDOM_ULONG", 2, 1, VmRandomUlong);
	CREATE_FUNCTION_BINDING("SET_GAMESPEED", 1, 0, VmSetGamespeed);
	CREATE_FUNCTION_BINDING("CALL_IN_NEAR", 8, 1, VmCallInNear);
	CREATE_FUNCTION_BINDING("OVERRIDE_STATE_ANIMATION", 2, 0, VmOverrideStateAnimation);
	CREATE_FUNCTION_BINDING("CREATURE_CREATE_RELATIVE_TO_CREATURE", 6, 1, VmCreatureCreateRelativeToCreature);
	CREATE_FUNCTION_BINDING("CREATURE_LEARN_EVERYTHING", 1, 0, VmCreatureLearnEverything);
	CREATE_FUNCTION_BINDING("CREATURE_SET_KNOWS_ACTION", 4, 0, VmCreatureSetKnowsAction);
	CREATE_FUNCTION_BINDING("CREATURE_SET_AGENDA_PRIORITY", 2, 0, VmCreatureSetAgendaPriority);
	CREATE_FUNCTION_BINDING("CREATURE_TURN_OFF_ALL_DESIRES", 1, 0, VmCreatureTurnOffAllDesires);
	CREATE_FUNCTION_BINDING("CREATURE_LEARN_DISTINCTION_ABOUT_ACTIVITY_OBJECT", 4, 0,
	                        VmCreatureLearnDistinctionAboutActivityObject);
	CREATE_FUNCTION_BINDING("CREATURE_DO_ACTION", 4, 0, VmCreatureDoAction);
	CREATE_FUNCTION_BINDING("IN_CREATURE_HAND", 2, 1, VmInCreatureHand);
	CREATE_FUNCTION_BINDING("CREATURE_SET_DESIRE_VALUE", 3, 0, VmCreatureSetDesireValue);
	CREATE_FUNCTION_BINDING("CREATURE_SET_DESIRE_ACTIVATED", 3, 0, VmCreatureSetDesireActivated78);
	CREATE_FUNCTION_BINDING("CREATURE_SET_DESIRE_ACTIVATED", 2, 0, VmCreatureSetDesireActivated79);
	CREATE_FUNCTION_BINDING("CREATURE_SET_DESIRE_MAXIMUM", 3, 0, VmCreatureSetDesireMaximum);
	CREATE_FUNCTION_BINDING("CONVERT_CAMERA_POSITION", 1, 3, VmConvertCameraPosition);
	CREATE_FUNCTION_BINDING("CONVERT_CAMERA_FOCUS", 1, 3, VmConvertCameraFocus);
	CREATE_FUNCTION_BINDING("CREATURE_SET_PLAYER", 1, 0, VmCreatureSetPlayer);
	CREATE_FUNCTION_BINDING("START_COUNTDOWN_TIMER", 1, 0, VmStartCountdownTimer);
	CREATE_FUNCTION_BINDING("CREATURE_INITIALISE_NUM_TIMES_PERFORMED_ACTION", 2, 0,
	                        VmCreatureInitialiseNumTimesPerformedAction);
	CREATE_FUNCTION_BINDING("CREATURE_GET_NUM_TIMES_ACTION_PERFORMED", 2, 1, VmCreatureGetNumTimesActionPerformed);
	CREATE_FUNCTION_BINDING("REMOVE_COUNTDOWN_TIMER", 0, 0, VmRemoveCountdownTimer);
	CREATE_FUNCTION_BINDING("GET_OBJECT_DROPPED", 1, 1, VmGetObjectDropped);
	CREATE_FUNCTION_BINDING("CLEAR_DROPPED_BY_OBJECT", 1, 0, VmClearDroppedByObject);
	CREATE_FUNCTION_BINDING("CREATE_REACTION", 2, 0, VmCreateReaction);
	CREATE_FUNCTION_BINDING("REMOVE_REACTION", 1, 0, VmRemoveReaction);
	CREATE_FUNCTION_BINDING("GET_COUNTDOWN_TIMER", 0, 1, VmGetCountdownTimer);
	CREATE_FUNCTION_BINDING("START_DUAL_CAMERA", 2, 0, VmStartDualCamera);
	CREATE_FUNCTION_BINDING("UPDATE_DUAL_CAMERA", 2, 0, VmUpdateDualCamera);
	CREATE_FUNCTION_BINDING("RELEASE_DUAL_CAMERA", 0, 0, VmReleaseDualCamera);
	CREATE_FUNCTION_BINDING("SET_CREATURE_HELP", 1, 0, VmSetCreatureHelp);
	CREATE_FUNCTION_BINDING("GET_TARGET_OBJECT", 1, 1, VmGetTargetObject);
	CREATE_FUNCTION_BINDING("CREATURE_DESIRE_IS", 2, 1, VmCreatureDesireIs);
	CREATE_FUNCTION_BINDING("COUNTDOWN_TIMER_EXISTS", 0, 1, VmCountdownTimerExists);
	CREATE_FUNCTION_BINDING("LOOK_GAME_THING", 2, 0, VmLookGameThing);
	CREATE_FUNCTION_BINDING("GET_OBJECT_DESTINATION", 1, 3, VmGetObjectDestination);
	CREATE_FUNCTION_BINDING("CREATURE_FORCE_FINISH", 1, 0, VmCreatureForceFinish);
	CREATE_FUNCTION_BINDING("HIDE_COUNTDOWN_TIMER", 0, 0, VmHideCountdownTimer);
	CREATE_FUNCTION_BINDING("GET_ACTION_TEXT_FOR_OBJECT", 1, 1, VmGetActionTextForObject);
	CREATE_FUNCTION_BINDING("CREATE_DUAL_CAMERA_WITH_POINT", 4, 0, VmCreateDualCameraWithPoint);
	CREATE_FUNCTION_BINDING("SET_CAMERA_TO_FACE_OBJECT", 2, 0, VmSetCameraToFaceObject);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_TO_FACE_OBJECT", 3, 0, VmMoveCameraToFaceObject);
	CREATE_FUNCTION_BINDING("GET_MOON_PERCENTAGE", 0, 1, VmGetMoonPercentage);
	CREATE_FUNCTION_BINDING("POPULATE_CONTAINER", 4, 0, VmPopulateContainer);
	CREATE_FUNCTION_BINDING("ADD_REFERENCE", 1, 1, VmAddReference);
	CREATE_FUNCTION_BINDING("REMOVE_REFERENCE", 1, 1, VmRemoveReference);
	CREATE_FUNCTION_BINDING("SET_GAME_TIME", 1, 0, VmSetGameTime);
	CREATE_FUNCTION_BINDING("GET_GAME_TIME", 0, 1, VmGetGameTime);
	CREATE_FUNCTION_BINDING("GET_REAL_TIME", 0, 1, VmGetRealTime);
	CREATE_FUNCTION_BINDING("GET_REAL_DAY", 0, 1, VmGetRealDay115);
	CREATE_FUNCTION_BINDING("GET_REAL_DAY", 0, 1, VmGetRealDay116);
	CREATE_FUNCTION_BINDING("GET_REAL_MONTH", 0, 1, VmGetRealMonth);
	CREATE_FUNCTION_BINDING("GET_REAL_YEAR", 0, 1, VmGetRealYear);
	CREATE_FUNCTION_BINDING("RUN_CAMERA_PATH", 1, 0, VmRunCameraPath);
	CREATE_FUNCTION_BINDING("START_DIALOGUE", 0, 1, VmStartDialogue);
	CREATE_FUNCTION_BINDING("END_DIALOGUE", 0, 0, VmEndDialogue);
	CREATE_FUNCTION_BINDING("IS_DIALOGUE_READY", 0, 1, VmIsDialogueReady);
	CREATE_FUNCTION_BINDING("CHANGE_WEATHER_PROPERTIES", 6, 0, VmChangeWeatherProperties);
	CREATE_FUNCTION_BINDING("CHANGE_LIGHTNING_PROPERTIES", 5, 0, VmChangeLightningProperties);
	CREATE_FUNCTION_BINDING("CHANGE_TIME_FADE_PROPERTIES", 3, 0, VmChangeTimeFadeProperties);
	CREATE_FUNCTION_BINDING("CHANGE_CLOUD_PROPERTIES", 4, 0, VmChangeCloudProperties);
	CREATE_FUNCTION_BINDING("SET_HEADING_AND_SPEED", 5, 0, VmSetHeadingAndSpeed);
	CREATE_FUNCTION_BINDING("START_GAME_SPEED", 0, 0, VmStartGameSpeed);
	CREATE_FUNCTION_BINDING("END_GAME_SPEED", 0, 0, VmEndGameSpeed);
	CREATE_FUNCTION_BINDING("BUILD_BUILDING", 4, 0, VmBuildBuilding);
	CREATE_FUNCTION_BINDING("SET_AFFECTED_BY_WIND", 2, 0, VmSetAffectedByWind);
	CREATE_FUNCTION_BINDING("WIDESCREEN_TRANSISTION_FINISHED", 0, 1, VmWidescreenTransistionFinished);
	CREATE_FUNCTION_BINDING("GET_RESOURCE", 2, 1, VmGetResource);
	CREATE_FUNCTION_BINDING("ADD_RESOURCE", 3, 1, VmAddResource);
	CREATE_FUNCTION_BINDING("REMOVE_RESOURCE", 3, 1, VmRemoveResource);
	CREATE_FUNCTION_BINDING("GET_TARGET_RELATIVE_POS", 8, 3, VmGetTargetRelativePos);
	CREATE_FUNCTION_BINDING("STOP_POINTING", 1, 0, VmStopPointing);
	CREATE_FUNCTION_BINDING("STOP_LOOKING", 1, 0, VmStopLooking);
	CREATE_FUNCTION_BINDING("LOOK_AT_POSITION", 4, 0, VmLookAtPosition);
	CREATE_FUNCTION_BINDING("PLAY_SPIRIT_ANIM", 5, 0, VmPlaySpiritAnim);
	CREATE_FUNCTION_BINDING("CALL_IN_NOT_NEAR", 8, 1, VmCallInNotNear);
	CREATE_FUNCTION_BINDING("SET_CAMERA_ZONE", 1, 0, VmSetCameraZone);
	CREATE_FUNCTION_BINDING("GET_OBJECT_STATE", 1, 1, VmGetObjectState);
	CREATE_FUNCTION_BINDING("REVEAL_COUNTDOWN_TIMER", 0, 0, VmRevealCountdownTimer);
	CREATE_FUNCTION_BINDING("SET_TIMER_TIME", 2, 0, VmSetTimerTime);
	CREATE_FUNCTION_BINDING("CREATE_TIMER", 1, 1, VmCreateTimer);
	CREATE_FUNCTION_BINDING("GET_TIMER_TIME_REMAINING", 1, 1, VmGetTimerTimeRemaining);
	CREATE_FUNCTION_BINDING("GET_TIMER_TIME_SINCE_SET", 1, 1, VmGetTimerTimeSinceSet);
	CREATE_FUNCTION_BINDING("MOVE_MUSIC", 2, 0, VmMoveMusic);
	CREATE_FUNCTION_BINDING("GET_INCLUSION_DISTANCE", 0, 1, VmGetInclusionDistance);
	CREATE_FUNCTION_BINDING("GET_LAND_HEIGHT", 3, 1, VmGetLandHeight);
	CREATE_FUNCTION_BINDING("LOAD_MAP", 1, 0, VmLoadMap);
	CREATE_FUNCTION_BINDING("STOP_ALL_SCRIPTS_EXCLUDING", 1, 0, VmStopAllScriptsExcluding);
	CREATE_FUNCTION_BINDING("STOP_ALL_SCRIPTS_IN_FILES_EXCLUDING", 1, 0, VmStopAllScriptsInFilesExcluding);
	CREATE_FUNCTION_BINDING("STOP_SCRIPT", 1, 0, VmStopScript);
	CREATE_FUNCTION_BINDING("CLEAR_CLICKED_OBJECT", 0, 0, VmClearClickedObject);
	CREATE_FUNCTION_BINDING("CLEAR_CLICKED_POSITION", 0, 0, VmClearClickedPosition);
	CREATE_FUNCTION_BINDING("POSITION_CLICKED", 4, 1, VmPositionClicked);
	CREATE_FUNCTION_BINDING("RELEASE_FROM_SCRIPT", 1, 0, VmReleaseFromScript);
	CREATE_FUNCTION_BINDING("GET_OBJECT_HAND_IS_OVER", 0, 1, VmGetObjectHandIsOver);
	CREATE_FUNCTION_BINDING("ID_POISONED_SIZE", 1, 1, VmIdPoisonedSize);
	CREATE_FUNCTION_BINDING("IS_POISONED", 1, 1, VmIsPoisoned);
	CREATE_FUNCTION_BINDING("CALL_POISONED_IN", 4, 1, VmCallPoisonedIn);
	CREATE_FUNCTION_BINDING("CALL_NOT_POISONED_IN", 4, 1, VmCallNotPoisonedIn);
	CREATE_FUNCTION_BINDING("SPIRIT_PLAYED", 1, 1, VmSpiritPlayed);
	CREATE_FUNCTION_BINDING("CLING_SPIRIT", 3, 0, VmClingSpirit);
	CREATE_FUNCTION_BINDING("FLY_SPIRIT", 3, 0, VmFlySpirit);
	CREATE_FUNCTION_BINDING("SET_ID_MOVEABLE", 2, 0, VmSetIdMoveable);
	CREATE_FUNCTION_BINDING("SET_ID_PICKUPABLE", 2, 0, VmSetIdPickupable);
	CREATE_FUNCTION_BINDING("IS_ON_FIRE", 1, 1, VmIsOnFire);
	CREATE_FUNCTION_BINDING("IS_FIRE_NEAR", 4, 1, VmIsFireNear);
	CREATE_FUNCTION_BINDING("STOP_SCRIPTS_IN_FILES", 1, 0, VmStopScriptsInFiles);
	CREATE_FUNCTION_BINDING("SET_POISONED", 2, 0, VmSetPoisoned);
	CREATE_FUNCTION_BINDING("SET_TEMPERATURE", 2, 0, VmSetTemperature);
	CREATE_FUNCTION_BINDING("SET_ON_FIRE", 3, 0, VmSetOnFire);
	CREATE_FUNCTION_BINDING("SET_TARGET", 5, 0, VmSetTarget);
	CREATE_FUNCTION_BINDING("WALK_PATH", 5, 0, VmWalkPath);
	CREATE_FUNCTION_BINDING("FOCUS_AND_POSITION_FOLLOW", 2, 0, VmFocusAndPositionFollow);
	CREATE_FUNCTION_BINDING("GET_WALK_PATH_PERCENTAGE", 1, 1, VmGetWalkPathPercentage);
	CREATE_FUNCTION_BINDING("CAMERA_PROPERTIES", 4, 0, VmCameraProperties);
	CREATE_FUNCTION_BINDING("ENABLE_DISABLE_MUSIC", 2, 0, VmEnableDisableMusic);
	CREATE_FUNCTION_BINDING("GET_MUSIC_OBJ_DISTANCE", 1, 1, VmGetMusicObjDistance);
	CREATE_FUNCTION_BINDING("GET_MUSIC_ENUM_DISTANCE", 1, 1, VmGetMusicEnumDistance);
	CREATE_FUNCTION_BINDING("SET_MUSIC_PLAY_POSITION", 4, 0, VmSetMusicPlayPosition);
	CREATE_FUNCTION_BINDING("ATTACH_OBJECT_LEASH_TO_OBJECT", 2, 0, VmAttachObjectLeashToObject);
	CREATE_FUNCTION_BINDING("ATTACH_OBJECT_LEASH_TO_HAND", 1, 0, VmAttachObjectLeashToHand);
	CREATE_FUNCTION_BINDING("DETACH_OBJECT_LEASH", 1, 0, VmDetachObjectLeash);
	CREATE_FUNCTION_BINDING("SET_CREATURE_ONLY_DESIRE", 3, 0, VmSetCreatureOnlyDesire);
	CREATE_FUNCTION_BINDING("SET_CREATURE_ONLY_DESIRE_OFF", 1, 0, VmSetCreatureOnlyDesireOff);
	CREATE_FUNCTION_BINDING("RESTART_MUSIC", 1, 0, VmRestartMusic);
	CREATE_FUNCTION_BINDING("MUSIC_PLAYED", 1, 1, VmMusicPlayed191);
	CREATE_FUNCTION_BINDING("IS_OF_TYPE", 3, 1, VmIsOfType);
	CREATE_FUNCTION_BINDING("CLEAR_HIT_OBJECT", 0, 0, VmClearHitObject);
	CREATE_FUNCTION_BINDING("GAME_THING_HIT", 1, 1, VmGameThingHit);
	CREATE_FUNCTION_BINDING("SPELL_AT_THING", 8, 1, VmSpellAtThing);
	CREATE_FUNCTION_BINDING("SPELL_AT_POS", 10, 1, VmSpellAtPos);
	CREATE_FUNCTION_BINDING("CALL_PLAYER_CREATURE", 1, 1, VmCallPlayerCreature);
	CREATE_FUNCTION_BINDING("GET_SLOWEST_SPEED", 1, 1, VmGetSlowestSpeed);
	CREATE_FUNCTION_BINDING("GET_OBJECT_HELD", 0, 1, VmGetObjectHeld199);
	CREATE_FUNCTION_BINDING("HELP_SYSTEM_ON", 0, 1, VmHelpSystemOn);
	CREATE_FUNCTION_BINDING("SHAKE_CAMERA", 6, 0, VmShakeCamera);
	CREATE_FUNCTION_BINDING("SET_ANIMATION_MODIFY", 2, 0, VmSetAnimationModify);
	CREATE_FUNCTION_BINDING("SET_AVI_SEQUENCE", 2, 0, VmSetAviSequence);
	CREATE_FUNCTION_BINDING("PLAY_GESTURE", 5, 0, VmPlayGesture);
	CREATE_FUNCTION_BINDING("DEV_FUNCTION", 1, 0, VmDevFunction);
	CREATE_FUNCTION_BINDING("HAS_MOUSE_WHEEL", 0, 1, VmHasMouseWheel);
	CREATE_FUNCTION_BINDING("NUM_MOUSE_BUTTONS", 0, 1, VmNumMouseButtons);
	CREATE_FUNCTION_BINDING("SET_CREATURE_DEV_STAGE", 2, 0, VmSetCreatureDevStage);
	CREATE_FUNCTION_BINDING("SET_FIXED_CAM_ROTATION", 4, 0, VmSetFixedCamRotation);
	CREATE_FUNCTION_BINDING("SWAP_CREATURE", 2, 0, VmSwapCreature);
	CREATE_FUNCTION_BINDING("GET_ARENA", 5, 1, VmGetArena);
	CREATE_FUNCTION_BINDING("GET_FOOTBALL_PITCH", 1, 1, VmGetFootballPitch);
	CREATE_FUNCTION_BINDING("STOP_ALL_GAMES", 1, 0, VmStopAllGames);
	CREATE_FUNCTION_BINDING("ATTACH_TO_GAME", 3, 0, VmAttachToGame);
	CREATE_FUNCTION_BINDING("DETACH_FROM_GAME", 3, 0, VmDetachFromGame);
	CREATE_FUNCTION_BINDING("DETACH_UNDEFINED_FROM_GAME", 2, 0, VmDetachUndefinedFromGame);
	CREATE_FUNCTION_BINDING("SET_ONLY_FOR_SCRIPTS", 2, 0, VmSetOnlyForScripts);
	CREATE_FUNCTION_BINDING("START_MATCH_WITH_REFEREE", 2, 0, VmStartMatchWithReferee);
	CREATE_FUNCTION_BINDING("GAME_TEAM_SIZE", 2, 0, VmGameTeamSize);
	CREATE_FUNCTION_BINDING("GAME_TYPE", 1, 1, VmGameType);
	CREATE_FUNCTION_BINDING("GAME_SUB_TYPE", 1, 1, VmGameSubType);
	CREATE_FUNCTION_BINDING("IS_LEASHED", 1, 1, VmIsLeashed);
	CREATE_FUNCTION_BINDING("SET_CREATURE_HOME", 4, 0, VmSetCreatureHome);
	CREATE_FUNCTION_BINDING("GET_HIT_OBJECT", 0, 1, VmGetHitObject);
	CREATE_FUNCTION_BINDING("GET_OBJECT_WHICH_HIT", 0, 1, VmGetObjectWhichHit);
	CREATE_FUNCTION_BINDING("GET_NEAREST_TOWN_OF_PLAYER", 5, 1, VmGetNearestTownOfPlayer);
	CREATE_FUNCTION_BINDING("SPELL_AT_POINT", 5, 1, VmSpellAtPoint);
	CREATE_FUNCTION_BINDING("SET_ATTACK_OWN_TOWN", 2, 0, VmSetAttackOwnTown);
	CREATE_FUNCTION_BINDING("IS_FIGHTING", 1, 1, VmIsFighting);
	CREATE_FUNCTION_BINDING("SET_MAGIC_RADIUS", 2, 0, VmSetMagicRadius);
	CREATE_FUNCTION_BINDING("TEMP_TEXT_WITH_NUMBER", 4, 0, VmTempTextWithNumber);
	CREATE_FUNCTION_BINDING("RUN_TEXT_WITH_NUMBER", 4, 0, VmRunTextWithNumber);
	CREATE_FUNCTION_BINDING("CREATURE_SPELL_REVERSION", 2, 0, VmCreatureSpellReversion);
	CREATE_FUNCTION_BINDING("GET_DESIRE", 2, 1, VmGetDesire);
	CREATE_FUNCTION_BINDING("GET_EVENTS_PER_SECOND", 1, 1, VmGetEventsPerSecond);
	CREATE_FUNCTION_BINDING("GET_TIME_SINCE", 1, 1, VmGetTimeSince);
	CREATE_FUNCTION_BINDING("GET_TOTAL_EVENTS", 1, 1, VmGetTotalEvents);
	CREATE_FUNCTION_BINDING("UPDATE_SNAPSHOT", -1, 0, VmUpdateSnapshot);
	CREATE_FUNCTION_BINDING("CREATE_REWARD", 5, 1, VmCreateReward);
	CREATE_FUNCTION_BINDING("CREATE_REWARD_IN_TOWN", 6, 1, VmCreateRewardInTown);
	CREATE_FUNCTION_BINDING("SET_FADE", 4, 0, VmSetFade);
	CREATE_FUNCTION_BINDING("SET_FADE_IN", 1, 0, VmSetFadeIn);
	CREATE_FUNCTION_BINDING("FADE_FINISHED", 0, 1, VmFadeFinished);
	CREATE_FUNCTION_BINDING("SET_PLAYER_MAGIC", 3, 0, VmSetPlayerMagic);
	CREATE_FUNCTION_BINDING("HAS_PLAYER_MAGIC", 2, 1, VmHasPlayerMagic);
	CREATE_FUNCTION_BINDING("SPIRIT_SPEAKS", 2, 1, VmSpiritSpeaks);
	CREATE_FUNCTION_BINDING("BELIEF_FOR_PLAYER", 2, 1, VmBeliefForPlayer);
	CREATE_FUNCTION_BINDING("GET_HELP", 1, 1, VmGetHelp);
	CREATE_FUNCTION_BINDING("SET_LEASH_WORKS", 2, 0, VmSetLeashWorks);
	CREATE_FUNCTION_BINDING("LOAD_MY_CREATURE", 3, 0, VmLoadMyCreature);
	CREATE_FUNCTION_BINDING("OBJECT_RELATIVE_BELIEF", 3, 0, VmObjectRelativeBelief);
	CREATE_FUNCTION_BINDING("CREATE_WITH_ANGLE_AND_SCALE", 7, 1, VmCreateWithAngleAndScale);
	CREATE_FUNCTION_BINDING("SET_HELP_SYSTEM", 1, 0, VmSetHelpSystem);
	CREATE_FUNCTION_BINDING("SET_VIRTUAL_INFLUENCE", 2, 0, VmSetVirtualInfluence);
	CREATE_FUNCTION_BINDING("SET_ACTIVE", 2, 0, VmSetActive);
	CREATE_FUNCTION_BINDING("THING_VALID", 1, 1, VmThingValid);
	CREATE_FUNCTION_BINDING("VORTEX_FADE_OUT", 1, 0, VmVortexFadeOut);
	CREATE_FUNCTION_BINDING("REMOVE_REACTION_OF_TYPE", 2, 0, VmRemoveReactionOfType);
	CREATE_FUNCTION_BINDING("CREATURE_LEARN_EVERYTHING_EXCLUDING", 2, 0, VmCreatureLearnEverythingExcluding);
	CREATE_FUNCTION_BINDING("PLAYED_PERCENTAGE", 1, 1, VmPlayedPercentage);
	CREATE_FUNCTION_BINDING("OBJECT_CAST_BY_OBJECT", 2, 1, VmObjectCastByObject);
	CREATE_FUNCTION_BINDING("IS_WIND_MAGIC_AT_POS", 1, 1, VmIsWindMagicAtPos);
	CREATE_FUNCTION_BINDING("CREATE_MIST", 9, 1, VmCreateMist);
	CREATE_FUNCTION_BINDING("SET_MIST_FADE", 6, 0, VmSetMistFade);
	CREATE_FUNCTION_BINDING("GET_OBJECT_FADE", 1, 1, VmGetObjectFade);
	CREATE_FUNCTION_BINDING("PLAY_HAND_DEMO", 3, 0, VmPlayHandDemo);
	CREATE_FUNCTION_BINDING("IS_PLAYING_HAND_DEMO", 0, 1, VmIsPlayingHandDemo);
	CREATE_FUNCTION_BINDING("GET_ARSE_POSITION", 1, 3, VmGetArsePosition);
	CREATE_FUNCTION_BINDING("IS_LEASHED_TO_OBJECT", 2, 1, VmIsLeashedToObject);
	CREATE_FUNCTION_BINDING("GET_INTERACTION_MAGNITUDE", 1, 1, VmGetInteractionMagnitude);
	CREATE_FUNCTION_BINDING("IS_CREATURE_AVAILABLE", 1, 1, VmIsCreatureAvailable);
	CREATE_FUNCTION_BINDING("CREATE_HIGHLIGHT", 5, 1, VmCreateHighlight);
	CREATE_FUNCTION_BINDING("GET_OBJECT_HELD", 1, 1, VmGetObjectHeld273);
	CREATE_FUNCTION_BINDING("GET_ACTION_COUNT", 2, 1, VmGetActionCount);
	CREATE_FUNCTION_BINDING("GET_OBJECT_LEASH_TYPE", 1, 1, VmGetObjectLeashType);
	CREATE_FUNCTION_BINDING("SET_FOCUS_FOLLOW", 1, 0, VmSetFocusFollow);
	CREATE_FUNCTION_BINDING("SET_POSITION_FOLLOW", 1, 0, VmSetPositionFollow);
	CREATE_FUNCTION_BINDING("SET_FOCUS_AND_POSITION_FOLLOW", 2, 0, VmSetFocusAndPositionFollow);
	CREATE_FUNCTION_BINDING("SET_CAMERA_LENS", 1, 0, VmSetCameraLens);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_LENS", 2, 0, VmMoveCameraLens);
	CREATE_FUNCTION_BINDING("CREATURE_REACTION", 2, 0, VmCreatureReaction);
	CREATE_FUNCTION_BINDING("CREATURE_IN_DEV_SCRIPT", 2, 0, VmCreatureInDevScript);
	CREATE_FUNCTION_BINDING("STORE_CAMERA_DETAILS", 0, 0, VmStoreCameraDetails);
	CREATE_FUNCTION_BINDING("RESTORE_CAMERA_DETAILS", 0, 0, VmRestoreCameraDetails);
	CREATE_FUNCTION_BINDING("START_ANGLE_SOUND", 1, 0, VmStartAngleSound285);
	CREATE_FUNCTION_BINDING("SET_CAMERA_POS_FOC_LENS", 7, 0, VmSetCameraPosFocLens);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_POS_FOC_LENS", 8, 0, VmMoveCameraPosFocLens);
	CREATE_FUNCTION_BINDING("GAME_TIME_ON_OFF", 1, 0, VmGameTimeOnOff);
	CREATE_FUNCTION_BINDING("MOVE_GAME_TIME", 2, 0, VmMoveGameTime);
	CREATE_FUNCTION_BINDING("SET_HIGH_GRAPHICS_DETAIL", 2, 0, VmSetHighGraphicsDetail);
	CREATE_FUNCTION_BINDING("SET_SKELETON", 2, 0, VmSetSkeleton);
	CREATE_FUNCTION_BINDING("IS_SKELETON", 1, 1, VmIsSkeleton);
	CREATE_FUNCTION_BINDING("PLAYER_SPELL_CAST_TIME", 1, 1, VmPlayerSpellCastTime);
	CREATE_FUNCTION_BINDING("PLAYER_SPELL_LAST_CAST", 1, 1, VmPlayerSpellLastCast);
	CREATE_FUNCTION_BINDING("GET_LAST_SPELL_CAST_POS", 1, 3, VmGetLastSpellCastPos);
	CREATE_FUNCTION_BINDING("ADD_SPOT_VISUAL_TARGET_POS", 4, 0, VmAddSpotVisualTargetPos);
	CREATE_FUNCTION_BINDING("ADD_SPOT_VISUAL_TARGET_OBJECT", 2, 0, VmAddSpotVisualTargetObject);
	CREATE_FUNCTION_BINDING("SET_INDESTRUCTABLE", 2, 0, VmSetIndestructable);
	CREATE_FUNCTION_BINDING("SET_GRAPHICS_CLIPPING", 2, 0, VmSetGraphicsClipping);
}

void Game::InitFunctionsTable2()
{
	CREATE_FUNCTION_BINDING("SPIRIT_APPEAR", 1, 0, VmSpiritAppear);
	CREATE_FUNCTION_BINDING("SPIRIT_DISAPPEAR", 1, 0, VmSpiritDisappear);
	CREATE_FUNCTION_BINDING("SET_FOCUS_ON_OBJECT", 2, 0, VmSetFocusOnObject);
	CREATE_FUNCTION_BINDING("RELEASE_OBJECT_FOCUS", 1, 0, VmReleaseObjectFocus);
	CREATE_FUNCTION_BINDING("IMMERSION_EXISTS", 0, 1, VmImmersionExists);
	CREATE_FUNCTION_BINDING("SET_DRAW_LEASH", 1, 0, VmSetDrawLeash);
	CREATE_FUNCTION_BINDING("SET_DRAW_HIGHLIGHT", 1, 0, VmSetDrawHighlight);
	CREATE_FUNCTION_BINDING("SET_OPEN_CLOSE", 2, 0, VmSetOpenClose);
	CREATE_FUNCTION_BINDING("SET_INTRO_BUILDING", 1, 0, VmSetIntroBuilding);
	CREATE_FUNCTION_BINDING("CREATURE_FORCE_FRIENDS", 3, 0, VmCreatureForceFriends);
	CREATE_FUNCTION_BINDING("MOVE_COMPUTER_PLAYER_POSITION", 6, 0, VmMoveComputerPlayerPosition);
	CREATE_FUNCTION_BINDING("ENABLE_DISABLE_COMPUTER_PLAYER", 2, 0, VmEnableDisableComputerPlayer311);
	CREATE_FUNCTION_BINDING("GET_COMPUTER_PLAYER_POSITION", 1, 3, VmGetComputerPlayerPosition);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_POSITION", 5, 0, VmSetComputerPlayerPosition);
	CREATE_FUNCTION_BINDING("GET_STORED_CAMERA_POSITION", 0, 3, VmGetStoredCameraPosition);
	CREATE_FUNCTION_BINDING("GET_STORED_CAMERA_FOCUS", 0, 3, VmGetStoredCameraFocus);
	CREATE_FUNCTION_BINDING("CALL_NEAR_IN_STATE", 8, 1, VmCallNearInState);
	CREATE_FUNCTION_BINDING("SET_CREATURE_SOUND", 1, 0, VmSetCreatureSound);
	CREATE_FUNCTION_BINDING("CREATURE_INTERACTING_WITH", 2, 1, VmCreatureInteractingWith);
	CREATE_FUNCTION_BINDING("SET_SUN_DRAW", 1, 0, VmSetSunDraw);
	CREATE_FUNCTION_BINDING("OBJECT_INFO_BITS", 1, 1, VmObjectInfoBits);
	CREATE_FUNCTION_BINDING("SET_HURT_BY_FIRE", 2, 0, VmSetHurtByFire);
	CREATE_FUNCTION_BINDING("CONFINED_OBJECT", 5, 0, VmConfinedObject);
	CREATE_FUNCTION_BINDING("CLEAR_CONFINED_OBJECT", 1, 0, VmClearConfinedObject);
	CREATE_FUNCTION_BINDING("GET_OBJECT_FLOCK", 1, 1, VmGetObjectFlock);
	CREATE_FUNCTION_BINDING("SET_PLAYER_BELIEF", 3, 0, VmSetPlayerBelief);
	CREATE_FUNCTION_BINDING("PLAY_JC_SPECIAL", 1, 0, VmPlayJcSpecial);
	CREATE_FUNCTION_BINDING("IS_PLAYING_JC_SPECIAL", 1, 1, VmIsPlayingJcSpecial);
	CREATE_FUNCTION_BINDING("VORTEX_PARAMETERS", 8, 0, VmVortexParameters);
	CREATE_FUNCTION_BINDING("LOAD_CREATURE", 6, 0, VmLoadCreature);
	CREATE_FUNCTION_BINDING("IS_SPELL_CHARGING", 1, 1, VmIsSpellCharging);
	CREATE_FUNCTION_BINDING("IS_THAT_SPELL_CHARGING", 2, 1, VmIsThatSpellCharging);
	CREATE_FUNCTION_BINDING("OPPOSING_CREATURE", 1, 1, VmOpposingCreature);
	CREATE_FUNCTION_BINDING("FLOCK_WITHIN_LIMITS", 1, 1, VmFlockWithinLimits);
	CREATE_FUNCTION_BINDING("HIGHLIGHT_PROPERTIES", 3, 0, VmHighlightProperties);
	CREATE_FUNCTION_BINDING("LAST_MUSIC_LINE", 1, 1, VmLastMusicLine);
	CREATE_FUNCTION_BINDING("HAND_DEMO_TRIGGER", 0, 1, VmHandDemoTrigger);
	CREATE_FUNCTION_BINDING("GET_BELLY_POSITION", 1, 3, VmGetBellyPosition);
	CREATE_FUNCTION_BINDING("SET_CREATURE_CREED_PROPERTIES", 5, 0, VmSetCreatureCreedProperties);
	CREATE_FUNCTION_BINDING("GAME_THING_CAN_VIEW_CAMERA", 2, 1, VmGameThingCanViewCamera);
	CREATE_FUNCTION_BINDING("GAME_PLAY_SAY_SOUND_EFFECT", 6, 0, VmGamePlaySaySoundEffect);
	CREATE_FUNCTION_BINDING("SET_TOWN_DESIRE_BOOST", 3, 0, VmSetTownDesireBoost);
	CREATE_FUNCTION_BINDING("IS_LOCKED_INTERACTION", 1, 1, VmIsLockedInteraction);
	CREATE_FUNCTION_BINDING("SET_CREATURE_NAME", 2, 0, VmSetCreatureName);
	CREATE_FUNCTION_BINDING("COMPUTER_PLAYER_READY", 1, 1, VmComputerPlayerReady);
	CREATE_FUNCTION_BINDING("ENABLE_DISABLE_COMPUTER_PLAYER", 2, 0, VmEnableDisableComputerPlayer345);
	CREATE_FUNCTION_BINDING("CLEAR_ACTOR_MIND", 1, 0, VmClearActorMind);
	CREATE_FUNCTION_BINDING("ENTER_EXIT_CITADEL", 1, 0, VmEnterExitCitadel);
	CREATE_FUNCTION_BINDING("START_ANGLE_SOUND", 1, 0, VmStartAngleSound348);
	CREATE_FUNCTION_BINDING("THING_JC_SPECIAL", 3, 0, VmThingJcSpecial);
	CREATE_FUNCTION_BINDING("MUSIC_PLAYED", 1, 1, VmMusicPlayed350);
	CREATE_FUNCTION_BINDING("UPDATE_SNAPSHOT_PICTURE", 11, 0, VmUpdateSnapshotPicture);
	CREATE_FUNCTION_BINDING("STOP_SCRIPTS_IN_FILES_EXCLUDING", 2, 0, VmStopScriptsInFilesExcluding);
	CREATE_FUNCTION_BINDING("CREATE_RANDOM_VILLAGER_OF_TRIBE", 4, 1, VmCreateRandomVillagerOfTribe);
	CREATE_FUNCTION_BINDING("TOGGLE_LEASH", 1, 0, VmToggleLeash);
	CREATE_FUNCTION_BINDING("GAME_SET_MANA", 2, 0, VmGameSetMana);
	CREATE_FUNCTION_BINDING("SET_MAGIC_PROPERTIES", 3, 0, VmSetMagicProperties);
	CREATE_FUNCTION_BINDING("SET_GAME_SOUND", 1, 0, VmSetGameSound);
	CREATE_FUNCTION_BINDING("SEX_IS_MALE", 1, 1, VmSexIsMale);
	CREATE_FUNCTION_BINDING("GET_FIRST_HELP", 1, 1, VmGetFirstHelp);
	CREATE_FUNCTION_BINDING("GET_LAST_HELP", 1, 1, VmGetLastHelp);
	CREATE_FUNCTION_BINDING("IS_ACTIVE", 1, 1, VmIsActive);
	CREATE_FUNCTION_BINDING("SET_BOOKMARK_POSITION", 4, 0, VmSetBookmarkPosition);
	CREATE_FUNCTION_BINDING("SET_SCAFFOLD_PROPERTIES", 4, 0, VmSetScaffoldProperties);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_PERSONALITY", 3, 0, VmSetComputerPlayerPersonality);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_SUPPRESSION", 3, 0, VmSetComputerPlayerSuppression);
	CREATE_FUNCTION_BINDING("FORCE_COMPUTER_PLAYER_ACTION", 4, 0, VmForceComputerPlayerAction);
	CREATE_FUNCTION_BINDING("QUEUE_COMPUTER_PLAYER_ACTION", 4, 0, VmQueueComputerPlayerAction);
	CREATE_FUNCTION_BINDING("GET_TOWN_WITH_ID", 1, 1, VmGetTownWithId);
	CREATE_FUNCTION_BINDING("SET_DISCIPLE", 3, 0, VmSetDisciple);
	CREATE_FUNCTION_BINDING("RELEASE_COMPUTER_PLAYER", 1, 0, VmReleaseComputerPlayer);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_SPEED", 2, 0, VmSetComputerPlayerSpeed);
	CREATE_FUNCTION_BINDING("SET_FOCUS_FOLLOW_COMPUTER_PLAYER", 1, 0, VmSetFocusFollowComputerPlayer);
	CREATE_FUNCTION_BINDING("SET_POSITION_FOLLOW_COMPUTER_PLAYER", 1, 0, VmSetPositionFollowComputerPlayer);
	CREATE_FUNCTION_BINDING("CALL_COMPUTER_PLAYER", 1, 1, VmCallComputerPlayer);
	CREATE_FUNCTION_BINDING("CALL_BUILDING_IN_TOWN", 4, 1, VmCallBuildingInTown);
	CREATE_FUNCTION_BINDING("SET_CAN_BUILD_WORSHIPSITE", 2, 0, VmSetCanBuildWorshipsite);
	CREATE_FUNCTION_BINDING("GET_FACING_CAMERA_POSITION", 1, 3, VmGetFacingCameraPosition);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_ATTITUDE", 3, 0, VmSetComputerPlayerAttitude);
	CREATE_FUNCTION_BINDING("GET_COMPUTER_PLAYER_ATTITUDE", 2, 1, VmGetComputerPlayerAttitude);
	CREATE_FUNCTION_BINDING("LOAD_COMPUTER_PLAYER_PERSONALITY", 2, 0, VmLoadComputerPlayerPersonality);
	CREATE_FUNCTION_BINDING("SAVE_COMPUTER_PLAYER_PERSONALITY", 2, 0, VmSaveComputerPlayerPersonality);
	CREATE_FUNCTION_BINDING("SET_PLAYER_ALLY", 3, 0, VmSetPlayerAlly);
	CREATE_FUNCTION_BINDING("CALL_FLYING", 7, 1, VmCallFlying);
	CREATE_FUNCTION_BINDING("SET_OBJECT_FADE_IN", 2, 0, VmSetObjectFadeIn);
	CREATE_FUNCTION_BINDING("IS_AFFECTED_BY_SPELL", 1, 1, VmIsAffectedBySpell);
	CREATE_FUNCTION_BINDING("SET_MAGIC_IN_OBJECT", 3, 0, VmSetMagicInObject);
	CREATE_FUNCTION_BINDING("ID_ADULT_SIZE", 1, 1, VmIdAdultSize);
	CREATE_FUNCTION_BINDING("OBJECT_CAPACITY", 1, 1, VmObjectCapacity);
	CREATE_FUNCTION_BINDING("OBJECT_ADULT_CAPACITY", 1, 1, VmObjectAdultCapacity);
	CREATE_FUNCTION_BINDING("SET_CREATURE_AUTO_FIGHTING", 2, 0, VmSetCreatureAutoFighting);
	CREATE_FUNCTION_BINDING("IS_AUTO_FIGHTING", 1, 1, VmIsAutoFighting);
	CREATE_FUNCTION_BINDING("SET_CREATURE_QUEUE_FIGHT_MOVE", 2, 0, VmSetCreatureQueueFightMove);
	CREATE_FUNCTION_BINDING("SET_CREATURE_QUEUE_FIGHT_SPELL", 2, 0, VmSetCreatureQueueFightSpell);
	CREATE_FUNCTION_BINDING("SET_CREATURE_QUEUE_FIGHT_STEP", 2, 0, VmSetCreatureQueueFightStep);
	CREATE_FUNCTION_BINDING("GET_CREATURE_FIGHT_ACTION", 1, 1, VmGetCreatureFightAction);
	CREATE_FUNCTION_BINDING("CREATURE_FIGHT_QUEUE_HITS", 1, 1, VmCreatureFightQueueHits);
	CREATE_FUNCTION_BINDING("SQUARE_ROOT", 1, 1, VmSquareRoot);
	CREATE_FUNCTION_BINDING("GET_PLAYER_ALLY", 2, 1, VmGetPlayerAlly);
	CREATE_FUNCTION_BINDING("SET_PLAYER_WIND_RESISTANCE", 2, 1, VmSetPlayerWindResistance);
	CREATE_FUNCTION_BINDING("GET_PLAYER_WIND_RESISTANCE", 2, 1, VmGetPlayerWindResistance);
	CREATE_FUNCTION_BINDING("PAUSE_UNPAUSE_CLIMATE_SYSTEM", 1, 0, VmPauseUnpauseClimateSystem);
	CREATE_FUNCTION_BINDING("PAUSE_UNPAUSE_STORM_CREATION_IN_CLIMATE_SYSTEM", 1, 0, VmPauseUnpauseStormCreationInClimateSystem);
	CREATE_FUNCTION_BINDING("GET_MANA_FOR_SPELL", 1, 1, VmGetManaForSpell);
	CREATE_FUNCTION_BINDING("KILL_STORMS_IN_AREA", 4, 0, VmKillStormsInArea);
	CREATE_FUNCTION_BINDING("INSIDE_TEMPLE", 0, 1, VmInsideTemple);
	CREATE_FUNCTION_BINDING("RESTART_OBJECT", 1, 0, VmRestartObject);
	CREATE_FUNCTION_BINDING("SET_GAME_TIME_PROPERTIES", 3, 0, VmSetGameTimeProperties);
	CREATE_FUNCTION_BINDING("RESET_GAME_TIME_PROPERTIES", 0, 0, VmResetGameTimeProperties);
	CREATE_FUNCTION_BINDING("SOUND_EXISTS", 0, 1, VmSoundExists);
	CREATE_FUNCTION_BINDING("GET_TOWN_WORSHIP_DEATHS", 1, 1, VmGetTownWorshipDeaths);
	CREATE_FUNCTION_BINDING("GAME_CLEAR_DIALOGUE", 0, 0, VmGameClearDialogue);
	CREATE_FUNCTION_BINDING("GAME_CLOSE_DIALOGUE", 0, 0, VmGameCloseDialogue);
	CREATE_FUNCTION_BINDING("GET_HAND_STATE", 0, 1, VmGetHandState);
	CREATE_FUNCTION_BINDING("SET_INTERFACE_CITADEL", 1, 0, VmSetInterfaceCitadel);
	CREATE_FUNCTION_BINDING("MAP_SCRIPT_FUNCTION", 1, 0, VmMapScriptFunction);
	CREATE_FUNCTION_BINDING("WITHIN_ROTATION", 0, 1, VmWithinRotation);
	CREATE_FUNCTION_BINDING("GET_PLAYER_TOWN_TOTAL", 1, 1, VmGetPlayerTownTotal);
	CREATE_FUNCTION_BINDING("SPIRIT_SCREEN_POINT", 3, 0, VmSpiritScreenPoint);
	CREATE_FUNCTION_BINDING("KEY_DOWN", 1, 1, VmKeyDown);
	CREATE_FUNCTION_BINDING("SET_FIGHT_EXIT", 1, 0, VmSetFightExit);
	CREATE_FUNCTION_BINDING("GET_OBJECT_CLICKED", 0, 1, VmGetObjectClicked);
	CREATE_FUNCTION_BINDING("GET_MANA", 1, 1, VmGetMana);
	CREATE_FUNCTION_BINDING("CLEAR_PLAYER_SPELL_CHARGING", 1, 0, VmClearPlayerSpellCharging);
	CREATE_FUNCTION_BINDING("STOP_SOUND_EFFECT", 3, 0, VmStopSoundEffect);
	CREATE_FUNCTION_BINDING("GET_TOTEM_STATUE", 1, 1, VmGetTotemStatue);
	CREATE_FUNCTION_BINDING("SET_SET_ON_FIRE", 2, 0, VmSetSetOnFire);
	CREATE_FUNCTION_BINDING("SET_LAND_BALANCE", 2, 0, VmSetLandBalance);
	CREATE_FUNCTION_BINDING("SET_OBJECT_BELIEF_SCALE", 2, 0, VmSetObjectBeliefScale);
	CREATE_FUNCTION_BINDING("START_IMMERSION", 1, 0, VmStartImmersion);
	CREATE_FUNCTION_BINDING("STOP_IMMERSION", 1, 0, VmStopImmersion);
	CREATE_FUNCTION_BINDING("STOP_ALL_IMMERSION", 0, 0, VmStopAllImmersion);
	CREATE_FUNCTION_BINDING("SET_CREATURE_IN_TEMPLE", 1, 0, VmSetCreatureInTemple);
	CREATE_FUNCTION_BINDING("GAME_DRAW_TEXT", 7, 0, VmGameDrawText);
	CREATE_FUNCTION_BINDING("GAME_DRAW_TEMP_TEXT", 7, 0, VmGameDrawTempText);
	CREATE_FUNCTION_BINDING("FADE_ALL_DRAW_TEXT", 1, 0, VmFadeAllDrawText);
	CREATE_FUNCTION_BINDING("SET_DRAW_TEXT_COLOUR", 3, 0, VmSetDrawTextColour);
	CREATE_FUNCTION_BINDING("SET_CLIPPING_WINDOW", 5, 0, VmSetClippingWindow);
	CREATE_FUNCTION_BINDING("CLEAR_CLIPPING_WINDOW", 1, 0, VmClearClippingWindow);
	CREATE_FUNCTION_BINDING("SAVE_GAME_IN_SLOT", 1, 0, VmSaveGameInSlot);
	CREATE_FUNCTION_BINDING("SET_OBJECT_CARRYING", 2, 0, VmSetObjectCarrying);
	CREATE_FUNCTION_BINDING("POS_VALID_FOR_CREATURE", 3, 1, VmPosValidForCreature);
	CREATE_FUNCTION_BINDING("GET_TIME_SINCE_OBJECT_ATTACKED", 2, 1, VmGetTimeSinceObjectAttacked);
	CREATE_FUNCTION_BINDING("GET_TOWN_AND_VILLAGER_HEALTH_TOTAL", 1, 1, VmGetTownAndVillagerHealthTotal);
	CREATE_FUNCTION_BINDING("GAME_ADD_FOR_BUILDING", 2, 0, VmGameAddForBuilding);
	CREATE_FUNCTION_BINDING("ENABLE_DISABLE_ALIGNMENT_MUSIC", 1, 0, VmEnableDisableAlignmentMusic);
	CREATE_FUNCTION_BINDING("GET_DEAD_LIVING", 4, 1, VmGetDeadLiving);
	CREATE_FUNCTION_BINDING("ATTACH_SOUND_TAG", 4, 0, VmAttachSoundTag);
	CREATE_FUNCTION_BINDING("DETACH_SOUND_TAG", 3, 0, VmDetachSoundTag);
	CREATE_FUNCTION_BINDING("GET_SACRIFICE_TOTAL", 1, 1, VmGetSacrificeTotal);
	CREATE_FUNCTION_BINDING("GAME_SOUND_PLAYING", 2, 1, VmGameSoundPlaying);
	CREATE_FUNCTION_BINDING("GET_TEMPLE_POSITION", 1, 3, VmGetTemplePosition);
	CREATE_FUNCTION_BINDING("CREATURE_AUTOSCALE", 3, 0, VmCreatureAutoscale);
	CREATE_FUNCTION_BINDING("GET_SPELL_ICON_IN_TEMPLE", 2, 1, VmGetSpellIconInTemple);
	CREATE_FUNCTION_BINDING("GAME_CLEAR_COMPUTER_PLAYER_ACTIONS", 1, 0, VmGameClearComputerPlayerActions);
	CREATE_FUNCTION_BINDING("GET_FIRST_IN_CONTAINER", 1, 1, VmGetFirstInContainer);
	CREATE_FUNCTION_BINDING("GET_NEXT_IN_CONTAINER", 2, 1, VmGetNextInContainer);
	CREATE_FUNCTION_BINDING("GET_TEMPLE_ENTRANCE_POSITION", 3, 3, VmGetTempleEntrancePosition);
	CREATE_FUNCTION_BINDING("SAY_SOUND_EFFECT_PLAYING", 2, 1, VmSaySoundEffectPlaying);
	CREATE_FUNCTION_BINDING("SET_HAND_DEMO_KEYS", 1, 0, VmSetHandDemoKeys);
	CREATE_FUNCTION_BINDING("CAN_SKIP_TUTORIAL", 0, 1, VmCanSkipTutorial);
	CREATE_FUNCTION_BINDING("CAN_SKIP_CREATURE_TRAINING", 0, 1, VmCanSkipCreatureTraining);
	CREATE_FUNCTION_BINDING("IS_KEEPING_OLD_CREATURE", 0, 1, VmIsKeepingOldCreature);
	CREATE_FUNCTION_BINDING("CURRENT_PROFILE_HAS_CREATURE", 0, 1, VmCurrentProfileHasCreature);
}

void Game::VmNone() {} // 000 NONE

void Game::VmSetCameraPosition() // 001 SET_CAMERA_POSITION
{
	const auto position = PopVec();
	// TODO(Daniels118): check if cinema mode is enabled
	_camera->SetOrigin(position);
}

void Game::VmSetCameraFocus() // 002 SET_CAMERA_FOCUS
{
	const auto position = PopVec();
	// TODO(Daniels118): check if cinema mode is enabled
	_camera->SetFocus(position);
}

void Game::VmMoveCameraPosition() // 003 MOVE_CAMERA_POSITION
{
	// const auto time = _lhvm->Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
}

void Game::VmMoveCameraFocus() // 004 MOVE_CAMERA_FOCUS
{
	// const auto time = _lhvm->Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
}

void Game::VmGetCameraPosition() // 005 GET_CAMERA_POSITION
{
	const auto position = _camera->GetOrigin();
	PushVec(position);
}

void Game::VmGetCameraFocus() // 006 GET_CAMERA_FOCUS
{
	const auto focus = _camera->GetFocus();
	PushVec(focus);
}

void Game::VmSpiritEject() // 007 SPIRIT_EJECT
{
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSpiritHome() // 008 SPIRIT_HOME
{
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSpiritPointPos() // 009 SPIRIT_POINT_POS
{
	// const auto inWorld = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto position = PopVec();
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSpiritPointGameThing() // 010 SPIRIT_POINT_GAME_THING
{
	// const auto inWorld = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto target = _lhvm->Pop().uintVal;
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGameThingFieldOfView() // 011 GAME_THING_FIELD_OF_VIEW
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmPosFieldOfView() // 012 POS_FIELD_OF_VIEW
{
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmRunText() // 013 RUN_TEXT
{
	// const auto withInteraction = _lhvm->Pop().intVal;
	// const auto textID = _lhvm->Pop().intVal;
	// const auto singleLine = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmTempText() // 014 TEMP_TEXT
{
	// const auto withInteraction = _lhvm->Pop().intVal;
	// const auto string = PopString();
	// const auto singleLine = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmTextRead() // 015 TEXT_READ
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmGameThingClicked() // 016 GAME_THING_CLICKED
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmSetScriptState() // 017 SET_SCRIPT_STATE
{
	// const auto state = _lhvm->Pop().intVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetScriptStatePos() // 018 SET_SCRIPT_STATE_POS
{
	// const auto position = PopVec();
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetScriptFloat() // 019 SET_SCRIPT_FLOAT
{
	// const auto value = _lhvm->Popf();
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetScriptUlong() // 020 SET_SCRIPT_ULONG
{
	// const auto loop = _lhvm->Pop().intVal;
	// const auto animation = _lhvm->Pop().intVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetProperty() // 021 GET_PROPERTY
{
	// const auto object = _lhvm->Pop().uintVal;
	// const auto prop = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmSetProperty() // 022 SET_PROPERTY
{
	// const auto val = _lhvm->Popf();
	// const auto object = _lhvm->Pop().uintVal;
	// const auto prop = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetPosition() // 023 GET_POSITION
{
	const auto objId = _lhvm->Pop().uintVal;

	glm::vec3 position(0.0f);
	const auto pObject = _refManager.GetTarget(objId);
	if (pObject != nullptr)
	{
		auto& registry = Locator::entitiesRegistry::value();
		position = registry.Get<ecs::components::Transform>(*pObject).position;
	}

	PushVec(position);
}

void Game::VmSetPosition() // 024 SET_POSITION
{
	auto position = PopVec();
	const auto objId = _lhvm->Pop().uintVal;

	const auto pObject = _refManager.GetTarget(objId);
	if (pObject != nullptr)
	{
		const auto& island = Locator::terrainSystem::value();
		position.y = island.GetHeightAt(glm::vec2(position.x, position.z));
		auto& registry = Locator::entitiesRegistry::value();
		registry.Get<ecs::components::Transform>(*pObject).position = position;
	}
}

void Game::VmGetDistance() // 025 GET_DISTANCE
{
	const auto p1 = PopVec();
	const auto p0 = PopVec();
	const auto distance = glm::length(p1 - p0);
	_lhvm->Pushf(distance);
}

void Game::VmCall() // 026 CALL
{
	// const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto position = PopVec();
	// const auto subtype = _lhvm->Pop().intVal;
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmCreate() // 027 CREATE
{
	const auto position = PopVec();
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = static_cast<script::ObjectType>(_lhvm->Pop().intVal);

	const auto object = CreateScriptObject(type, subtype, position, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	const auto objId = _refManager.GetReference(object);

	_lhvm->Pusho(objId);
}

void Game::VmRandom() // 028 RANDOM
{
	const auto max = _lhvm->Popf();
	const auto min = _lhvm->Popf();
	float random = min + (max - min) * ((float)rand()) / (float)RAND_MAX;
	_lhvm->Pushf(random);
}

void Game::VmDllGettime() // 029 DLL_GETTIME
{
	_lhvm->Pushf((float)_turnCount / 10.0f); // TODO(Daniels118): should it be divided by 10 or not?
}

void Game::VmStartCameraControl() // 030 START_CAMERA_CONTROL
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmEndCameraControl() // 031 END_CAMERA_CONTROL
{
	// TODO(Daniels118): implement this
}

void Game::VmSetWidescreen() // 032 SET_WIDESCREEN
{
	// const auto enabled = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmMoveGameThing() // 033 MOVE_GAME_THING
{
	// const auto radius = _lhvm->Popf();
	// const auto position = PopVec();
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetFocus() // 034 SET_FOCUS
{
	// const auto position = PopVec();
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmHasCameraArrived() // 035 HAS_CAMERA_ARRIVED
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmFlockCreate() // 036 FLOCK_CREATE
{
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmFlockAttach() // 037 FLOCK_ATTACH
{
	// const auto asLeader = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto flock = _lhvm->Pop().uintVal;
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmFlockDetach() // 038 FLOCK_DETACH
{
	// const auto flock = _lhvm->Pop().uintVal;
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmFlockDisband() // 039 FLOCK_DISBAND
{
	// const auto flock = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmIdSize() // 040 ID_SIZE
{
	// const auto container = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmFlockMember() // 041 FLOCK_MEMBER
{
	// const auto flock = _lhvm->Pop().uintVal;
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmGetHandPosition() // 042 GET_HAND_POSITION
{
	auto& handTransform = Locator::entitiesRegistry::value().Get<ecs::components::Transform>(_handEntity);

	PushVec(handTransform.position);
}

void Game::VmPlaySoundEffect() // 043 PLAY_SOUND_EFFECT
{
	// const auto withPosition = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto position = PopVec();
	// const auto soundbank = _lhvm->Pop().intVal;
	// const auto sound = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmStartMusic() // 044 START_MUSIC
{
	// const auto music = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmStopMusic() // 045 STOP_MUSIC
{
	// TODO(Daniels118): implement this
}

void Game::VmAttachMusic() // 046 ATTACH_MUSIC
{
	// const auto target = _lhvm->Pop().uintVal;
	// const auto music = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmDetachMusic() // 047 DETACH_MUSIC
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmObjectDelete() // 048 OBJECT_DELETE
{
	// const auto withFade = _lhvm->Pop().intVal;
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmFocusFollow() // 049 FOCUS_FOLLOW
{
	// const auto target = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmPositionFollow() // 050 POSITION_FOLLOW
{
	// const auto target = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmCallNear() // 051 CALL_NEAR
{
	// const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto radius = _lhvm->Popf();
	// const auto position = PopVec();
	// const auto subtype = _lhvm->Pop().intVal;
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSpecialEffectPosition() // 052 SPECIAL_EFFECT_POSITION
{
	// const auto duration = _lhvm->Popf();
	// const auto position = PopVec();
	// const auto effect = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSpecialEffectObject() // 053 SPECIAL_EFFECT_OBJECT
{
	// const auto duration = _lhvm->Popf();
	// const auto target = _lhvm->Pop().uintVal;
	// const auto effect = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmDanceCreate() // 054 DANCE_CREATE
{
	// const auto duration = _lhvm->Popf();
	// const auto position = PopVec();
	// const auto type = _lhvm->Pop().intVal;
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmCallIn() // 055 CALL_IN
{
	// const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto container = _lhvm->Pop().uintVal;
	// const auto subtype = _lhvm->Pop().intVal;
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmChangeInnerOuterProperties() // 056 CHANGE_INNER_OUTER_PROPERTIES
{
	// const auto calm = _lhvm->Popf();
	// const auto outer = _lhvm->Popf();
	// const auto inner = _lhvm->Popf();
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSnapshot() // 057 SNAPSHOT
{
	// const auto challengeId = _lhvm->Pop().intVal;
	// const auto argc = _lhvm->Pop().intVal;
	// const auto argv = PopVarArg(argc);
	// const auto reminderScript = PopString();
	// const auto titleStrID = _lhvm->Pop().intVal;
	// const auto alignment = _lhvm->Popf();
	// const auto success = _lhvm->Popf();
	// const auto focus = PopVec();
	// const auto position = PopVec();
	// const auto quest = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmGetAlignment() // 058 GET_ALIGNMENT
{
	// const auto zero = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmSetAlignment() // 059 SET_ALIGNMENT
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmInfluenceObject() // 060 INFLUENCE_OBJECT
{
	// const auto anti = _lhvm->Pop().intVal;
	// const auto zero = _lhvm->Pop().intVal;
	// const auto radius = _lhvm->Popf();
	// const auto target = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmInfluencePosition() // 061 INFLUENCE_POSITION
{
	// const auto anti = _lhvm->Pop().intVal;
	// const auto zero = _lhvm->Pop().intVal;
	// const auto radius = _lhvm->Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmGetInfluence() // 062 GET_INFLUENCE
{
	// const auto position = PopVec();
	// const auto raw = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmSetInterfaceInteraction() // 063 SET_INTERFACE_INTERACTION
{
	// const auto level = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmPlayed() // 064 PLAYED
{
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmRandomUlong() // 065 RANDOM_ULONG
{
	// const auto max = _lhvm->Pop().intVal;
	// const auto min = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmSetGamespeed() // 066 SET_GAMESPEED
{
	// const auto speed = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmCallInNear() // 067 CALL_IN_NEAR
{
	// const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto radius = _lhvm->Popf();
	// const auto pos = PopVec();
	// const auto container = _lhvm->Pop().uintVal;
	// const auto subtype = _lhvm->Pop().intVal;
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmOverrideStateAnimation() // 068 OVERRIDE_STATE_ANIMATION
{
	// const auto animType = _lhvm->Pop().intVal;
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreatureCreateRelativeToCreature() // 069 CREATURE_CREATE_RELATIVE_TO_CREATURE
{
	// const auto type = _lhvm->Pop().intVal;
	// const auto position = PopVec();
	// const auto scale = _lhvm->Popf();
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmCreatureLearnEverything() // 070 CREATURE_LEARN_EVERYTHING
{
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreatureSetKnowsAction() // 071 CREATURE_SET_KNOWS_ACTION
{
	// const auto knows = _lhvm->Pop().intVal;
	// const auto action = _lhvm->Pop().intVal;
	// const auto typeOfAction = _lhvm->Pop().intVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreatureSetAgendaPriority() // 072 CREATURE_SET_AGENDA_PRIORITY
{
	// const auto priority = _lhvm->Popf();
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreatureTurnOffAllDesires() // 073 CREATURE_TURN_OFF_ALL_DESIRES
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreatureLearnDistinctionAboutActivityObject() // 074 CREATURE_LEARN_DISTINCTION_ABOUT_ACTIVITY_OBJECT
{
	// const auto unk3 = _lhvm->Pop().intVal;
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreatureDoAction() // 075 CREATURE_DO_ACTION
{
	// const auto withObject = _lhvm->Pop().uintVal;
	// const auto target = _lhvm->Pop().uintVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmInCreatureHand() // 076 IN_CREATURE_HAND
{
	// const auto creature = _lhvm->Pop().uintVal;
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmCreatureSetDesireValue() // 077 CREATURE_SET_DESIRE_VALUE
{
	// const auto value = _lhvm->Popf();
	// const auto desire = _lhvm->Pop().intVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreatureSetDesireActivated78() // 078 CREATURE_SET_DESIRE_ACTIVATED
{
	// const auto active = _lhvm->Pop().intVal;
	// const auto desire = _lhvm->Pop().intVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreatureSetDesireActivated79() // 079 CREATURE_SET_DESIRE_ACTIVATED
{
	// const auto active = _lhvm->Pop().intVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreatureSetDesireMaximum() // 080 CREATURE_SET_DESIRE_MAXIMUM
{
	// const auto value = _lhvm->Popf();
	// const auto desire = _lhvm->Pop().intVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmConvertCameraPosition() // 081 CONVERT_CAMERA_POSITION
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::VmConvertCameraFocus() // 082 CONVERT_CAMERA_FOCUS
{
	// const auto camera_enum = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::VmCreatureSetPlayer() // 083 CREATURE_SET_PLAYER
{
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmStartCountdownTimer() // 084 START_COUNTDOWN_TIMER
{
	// const auto timeout = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmCreatureInitialiseNumTimesPerformedAction() // 085 CREATURE_INITIALISE_NUM_TIMES_PERFORMED_ACTION
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreatureGetNumTimesActionPerformed() // 086 CREATURE_GET_NUM_TIMES_ACTION_PERFORMED
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmRemoveCountdownTimer() // 087 REMOVE_COUNTDOWN_TIMER
{
	// TODO(Daniels118): implement this
}

void Game::VmGetObjectDropped() // 088 GET_OBJECT_DROPPED
{
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmClearDroppedByObject() // 089 CLEAR_DROPPED_BY_OBJECT
{
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreateReaction() // 090 CREATE_REACTION
{
	// const auto reaction = _lhvm->Pop().intVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmRemoveReaction() // 091 REMOVE_REACTION
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetCountdownTimer() // 092 GET_COUNTDOWN_TIMER
{
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmStartDualCamera() // 093 START_DUAL_CAMERA
{
	// const auto obj2 = _lhvm->Pop().uintVal;
	// const auto obj1 = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmUpdateDualCamera() // 094 UPDATE_DUAL_CAMERA
{
	// const auto obj2 = _lhvm->Pop().uintVal;
	// const auto obj1 = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmReleaseDualCamera() // 095 RELEASE_DUAL_CAMERA
{
	// TODO(Daniels118): implement this
}

void Game::VmSetCreatureHelp() // 096 SET_CREATURE_HELP
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetTargetObject() // 097 GET_TARGET_OBJECT
{
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmCreatureDesireIs() // 098 CREATURE_DESIRE_IS
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmCountdownTimerExists() // 099 COUNTDOWN_TIMER_EXISTS
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmLookGameThing() // 100 LOOK_GAME_THING
{
	// const auto target = _lhvm->Pop().uintVal;
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetObjectDestination() // 101 GET_OBJECT_DESTINATION
{
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::VmCreatureForceFinish() // 102 CREATURE_FORCE_FINISH
{
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmHideCountdownTimer() // 103 HIDE_COUNTDOWN_TIMER
{
	// TODO(Daniels118): implement this
}

void Game::VmGetActionTextForObject() // 104 GET_ACTION_TEXT_FOR_OBJECT
{
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmCreateDualCameraWithPoint() // 105 CREATE_DUAL_CAMERA_WITH_POINT
{
	// const auto position = PopVec();
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetCameraToFaceObject() // 106 SET_CAMERA_TO_FACE_OBJECT
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmMoveCameraToFaceObject() // 107 MOVE_CAMERA_TO_FACE_OBJECT
{
	// const auto time = _lhvm->Popf();
	// const auto distance = _lhvm->Popf();
	// const auto target = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetMoonPercentage() // 108 GET_MOON_PERCENTAGE
{
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmPopulateContainer() // 109 POPULATE_CONTAINER
{
	// const auto subtype = _lhvm->Pop().intVal;
	// const auto type = _lhvm->Pop().intVal;
	// const auto quantity = _lhvm->Popf();
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmAddReference() // 110 ADD_REFERENCE
{
	const auto objId = _lhvm->Pop().uintVal;
	AddObjectReference(objId);
	_lhvm->Pusho(objId);
}

void Game::VmRemoveReference() // 111 REMOVE_REFERENCE
{
	const auto objId = _lhvm->Pop().uintVal;
	RemoveObjectReference(objId);
	_lhvm->Pusho(objId);
}

void Game::VmSetGameTime() // 112 SET_GAME_TIME
{
	// const auto time = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmGetGameTime() // 113 GET_GAME_TIME
{
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetRealTime() // 114 GET_REAL_TIME
{
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetRealDay115() // 115 GET_REAL_DAY
{
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetRealDay116() // 116 GET_REAL_DAY
{
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetRealMonth() // 117 GET_REAL_MONTH
{
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetRealYear() // 118 GET_REAL_YEAR
{
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmRunCameraPath() // 119 RUN_CAMERA_PATH
{
	// const auto cameraEnum = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmStartDialogue() // 120 START_DIALOGUE
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmEndDialogue() // 121 END_DIALOGUE
{
	// TODO(Daniels118): implement this
}

void Game::VmIsDialogueReady() // 122 IS_DIALOGUE_READY
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmChangeWeatherProperties() // 123 CHANGE_WEATHER_PROPERTIES
{
	// const auto fallspeed = _lhvm->Popf();
	// const auto overcast = _lhvm->Popf();
	// const auto snowfall = _lhvm->Popf();
	// const auto rainfall = _lhvm->Popf();
	// const auto temperature = _lhvm->Popf();
	// const auto storm = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmChangeLightningProperties() // 124 CHANGE_LIGHTNING_PROPERTIES
{
	// const auto forkmax = _lhvm->Popf();
	// const auto forkmin = _lhvm->Popf();
	// const auto sheetmax = _lhvm->Popf();
	// const auto sheetmin = _lhvm->Popf();
	// const auto storm = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmChangeTimeFadeProperties() // 125 CHANGE_TIME_FADE_PROPERTIES
{
	// const auto fadeTime = _lhvm->Popf();
	// const auto duration = _lhvm->Popf();
	// const auto storm = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmChangeCloudProperties() // 126 CHANGE_CLOUD_PROPERTIES
{
	// const auto elevation = _lhvm->Popf();
	// const auto blackness = _lhvm->Popf();
	// const auto numClouds = _lhvm->Popf();
	// const auto storm = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetHeadingAndSpeed() // 127 SET_HEADING_AND_SPEED
{
	// const auto speed = _lhvm->Popf();
	// const auto position = PopVec();
	// const auto unk0 = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmStartGameSpeed() // 128 START_GAME_SPEED
{
	// TODO(Daniels118): implement this
}

void Game::VmEndGameSpeed() // 129 END_GAME_SPEED
{
	// TODO(Daniels118): implement this
}

void Game::VmBuildBuilding() // 130 BUILD_BUILDING
{
	// const auto desire = _lhvm->Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
}

void Game::VmSetAffectedByWind() // 131 SET_AFFECTED_BY_WIND
{
	// const auto object = _lhvm->Pop().uintVal;
	// const auto enabled = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmWidescreenTransistionFinished() // 132 WIDESCREEN_TRANSISTION_FINISHED
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmGetResource() // 133 GET_RESOURCE
{
	// const auto container = _lhvm->Pop().uintVal;
	// const auto resource = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmAddResource() // 134 ADD_RESOURCE
{
	// const auto container = _lhvm->Pop().uintVal;
	// const auto quantity = _lhvm->Popf();
	// const auto resource = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmRemoveResource() // 135 REMOVE_RESOURCE
{
	// const auto container = _lhvm->Pop().uintVal;
	// const auto quantity = _lhvm->Popf();
	// const auto resource = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetTargetRelativePos() // 136 GET_TARGET_RELATIVE_POS
{
	// const auto angle = _lhvm->Popf();
	// const auto distance = _lhvm->Popf();
	// const auto to = PopVec();
	// const auto from = PopVec();
	// TODO(Daniels118): implement this
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::VmStopPointing() // 137 STOP_POINTING
{
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmStopLooking() // 138 STOP_LOOKING
{
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmLookAtPosition() // 139 LOOK_AT_POSITION
{
	// const auto position = PopVec();
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmPlaySpiritAnim() // 140 PLAY_SPIRIT_ANIM
{
	// const auto unk4 = _lhvm->Pop().intVal;
	// const auto unk3 = _lhvm->Pop().intVal;
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmCallInNotNear() // 141 CALL_IN_NOT_NEAR
{
	// const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto radius = _lhvm->Popf();
	// const auto pos = PopVec();
	// const auto container = _lhvm->Pop().uintVal;
	// const auto subtype = _lhvm->Pop().intVal;
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSetCameraZone() // 142 SET_CAMERA_ZONE
{
	// const auto filename = PopString();
	// TODO(Daniels118): implement this
}

void Game::VmGetObjectState() // 143 GET_OBJECT_STATE
{
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmRevealCountdownTimer() // 144 REVEAL_COUNTDOWN_TIMER
{
	// TODO(Daniels118): implement this
}

void Game::VmSetTimerTime() // 145 SET_TIMER_TIME
{
	// const auto time = _lhvm->Popf();
	// const auto timer = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreateTimer() // 146 CREATE_TIMER
{
	// const auto timeout = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmGetTimerTimeRemaining() // 147 GET_TIMER_TIME_REMAINING
{
	// const auto timer = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetTimerTimeSinceSet() // 148 GET_TIMER_TIME_SINCE_SET
{
	// const auto timer = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmMoveMusic() // 149 MOVE_MUSIC
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetInclusionDistance() // 150 GET_INCLUSION_DISTANCE
{
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetLandHeight() // 151 GET_LAND_HEIGHT
{
	const auto position = PopVec();

	const auto& island = Locator::terrainSystem::value();
	const auto elevation = island.GetHeightAt(glm::vec2(position.x, position.z));

	_lhvm->Pushf(elevation);
}

void Game::VmLoadMap() // 152 LOAD_MAP
{
	const auto path = PopString();

	auto& fileSystem = Locator::filesystem::value();
	auto mapPath = fileSystem.GetGamePath() / path;
	LoadMap(mapPath);
}

void Game::VmStopAllScriptsExcluding() // 153 STOP_ALL_SCRIPTS_EXCLUDING
{
	const auto scriptNames = PopString();

	const auto names = GetUniqueWords(scriptNames);
	_lhvm->StopScripts([&names](const std::string& name, [[maybe_unused]] const std::string& filename) -> bool {
		return !names.contains(name);
	});
}

void Game::VmStopAllScriptsInFilesExcluding() // 154 STOP_ALL_SCRIPTS_IN_FILES_EXCLUDING
{
	const auto sourceFilenames = PopString();

	const auto filenames = GetUniqueWords(sourceFilenames);
	_lhvm->StopScripts([&filenames]([[maybe_unused]] const std::string& name, const std::string& filename) -> bool {
		return !filenames.contains(filename);
	});
}

void Game::VmStopScript() // 155 STOP_SCRIPT
{
	const auto scriptName = PopString();
	_lhvm->StopScripts([&scriptName](const std::string& name, [[maybe_unused]] const std::string& filename) -> bool {
		return name == scriptName;
	});
}

void Game::VmClearClickedObject() // 156 CLEAR_CLICKED_OBJECT
{
	// TODO(Daniels118): implement this
}

void Game::VmClearClickedPosition() // 157 CLEAR_CLICKED_POSITION
{
	// TODO(Daniels118): implement this
}

void Game::VmPositionClicked() // 158 POSITION_CLICKED
{
	// const auto unk3 = _lhvm->Pop().intVal;
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmReleaseFromScript() // 159 RELEASE_FROM_SCRIPT
{
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetObjectHandIsOver() // 160 GET_OBJECT_HAND_IS_OVER
{
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmIdPoisonedSize() // 161 ID_POISONED_SIZE
{
	// const auto container = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmIsPoisoned() // 162 IS_POISONED
{
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmCallPoisonedIn() // 163 CALL_POISONED_IN
{
	// const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto container = _lhvm->Pop().uintVal;
	// const auto subtype = _lhvm->Pop().intVal;
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmCallNotPoisonedIn() // 164 CALL_NOT_POISONED_IN
{
	// const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto container = _lhvm->Pop().uintVal;
	// const auto subtype = _lhvm->Pop().intVal;
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSpiritPlayed() // 165 SPIRIT_PLAYED
{
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmClingSpirit() // 166 CLING_SPIRIT
{
	// const auto yPercent = _lhvm->Popf();
	// const auto xPercent = _lhvm->Popf();
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmFlySpirit() // 167 FLY_SPIRIT
{
	// const auto yPercent = _lhvm->Popf();
	// const auto xPercent = _lhvm->Popf();
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetIdMoveable() // 168 SET_ID_MOVEABLE
{
	// const auto obj = _lhvm->Pop().uintVal;
	// const auto moveable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetIdPickupable() // 169 SET_ID_PICKUPABLE
{
	// const auto obj = _lhvm->Pop().uintVal;
	// const auto pickupable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmIsOnFire() // 170 IS_ON_FIRE
{
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmIsFireNear() // 171 IS_FIRE_NEAR
{
	// const auto radius = _lhvm->Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmStopScriptsInFiles() // 172 STOP_SCRIPTS_IN_FILES
{
	const auto sourceFilenames = PopString();

	const auto names = GetUniqueWords(sourceFilenames);
	_lhvm->StopScripts([&names]([[maybe_unused]] const std::string& name, const std::string& filename) -> bool {
		return names.contains(filename);
	});
}

void Game::VmSetPoisoned() // 173 SET_POISONED
{
	// const auto obj = _lhvm->Pop().uintVal;
	// const auto poisoned = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetTemperature() // 174 SET_TEMPERATURE
{
	// const auto temperature = _lhvm->Popf();
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetOnFire() // 175 SET_ON_FIRE
{
	// const auto burnSpeed = _lhvm->Popf();
	// const auto object = _lhvm->Pop().uintVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetTarget() // 176 SET_TARGET
{
	// const auto time = _lhvm->Popf();
	// const auto position = PopVec();
	// const auto obj = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmWalkPath() // 177 WALK_PATH
{
	// const auto valTo = _lhvm->Popf();
	// const auto valFrom = _lhvm->Popf();
	// const auto camera_enum = _lhvm->Pop().intVal;
	// const auto forward = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmFocusAndPositionFollow() // 178 FOCUS_AND_POSITION_FOLLOW
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetWalkPathPercentage() // 179 GET_WALK_PATH_PERCENTAGE
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmCameraProperties() // 180 CAMERA_PROPERTIES
{
	// const auto enableBehind = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto angle = _lhvm->Popf();
	// const auto speed = _lhvm->Popf();
	// const auto distance = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmEnableDisableMusic() // 181 ENABLE_DISABLE_MUSIC
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetMusicObjDistance() // 182 GET_MUSIC_OBJ_DISTANCE
{
	// const auto source = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetMusicEnumDistance() // 183 GET_MUSIC_ENUM_DISTANCE
{
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmSetMusicPlayPosition() // 184 SET_MUSIC_PLAY_POSITION
{
	// const auto unk3 = _lhvm->Pop().intVal;
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmAttachObjectLeashToObject() // 185 ATTACH_OBJECT_LEASH_TO_OBJECT
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmAttachObjectLeashToHand() // 186 ATTACH_OBJECT_LEASH_TO_HAND
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmDetachObjectLeash() // 187 DETACH_OBJECT_LEASH
{
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetCreatureOnlyDesire() // 188 SET_CREATURE_ONLY_DESIRE
{
	// const auto value = _lhvm->Popf();
	// const auto desire = _lhvm->Pop().intVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetCreatureOnlyDesireOff() // 189 SET_CREATURE_ONLY_DESIRE_OFF
{
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmRestartMusic() // 190 RESTART_MUSIC
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmMusicPlayed191() // 191 MUSIC_PLAYED
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmIsOfType() // 192 IS_OF_TYPE
{
	// const auto subtype = _lhvm->Pop().intVal;
	// const auto type = _lhvm->Pop().intVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmClearHitObject() // 193 CLEAR_HIT_OBJECT
{
	// TODO(Daniels118): implement this
}

void Game::VmGameThingHit() // 194 GAME_THING_HIT
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmSpellAtThing() // 195 SPELL_AT_THING
{
	// const auto curl = _lhvm->Popf();
	// const auto duration = _lhvm->Popf();
	// const auto radius = _lhvm->Popf();
	// const auto from = PopVec();
	// const auto target = _lhvm->Pop().uintVal;
	// const auto spell = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSpellAtPos() // 196 SPELL_AT_POS
{
	// const auto curl = _lhvm->Popf();
	// const auto duration = _lhvm->Popf();
	// const auto radius = _lhvm->Popf();
	// const auto from = PopVec();
	// const auto target = PopVec();
	// const auto spell = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmCallPlayerCreature() // 197 CALL_PLAYER_CREATURE
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmGetSlowestSpeed() // 198 GET_SLOWEST_SPEED
{
	// const auto flock = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetObjectHeld199() // 199 GET_OBJECT_HELD
{
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmHelpSystemOn() // 200 HELP_SYSTEM_ON
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmShakeCamera() // 201 SHAKE_CAMERA
{
	// const auto duration = _lhvm->Popf();
	// const auto amplitude = _lhvm->Popf();
	// const auto radius = _lhvm->Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
}

void Game::VmSetAnimationModify() // 202 SET_ANIMATION_MODIFY
{
	// const auto creature = _lhvm->Pop().uintVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetAviSequence() // 203 SET_AVI_SEQUENCE
{
	// const auto aviSequence = _lhvm->Pop().intVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmPlayGesture() // 204 PLAY_GESTURE
{
	// const auto unk4 = _lhvm->Pop().intVal;
	// const auto unk3 = _lhvm->Pop().intVal;
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmDevFunction() // 205 DEV_FUNCTION
{
	// const auto func = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmHasMouseWheel() // 206 HAS_MOUSE_WHEEL
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmNumMouseButtons() // 207 NUM_MOUSE_BUTTONS
{
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmSetCreatureDevStage() // 208 SET_CREATURE_DEV_STAGE
{
	// const auto stage = _lhvm->Pop().intVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetFixedCamRotation() // 209 SET_FIXED_CAM_ROTATION
{
	// const auto unk3 = _lhvm->Pop().intVal;
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSwapCreature() // 210 SWAP_CREATURE
{
	// const auto toCreature = _lhvm->Pop().uintVal;
	// const auto fromCreature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetArena() // 211 GET_ARENA
{
	// const auto unk4 = _lhvm->Pop().intVal;
	// const auto unk3 = _lhvm->Pop().intVal;
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmGetFootballPitch() // 212 GET_FOOTBALL_PITCH
{
	// const auto town = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmStopAllGames() // 213 STOP_ALL_GAMES
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmAttachToGame() // 214 ATTACH_TO_GAME
{
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmDetachFromGame() // 215 DETACH_FROM_GAME
{
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmDetachUndefinedFromGame() // 216 DETACH_UNDEFINED_FROM_GAME
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetOnlyForScripts() // 217 SET_ONLY_FOR_SCRIPTS
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmStartMatchWithReferee() // 218 START_MATCH_WITH_REFEREE
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGameTeamSize() // 219 GAME_TEAM_SIZE
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGameType() // 220 GAME_TYPE
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmGameSubType() // 221 GAME_SUB_TYPE
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmIsLeashed() // 222 IS_LEASHED
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmSetCreatureHome() // 223 SET_CREATURE_HOME
{
	// const auto position = PopVec();
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetHitObject() // 224 GET_HIT_OBJECT
{
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmGetObjectWhichHit() // 225 GET_OBJECT_WHICH_HIT
{
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmGetNearestTownOfPlayer() // 226 GET_NEAREST_TOWN_OF_PLAYER
{
	// const auto unk4 = _lhvm->Pop().intVal;
	// const auto unk3 = _lhvm->Pop().intVal;
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSpellAtPoint() // 227 SPELL_AT_POINT
{
	// const auto radius = _lhvm->Popf();
	// const auto position = PopVec();
	// const auto spell = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSetAttackOwnTown() // 228 SET_ATTACK_OWN_TOWN
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmIsFighting() // 229 IS_FIGHTING
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmSetMagicRadius() // 230 SET_MAGIC_RADIUS
{
	// const auto radius = _lhvm->Popf();
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmTempTextWithNumber() // 231 TEMP_TEXT_WITH_NUMBER
{
	// const auto withInteraction = _lhvm->Pop().intVal;
	// const auto value = _lhvm->Popf();
	// const auto format = PopString();
	// const auto singleLine = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmRunTextWithNumber() // 232 RUN_TEXT_WITH_NUMBER
{
	// const auto withInteraction = _lhvm->Pop().intVal;
	// const auto number = _lhvm->Popf();
	// const auto string = _lhvm->Pop().intVal;
	// const auto singleLine = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmCreatureSpellReversion() // 233 CREATURE_SPELL_REVERSION
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetDesire() // 234 GET_DESIRE
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetEventsPerSecond() // 235 GET_EVENTS_PER_SECOND
{
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetTimeSince() // 236 GET_TIME_SINCE
{
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetTotalEvents() // 237 GET_TOTAL_EVENTS
{
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmUpdateSnapshot() // 238 UPDATE_SNAPSHOT
{
	// const auto challengeId = _lhvm->Pop().intVal;
	// const auto argc = _lhvm->Pop().intVal;
	// const auto argv = PopVarArg(argc);
	// const auto reminderScript = PopString();
	// const auto titleStrID = _lhvm->Pop().intVal;
	// const auto alignment = _lhvm->Popf();
	// const auto success = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmCreateReward() // 239 CREATE_REWARD
{
	// const auto fromSky = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto position = PopVec();
	// const auto reward = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmCreateRewardInTown() // 240 CREATE_REWARD_IN_TOWN
{
	// const auto fromSky = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto position = PopVec();
	// const auto town = _lhvm->Pop().uintVal;
	// const auto reward = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSetFade() // 241 SET_FADE
{
	// const auto time = _lhvm->Popf();
	// const auto blue = _lhvm->Popf();
	// const auto green = _lhvm->Popf();
	// const auto red = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmSetFadeIn() // 242 SET_FADE_IN
{
	// const auto duration = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmFadeFinished() // 243 FADE_FINISHED
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmSetPlayerMagic() // 244 SET_PLAYER_MAGIC
{
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmHasPlayerMagic() // 245 HAS_PLAYER_MAGIC
{
	// const auto player = _lhvm->Popf();
	// const auto spell = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmSpiritSpeaks() // 246 SPIRIT_SPEAKS
{
	// const auto textID = _lhvm->Pop().intVal;
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmBeliefForPlayer() // 247 BELIEF_FOR_PLAYER
{
	// const auto player = _lhvm->Popf();
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetHelp() // 248 GET_HELP
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmSetLeashWorks() // 249 SET_LEASH_WORKS
{
	// const auto creature = _lhvm->Pop().uintVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmLoadMyCreature() // 250 LOAD_MY_CREATURE
{
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
}

void Game::VmObjectRelativeBelief() // 251 OBJECT_RELATIVE_BELIEF
{
	// const auto belief = _lhvm->Popf();
	// const auto player = _lhvm->Popf();
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreateWithAngleAndScale() // 252 CREATE_WITH_ANGLE_AND_SCALE
{
	const auto position = PopVec();
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = static_cast<script::ObjectType>(_lhvm->Pop().intVal);
	const auto scale = _lhvm->Popf();
	const auto angle = _lhvm->Popf();

	entt::entity object = CreateScriptObject(type, subtype, position, 0.0f, 0.0f, angle, 0.0f, scale);
	const auto objId = _refManager.GetReference(object);

	_lhvm->Pusho(objId);
}

void Game::VmSetHelpSystem() // 253 SET_HELP_SYSTEM
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetVirtualInfluence() // 254 SET_VIRTUAL_INFLUENCE
{
	// const auto player = _lhvm->Popf();
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetActive() // 255 SET_ACTIVE
{
	// const auto object = _lhvm->Pop().uintVal;
	// const auto active = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmThingValid() // 256 THING_VALID
{
	const auto objId = _lhvm->Pop().uintVal;
	// TODO is this the right way?
	bool valid = false;
	const auto pObject = _refManager.GetTarget(objId);
	if (pObject != nullptr)
	{
		auto& registry = Locator::entitiesRegistry::value();
		ecs::components::Transform* r = registry.TryGet<ecs::components::Transform>(*pObject);
		valid = r != nullptr;
	}
	_lhvm->Pushb(valid);
}

void Game::VmVortexFadeOut() // 257 VORTEX_FADE_OUT
{
	// const auto vortex = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmRemoveReactionOfType() // 258 REMOVE_REACTION_OF_TYPE
{
	// const auto reaction = _lhvm->Pop().intVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreatureLearnEverythingExcluding() // 259 CREATURE_LEARN_EVERYTHING_EXCLUDING
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmPlayedPercentage() // 260 PLAYED_PERCENTAGE
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmObjectCastByObject() // 261 OBJECT_CAST_BY_OBJECT
{
	// const auto caster = _lhvm->Pop().uintVal;
	// const auto spellInstance = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmIsWindMagicAtPos() // 262 IS_WIND_MAGIC_AT_POS
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmCreateMist() // 263 CREATE_MIST
{
	// const auto heightRatio = _lhvm->Popf();
	// const auto transparency = _lhvm->Popf();
	// const auto b = _lhvm->Popf();
	// const auto g = _lhvm->Popf();
	// const auto r = _lhvm->Popf();
	// const auto scale = _lhvm->Popf();
	// const auto pos = PopVec();
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSetMistFade() // 264 SET_MIST_FADE
{
	// const auto duration = _lhvm->Popf();
	// const auto endTransparency = _lhvm->Popf();
	// const auto startTransparency = _lhvm->Popf();
	// const auto endScale = _lhvm->Popf();
	// const auto startScale = _lhvm->Popf();
	// const auto mist = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetObjectFade() // 265 GET_OBJECT_FADE
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmPlayHandDemo() // 266 PLAY_HAND_DEMO
{
	// const auto withoutHandModify = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto withPause = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto string = PopString();
	// TODO(Daniels118): implement this
}

void Game::VmIsPlayingHandDemo() // 267 IS_PLAYING_HAND_DEMO
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmGetArsePosition() // 268 GET_ARSE_POSITION
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::VmIsLeashedToObject() // 269 IS_LEASHED_TO_OBJECT
{
	// const auto target = _lhvm->Pop().uintVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmGetInteractionMagnitude() // 270 GET_INTERACTION_MAGNITUDE
{
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmIsCreatureAvailable() // 271 IS_CREATURE_AVAILABLE
{
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmCreateHighlight() // 272 CREATE_HIGHLIGHT
{
	// const auto challengeID = _lhvm->Pop().intVal;
	// const auto position = PopVec();
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmGetObjectHeld273() // 273 GET_OBJECT_HELD
{
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmGetActionCount() // 274 GET_ACTION_COUNT
{
	// const auto creature = _lhvm->Pop().uintVal;
	// const auto action = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetObjectLeashType() // 275 GET_OBJECT_LEASH_TYPE
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmSetFocusFollow() // 276 SET_FOCUS_FOLLOW
{
	// const auto target = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetPositionFollow() // 277 SET_POSITION_FOLLOW
{
	// const auto target = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetFocusAndPositionFollow() // 278 SET_FOCUS_AND_POSITION_FOLLOW
{
	// const auto distance = _lhvm->Popf();
	// const auto target = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetCameraLens() // 279 SET_CAMERA_LENS
{
	// const auto lens = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmMoveCameraLens() // 280 MOVE_CAMERA_LENS
{
	// const auto time = _lhvm->Popf();
	// const auto lens = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmCreatureReaction() // 281 CREATURE_REACTION
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmCreatureInDevScript() // 282 CREATURE_IN_DEV_SCRIPT
{
	// const auto creature = _lhvm->Pop().uintVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmStoreCameraDetails() // 283 STORE_CAMERA_DETAILS
{
	// TODO(Daniels118): implement this
}

void Game::VmRestoreCameraDetails() // 284 RESTORE_CAMERA_DETAILS
{
	// TODO(Daniels118): implement this
}

void Game::VmStartAngleSound285() // 285 START_ANGLE_SOUND
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetCameraPosFocLens() // 286 SET_CAMERA_POS_FOC_LENS
{
	// const auto unk6 = _lhvm->Pop().intVal;
	// const auto unk5 = _lhvm->Pop().intVal;
	// const auto unk4 = _lhvm->Pop().intVal;
	// const auto unk3 = _lhvm->Pop().intVal;
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmMoveCameraPosFocLens() // 287 MOVE_CAMERA_POS_FOC_LENS
{
	// const auto unk7 = _lhvm->Pop().intVal;
	// const auto unk6 = _lhvm->Pop().intVal;
	// const auto unk5 = _lhvm->Pop().intVal;
	// const auto unk4 = _lhvm->Pop().intVal;
	// const auto unk3 = _lhvm->Pop().intVal;
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGameTimeOnOff() // 288 GAME_TIME_ON_OFF
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmMoveGameTime() // 289 MOVE_GAME_TIME
{
	// const auto duration = _lhvm->Popf();
	// const auto hourOfTheDay = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmSetHighGraphicsDetail() // 290 SET_HIGH_GRAPHICS_DETAIL
{
	// const auto object = _lhvm->Pop().uintVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetSkeleton() // 291 SET_SKELETON
{
	// const auto object = _lhvm->Pop().uintVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmIsSkeleton() // 292 IS_SKELETON
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmPlayerSpellCastTime() // 293 PLAYER_SPELL_CAST_TIME
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmPlayerSpellLastCast() // 294 PLAYER_SPELL_LAST_CAST
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmGetLastSpellCastPos() // 295 GET_LAST_SPELL_CAST_POS
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::VmAddSpotVisualTargetPos() // 296 ADD_SPOT_VISUAL_TARGET_POS
{
	// const auto position = PopVec();
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmAddSpotVisualTargetObject() // 297 ADD_SPOT_VISUAL_TARGET_OBJECT
{
	// const auto target = _lhvm->Pop().uintVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetIndestructable() // 298 SET_INDESTRUCTABLE
{
	// const auto object = _lhvm->Pop().uintVal;
	// const auto indestructible = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetGraphicsClipping() // 299 SET_GRAPHICS_CLIPPING
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSpiritAppear() // 300 SPIRIT_APPEAR
{
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSpiritDisappear() // 301 SPIRIT_DISAPPEAR
{
	// const auto spirit = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetFocusOnObject() // 302 SET_FOCUS_ON_OBJECT
{
	// const auto target = _lhvm->Pop().uintVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmReleaseObjectFocus() // 303 RELEASE_OBJECT_FOCUS
{
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmImmersionExists() // 304 IMMERSION_EXISTS
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmSetDrawLeash() // 305 SET_DRAW_LEASH
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetDrawHighlight() // 306 SET_DRAW_HIGHLIGHT
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetOpenClose() // 307 SET_OPEN_CLOSE
{
	// const auto object = _lhvm->Pop().uintVal;
	// const auto open = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetIntroBuilding() // 308 SET_INTRO_BUILDING
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmCreatureForceFriends() // 309 CREATURE_FORCE_FRIENDS
{
	// const auto targetCreature = _lhvm->Pop().uintVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmMoveComputerPlayerPosition() // 310 MOVE_COMPUTER_PLAYER_POSITION
{
	// const auto withFixedHeight = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto speed = _lhvm->Popf();
	// const auto position = PopVec();
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmEnableDisableComputerPlayer311() // 311 ENABLE_DISABLE_COMPUTER_PLAYER
{
	// const auto player = _lhvm->Popf();
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmGetComputerPlayerPosition() // 312 GET_COMPUTER_PLAYER_POSITION
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::VmSetComputerPlayerPosition() // 313 SET_COMPUTER_PLAYER_POSITION
{
	// const auto withFixedHeight = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto position = PopVec();
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmGetStoredCameraPosition() // 314 GET_STORED_CAMERA_POSITION
{
	// TODO(Daniels118): implement this
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::VmGetStoredCameraFocus() // 315 GET_STORED_CAMERA_FOCUS
{
	// TODO(Daniels118): implement this
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::VmCallNearInState() // 316 CALL_NEAR_IN_STATE
{
	// const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto radius = _lhvm->Popf();
	// const auto position = PopVec();
	// const auto state = _lhvm->Pop().intVal;
	// const auto subtype = _lhvm->Pop().intVal;
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSetCreatureSound() // 317 SET_CREATURE_SOUND
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmCreatureInteractingWith() // 318 CREATURE_INTERACTING_WITH
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmSetSunDraw() // 319 SET_SUN_DRAW
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmObjectInfoBits() // 320 OBJECT_INFO_BITS
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmSetHurtByFire() // 321 SET_HURT_BY_FIRE
{
	// const auto object = _lhvm->Pop().uintVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmConfinedObject() // 322 CONFINED_OBJECT
{
	// const auto unk4 = _lhvm->Pop().intVal;
	// const auto unk3 = _lhvm->Pop().intVal;
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmClearConfinedObject() // 323 CLEAR_CONFINED_OBJECT
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetObjectFlock() // 324 GET_OBJECT_FLOCK
{
	// const auto member = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSetPlayerBelief() // 325 SET_PLAYER_BELIEF
{
	// const auto belief = _lhvm->Popf();
	// const auto player = _lhvm->Popf();
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmPlayJcSpecial() // 326 PLAY_JC_SPECIAL
{
	// const auto feature = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmIsPlayingJcSpecial() // 327 IS_PLAYING_JC_SPECIAL
{
	// const auto feature = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmVortexParameters() // 328 VORTEX_PARAMETERS
{
	// const auto flock = _lhvm->Pop().uintVal;
	// const auto radius = _lhvm->Popf();
	// const auto distance = _lhvm->Popf();
	// const auto position = PopVec();
	// const auto town = _lhvm->Pop().uintVal;
	// const auto vortex = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmLoadCreature() // 329 LOAD_CREATURE
{
	// const auto position = PopVec();
	// const auto player = _lhvm->Popf();
	// const auto mindFilename = PopString();
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmIsSpellCharging() // 330 IS_SPELL_CHARGING
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmIsThatSpellCharging() // 331 IS_THAT_SPELL_CHARGING
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmOpposingCreature() // 332 OPPOSING_CREATURE
{
	// const auto god = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmFlockWithinLimits() // 333 FLOCK_WITHIN_LIMITS
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmHighlightProperties() // 334 HIGHLIGHT_PROPERTIES
{
	// const auto category = _lhvm->Pop().intVal;
	// const auto text = _lhvm->Pop().intVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmLastMusicLine() // 335 LAST_MUSIC_LINE
{
	// const auto line = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmHandDemoTrigger() // 336 HAND_DEMO_TRIGGER
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmGetBellyPosition() // 337 GET_BELLY_POSITION
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::VmSetCreatureCreedProperties() // 338 SET_CREATURE_CREED_PROPERTIES
{
	// const auto time = _lhvm->Popf();
	// const auto power = _lhvm->Popf();
	// const auto scale = _lhvm->Popf();
	// const auto handGlow = _lhvm->Pop().intVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmGameThingCanViewCamera() // 339 GAME_THING_CAN_VIEW_CAMERA
{
	// const auto degrees = _lhvm->Popf();
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmGamePlaySaySoundEffect() // 340 GAME_PLAY_SAY_SOUND_EFFECT
{
	// const auto withPosition = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto position = PopVec();
	// const auto sound = _lhvm->Pop().intVal;
	// const auto extra = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetTownDesireBoost() // 341 SET_TOWN_DESIRE_BOOST
{
	// const auto boost = _lhvm->Popf();
	// const auto desire = _lhvm->Pop().intVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmIsLockedInteraction() // 342 IS_LOCKED_INTERACTION
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmSetCreatureName() // 343 SET_CREATURE_NAME
{
	// const auto textID = _lhvm->Pop().intVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmComputerPlayerReady() // 344 COMPUTER_PLAYER_READY
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmEnableDisableComputerPlayer345() // 345 ENABLE_DISABLE_COMPUTER_PLAYER
{
	// const auto player = _lhvm->Popf();
	// const auto pause = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmClearActorMind() // 346 CLEAR_ACTOR_MIND
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmEnterExitCitadel() // 347 ENTER_EXIT_CITADEL
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmStartAngleSound348() // 348 START_ANGLE_SOUND
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmThingJcSpecial() // 349 THING_JC_SPECIAL
{
	// const auto target = _lhvm->Pop().uintVal;
	// const auto feature = _lhvm->Pop().intVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmMusicPlayed350() // 350 MUSIC_PLAYED
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmUpdateSnapshotPicture() // 351 UPDATE_SNAPSHOT_PICTURE
{
	// const auto challengeID = _lhvm->Pop().intVal;
	// const auto takingPicture = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto titleStrID = _lhvm->Pop().intVal;
	// const auto alignment = _lhvm->Popf();
	// const auto success = _lhvm->Popf();
	// const auto focus = PopVec();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
}

void Game::VmStopScriptsInFilesExcluding() // 352 STOP_SCRIPTS_IN_FILES_EXCLUDING
{
	const auto scriptNames = PopString();
	const auto sourceFilenames = PopString();

	const auto names = GetUniqueWords(scriptNames);
	const auto filenames = GetUniqueWords(sourceFilenames);
	_lhvm->StopScripts([&names, &filenames](const std::string& name, const std::string& filename) -> bool {
		return filenames.contains(filename) && !names.contains(name);
	});
}

void Game::VmCreateRandomVillagerOfTribe() // 353 CREATE_RANDOM_VILLAGER_OF_TRIBE
{
	// const auto position = PopVec();
	// const auto tribe = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmToggleLeash() // 354 TOGGLE_LEASH
{
	// const auto player = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGameSetMana() // 355 GAME_SET_MANA
{
	// const auto mana = _lhvm->Popf();
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetMagicProperties() // 356 SET_MAGIC_PROPERTIES
{
	// const auto duration = _lhvm->Popf();
	// const auto magicType = _lhvm->Pop().intVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetGameSound() // 357 SET_GAME_SOUND
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSexIsMale() // 358 SEX_IS_MALE
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmGetFirstHelp() // 359 GET_FIRST_HELP
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetLastHelp() // 360 GET_LAST_HELP
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmIsActive() // 361 IS_ACTIVE
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmSetBookmarkPosition() // 362 SET_BOOKMARK_POSITION
{
	// const auto unk3 = _lhvm->Pop().intVal;
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetScaffoldProperties() // 363 SET_SCAFFOLD_PROPERTIES
{
	// const auto destroy = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto size = _lhvm->Popf();
	// const auto type = _lhvm->Pop().intVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetComputerPlayerPersonality() // 364 SET_COMPUTER_PLAYER_PERSONALITY
{
	// const auto probability = _lhvm->Popf();
	// const auto aspect = PopString();
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmSetComputerPlayerSuppression() // 365 SET_COMPUTER_PLAYER_SUPPRESSION
{
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmForceComputerPlayerAction() // 366 FORCE_COMPUTER_PLAYER_ACTION
{
	// const auto obj2 = _lhvm->Pop().uintVal;
	// const auto obj1 = _lhvm->Pop().uintVal;
	// const auto action = PopString();
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmQueueComputerPlayerAction() // 367 QUEUE_COMPUTER_PLAYER_ACTION
{
	// const auto obj2 = _lhvm->Pop().uintVal;
	// const auto obj1 = _lhvm->Pop().uintVal;
	// const auto action = PopString();
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmGetTownWithId() // 368 GET_TOWN_WITH_ID
{
	// const auto id = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSetDisciple() // 369 SET_DISCIPLE
{
	// const auto withSound = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto discipleType = _lhvm->Pop().intVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmReleaseComputerPlayer() // 370 RELEASE_COMPUTER_PLAYER
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmSetComputerPlayerSpeed() // 371 SET_COMPUTER_PLAYER_SPEED
{
	// const auto speed = _lhvm->Popf();
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmSetFocusFollowComputerPlayer() // 372 SET_FOCUS_FOLLOW_COMPUTER_PLAYER
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmSetPositionFollowComputerPlayer() // 373 SET_POSITION_FOLLOW_COMPUTER_PLAYER
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmCallComputerPlayer() // 374 CALL_COMPUTER_PLAYER
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmCallBuildingInTown() // 375 CALL_BUILDING_IN_TOWN
{
	// const auto unk3 = _lhvm->Pop().intVal;
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmSetCanBuildWorshipsite() // 376 SET_CAN_BUILD_WORSHIPSITE
{
	// const auto object = _lhvm->Pop().uintVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmGetFacingCameraPosition() // 377 GET_FACING_CAMERA_POSITION
{
	// const auto distance = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::VmSetComputerPlayerAttitude() // 378 SET_COMPUTER_PLAYER_ATTITUDE
{
	// const auto attitude = _lhvm->Popf();
	// const auto player2 = _lhvm->Popf();
	// const auto player1 = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmGetComputerPlayerAttitude() // 379 GET_COMPUTER_PLAYER_ATTITUDE
{
	// const auto player2 = _lhvm->Popf();
	// const auto player1 = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmLoadComputerPlayerPersonality() // 380 LOAD_COMPUTER_PLAYER_PERSONALITY
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSaveComputerPlayerPersonality() // 381 SAVE_COMPUTER_PLAYER_PERSONALITY
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetPlayerAlly() // 382 SET_PLAYER_ALLY
{
	// const auto percentage = _lhvm->Popf();
	// const auto player2 = _lhvm->Popf();
	// const auto player1 = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmCallFlying() // 383 CALL_FLYING
{
	// const auto excluding = static_cast<bool>(_lhvm->Pop().intVal);
	// const auto radius = _lhvm->Popf();
	// const auto position = PopVec();
	// const auto subtype = _lhvm->Pop().intVal;
	// const auto type = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSetObjectFadeIn() // 384 SET_OBJECT_FADE_IN
{
	// const auto time = _lhvm->Popf();
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmIsAffectedBySpell() // 385 IS_AFFECTED_BY_SPELL
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmSetMagicInObject() // 386 SET_MAGIC_IN_OBJECT
{
	// const auto object = _lhvm->Pop().uintVal;
	// const auto MAGIC_TYPE = _lhvm->Pop().intVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmIdAdultSize() // 387 ID_ADULT_SIZE
{
	// const auto container = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmObjectCapacity() // 388 OBJECT_CAPACITY
{
	// const auto container = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmObjectAdultCapacity() // 389 OBJECT_ADULT_CAPACITY
{
	// const auto container = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmSetCreatureAutoFighting() // 390 SET_CREATURE_AUTO_FIGHTING
{
	// const auto creature = _lhvm->Pop().uintVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmIsAutoFighting() // 391 IS_AUTO_FIGHTING
{
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmSetCreatureQueueFightMove() // 392 SET_CREATURE_QUEUE_FIGHT_MOVE
{
	// const auto move = _lhvm->Pop().intVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetCreatureQueueFightSpell() // 393 SET_CREATURE_QUEUE_FIGHT_SPELL
{
	// const auto spell = _lhvm->Pop().intVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetCreatureQueueFightStep() // 394 SET_CREATURE_QUEUE_FIGHT_STEP
{
	// const auto step = _lhvm->Pop().intVal;
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetCreatureFightAction() // 395 GET_CREATURE_FIGHT_ACTION
{
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmCreatureFightQueueHits() // 396 CREATURE_FIGHT_QUEUE_HITS
{
	// const auto creature = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmSquareRoot() // 397 SQUARE_ROOT
{
	// const auto value = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetPlayerAlly() // 398 GET_PLAYER_ALLY
{
	// const auto player2 = _lhvm->Popf();
	// const auto player1 = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmSetPlayerWindResistance() // 399 SET_PLAYER_WIND_RESISTANCE
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmGetPlayerWindResistance() // 400 GET_PLAYER_WIND_RESISTANCE
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmPauseUnpauseClimateSystem() // 401 PAUSE_UNPAUSE_CLIMATE_SYSTEM
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmPauseUnpauseStormCreationInClimateSystem() // 402 PAUSE_UNPAUSE_STORM_CREATION_IN_CLIMATE_SYSTEM
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmGetManaForSpell() // 403 GET_MANA_FOR_SPELL
{
	// const auto spell = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmKillStormsInArea() // 404 KILL_STORMS_IN_AREA
{
	// const auto radius = _lhvm->Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
}

void Game::VmInsideTemple() // 405 INSIDE_TEMPLE
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmRestartObject() // 406 RESTART_OBJECT
{
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetGameTimeProperties() // 407 SET_GAME_TIME_PROPERTIES
{
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmResetGameTimeProperties() // 408 RESET_GAME_TIME_PROPERTIES
{
	// TODO(Daniels118): implement this
}

void Game::VmSoundExists() // 409 SOUND_EXISTS
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmGetTownWorshipDeaths() // 410 GET_TOWN_WORSHIP_DEATHS
{
	// const auto town = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGameClearDialogue() // 411 GAME_CLEAR_DIALOGUE
{
	// TODO(Daniels118): implement this
}

void Game::VmGameCloseDialogue() // 412 GAME_CLOSE_DIALOGUE
{
	// TODO(Daniels118): implement this
}

void Game::VmGetHandState() // 413 GET_HAND_STATE
{
	// TODO(Daniels118): implement this
	_lhvm->Pushi(0);
}

void Game::VmSetInterfaceCitadel() // 414 SET_INTERFACE_CITADEL
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmMapScriptFunction() // 415 MAP_SCRIPT_FUNCTION
{
	// const auto command = PopString();
	// TODO(Daniels118): implement this
}

void Game::VmWithinRotation() // 416 WITHIN_ROTATION
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmGetPlayerTownTotal() // 417 GET_PLAYER_TOWN_TOTAL
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmSpiritScreenPoint() // 418 SPIRIT_SCREEN_POINT
{
	// const auto unk2 = _lhvm->Pop().intVal;
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmKeyDown() // 419 KEY_DOWN
{
	const auto key = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this translate key to physical key code
	_lhvm->Pushb(_keysdown.contains(key));
}

void Game::VmSetFightExit() // 420 SET_FIGHT_EXIT
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetObjectClicked() // 421 GET_OBJECT_CLICKED
{
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmGetMana() // 422 GET_MANA
{
	// const auto worshipSite = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmClearPlayerSpellCharging() // 423 CLEAR_PLAYER_SPELL_CHARGING
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmStopSoundEffect() // 424 STOP_SOUND_EFFECT
{
	// const auto soundbank = _lhvm->Pop().intVal;
	// const auto sound = _lhvm->Pop().intVal;
	// const auto alwaysFalse = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmGetTotemStatue() // 425 GET_TOTEM_STATUE
{
	// const auto town = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmSetSetOnFire() // 426 SET_SET_ON_FIRE
{
	// const auto object = _lhvm->Pop().uintVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmSetLandBalance() // 427 SET_LAND_BALANCE
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetObjectBeliefScale() // 428 SET_OBJECT_BELIEF_SCALE
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmStartImmersion() // 429 START_IMMERSION
{
	// const auto effect = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmStopImmersion() // 430 STOP_IMMERSION
{
	// const auto effect = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmStopAllImmersion() // 431 STOP_ALL_IMMERSION
{
	// TODO(Daniels118): implement this
}

void Game::VmSetCreatureInTemple() // 432 SET_CREATURE_IN_TEMPLE
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmGameDrawText() // 433 GAME_DRAW_TEXT
{
	// const auto fade = _lhvm->Popf();
	// const auto size = _lhvm->Popf();
	// const auto height = _lhvm->Popf();
	// const auto width = _lhvm->Popf();
	// const auto down = _lhvm->Popf();
	// const auto across = _lhvm->Popf();
	// const auto textID = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGameDrawTempText() // 434 GAME_DRAW_TEMP_TEXT
{
	// const auto fade = _lhvm->Popf();
	// const auto size = _lhvm->Popf();
	// const auto height = _lhvm->Popf();
	// const auto width = _lhvm->Popf();
	// const auto down = _lhvm->Popf();
	// const auto across = _lhvm->Popf();
	// const auto string = PopString();
	// TODO(Daniels118): implement this
}

void Game::VmFadeAllDrawText() // 435 FADE_ALL_DRAW_TEXT
{
	// const auto time = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmSetDrawTextColour() // 436 SET_DRAW_TEXT_COLOUR
{
	// const auto blue = _lhvm->Popf();
	// const auto green = _lhvm->Popf();
	// const auto red = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmSetClippingWindow() // 437 SET_CLIPPING_WINDOW
{
	// const auto time = _lhvm->Popf();
	// const auto height = _lhvm->Popf();
	// const auto width = _lhvm->Popf();
	// const auto down = _lhvm->Popf();
	// const auto across = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmClearClippingWindow() // 438 CLEAR_CLIPPING_WINDOW
{
	// const auto time = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmSaveGameInSlot() // 439 SAVE_GAME_IN_SLOT
{
	// const auto slot = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmSetObjectCarrying() // 440 SET_OBJECT_CARRYING
{
	// const auto carriedObj = _lhvm->Pop().intVal;
	// const auto object = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
}

void Game::VmPosValidForCreature() // 441 POS_VALID_FOR_CREATURE
{
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmGetTimeSinceObjectAttacked() // 442 GET_TIME_SINCE_OBJECT_ATTACKED
{
	// const auto town = _lhvm->Pop().uintVal;
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGetTownAndVillagerHealthTotal() // 443 GET_TOWN_AND_VILLAGER_HEALTH_TOTAL
{
	// const auto town = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGameAddForBuilding() // 444 GAME_ADD_FOR_BUILDING
{
	// const auto unk1 = _lhvm->Pop().intVal;
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmEnableDisableAlignmentMusic() // 445 ENABLE_DISABLE_ALIGNMENT_MUSIC
{
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmGetDeadLiving() // 446 GET_DEAD_LIVING
{
	// const auto radius = _lhvm->Popf();
	// const auto position = PopVec();
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmAttachSoundTag() // 447 ATTACH_SOUND_TAG
{
	// const auto target = _lhvm->Pop().uintVal;
	// const auto soundbank = _lhvm->Pop().intVal;
	// const auto sound = _lhvm->Pop().intVal;
	// const auto threeD = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmDetachSoundTag() // 448 DETACH_SOUND_TAG
{
	// const auto target = _lhvm->Pop().uintVal;
	// const auto soundbank = _lhvm->Pop().intVal;
	// const auto sound = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmGetSacrificeTotal() // 449 GET_SACRIFICE_TOTAL
{
	// const auto worshipSite = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushf(0.0f);
}

void Game::VmGameSoundPlaying() // 450 GAME_SOUND_PLAYING
{
	// const auto soundbank = _lhvm->Pop().intVal;
	// const auto sound = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmGetTemplePosition() // 451 GET_TEMPLE_POSITION
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::VmCreatureAutoscale() // 452 CREATURE_AUTOSCALE
{
	// const auto size = _lhvm->Popf();
	// const auto creature = _lhvm->Pop().uintVal;
	// const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
}

void Game::VmGetSpellIconInTemple() // 453 GET_SPELL_ICON_IN_TEMPLE
{
	// const auto temple = _lhvm->Pop().uintVal;
	// const auto spell = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmGameClearComputerPlayerActions() // 454 GAME_CLEAR_COMPUTER_PLAYER_ACTIONS
{
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
}

void Game::VmGetFirstInContainer() // 455 GET_FIRST_IN_CONTAINER
{
	// const auto container = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmGetNextInContainer() // 456 GET_NEXT_IN_CONTAINER
{
	// const auto after = _lhvm->Pop().uintVal;
	// const auto container = _lhvm->Pop().uintVal;
	// TODO(Daniels118): implement this
	_lhvm->Pusho(0);
}

void Game::VmGetTempleEntrancePosition() // 457 GET_TEMPLE_ENTRANCE_POSITION
{
	// const auto height = _lhvm->Popf();
	// const auto radius = _lhvm->Popf();
	// const auto player = _lhvm->Popf();
	// TODO(Daniels118): implement this
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::VmSaySoundEffectPlaying() // 458 SAY_SOUND_EFFECT_PLAYING
{
	// const auto sound = _lhvm->Pop().intVal;
	// const auto alwaysFalse = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmSetHandDemoKeys() // 459 SET_HAND_DEMO_KEYS
{
	// const auto unk0 = _lhvm->Pop().intVal;
	// TODO(Daniels118): implement this
}

void Game::VmCanSkipTutorial() // 460 CAN_SKIP_TUTORIAL
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmCanSkipCreatureTraining() // 461 CAN_SKIP_CREATURE_TRAINING
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmIsKeepingOldCreature() // 462 IS_KEEPING_OLD_CREATURE
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

void Game::VmCurrentProfileHasCreature() // 463 CURRENT_PROFILE_HAS_CREATURE
{
	// TODO(Daniels118): implement this
	_lhvm->Pushb(false);
}

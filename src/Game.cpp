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
	    fileSystem.GetPath<filesystem::Path::Citadel>() / "engine", false, [&meshManager](const std::filesystem::path& f) {
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
	    });

	pack::PackFile pack;
#if __ANDROID__
	//  Android has a complicated permissions API, must call java code to read contents.
	pack.Open(fileSystem.ReadAll(fileSystem.GetPath<Path::Data>() / "AllMeshes.g3d"));
#else
	pack.Open(fileSystem.GetPath<Path::Data>(true) / "AllMeshes.g3d");
#endif
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
#if __ANDROID__
	//  Android has a complicated permissions API, must call java code to read contents.
	animationPack.Open(fileSystem.ReadAll(fileSystem.GetPath<Path::Data>() / "AllAnims.anm"));
#else
	animationPack.Open(fileSystem.GetPath<Path::Data>(true) / "AllAnims.anm");
#endif
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
		    soundPack.Open(fileSystem.ReadAll(f));
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
		if (f.extension() == ".raw")
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

void Game::StopTaskCallback(const uint32_t taskNumber)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("lhvm"), "Task {} stopped", taskNumber);
}

void Game::LhvmErrorCallback(const LHVM::ErrorCode code, [[maybe_unused]] const std::string v0,
                             [[maybe_unused]] const uint32_t v1)
{
	const auto msg = LHVM::Error_Msg.at(static_cast<int>(code));
	SPDLOG_LOGGER_DEBUG(spdlog::get("lhvm"), "{}", msg);
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
	return glm::vec3(x, y, z);
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

std::vector<float_t> Game::PopVarArg(const int32_t argc)
{
	std::vector<float_t> vals;
	vals.resize(argc);
	for (int i = argc - 1; i >= 0; i--)
	{
		vals[i] = _lhvm->Popf();
	}
	return vals;
}

entt::entity Game::CreateScriptObject(const SCRIPT_OBJECT_TYPE type, uint32_t subtype, const glm::vec3& position,
									  float altitude, float xAngleRadians, float yAngleRadians, const float zAngleRadians,
									  const float scale)
{
	// TODO: handle all types
	switch (type)
	{

	case SCRIPT_OBJECT_TYPE::SCRIPT_OBJECT_TYPE_MOBILE_STATIC:
	case SCRIPT_OBJECT_TYPE::SCRIPT_OBJECT_TYPE_ROCK: // TODO: is this a MobileStatic???
		return MobileStaticArchetype::Create(position, static_cast<MobileStaticInfo>(subtype), altitude, xAngleRadians,
		                                     yAngleRadians, zAngleRadians, scale);

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

		_nativeCallEnterCallback = std::bind(&Game::NativeCallEnterCallback, this, std::placeholders::_1);
		_nativeCallExitCallback = std::bind(&Game::NativeCallExitCallback, this, std::placeholders::_1);
		_stopTaskCallback = std::bind(&Game::StopTaskCallback, this, std::placeholders::_1);
		_lhvmErrorCallback =
		    std::bind(&Game::LhvmErrorCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		_addObjectReference = std::bind(&Game::AddObjectReference, this, std::placeholders::_1);
		_removeObjectReference = std::bind(&Game::RemoveObjectReference, this, std::placeholders::_1);

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
	CREATE_FUNCTION_BINDING("NONE", 0, 0, Native000_NONE);
	CREATE_FUNCTION_BINDING("SET_CAMERA_POSITION", 3, 0, Native001_SET_CAMERA_POSITION);
	CREATE_FUNCTION_BINDING("SET_CAMERA_FOCUS", 3, 0, Native002_SET_CAMERA_FOCUS);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_POSITION", 4, 0, Native003_MOVE_CAMERA_POSITION);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_FOCUS", 4, 0, Native004_MOVE_CAMERA_FOCUS);
	CREATE_FUNCTION_BINDING("GET_CAMERA_POSITION", 0, 3, Native005_GET_CAMERA_POSITION);
	CREATE_FUNCTION_BINDING("GET_CAMERA_FOCUS", 0, 3, Native006_GET_CAMERA_FOCUS);
	CREATE_FUNCTION_BINDING("SPIRIT_EJECT", 1, 0, Native007_SPIRIT_EJECT);
	CREATE_FUNCTION_BINDING("SPIRIT_HOME", 1, 0, Native008_SPIRIT_HOME);
	CREATE_FUNCTION_BINDING("SPIRIT_POINT_POS", 5, 0, Native009_SPIRIT_POINT_POS);
	CREATE_FUNCTION_BINDING("SPIRIT_POINT_GAME_THING", 3, 0, Native010_SPIRIT_POINT_GAME_THING);
	CREATE_FUNCTION_BINDING("GAME_THING_FIELD_OF_VIEW", 1, 1, Native011_GAME_THING_FIELD_OF_VIEW);
	CREATE_FUNCTION_BINDING("POS_FIELD_OF_VIEW", 3, 1, Native012_POS_FIELD_OF_VIEW);
	CREATE_FUNCTION_BINDING("RUN_TEXT", 3, 0, Native013_RUN_TEXT);
	CREATE_FUNCTION_BINDING("TEMP_TEXT", 3, 0, Native014_TEMP_TEXT);
	CREATE_FUNCTION_BINDING("TEXT_READ", 0, 1, Native015_TEXT_READ);
	CREATE_FUNCTION_BINDING("GAME_THING_CLICKED", 1, 1, Native016_GAME_THING_CLICKED);
	CREATE_FUNCTION_BINDING("SET_SCRIPT_STATE", 2, 0, Native017_SET_SCRIPT_STATE);
	CREATE_FUNCTION_BINDING("SET_SCRIPT_STATE_POS", 4, 0, Native018_SET_SCRIPT_STATE_POS);
	CREATE_FUNCTION_BINDING("SET_SCRIPT_FLOAT", 2, 0, Native019_SET_SCRIPT_FLOAT);
	CREATE_FUNCTION_BINDING("SET_SCRIPT_ULONG", 3, 0, Native020_SET_SCRIPT_ULONG);
	CREATE_FUNCTION_BINDING("GET_PROPERTY", 2, 1, Native021_GET_PROPERTY);
	CREATE_FUNCTION_BINDING("SET_PROPERTY", 3, 0, Native022_SET_PROPERTY);
	CREATE_FUNCTION_BINDING("GET_POSITION", 1, 3, Native023_GET_POSITION);
	CREATE_FUNCTION_BINDING("SET_POSITION", 4, 0, Native024_SET_POSITION);
	CREATE_FUNCTION_BINDING("GET_DISTANCE", 6, 1, Native025_GET_DISTANCE);
	CREATE_FUNCTION_BINDING("CALL", 6, 1, Native026_CALL);
	CREATE_FUNCTION_BINDING("CREATE", 5, 1, Native027_CREATE);
	CREATE_FUNCTION_BINDING("RANDOM", 2, 1, Native028_RANDOM);
	CREATE_FUNCTION_BINDING("DLL_GETTIME", 0, 1, Native029_DLL_GETTIME);
	CREATE_FUNCTION_BINDING("START_CAMERA_CONTROL", 0, 1, Native030_START_CAMERA_CONTROL);
	CREATE_FUNCTION_BINDING("END_CAMERA_CONTROL", 0, 0, Native031_END_CAMERA_CONTROL);
	CREATE_FUNCTION_BINDING("SET_WIDESCREEN", 1, 0, Native032_SET_WIDESCREEN);
	CREATE_FUNCTION_BINDING("MOVE_GAME_THING", 5, 0, Native033_MOVE_GAME_THING);
	CREATE_FUNCTION_BINDING("SET_FOCUS", 4, 0, Native034_SET_FOCUS);
	CREATE_FUNCTION_BINDING("HAS_CAMERA_ARRIVED", 0, 1, Native035_HAS_CAMERA_ARRIVED);
	CREATE_FUNCTION_BINDING("FLOCK_CREATE", 3, 1, Native036_FLOCK_CREATE);
	CREATE_FUNCTION_BINDING("FLOCK_ATTACH", 3, 1, Native037_FLOCK_ATTACH);
	CREATE_FUNCTION_BINDING("FLOCK_DETACH", 2, 1, Native038_FLOCK_DETACH);
	CREATE_FUNCTION_BINDING("FLOCK_DISBAND", 1, 0, Native039_FLOCK_DISBAND);
	CREATE_FUNCTION_BINDING("ID_SIZE", 1, 1, Native040_ID_SIZE);
	CREATE_FUNCTION_BINDING("FLOCK_MEMBER", 2, 1, Native041_FLOCK_MEMBER);
	CREATE_FUNCTION_BINDING("GET_HAND_POSITION", 0, 3, Native042_GET_HAND_POSITION);
	CREATE_FUNCTION_BINDING("PLAY_SOUND_EFFECT", 6, 0, Native043_PLAY_SOUND_EFFECT);
	CREATE_FUNCTION_BINDING("START_MUSIC", 1, 0, Native044_START_MUSIC);
	CREATE_FUNCTION_BINDING("STOP_MUSIC", 0, 0, Native045_STOP_MUSIC);
	CREATE_FUNCTION_BINDING("ATTACH_MUSIC", 2, 0, Native046_ATTACH_MUSIC);
	CREATE_FUNCTION_BINDING("DETACH_MUSIC", 1, 0, Native047_DETACH_MUSIC);
	CREATE_FUNCTION_BINDING("OBJECT_DELETE", 2, 0, Native048_OBJECT_DELETE);
	CREATE_FUNCTION_BINDING("FOCUS_FOLLOW", 1, 0, Native049_FOCUS_FOLLOW);
	CREATE_FUNCTION_BINDING("POSITION_FOLLOW", 1, 0, Native050_POSITION_FOLLOW);
	CREATE_FUNCTION_BINDING("CALL_NEAR", 7, 1, Native051_CALL_NEAR);
	CREATE_FUNCTION_BINDING("SPECIAL_EFFECT_POSITION", 5, 1, Native052_SPECIAL_EFFECT_POSITION);
	CREATE_FUNCTION_BINDING("SPECIAL_EFFECT_OBJECT", 3, 1, Native053_SPECIAL_EFFECT_OBJECT);
	CREATE_FUNCTION_BINDING("DANCE_CREATE", 6, 1, Native054_DANCE_CREATE);
	CREATE_FUNCTION_BINDING("CALL_IN", 4, 1, Native055_CALL_IN);
	CREATE_FUNCTION_BINDING("CHANGE_INNER_OUTER_PROPERTIES", 4, 0, Native056_CHANGE_INNER_OUTER_PROPERTIES);
	CREATE_FUNCTION_BINDING("SNAPSHOT", -1, 0, Native057_SNAPSHOT);
	CREATE_FUNCTION_BINDING("GET_ALIGNMENT", 1, 1, Native058_GET_ALIGNMENT);
	CREATE_FUNCTION_BINDING("SET_ALIGNMENT", 2, 0, Native059_SET_ALIGNMENT);
	CREATE_FUNCTION_BINDING("INFLUENCE_OBJECT", 4, 1, Native060_INFLUENCE_OBJECT);
	CREATE_FUNCTION_BINDING("INFLUENCE_POSITION", 6, 1, Native061_INFLUENCE_POSITION);
	CREATE_FUNCTION_BINDING("GET_INFLUENCE", 5, 1, Native062_GET_INFLUENCE);
	CREATE_FUNCTION_BINDING("SET_INTERFACE_INTERACTION", 1, 0, Native063_SET_INTERFACE_INTERACTION);
	CREATE_FUNCTION_BINDING("PLAYED", 1, 1, Native064_PLAYED);
	CREATE_FUNCTION_BINDING("RANDOM_ULONG", 2, 1, Native065_RANDOM_ULONG);
	CREATE_FUNCTION_BINDING("SET_GAMESPEED", 1, 0, Native066_SET_GAMESPEED);
	CREATE_FUNCTION_BINDING("CALL_IN_NEAR", 8, 1, Native067_CALL_IN_NEAR);
	CREATE_FUNCTION_BINDING("OVERRIDE_STATE_ANIMATION", 2, 0, Native068_OVERRIDE_STATE_ANIMATION);
	CREATE_FUNCTION_BINDING("CREATURE_CREATE_RELATIVE_TO_CREATURE", 6, 1, Native069_CREATURE_CREATE_RELATIVE_TO_CREATURE);
	CREATE_FUNCTION_BINDING("CREATURE_LEARN_EVERYTHING", 1, 0, Native070_CREATURE_LEARN_EVERYTHING);
	CREATE_FUNCTION_BINDING("CREATURE_SET_KNOWS_ACTION", 4, 0, Native071_CREATURE_SET_KNOWS_ACTION);
	CREATE_FUNCTION_BINDING("CREATURE_SET_AGENDA_PRIORITY", 2, 0, Native072_CREATURE_SET_AGENDA_PRIORITY);
	CREATE_FUNCTION_BINDING("CREATURE_TURN_OFF_ALL_DESIRES", 1, 0, Native073_CREATURE_TURN_OFF_ALL_DESIRES);
	CREATE_FUNCTION_BINDING("CREATURE_LEARN_DISTINCTION_ABOUT_ACTIVITY_OBJECT", 4, 0,
	                        Native074_CREATURE_LEARN_DISTINCTION_ABOUT_ACTIVITY_OBJECT);
	CREATE_FUNCTION_BINDING("CREATURE_DO_ACTION", 4, 0, Native075_CREATURE_DO_ACTION);
	CREATE_FUNCTION_BINDING("IN_CREATURE_HAND", 2, 1, Native076_IN_CREATURE_HAND);
	CREATE_FUNCTION_BINDING("CREATURE_SET_DESIRE_VALUE", 3, 0, Native077_CREATURE_SET_DESIRE_VALUE);
	CREATE_FUNCTION_BINDING("CREATURE_SET_DESIRE_ACTIVATED", 3, 0, Native078_CREATURE_SET_DESIRE_ACTIVATED);
	CREATE_FUNCTION_BINDING("CREATURE_SET_DESIRE_ACTIVATED", 2, 0, Native079_CREATURE_SET_DESIRE_ACTIVATED);
	CREATE_FUNCTION_BINDING("CREATURE_SET_DESIRE_MAXIMUM", 3, 0, Native080_CREATURE_SET_DESIRE_MAXIMUM);
	CREATE_FUNCTION_BINDING("CONVERT_CAMERA_POSITION", 1, 3, Native081_CONVERT_CAMERA_POSITION);
	CREATE_FUNCTION_BINDING("CONVERT_CAMERA_FOCUS", 1, 3, Native082_CONVERT_CAMERA_FOCUS);
	CREATE_FUNCTION_BINDING("CREATURE_SET_PLAYER", 1, 0, Native083_CREATURE_SET_PLAYER);
	CREATE_FUNCTION_BINDING("START_COUNTDOWN_TIMER", 1, 0, Native084_START_COUNTDOWN_TIMER);
	CREATE_FUNCTION_BINDING("CREATURE_INITIALISE_NUM_TIMES_PERFORMED_ACTION", 2, 0,
	                        Native085_CREATURE_INITIALISE_NUM_TIMES_PERFORMED_ACTION);
	CREATE_FUNCTION_BINDING("CREATURE_GET_NUM_TIMES_ACTION_PERFORMED", 2, 1, Native086_CREATURE_GET_NUM_TIMES_ACTION_PERFORMED);
	CREATE_FUNCTION_BINDING("REMOVE_COUNTDOWN_TIMER", 0, 0, Native087_REMOVE_COUNTDOWN_TIMER);
	CREATE_FUNCTION_BINDING("GET_OBJECT_DROPPED", 1, 1, Native088_GET_OBJECT_DROPPED);
	CREATE_FUNCTION_BINDING("CLEAR_DROPPED_BY_OBJECT", 1, 0, Native089_CLEAR_DROPPED_BY_OBJECT);
	CREATE_FUNCTION_BINDING("CREATE_REACTION", 2, 0, Native090_CREATE_REACTION);
	CREATE_FUNCTION_BINDING("REMOVE_REACTION", 1, 0, Native091_REMOVE_REACTION);
	CREATE_FUNCTION_BINDING("GET_COUNTDOWN_TIMER", 0, 1, Native092_GET_COUNTDOWN_TIMER);
	CREATE_FUNCTION_BINDING("START_DUAL_CAMERA", 2, 0, Native093_START_DUAL_CAMERA);
	CREATE_FUNCTION_BINDING("UPDATE_DUAL_CAMERA", 2, 0, Native094_UPDATE_DUAL_CAMERA);
	CREATE_FUNCTION_BINDING("RELEASE_DUAL_CAMERA", 0, 0, Native095_RELEASE_DUAL_CAMERA);
	CREATE_FUNCTION_BINDING("SET_CREATURE_HELP", 1, 0, Native096_SET_CREATURE_HELP);
	CREATE_FUNCTION_BINDING("GET_TARGET_OBJECT", 1, 1, Native097_GET_TARGET_OBJECT);
	CREATE_FUNCTION_BINDING("CREATURE_DESIRE_IS", 2, 1, Native098_CREATURE_DESIRE_IS);
	CREATE_FUNCTION_BINDING("COUNTDOWN_TIMER_EXISTS", 0, 1, Native099_COUNTDOWN_TIMER_EXISTS);
	CREATE_FUNCTION_BINDING("LOOK_GAME_THING", 2, 0, Native100_LOOK_GAME_THING);
	CREATE_FUNCTION_BINDING("GET_OBJECT_DESTINATION", 1, 3, Native101_GET_OBJECT_DESTINATION);
	CREATE_FUNCTION_BINDING("CREATURE_FORCE_FINISH", 1, 0, Native102_CREATURE_FORCE_FINISH);
	CREATE_FUNCTION_BINDING("HIDE_COUNTDOWN_TIMER", 0, 0, Native103_HIDE_COUNTDOWN_TIMER);
	CREATE_FUNCTION_BINDING("GET_ACTION_TEXT_FOR_OBJECT", 1, 1, Native104_GET_ACTION_TEXT_FOR_OBJECT);
	CREATE_FUNCTION_BINDING("CREATE_DUAL_CAMERA_WITH_POINT", 4, 0, Native105_CREATE_DUAL_CAMERA_WITH_POINT);
	CREATE_FUNCTION_BINDING("SET_CAMERA_TO_FACE_OBJECT", 2, 0, Native106_SET_CAMERA_TO_FACE_OBJECT);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_TO_FACE_OBJECT", 3, 0, Native107_MOVE_CAMERA_TO_FACE_OBJECT);
	CREATE_FUNCTION_BINDING("GET_MOON_PERCENTAGE", 0, 1, Native108_GET_MOON_PERCENTAGE);
	CREATE_FUNCTION_BINDING("POPULATE_CONTAINER", 4, 0, Native109_POPULATE_CONTAINER);
	CREATE_FUNCTION_BINDING("ADD_REFERENCE", 1, 1, Native110_ADD_REFERENCE);
	CREATE_FUNCTION_BINDING("REMOVE_REFERENCE", 1, 1, Native111_REMOVE_REFERENCE);
	CREATE_FUNCTION_BINDING("SET_GAME_TIME", 1, 0, Native112_SET_GAME_TIME);
	CREATE_FUNCTION_BINDING("GET_GAME_TIME", 0, 1, Native113_GET_GAME_TIME);
	CREATE_FUNCTION_BINDING("GET_REAL_TIME", 0, 1, Native114_GET_REAL_TIME);
	CREATE_FUNCTION_BINDING("GET_REAL_DAY", 0, 1, Native115_GET_REAL_DAY);
	CREATE_FUNCTION_BINDING("GET_REAL_DAY", 0, 1, Native116_GET_REAL_DAY);
	CREATE_FUNCTION_BINDING("GET_REAL_MONTH", 0, 1, Native117_GET_REAL_MONTH);
	CREATE_FUNCTION_BINDING("GET_REAL_YEAR", 0, 1, Native118_GET_REAL_YEAR);
	CREATE_FUNCTION_BINDING("RUN_CAMERA_PATH", 1, 0, Native119_RUN_CAMERA_PATH);
	CREATE_FUNCTION_BINDING("START_DIALOGUE", 0, 1, Native120_START_DIALOGUE);
	CREATE_FUNCTION_BINDING("END_DIALOGUE", 0, 0, Native121_END_DIALOGUE);
	CREATE_FUNCTION_BINDING("IS_DIALOGUE_READY", 0, 1, Native122_IS_DIALOGUE_READY);
	CREATE_FUNCTION_BINDING("CHANGE_WEATHER_PROPERTIES", 6, 0, Native123_CHANGE_WEATHER_PROPERTIES);
	CREATE_FUNCTION_BINDING("CHANGE_LIGHTNING_PROPERTIES", 5, 0, Native124_CHANGE_LIGHTNING_PROPERTIES);
	CREATE_FUNCTION_BINDING("CHANGE_TIME_FADE_PROPERTIES", 3, 0, Native125_CHANGE_TIME_FADE_PROPERTIES);
	CREATE_FUNCTION_BINDING("CHANGE_CLOUD_PROPERTIES", 4, 0, Native126_CHANGE_CLOUD_PROPERTIES);
	CREATE_FUNCTION_BINDING("SET_HEADING_AND_SPEED", 5, 0, Native127_SET_HEADING_AND_SPEED);
	CREATE_FUNCTION_BINDING("START_GAME_SPEED", 0, 0, Native128_START_GAME_SPEED);
	CREATE_FUNCTION_BINDING("END_GAME_SPEED", 0, 0, Native129_END_GAME_SPEED);
	CREATE_FUNCTION_BINDING("BUILD_BUILDING", 4, 0, Native130_BUILD_BUILDING);
	CREATE_FUNCTION_BINDING("SET_AFFECTED_BY_WIND", 2, 0, Native131_SET_AFFECTED_BY_WIND);
	CREATE_FUNCTION_BINDING("WIDESCREEN_TRANSISTION_FINISHED", 0, 1, Native132_WIDESCREEN_TRANSISTION_FINISHED);
	CREATE_FUNCTION_BINDING("GET_RESOURCE", 2, 1, Native133_GET_RESOURCE);
	CREATE_FUNCTION_BINDING("ADD_RESOURCE", 3, 1, Native134_ADD_RESOURCE);
	CREATE_FUNCTION_BINDING("REMOVE_RESOURCE", 3, 1, Native135_REMOVE_RESOURCE);
	CREATE_FUNCTION_BINDING("GET_TARGET_RELATIVE_POS", 8, 3, Native136_GET_TARGET_RELATIVE_POS);
	CREATE_FUNCTION_BINDING("STOP_POINTING", 1, 0, Native137_STOP_POINTING);
	CREATE_FUNCTION_BINDING("STOP_LOOKING", 1, 0, Native138_STOP_LOOKING);
	CREATE_FUNCTION_BINDING("LOOK_AT_POSITION", 4, 0, Native139_LOOK_AT_POSITION);
	CREATE_FUNCTION_BINDING("PLAY_SPIRIT_ANIM", 5, 0, Native140_PLAY_SPIRIT_ANIM);
	CREATE_FUNCTION_BINDING("CALL_IN_NOT_NEAR", 8, 1, Native141_CALL_IN_NOT_NEAR);
	CREATE_FUNCTION_BINDING("SET_CAMERA_ZONE", 1, 0, Native142_SET_CAMERA_ZONE);
	CREATE_FUNCTION_BINDING("GET_OBJECT_STATE", 1, 1, Native143_GET_OBJECT_STATE);
	CREATE_FUNCTION_BINDING("REVEAL_COUNTDOWN_TIMER", 0, 0, Native144_REVEAL_COUNTDOWN_TIMER);
	CREATE_FUNCTION_BINDING("SET_TIMER_TIME", 2, 0, Native145_SET_TIMER_TIME);
	CREATE_FUNCTION_BINDING("CREATE_TIMER", 1, 1, Native146_CREATE_TIMER);
	CREATE_FUNCTION_BINDING("GET_TIMER_TIME_REMAINING", 1, 1, Native147_GET_TIMER_TIME_REMAINING);
	CREATE_FUNCTION_BINDING("GET_TIMER_TIME_SINCE_SET", 1, 1, Native148_GET_TIMER_TIME_SINCE_SET);
	CREATE_FUNCTION_BINDING("MOVE_MUSIC", 2, 0, Native149_MOVE_MUSIC);
	CREATE_FUNCTION_BINDING("GET_INCLUSION_DISTANCE", 0, 1, Native150_GET_INCLUSION_DISTANCE);
	CREATE_FUNCTION_BINDING("GET_LAND_HEIGHT", 3, 1, Native151_GET_LAND_HEIGHT);
	CREATE_FUNCTION_BINDING("LOAD_MAP", 1, 0, Native152_LOAD_MAP);
	CREATE_FUNCTION_BINDING("STOP_ALL_SCRIPTS_EXCLUDING", 1, 0, Native153_STOP_ALL_SCRIPTS_EXCLUDING);
	CREATE_FUNCTION_BINDING("STOP_ALL_SCRIPTS_IN_FILES_EXCLUDING", 1, 0, Native154_STOP_ALL_SCRIPTS_IN_FILES_EXCLUDING);
	CREATE_FUNCTION_BINDING("STOP_SCRIPT", 1, 0, Native155_STOP_SCRIPT);
	CREATE_FUNCTION_BINDING("CLEAR_CLICKED_OBJECT", 0, 0, Native156_CLEAR_CLICKED_OBJECT);
	CREATE_FUNCTION_BINDING("CLEAR_CLICKED_POSITION", 0, 0, Native157_CLEAR_CLICKED_POSITION);
	CREATE_FUNCTION_BINDING("POSITION_CLICKED", 4, 1, Native158_POSITION_CLICKED);
	CREATE_FUNCTION_BINDING("RELEASE_FROM_SCRIPT", 1, 0, Native159_RELEASE_FROM_SCRIPT);
	CREATE_FUNCTION_BINDING("GET_OBJECT_HAND_IS_OVER", 0, 1, Native160_GET_OBJECT_HAND_IS_OVER);
	CREATE_FUNCTION_BINDING("ID_POISONED_SIZE", 1, 1, Native161_ID_POISONED_SIZE);
	CREATE_FUNCTION_BINDING("IS_POISONED", 1, 1, Native162_IS_POISONED);
	CREATE_FUNCTION_BINDING("CALL_POISONED_IN", 4, 1, Native163_CALL_POISONED_IN);
	CREATE_FUNCTION_BINDING("CALL_NOT_POISONED_IN", 4, 1, Native164_CALL_NOT_POISONED_IN);
	CREATE_FUNCTION_BINDING("SPIRIT_PLAYED", 1, 1, Native165_SPIRIT_PLAYED);
	CREATE_FUNCTION_BINDING("CLING_SPIRIT", 3, 0, Native166_CLING_SPIRIT);
	CREATE_FUNCTION_BINDING("FLY_SPIRIT", 3, 0, Native167_FLY_SPIRIT);
	CREATE_FUNCTION_BINDING("SET_ID_MOVEABLE", 2, 0, Native168_SET_ID_MOVEABLE);
	CREATE_FUNCTION_BINDING("SET_ID_PICKUPABLE", 2, 0, Native169_SET_ID_PICKUPABLE);
	CREATE_FUNCTION_BINDING("IS_ON_FIRE", 1, 1, Native170_IS_ON_FIRE);
	CREATE_FUNCTION_BINDING("IS_FIRE_NEAR", 4, 1, Native171_IS_FIRE_NEAR);
	CREATE_FUNCTION_BINDING("STOP_SCRIPTS_IN_FILES", 1, 0, Native172_STOP_SCRIPTS_IN_FILES);
	CREATE_FUNCTION_BINDING("SET_POISONED", 2, 0, Native173_SET_POISONED);
	CREATE_FUNCTION_BINDING("SET_TEMPERATURE", 2, 0, Native174_SET_TEMPERATURE);
	CREATE_FUNCTION_BINDING("SET_ON_FIRE", 3, 0, Native175_SET_ON_FIRE);
	CREATE_FUNCTION_BINDING("SET_TARGET", 5, 0, Native176_SET_TARGET);
	CREATE_FUNCTION_BINDING("WALK_PATH", 5, 0, Native177_WALK_PATH);
	CREATE_FUNCTION_BINDING("FOCUS_AND_POSITION_FOLLOW", 2, 0, Native178_FOCUS_AND_POSITION_FOLLOW);
	CREATE_FUNCTION_BINDING("GET_WALK_PATH_PERCENTAGE", 1, 1, Native179_GET_WALK_PATH_PERCENTAGE);
	CREATE_FUNCTION_BINDING("CAMERA_PROPERTIES", 4, 0, Native180_CAMERA_PROPERTIES);
	CREATE_FUNCTION_BINDING("ENABLE_DISABLE_MUSIC", 2, 0, Native181_ENABLE_DISABLE_MUSIC);
	CREATE_FUNCTION_BINDING("GET_MUSIC_OBJ_DISTANCE", 1, 1, Native182_GET_MUSIC_OBJ_DISTANCE);
	CREATE_FUNCTION_BINDING("GET_MUSIC_ENUM_DISTANCE", 1, 1, Native183_GET_MUSIC_ENUM_DISTANCE);
	CREATE_FUNCTION_BINDING("SET_MUSIC_PLAY_POSITION", 4, 0, Native184_SET_MUSIC_PLAY_POSITION);
	CREATE_FUNCTION_BINDING("ATTACH_OBJECT_LEASH_TO_OBJECT", 2, 0, Native185_ATTACH_OBJECT_LEASH_TO_OBJECT);
	CREATE_FUNCTION_BINDING("ATTACH_OBJECT_LEASH_TO_HAND", 1, 0, Native186_ATTACH_OBJECT_LEASH_TO_HAND);
	CREATE_FUNCTION_BINDING("DETACH_OBJECT_LEASH", 1, 0, Native187_DETACH_OBJECT_LEASH);
	CREATE_FUNCTION_BINDING("SET_CREATURE_ONLY_DESIRE", 3, 0, Native188_SET_CREATURE_ONLY_DESIRE);
	CREATE_FUNCTION_BINDING("SET_CREATURE_ONLY_DESIRE_OFF", 1, 0, Native189_SET_CREATURE_ONLY_DESIRE_OFF);
	CREATE_FUNCTION_BINDING("RESTART_MUSIC", 1, 0, Native190_RESTART_MUSIC);
	CREATE_FUNCTION_BINDING("MUSIC_PLAYED", 1, 1, Native191_MUSIC_PLAYED);
	CREATE_FUNCTION_BINDING("IS_OF_TYPE", 3, 1, Native192_IS_OF_TYPE);
	CREATE_FUNCTION_BINDING("CLEAR_HIT_OBJECT", 0, 0, Native193_CLEAR_HIT_OBJECT);
	CREATE_FUNCTION_BINDING("GAME_THING_HIT", 1, 1, Native194_GAME_THING_HIT);
	CREATE_FUNCTION_BINDING("SPELL_AT_THING", 8, 1, Native195_SPELL_AT_THING);
	CREATE_FUNCTION_BINDING("SPELL_AT_POS", 10, 1, Native196_SPELL_AT_POS);
	CREATE_FUNCTION_BINDING("CALL_PLAYER_CREATURE", 1, 1, Native197_CALL_PLAYER_CREATURE);
	CREATE_FUNCTION_BINDING("GET_SLOWEST_SPEED", 1, 1, Native198_GET_SLOWEST_SPEED);
	CREATE_FUNCTION_BINDING("GET_OBJECT_HELD", 0, 1, Native199_GET_OBJECT_HELD);
	CREATE_FUNCTION_BINDING("HELP_SYSTEM_ON", 0, 1, Native200_HELP_SYSTEM_ON);
	CREATE_FUNCTION_BINDING("SHAKE_CAMERA", 6, 0, Native201_SHAKE_CAMERA);
	CREATE_FUNCTION_BINDING("SET_ANIMATION_MODIFY", 2, 0, Native202_SET_ANIMATION_MODIFY);
	CREATE_FUNCTION_BINDING("SET_AVI_SEQUENCE", 2, 0, Native203_SET_AVI_SEQUENCE);
	CREATE_FUNCTION_BINDING("PLAY_GESTURE", 5, 0, Native204_PLAY_GESTURE);
	CREATE_FUNCTION_BINDING("DEV_FUNCTION", 1, 0, Native205_DEV_FUNCTION);
	CREATE_FUNCTION_BINDING("HAS_MOUSE_WHEEL", 0, 1, Native206_HAS_MOUSE_WHEEL);
	CREATE_FUNCTION_BINDING("NUM_MOUSE_BUTTONS", 0, 1, Native207_NUM_MOUSE_BUTTONS);
	CREATE_FUNCTION_BINDING("SET_CREATURE_DEV_STAGE", 2, 0, Native208_SET_CREATURE_DEV_STAGE);
	CREATE_FUNCTION_BINDING("SET_FIXED_CAM_ROTATION", 4, 0, Native209_SET_FIXED_CAM_ROTATION);
	CREATE_FUNCTION_BINDING("SWAP_CREATURE", 2, 0, Native210_SWAP_CREATURE);
	CREATE_FUNCTION_BINDING("GET_ARENA", 5, 1, Native211_GET_ARENA);
	CREATE_FUNCTION_BINDING("GET_FOOTBALL_PITCH", 1, 1, Native212_GET_FOOTBALL_PITCH);
	CREATE_FUNCTION_BINDING("STOP_ALL_GAMES", 1, 0, Native213_STOP_ALL_GAMES);
	CREATE_FUNCTION_BINDING("ATTACH_TO_GAME", 3, 0, Native214_ATTACH_TO_GAME);
	CREATE_FUNCTION_BINDING("DETACH_FROM_GAME", 3, 0, Native215_DETACH_FROM_GAME);
	CREATE_FUNCTION_BINDING("DETACH_UNDEFINED_FROM_GAME", 2, 0, Native216_DETACH_UNDEFINED_FROM_GAME);
	CREATE_FUNCTION_BINDING("SET_ONLY_FOR_SCRIPTS", 2, 0, Native217_SET_ONLY_FOR_SCRIPTS);
	CREATE_FUNCTION_BINDING("START_MATCH_WITH_REFEREE", 2, 0, Native218_START_MATCH_WITH_REFEREE);
	CREATE_FUNCTION_BINDING("GAME_TEAM_SIZE", 2, 0, Native219_GAME_TEAM_SIZE);
	CREATE_FUNCTION_BINDING("GAME_TYPE", 1, 1, Native220_GAME_TYPE);
	CREATE_FUNCTION_BINDING("GAME_SUB_TYPE", 1, 1, Native221_GAME_SUB_TYPE);
	CREATE_FUNCTION_BINDING("IS_LEASHED", 1, 1, Native222_IS_LEASHED);
	CREATE_FUNCTION_BINDING("SET_CREATURE_HOME", 4, 0, Native223_SET_CREATURE_HOME);
	CREATE_FUNCTION_BINDING("GET_HIT_OBJECT", 0, 1, Native224_GET_HIT_OBJECT);
	CREATE_FUNCTION_BINDING("GET_OBJECT_WHICH_HIT", 0, 1, Native225_GET_OBJECT_WHICH_HIT);
	CREATE_FUNCTION_BINDING("GET_NEAREST_TOWN_OF_PLAYER", 5, 1, Native226_GET_NEAREST_TOWN_OF_PLAYER);
	CREATE_FUNCTION_BINDING("SPELL_AT_POINT", 5, 1, Native227_SPELL_AT_POINT);
	CREATE_FUNCTION_BINDING("SET_ATTACK_OWN_TOWN", 2, 0, Native228_SET_ATTACK_OWN_TOWN);
	CREATE_FUNCTION_BINDING("IS_FIGHTING", 1, 1, Native229_IS_FIGHTING);
	CREATE_FUNCTION_BINDING("SET_MAGIC_RADIUS", 2, 0, Native230_SET_MAGIC_RADIUS);
	CREATE_FUNCTION_BINDING("TEMP_TEXT_WITH_NUMBER", 4, 0, Native231_TEMP_TEXT_WITH_NUMBER);
	CREATE_FUNCTION_BINDING("RUN_TEXT_WITH_NUMBER", 4, 0, Native232_RUN_TEXT_WITH_NUMBER);
	CREATE_FUNCTION_BINDING("CREATURE_SPELL_REVERSION", 2, 0, Native233_CREATURE_SPELL_REVERSION);
	CREATE_FUNCTION_BINDING("GET_DESIRE", 2, 1, Native234_GET_DESIRE);
	CREATE_FUNCTION_BINDING("GET_EVENTS_PER_SECOND", 1, 1, Native235_GET_EVENTS_PER_SECOND);
	CREATE_FUNCTION_BINDING("GET_TIME_SINCE", 1, 1, Native236_GET_TIME_SINCE);
	CREATE_FUNCTION_BINDING("GET_TOTAL_EVENTS", 1, 1, Native237_GET_TOTAL_EVENTS);
	CREATE_FUNCTION_BINDING("UPDATE_SNAPSHOT", -1, 0, Native238_UPDATE_SNAPSHOT);
	CREATE_FUNCTION_BINDING("CREATE_REWARD", 5, 1, Native239_CREATE_REWARD);
	CREATE_FUNCTION_BINDING("CREATE_REWARD_IN_TOWN", 6, 1, Native240_CREATE_REWARD_IN_TOWN);
	CREATE_FUNCTION_BINDING("SET_FADE", 4, 0, Native241_SET_FADE);
	CREATE_FUNCTION_BINDING("SET_FADE_IN", 1, 0, Native242_SET_FADE_IN);
	CREATE_FUNCTION_BINDING("FADE_FINISHED", 0, 1, Native243_FADE_FINISHED);
	CREATE_FUNCTION_BINDING("SET_PLAYER_MAGIC", 3, 0, Native244_SET_PLAYER_MAGIC);
	CREATE_FUNCTION_BINDING("HAS_PLAYER_MAGIC", 2, 1, Native245_HAS_PLAYER_MAGIC);
	CREATE_FUNCTION_BINDING("SPIRIT_SPEAKS", 2, 1, Native246_SPIRIT_SPEAKS);
	CREATE_FUNCTION_BINDING("BELIEF_FOR_PLAYER", 2, 1, Native247_BELIEF_FOR_PLAYER);
	CREATE_FUNCTION_BINDING("GET_HELP", 1, 1, Native248_GET_HELP);
	CREATE_FUNCTION_BINDING("SET_LEASH_WORKS", 2, 0, Native249_SET_LEASH_WORKS);
	CREATE_FUNCTION_BINDING("LOAD_MY_CREATURE", 3, 0, Native250_LOAD_MY_CREATURE);
	CREATE_FUNCTION_BINDING("OBJECT_RELATIVE_BELIEF", 3, 0, Native251_OBJECT_RELATIVE_BELIEF);
	CREATE_FUNCTION_BINDING("CREATE_WITH_ANGLE_AND_SCALE", 7, 1, Native252_CREATE_WITH_ANGLE_AND_SCALE);
	CREATE_FUNCTION_BINDING("SET_HELP_SYSTEM", 1, 0, Native253_SET_HELP_SYSTEM);
	CREATE_FUNCTION_BINDING("SET_VIRTUAL_INFLUENCE", 2, 0, Native254_SET_VIRTUAL_INFLUENCE);
	CREATE_FUNCTION_BINDING("SET_ACTIVE", 2, 0, Native255_SET_ACTIVE);
	CREATE_FUNCTION_BINDING("THING_VALID", 1, 1, Native256_THING_VALID);
	CREATE_FUNCTION_BINDING("VORTEX_FADE_OUT", 1, 0, Native257_VORTEX_FADE_OUT);
	CREATE_FUNCTION_BINDING("REMOVE_REACTION_OF_TYPE", 2, 0, Native258_REMOVE_REACTION_OF_TYPE);
	CREATE_FUNCTION_BINDING("CREATURE_LEARN_EVERYTHING_EXCLUDING", 2, 0, Native259_CREATURE_LEARN_EVERYTHING_EXCLUDING);
	CREATE_FUNCTION_BINDING("PLAYED_PERCENTAGE", 1, 1, Native260_PLAYED_PERCENTAGE);
	CREATE_FUNCTION_BINDING("OBJECT_CAST_BY_OBJECT", 2, 1, Native261_OBJECT_CAST_BY_OBJECT);
	CREATE_FUNCTION_BINDING("IS_WIND_MAGIC_AT_POS", 1, 1, Native262_IS_WIND_MAGIC_AT_POS);
	CREATE_FUNCTION_BINDING("CREATE_MIST", 9, 1, Native263_CREATE_MIST);
	CREATE_FUNCTION_BINDING("SET_MIST_FADE", 6, 0, Native264_SET_MIST_FADE);
	CREATE_FUNCTION_BINDING("GET_OBJECT_FADE", 1, 1, Native265_GET_OBJECT_FADE);
	CREATE_FUNCTION_BINDING("PLAY_HAND_DEMO", 3, 0, Native266_PLAY_HAND_DEMO);
	CREATE_FUNCTION_BINDING("IS_PLAYING_HAND_DEMO", 0, 1, Native267_IS_PLAYING_HAND_DEMO);
	CREATE_FUNCTION_BINDING("GET_ARSE_POSITION", 1, 3, Native268_GET_ARSE_POSITION);
	CREATE_FUNCTION_BINDING("IS_LEASHED_TO_OBJECT", 2, 1, Native269_IS_LEASHED_TO_OBJECT);
	CREATE_FUNCTION_BINDING("GET_INTERACTION_MAGNITUDE", 1, 1, Native270_GET_INTERACTION_MAGNITUDE);
	CREATE_FUNCTION_BINDING("IS_CREATURE_AVAILABLE", 1, 1, Native271_IS_CREATURE_AVAILABLE);
	CREATE_FUNCTION_BINDING("CREATE_HIGHLIGHT", 5, 1, Native272_CREATE_HIGHLIGHT);
	CREATE_FUNCTION_BINDING("GET_OBJECT_HELD", 1, 1, Native273_GET_OBJECT_HELD);
	CREATE_FUNCTION_BINDING("GET_ACTION_COUNT", 2, 1, Native274_GET_ACTION_COUNT);
	CREATE_FUNCTION_BINDING("GET_OBJECT_LEASH_TYPE", 1, 1, Native275_GET_OBJECT_LEASH_TYPE);
	CREATE_FUNCTION_BINDING("SET_FOCUS_FOLLOW", 1, 0, Native276_SET_FOCUS_FOLLOW);
	CREATE_FUNCTION_BINDING("SET_POSITION_FOLLOW", 1, 0, Native277_SET_POSITION_FOLLOW);
	CREATE_FUNCTION_BINDING("SET_FOCUS_AND_POSITION_FOLLOW", 2, 0, Native278_SET_FOCUS_AND_POSITION_FOLLOW);
	CREATE_FUNCTION_BINDING("SET_CAMERA_LENS", 1, 0, Native279_SET_CAMERA_LENS);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_LENS", 2, 0, Native280_MOVE_CAMERA_LENS);
	CREATE_FUNCTION_BINDING("CREATURE_REACTION", 2, 0, Native281_CREATURE_REACTION);
	CREATE_FUNCTION_BINDING("CREATURE_IN_DEV_SCRIPT", 2, 0, Native282_CREATURE_IN_DEV_SCRIPT);
	CREATE_FUNCTION_BINDING("STORE_CAMERA_DETAILS", 0, 0, Native283_STORE_CAMERA_DETAILS);
	CREATE_FUNCTION_BINDING("RESTORE_CAMERA_DETAILS", 0, 0, Native284_RESTORE_CAMERA_DETAILS);
	CREATE_FUNCTION_BINDING("START_ANGLE_SOUND", 1, 0, Native285_START_ANGLE_SOUND);
	CREATE_FUNCTION_BINDING("SET_CAMERA_POS_FOC_LENS", 7, 0, Native286_SET_CAMERA_POS_FOC_LENS);
	CREATE_FUNCTION_BINDING("MOVE_CAMERA_POS_FOC_LENS", 8, 0, Native287_MOVE_CAMERA_POS_FOC_LENS);
	CREATE_FUNCTION_BINDING("GAME_TIME_ON_OFF", 1, 0, Native288_GAME_TIME_ON_OFF);
	CREATE_FUNCTION_BINDING("MOVE_GAME_TIME", 2, 0, Native289_MOVE_GAME_TIME);
	CREATE_FUNCTION_BINDING("SET_HIGH_GRAPHICS_DETAIL", 2, 0, Native290_SET_HIGH_GRAPHICS_DETAIL);
	CREATE_FUNCTION_BINDING("SET_SKELETON", 2, 0, Native291_SET_SKELETON);
	CREATE_FUNCTION_BINDING("IS_SKELETON", 1, 1, Native292_IS_SKELETON);
	CREATE_FUNCTION_BINDING("PLAYER_SPELL_CAST_TIME", 1, 1, Native293_PLAYER_SPELL_CAST_TIME);
	CREATE_FUNCTION_BINDING("PLAYER_SPELL_LAST_CAST", 1, 1, Native294_PLAYER_SPELL_LAST_CAST);
	CREATE_FUNCTION_BINDING("GET_LAST_SPELL_CAST_POS", 1, 3, Native295_GET_LAST_SPELL_CAST_POS);
	CREATE_FUNCTION_BINDING("ADD_SPOT_VISUAL_TARGET_POS", 4, 0, Native296_ADD_SPOT_VISUAL_TARGET_POS);
	CREATE_FUNCTION_BINDING("ADD_SPOT_VISUAL_TARGET_OBJECT", 2, 0, Native297_ADD_SPOT_VISUAL_TARGET_OBJECT);
	CREATE_FUNCTION_BINDING("SET_INDESTRUCTABLE", 2, 0, Native298_SET_INDESTRUCTABLE);
	CREATE_FUNCTION_BINDING("SET_GRAPHICS_CLIPPING", 2, 0, Native299_SET_GRAPHICS_CLIPPING);
	CREATE_FUNCTION_BINDING("SPIRIT_APPEAR", 1, 0, Native300_SPIRIT_APPEAR);
	CREATE_FUNCTION_BINDING("SPIRIT_DISAPPEAR", 1, 0, Native301_SPIRIT_DISAPPEAR);
	CREATE_FUNCTION_BINDING("SET_FOCUS_ON_OBJECT", 2, 0, Native302_SET_FOCUS_ON_OBJECT);
	CREATE_FUNCTION_BINDING("RELEASE_OBJECT_FOCUS", 1, 0, Native303_RELEASE_OBJECT_FOCUS);
	CREATE_FUNCTION_BINDING("IMMERSION_EXISTS", 0, 1, Native304_IMMERSION_EXISTS);
	CREATE_FUNCTION_BINDING("SET_DRAW_LEASH", 1, 0, Native305_SET_DRAW_LEASH);
	CREATE_FUNCTION_BINDING("SET_DRAW_HIGHLIGHT", 1, 0, Native306_SET_DRAW_HIGHLIGHT);
	CREATE_FUNCTION_BINDING("SET_OPEN_CLOSE", 2, 0, Native307_SET_OPEN_CLOSE);
	CREATE_FUNCTION_BINDING("SET_INTRO_BUILDING", 1, 0, Native308_SET_INTRO_BUILDING);
	CREATE_FUNCTION_BINDING("CREATURE_FORCE_FRIENDS", 3, 0, Native309_CREATURE_FORCE_FRIENDS);
	CREATE_FUNCTION_BINDING("MOVE_COMPUTER_PLAYER_POSITION", 6, 0, Native310_MOVE_COMPUTER_PLAYER_POSITION);
	CREATE_FUNCTION_BINDING("ENABLE_DISABLE_COMPUTER_PLAYER", 2, 0, Native311_ENABLE_DISABLE_COMPUTER_PLAYER);
	CREATE_FUNCTION_BINDING("GET_COMPUTER_PLAYER_POSITION", 1, 3, Native312_GET_COMPUTER_PLAYER_POSITION);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_POSITION", 5, 0, Native313_SET_COMPUTER_PLAYER_POSITION);
	CREATE_FUNCTION_BINDING("GET_STORED_CAMERA_POSITION", 0, 3, Native314_GET_STORED_CAMERA_POSITION);
	CREATE_FUNCTION_BINDING("GET_STORED_CAMERA_FOCUS", 0, 3, Native315_GET_STORED_CAMERA_FOCUS);
	CREATE_FUNCTION_BINDING("CALL_NEAR_IN_STATE", 8, 1, Native316_CALL_NEAR_IN_STATE);
	CREATE_FUNCTION_BINDING("SET_CREATURE_SOUND", 1, 0, Native317_SET_CREATURE_SOUND);
	CREATE_FUNCTION_BINDING("CREATURE_INTERACTING_WITH", 2, 1, Native318_CREATURE_INTERACTING_WITH);
	CREATE_FUNCTION_BINDING("SET_SUN_DRAW", 1, 0, Native319_SET_SUN_DRAW);
	CREATE_FUNCTION_BINDING("OBJECT_INFO_BITS", 1, 1, Native320_OBJECT_INFO_BITS);
	CREATE_FUNCTION_BINDING("SET_HURT_BY_FIRE", 2, 0, Native321_SET_HURT_BY_FIRE);
	CREATE_FUNCTION_BINDING("CONFINED_OBJECT", 5, 0, Native322_CONFINED_OBJECT);
	CREATE_FUNCTION_BINDING("CLEAR_CONFINED_OBJECT", 1, 0, Native323_CLEAR_CONFINED_OBJECT);
	CREATE_FUNCTION_BINDING("GET_OBJECT_FLOCK", 1, 1, Native324_GET_OBJECT_FLOCK);
	CREATE_FUNCTION_BINDING("SET_PLAYER_BELIEF", 3, 0, Native325_SET_PLAYER_BELIEF);
	CREATE_FUNCTION_BINDING("PLAY_JC_SPECIAL", 1, 0, Native326_PLAY_JC_SPECIAL);
	CREATE_FUNCTION_BINDING("IS_PLAYING_JC_SPECIAL", 1, 1, Native327_IS_PLAYING_JC_SPECIAL);
	CREATE_FUNCTION_BINDING("VORTEX_PARAMETERS", 8, 0, Native328_VORTEX_PARAMETERS);
	CREATE_FUNCTION_BINDING("LOAD_CREATURE", 6, 0, Native329_LOAD_CREATURE);
	CREATE_FUNCTION_BINDING("IS_SPELL_CHARGING", 1, 1, Native330_IS_SPELL_CHARGING);
	CREATE_FUNCTION_BINDING("IS_THAT_SPELL_CHARGING", 2, 1, Native331_IS_THAT_SPELL_CHARGING);
	CREATE_FUNCTION_BINDING("OPPOSING_CREATURE", 1, 1, Native332_OPPOSING_CREATURE);
	CREATE_FUNCTION_BINDING("FLOCK_WITHIN_LIMITS", 1, 1, Native333_FLOCK_WITHIN_LIMITS);
	CREATE_FUNCTION_BINDING("HIGHLIGHT_PROPERTIES", 3, 0, Native334_HIGHLIGHT_PROPERTIES);
	CREATE_FUNCTION_BINDING("LAST_MUSIC_LINE", 1, 1, Native335_LAST_MUSIC_LINE);
	CREATE_FUNCTION_BINDING("HAND_DEMO_TRIGGER", 0, 1, Native336_HAND_DEMO_TRIGGER);
	CREATE_FUNCTION_BINDING("GET_BELLY_POSITION", 1, 3, Native337_GET_BELLY_POSITION);
	CREATE_FUNCTION_BINDING("SET_CREATURE_CREED_PROPERTIES", 5, 0, Native338_SET_CREATURE_CREED_PROPERTIES);
	CREATE_FUNCTION_BINDING("GAME_THING_CAN_VIEW_CAMERA", 2, 1, Native339_GAME_THING_CAN_VIEW_CAMERA);
	CREATE_FUNCTION_BINDING("GAME_PLAY_SAY_SOUND_EFFECT", 6, 0, Native340_GAME_PLAY_SAY_SOUND_EFFECT);
	CREATE_FUNCTION_BINDING("SET_TOWN_DESIRE_BOOST", 3, 0, Native341_SET_TOWN_DESIRE_BOOST);
	CREATE_FUNCTION_BINDING("IS_LOCKED_INTERACTION", 1, 1, Native342_IS_LOCKED_INTERACTION);
	CREATE_FUNCTION_BINDING("SET_CREATURE_NAME", 2, 0, Native343_SET_CREATURE_NAME);
	CREATE_FUNCTION_BINDING("COMPUTER_PLAYER_READY", 1, 1, Native344_COMPUTER_PLAYER_READY);
	CREATE_FUNCTION_BINDING("ENABLE_DISABLE_COMPUTER_PLAYER", 2, 0, Native345_ENABLE_DISABLE_COMPUTER_PLAYER);
	CREATE_FUNCTION_BINDING("CLEAR_ACTOR_MIND", 1, 0, Native346_CLEAR_ACTOR_MIND);
	CREATE_FUNCTION_BINDING("ENTER_EXIT_CITADEL", 1, 0, Native347_ENTER_EXIT_CITADEL);
	CREATE_FUNCTION_BINDING("START_ANGLE_SOUND", 1, 0, Native348_START_ANGLE_SOUND);
	CREATE_FUNCTION_BINDING("THING_JC_SPECIAL", 3, 0, Native349_THING_JC_SPECIAL);
	CREATE_FUNCTION_BINDING("MUSIC_PLAYED", 1, 1, Native350_MUSIC_PLAYED);
	CREATE_FUNCTION_BINDING("UPDATE_SNAPSHOT_PICTURE", 11, 0, Native351_UPDATE_SNAPSHOT_PICTURE);
	CREATE_FUNCTION_BINDING("STOP_SCRIPTS_IN_FILES_EXCLUDING", 2, 0, Native352_STOP_SCRIPTS_IN_FILES_EXCLUDING);
	CREATE_FUNCTION_BINDING("CREATE_RANDOM_VILLAGER_OF_TRIBE", 4, 1, Native353_CREATE_RANDOM_VILLAGER_OF_TRIBE);
	CREATE_FUNCTION_BINDING("TOGGLE_LEASH", 1, 0, Native354_TOGGLE_LEASH);
	CREATE_FUNCTION_BINDING("GAME_SET_MANA", 2, 0, Native355_GAME_SET_MANA);
	CREATE_FUNCTION_BINDING("SET_MAGIC_PROPERTIES", 3, 0, Native356_SET_MAGIC_PROPERTIES);
	CREATE_FUNCTION_BINDING("SET_GAME_SOUND", 1, 0, Native357_SET_GAME_SOUND);
	CREATE_FUNCTION_BINDING("SEX_IS_MALE", 1, 1, Native358_SEX_IS_MALE);
	CREATE_FUNCTION_BINDING("GET_FIRST_HELP", 1, 1, Native359_GET_FIRST_HELP);
	CREATE_FUNCTION_BINDING("GET_LAST_HELP", 1, 1, Native360_GET_LAST_HELP);
	CREATE_FUNCTION_BINDING("IS_ACTIVE", 1, 1, Native361_IS_ACTIVE);
	CREATE_FUNCTION_BINDING("SET_BOOKMARK_POSITION", 4, 0, Native362_SET_BOOKMARK_POSITION);
	CREATE_FUNCTION_BINDING("SET_SCAFFOLD_PROPERTIES", 4, 0, Native363_SET_SCAFFOLD_PROPERTIES);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_PERSONALITY", 3, 0, Native364_SET_COMPUTER_PLAYER_PERSONALITY);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_SUPPRESSION", 3, 0, Native365_SET_COMPUTER_PLAYER_SUPPRESSION);
	CREATE_FUNCTION_BINDING("FORCE_COMPUTER_PLAYER_ACTION", 4, 0, Native366_FORCE_COMPUTER_PLAYER_ACTION);
	CREATE_FUNCTION_BINDING("QUEUE_COMPUTER_PLAYER_ACTION", 4, 0, Native367_QUEUE_COMPUTER_PLAYER_ACTION);
	CREATE_FUNCTION_BINDING("GET_TOWN_WITH_ID", 1, 1, Native368_GET_TOWN_WITH_ID);
	CREATE_FUNCTION_BINDING("SET_DISCIPLE", 3, 0, Native369_SET_DISCIPLE);
	CREATE_FUNCTION_BINDING("RELEASE_COMPUTER_PLAYER", 1, 0, Native370_RELEASE_COMPUTER_PLAYER);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_SPEED", 2, 0, Native371_SET_COMPUTER_PLAYER_SPEED);
	CREATE_FUNCTION_BINDING("SET_FOCUS_FOLLOW_COMPUTER_PLAYER", 1, 0, Native372_SET_FOCUS_FOLLOW_COMPUTER_PLAYER);
	CREATE_FUNCTION_BINDING("SET_POSITION_FOLLOW_COMPUTER_PLAYER", 1, 0, Native373_SET_POSITION_FOLLOW_COMPUTER_PLAYER);
	CREATE_FUNCTION_BINDING("CALL_COMPUTER_PLAYER", 1, 1, Native374_CALL_COMPUTER_PLAYER);
	CREATE_FUNCTION_BINDING("CALL_BUILDING_IN_TOWN", 4, 1, Native375_CALL_BUILDING_IN_TOWN);
	CREATE_FUNCTION_BINDING("SET_CAN_BUILD_WORSHIPSITE", 2, 0, Native376_SET_CAN_BUILD_WORSHIPSITE);
	CREATE_FUNCTION_BINDING("GET_FACING_CAMERA_POSITION", 1, 3, Native377_GET_FACING_CAMERA_POSITION);
	CREATE_FUNCTION_BINDING("SET_COMPUTER_PLAYER_ATTITUDE", 3, 0, Native378_SET_COMPUTER_PLAYER_ATTITUDE);
	CREATE_FUNCTION_BINDING("GET_COMPUTER_PLAYER_ATTITUDE", 2, 1, Native379_GET_COMPUTER_PLAYER_ATTITUDE);
	CREATE_FUNCTION_BINDING("LOAD_COMPUTER_PLAYER_PERSONALITY", 2, 0, Native380_LOAD_COMPUTER_PLAYER_PERSONALITY);
	CREATE_FUNCTION_BINDING("SAVE_COMPUTER_PLAYER_PERSONALITY", 2, 0, Native381_SAVE_COMPUTER_PLAYER_PERSONALITY);
	CREATE_FUNCTION_BINDING("SET_PLAYER_ALLY", 3, 0, Native382_SET_PLAYER_ALLY);
	CREATE_FUNCTION_BINDING("CALL_FLYING", 7, 1, Native383_CALL_FLYING);
	CREATE_FUNCTION_BINDING("SET_OBJECT_FADE_IN", 2, 0, Native384_SET_OBJECT_FADE_IN);
	CREATE_FUNCTION_BINDING("IS_AFFECTED_BY_SPELL", 1, 1, Native385_IS_AFFECTED_BY_SPELL);
	CREATE_FUNCTION_BINDING("SET_MAGIC_IN_OBJECT", 3, 0, Native386_SET_MAGIC_IN_OBJECT);
	CREATE_FUNCTION_BINDING("ID_ADULT_SIZE", 1, 1, Native387_ID_ADULT_SIZE);
	CREATE_FUNCTION_BINDING("OBJECT_CAPACITY", 1, 1, Native388_OBJECT_CAPACITY);
	CREATE_FUNCTION_BINDING("OBJECT_ADULT_CAPACITY", 1, 1, Native389_OBJECT_ADULT_CAPACITY);
	CREATE_FUNCTION_BINDING("SET_CREATURE_AUTO_FIGHTING", 2, 0, Native390_SET_CREATURE_AUTO_FIGHTING);
	CREATE_FUNCTION_BINDING("IS_AUTO_FIGHTING", 1, 1, Native391_IS_AUTO_FIGHTING);
	CREATE_FUNCTION_BINDING("SET_CREATURE_QUEUE_FIGHT_MOVE", 2, 0, Native392_SET_CREATURE_QUEUE_FIGHT_MOVE);
	CREATE_FUNCTION_BINDING("SET_CREATURE_QUEUE_FIGHT_SPELL", 2, 0, Native393_SET_CREATURE_QUEUE_FIGHT_SPELL);
	CREATE_FUNCTION_BINDING("SET_CREATURE_QUEUE_FIGHT_STEP", 2, 0, Native394_SET_CREATURE_QUEUE_FIGHT_STEP);
	CREATE_FUNCTION_BINDING("GET_CREATURE_FIGHT_ACTION", 1, 1, Native395_GET_CREATURE_FIGHT_ACTION);
	CREATE_FUNCTION_BINDING("CREATURE_FIGHT_QUEUE_HITS", 1, 1, Native396_CREATURE_FIGHT_QUEUE_HITS);
	CREATE_FUNCTION_BINDING("SQUARE_ROOT", 1, 1, Native397_SQUARE_ROOT);
	CREATE_FUNCTION_BINDING("GET_PLAYER_ALLY", 2, 1, Native398_GET_PLAYER_ALLY);
	CREATE_FUNCTION_BINDING("SET_PLAYER_WIND_RESISTANCE", 2, 1, Native399_SET_PLAYER_WIND_RESISTANCE);
	CREATE_FUNCTION_BINDING("GET_PLAYER_WIND_RESISTANCE", 2, 1, Native400_GET_PLAYER_WIND_RESISTANCE);
	CREATE_FUNCTION_BINDING("PAUSE_UNPAUSE_CLIMATE_SYSTEM", 1, 0, Native401_PAUSE_UNPAUSE_CLIMATE_SYSTEM);
	CREATE_FUNCTION_BINDING("PAUSE_UNPAUSE_STORM_CREATION_IN_CLIMATE_SYSTEM", 1, 0,
	                        Native402_PAUSE_UNPAUSE_STORM_CREATION_IN_CLIMATE_SYSTEM);
	CREATE_FUNCTION_BINDING("GET_MANA_FOR_SPELL", 1, 1, Native403_GET_MANA_FOR_SPELL);
	CREATE_FUNCTION_BINDING("KILL_STORMS_IN_AREA", 4, 0, Native404_KILL_STORMS_IN_AREA);
	CREATE_FUNCTION_BINDING("INSIDE_TEMPLE", 0, 1, Native405_INSIDE_TEMPLE);
	CREATE_FUNCTION_BINDING("RESTART_OBJECT", 1, 0, Native406_RESTART_OBJECT);
	CREATE_FUNCTION_BINDING("SET_GAME_TIME_PROPERTIES", 3, 0, Native407_SET_GAME_TIME_PROPERTIES);
	CREATE_FUNCTION_BINDING("RESET_GAME_TIME_PROPERTIES", 0, 0, Native408_RESET_GAME_TIME_PROPERTIES);
	CREATE_FUNCTION_BINDING("SOUND_EXISTS", 0, 1, Native409_SOUND_EXISTS);
	CREATE_FUNCTION_BINDING("GET_TOWN_WORSHIP_DEATHS", 1, 1, Native410_GET_TOWN_WORSHIP_DEATHS);
	CREATE_FUNCTION_BINDING("GAME_CLEAR_DIALOGUE", 0, 0, Native411_GAME_CLEAR_DIALOGUE);
	CREATE_FUNCTION_BINDING("GAME_CLOSE_DIALOGUE", 0, 0, Native412_GAME_CLOSE_DIALOGUE);
	CREATE_FUNCTION_BINDING("GET_HAND_STATE", 0, 1, Native413_GET_HAND_STATE);
	CREATE_FUNCTION_BINDING("SET_INTERFACE_CITADEL", 1, 0, Native414_SET_INTERFACE_CITADEL);
	CREATE_FUNCTION_BINDING("MAP_SCRIPT_FUNCTION", 1, 0, Native415_MAP_SCRIPT_FUNCTION);
	CREATE_FUNCTION_BINDING("WITHIN_ROTATION", 0, 1, Native416_WITHIN_ROTATION);
	CREATE_FUNCTION_BINDING("GET_PLAYER_TOWN_TOTAL", 1, 1, Native417_GET_PLAYER_TOWN_TOTAL);
	CREATE_FUNCTION_BINDING("SPIRIT_SCREEN_POINT", 3, 0, Native418_SPIRIT_SCREEN_POINT);
	CREATE_FUNCTION_BINDING("KEY_DOWN", 1, 1, Native419_KEY_DOWN);
	CREATE_FUNCTION_BINDING("SET_FIGHT_EXIT", 1, 0, Native420_SET_FIGHT_EXIT);
	CREATE_FUNCTION_BINDING("GET_OBJECT_CLICKED", 0, 1, Native421_GET_OBJECT_CLICKED);
	CREATE_FUNCTION_BINDING("GET_MANA", 1, 1, Native422_GET_MANA);
	CREATE_FUNCTION_BINDING("CLEAR_PLAYER_SPELL_CHARGING", 1, 0, Native423_CLEAR_PLAYER_SPELL_CHARGING);
	CREATE_FUNCTION_BINDING("STOP_SOUND_EFFECT", 3, 0, Native424_STOP_SOUND_EFFECT);
	CREATE_FUNCTION_BINDING("GET_TOTEM_STATUE", 1, 1, Native425_GET_TOTEM_STATUE);
	CREATE_FUNCTION_BINDING("SET_SET_ON_FIRE", 2, 0, Native426_SET_SET_ON_FIRE);
	CREATE_FUNCTION_BINDING("SET_LAND_BALANCE", 2, 0, Native427_SET_LAND_BALANCE);
	CREATE_FUNCTION_BINDING("SET_OBJECT_BELIEF_SCALE", 2, 0, Native428_SET_OBJECT_BELIEF_SCALE);
	CREATE_FUNCTION_BINDING("START_IMMERSION", 1, 0, Native429_START_IMMERSION);
	CREATE_FUNCTION_BINDING("STOP_IMMERSION", 1, 0, Native430_STOP_IMMERSION);
	CREATE_FUNCTION_BINDING("STOP_ALL_IMMERSION", 0, 0, Native431_STOP_ALL_IMMERSION);
	CREATE_FUNCTION_BINDING("SET_CREATURE_IN_TEMPLE", 1, 0, Native432_SET_CREATURE_IN_TEMPLE);
	CREATE_FUNCTION_BINDING("GAME_DRAW_TEXT", 7, 0, Native433_GAME_DRAW_TEXT);
	CREATE_FUNCTION_BINDING("GAME_DRAW_TEMP_TEXT", 7, 0, Native434_GAME_DRAW_TEMP_TEXT);
	CREATE_FUNCTION_BINDING("FADE_ALL_DRAW_TEXT", 1, 0, Native435_FADE_ALL_DRAW_TEXT);
	CREATE_FUNCTION_BINDING("SET_DRAW_TEXT_COLOUR", 3, 0, Native436_SET_DRAW_TEXT_COLOUR);
	CREATE_FUNCTION_BINDING("SET_CLIPPING_WINDOW", 5, 0, Native437_SET_CLIPPING_WINDOW);
	CREATE_FUNCTION_BINDING("CLEAR_CLIPPING_WINDOW", 1, 0, Native438_CLEAR_CLIPPING_WINDOW);
	CREATE_FUNCTION_BINDING("SAVE_GAME_IN_SLOT", 1, 0, Native439_SAVE_GAME_IN_SLOT);
	CREATE_FUNCTION_BINDING("SET_OBJECT_CARRYING", 2, 0, Native440_SET_OBJECT_CARRYING);
	CREATE_FUNCTION_BINDING("POS_VALID_FOR_CREATURE", 3, 1, Native441_POS_VALID_FOR_CREATURE);
	CREATE_FUNCTION_BINDING("GET_TIME_SINCE_OBJECT_ATTACKED", 2, 1, Native442_GET_TIME_SINCE_OBJECT_ATTACKED);
	CREATE_FUNCTION_BINDING("GET_TOWN_AND_VILLAGER_HEALTH_TOTAL", 1, 1, Native443_GET_TOWN_AND_VILLAGER_HEALTH_TOTAL);
	CREATE_FUNCTION_BINDING("GAME_ADD_FOR_BUILDING", 2, 0, Native444_GAME_ADD_FOR_BUILDING);
	CREATE_FUNCTION_BINDING("ENABLE_DISABLE_ALIGNMENT_MUSIC", 1, 0, Native445_ENABLE_DISABLE_ALIGNMENT_MUSIC);
	CREATE_FUNCTION_BINDING("GET_DEAD_LIVING", 4, 1, Native446_GET_DEAD_LIVING);
	CREATE_FUNCTION_BINDING("ATTACH_SOUND_TAG", 4, 0, Native447_ATTACH_SOUND_TAG);
	CREATE_FUNCTION_BINDING("DETACH_SOUND_TAG", 3, 0, Native448_DETACH_SOUND_TAG);
	CREATE_FUNCTION_BINDING("GET_SACRIFICE_TOTAL", 1, 1, Native449_GET_SACRIFICE_TOTAL);
	CREATE_FUNCTION_BINDING("GAME_SOUND_PLAYING", 2, 1, Native450_GAME_SOUND_PLAYING);
	CREATE_FUNCTION_BINDING("GET_TEMPLE_POSITION", 1, 3, Native451_GET_TEMPLE_POSITION);
	CREATE_FUNCTION_BINDING("CREATURE_AUTOSCALE", 3, 0, Native452_CREATURE_AUTOSCALE);
	CREATE_FUNCTION_BINDING("GET_SPELL_ICON_IN_TEMPLE", 2, 1, Native453_GET_SPELL_ICON_IN_TEMPLE);
	CREATE_FUNCTION_BINDING("GAME_CLEAR_COMPUTER_PLAYER_ACTIONS", 1, 0, Native454_GAME_CLEAR_COMPUTER_PLAYER_ACTIONS);
	CREATE_FUNCTION_BINDING("GET_FIRST_IN_CONTAINER", 1, 1, Native455_GET_FIRST_IN_CONTAINER);
	CREATE_FUNCTION_BINDING("GET_NEXT_IN_CONTAINER", 2, 1, Native456_GET_NEXT_IN_CONTAINER);
	CREATE_FUNCTION_BINDING("GET_TEMPLE_ENTRANCE_POSITION", 3, 3, Native457_GET_TEMPLE_ENTRANCE_POSITION);
	CREATE_FUNCTION_BINDING("SAY_SOUND_EFFECT_PLAYING", 2, 1, Native458_SAY_SOUND_EFFECT_PLAYING);
	CREATE_FUNCTION_BINDING("SET_HAND_DEMO_KEYS", 1, 0, Native459_SET_HAND_DEMO_KEYS);
	CREATE_FUNCTION_BINDING("CAN_SKIP_TUTORIAL", 0, 1, Native460_CAN_SKIP_TUTORIAL);
	CREATE_FUNCTION_BINDING("CAN_SKIP_CREATURE_TRAINING", 0, 1, Native461_CAN_SKIP_CREATURE_TRAINING);
	CREATE_FUNCTION_BINDING("IS_KEEPING_OLD_CREATURE", 0, 1, Native462_IS_KEEPING_OLD_CREATURE);
	CREATE_FUNCTION_BINDING("CURRENT_PROFILE_HAS_CREATURE", 0, 1, Native463_CURRENT_PROFILE_HAS_CREATURE);
}

#pragma warning(push)
#pragma warning(disable : 4189)
void Game::Native000_NONE() {}

void Game::Native001_SET_CAMERA_POSITION()
{
	const auto position = PopVec();

	_camera->SetOrigin(position);
}

void Game::Native002_SET_CAMERA_FOCUS()
{
	const auto position = PopVec();

	_camera->SetFocus(position);
}

void Game::Native003_MOVE_CAMERA_POSITION()
{
	const auto time = _lhvm->Popf();
	const auto position = PopVec();
	// TODO
	
}

void Game::Native004_MOVE_CAMERA_FOCUS()
{
	const auto time = _lhvm->Popf();
	const auto position = PopVec();
	// TODO
}

void Game::Native005_GET_CAMERA_POSITION()
{
	const auto position = _camera->GetOrigin();
	PushVec(position);
}

void Game::Native006_GET_CAMERA_FOCUS()
{
	const auto focus = _camera->GetFocus();
	PushVec(focus);
}

void Game::Native007_SPIRIT_EJECT()
{
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native008_SPIRIT_HOME()
{
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native009_SPIRIT_POINT_POS()
{
	const auto inWorld = static_cast<bool>(_lhvm->Pop().intVal);
	const auto position = PopVec();
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native010_SPIRIT_POINT_GAME_THING()
{
	const auto inWorld = static_cast<bool>(_lhvm->Pop().intVal);
	const auto target = _lhvm->Pop().uintVal;
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native011_GAME_THING_FIELD_OF_VIEW()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native012_POS_FIELD_OF_VIEW()
{
	const auto position = PopVec();
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native013_RUN_TEXT()
{
	const auto withInteraction = _lhvm->Pop().intVal;
	const auto textID = _lhvm->Pop().intVal;
	const auto singleLine = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native014_TEMP_TEXT()
{
	const auto withInteraction = _lhvm->Pop().intVal;
	const auto string = PopString();
	const auto singleLine = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native015_TEXT_READ()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native016_GAME_THING_CLICKED()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native017_SET_SCRIPT_STATE()
{
	const auto state = _lhvm->Pop().intVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native018_SET_SCRIPT_STATE_POS()
{
	const auto position = PopVec();
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native019_SET_SCRIPT_FLOAT()
{
	const auto value = _lhvm->Popf();
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native020_SET_SCRIPT_ULONG()
{
	const auto loop = _lhvm->Pop().intVal;
	const auto animation = _lhvm->Pop().intVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native021_GET_PROPERTY()
{
	const auto object = _lhvm->Pop().uintVal;
	const auto prop = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native022_SET_PROPERTY()
{
	const auto val = _lhvm->Popf();
	const auto object = _lhvm->Pop().uintVal;
	const auto prop = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native023_GET_POSITION()
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

void Game::Native024_SET_POSITION()
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

void Game::Native025_GET_DISTANCE()
{
	const auto p1 = PopVec();
	const auto p0 = PopVec();
	const auto distance = glm::length(p1 - p0);
	_lhvm->Pushf(distance);
}

void Game::Native026_CALL()
{
	const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	const auto position = PopVec();
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native027_CREATE()
{
	const auto position = PopVec();
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = static_cast<SCRIPT_OBJECT_TYPE>(_lhvm->Pop().intVal);

	const auto object = CreateScriptObject(type, subtype, position, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	const auto objId = _refManager.GetReference(object);

	_lhvm->Pusho(objId);
}

void Game::Native028_RANDOM()
{
	const auto max = _lhvm->Popf();
	const auto min = _lhvm->Popf();
	float random = min + (max - min) * ((float)rand()) / (float)RAND_MAX;
	_lhvm->Pushf(random);
}

void Game::Native029_DLL_GETTIME()
{
	_lhvm->Pushf((float)_turnCount / 10.0f); // TODO: should it be divided by 10 or not?
}

void Game::Native030_START_CAMERA_CONTROL()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native031_END_CAMERA_CONTROL()
{
	// TODO
}

void Game::Native032_SET_WIDESCREEN()
{
	const auto enabled = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native033_MOVE_GAME_THING()
{
	const auto radius = _lhvm->Popf();
	const auto position = PopVec();
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native034_SET_FOCUS()
{
	const auto position = PopVec();
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native035_HAS_CAMERA_ARRIVED()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native036_FLOCK_CREATE()
{
	const auto position = PopVec();
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native037_FLOCK_ATTACH()
{
	const auto asLeader = static_cast<bool>(_lhvm->Pop().intVal);
	const auto flock = _lhvm->Pop().uintVal;
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native038_FLOCK_DETACH()
{
	const auto flock = _lhvm->Pop().uintVal;
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native039_FLOCK_DISBAND()
{
	const auto flock = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native040_ID_SIZE()
{
	const auto container = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native041_FLOCK_MEMBER()
{
	const auto flock = _lhvm->Pop().uintVal;
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native042_GET_HAND_POSITION()
{
	auto& handTransform = Locator::entitiesRegistry::value().Get<ecs::components::Transform>(_handEntity);

	PushVec(handTransform.position);
}

void Game::Native043_PLAY_SOUND_EFFECT()
{
	const auto withPosition = static_cast<bool>(_lhvm->Pop().intVal);
	const auto position = PopVec();
	const auto soundbank = _lhvm->Pop().intVal;
	const auto sound = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native044_START_MUSIC()
{
	const auto music = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native045_STOP_MUSIC()
{
	// TODO
}

void Game::Native046_ATTACH_MUSIC()
{
	const auto target = _lhvm->Pop().uintVal;
	const auto music = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native047_DETACH_MUSIC()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native048_OBJECT_DELETE()
{
	const auto withFade = _lhvm->Pop().intVal;
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native049_FOCUS_FOLLOW()
{
	const auto target = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native050_POSITION_FOLLOW()
{
	const auto target = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native051_CALL_NEAR()
{
	const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	const auto radius = _lhvm->Popf();
	const auto position = PopVec();
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native052_SPECIAL_EFFECT_POSITION()
{
	const auto duration = _lhvm->Popf();
	const auto position = PopVec();
	const auto effect = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native053_SPECIAL_EFFECT_OBJECT()
{
	const auto duration = _lhvm->Popf();
	const auto target = _lhvm->Pop().uintVal;
	const auto effect = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native054_DANCE_CREATE()
{
	const auto duration = _lhvm->Popf();
	const auto position = PopVec();
	const auto type = _lhvm->Pop().intVal;
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native055_CALL_IN()
{
	const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	const auto container = _lhvm->Pop().uintVal;
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native056_CHANGE_INNER_OUTER_PROPERTIES()
{
	const auto calm = _lhvm->Popf();
	const auto outer = _lhvm->Popf();
	const auto inner = _lhvm->Popf();
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native057_SNAPSHOT()
{
	const auto challengeId = _lhvm->Pop().intVal;
	const auto argc = _lhvm->Pop().intVal;
	const auto argv = PopVarArg(argc);
	const auto reminderScript = PopString();
	const auto titleStrID = _lhvm->Pop().intVal;
	const auto alignment = _lhvm->Popf();
	const auto success = _lhvm->Popf();
	const auto focus = PopVec();
	const auto position = PopVec();
	const auto quest = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native058_GET_ALIGNMENT()
{
	const auto zero = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native059_SET_ALIGNMENT()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native060_INFLUENCE_OBJECT()
{
	const auto anti = _lhvm->Pop().intVal;
	const auto zero = _lhvm->Pop().intVal;
	const auto radius = _lhvm->Popf();
	const auto target = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native061_INFLUENCE_POSITION()
{
	const auto anti = _lhvm->Pop().intVal;
	const auto zero = _lhvm->Pop().intVal;
	const auto radius = _lhvm->Popf();
	const auto position = PopVec();
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native062_GET_INFLUENCE()
{
	const auto position = PopVec();
	const auto raw = static_cast<bool>(_lhvm->Pop().intVal);
	const auto player = _lhvm->Popf();
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native063_SET_INTERFACE_INTERACTION()
{
	const auto level = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native064_PLAYED()
{
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native065_RANDOM_ULONG()
{
	const auto max = _lhvm->Pop().intVal;
	const auto min = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native066_SET_GAMESPEED()
{
	const auto speed = _lhvm->Popf();
	// TODO
}

void Game::Native067_CALL_IN_NEAR()
{
	const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	const auto radius = _lhvm->Popf();
	const auto pos = PopVec();
	const auto container = _lhvm->Pop().uintVal;
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native068_OVERRIDE_STATE_ANIMATION()
{
	const auto animType = _lhvm->Pop().intVal;
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native069_CREATURE_CREATE_RELATIVE_TO_CREATURE()
{
	const auto type = _lhvm->Pop().intVal;
	const auto position = PopVec();
	const auto scale = _lhvm->Popf();
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native070_CREATURE_LEARN_EVERYTHING()
{
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native071_CREATURE_SET_KNOWS_ACTION()
{
	const auto knows = _lhvm->Pop().intVal;
	const auto action = _lhvm->Pop().intVal;
	const auto typeOfAction = _lhvm->Pop().intVal;
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native072_CREATURE_SET_AGENDA_PRIORITY()
{
	const auto priority = _lhvm->Popf();
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native073_CREATURE_TURN_OFF_ALL_DESIRES()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native074_CREATURE_LEARN_DISTINCTION_ABOUT_ACTIVITY_OBJECT()
{
	const auto unk3 = _lhvm->Pop().intVal;
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native075_CREATURE_DO_ACTION()
{
	const auto withObject = _lhvm->Pop().uintVal;
	const auto target = _lhvm->Pop().uintVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native076_IN_CREATURE_HAND()
{
	const auto creature = _lhvm->Pop().uintVal;
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native077_CREATURE_SET_DESIRE_VALUE()
{
	const auto value = _lhvm->Popf();
	const auto desire = _lhvm->Pop().intVal;
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native078_CREATURE_SET_DESIRE_ACTIVATED()
{
	const auto active = _lhvm->Pop().intVal;
	const auto desire = _lhvm->Pop().intVal;
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native079_CREATURE_SET_DESIRE_ACTIVATED()
{
	const auto active = _lhvm->Pop().intVal;
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native080_CREATURE_SET_DESIRE_MAXIMUM()
{
	const auto value = _lhvm->Popf();
	const auto desire = _lhvm->Pop().intVal;
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native081_CONVERT_CAMERA_POSITION()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::Native082_CONVERT_CAMERA_FOCUS()
{
	const auto camera_enum = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::Native083_CREATURE_SET_PLAYER()
{
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native084_START_COUNTDOWN_TIMER()
{
	const auto timeout = _lhvm->Popf();
	// TODO
}

void Game::Native085_CREATURE_INITIALISE_NUM_TIMES_PERFORMED_ACTION()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native086_CREATURE_GET_NUM_TIMES_ACTION_PERFORMED()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native087_REMOVE_COUNTDOWN_TIMER()
{
	// TODO
}

void Game::Native088_GET_OBJECT_DROPPED()
{
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native089_CLEAR_DROPPED_BY_OBJECT()
{
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native090_CREATE_REACTION()
{
	const auto reaction = _lhvm->Pop().intVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native091_REMOVE_REACTION()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native092_GET_COUNTDOWN_TIMER()
{
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native093_START_DUAL_CAMERA()
{
	const auto obj2 = _lhvm->Pop().uintVal;
	const auto obj1 = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native094_UPDATE_DUAL_CAMERA()
{
	const auto obj2 = _lhvm->Pop().uintVal;
	const auto obj1 = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native095_RELEASE_DUAL_CAMERA()
{
	// TODO
}

void Game::Native096_SET_CREATURE_HELP()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native097_GET_TARGET_OBJECT()
{
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native098_CREATURE_DESIRE_IS()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native099_COUNTDOWN_TIMER_EXISTS()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native100_LOOK_GAME_THING()
{
	const auto target = _lhvm->Pop().uintVal;
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native101_GET_OBJECT_DESTINATION()
{
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::Native102_CREATURE_FORCE_FINISH()
{
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native103_HIDE_COUNTDOWN_TIMER()
{
	// TODO
}

void Game::Native104_GET_ACTION_TEXT_FOR_OBJECT()
{
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native105_CREATE_DUAL_CAMERA_WITH_POINT()
{
	const auto position = PopVec();
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native106_SET_CAMERA_TO_FACE_OBJECT()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native107_MOVE_CAMERA_TO_FACE_OBJECT()
{
	const auto time = _lhvm->Popf();
	const auto distance = _lhvm->Popf();
	const auto target = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native108_GET_MOON_PERCENTAGE()
{
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native109_POPULATE_CONTAINER()
{
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = _lhvm->Pop().intVal;
	const auto quantity = _lhvm->Popf();
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native110_ADD_REFERENCE()
{
	const auto objId = _lhvm->Pop().uintVal;
	AddObjectReference(objId);
	_lhvm->Pusho(objId);
}

void Game::Native111_REMOVE_REFERENCE()
{
	const auto objId = _lhvm->Pop().uintVal;
	RemoveObjectReference(objId);
	_lhvm->Pusho(objId);
}

void Game::Native112_SET_GAME_TIME()
{
	const auto time = _lhvm->Popf();
	// TODO
}

void Game::Native113_GET_GAME_TIME()
{
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native114_GET_REAL_TIME()
{
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native115_GET_REAL_DAY()
{
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native116_GET_REAL_DAY()
{
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native117_GET_REAL_MONTH()
{
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native118_GET_REAL_YEAR()
{
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native119_RUN_CAMERA_PATH()
{
	const auto cameraEnum = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native120_START_DIALOGUE()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native121_END_DIALOGUE()
{
	// TODO
}

void Game::Native122_IS_DIALOGUE_READY()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native123_CHANGE_WEATHER_PROPERTIES()
{
	const auto fallspeed = _lhvm->Popf();
	const auto overcast = _lhvm->Popf();
	const auto snowfall = _lhvm->Popf();
	const auto rainfall = _lhvm->Popf();
	const auto temperature = _lhvm->Popf();
	const auto storm = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native124_CHANGE_LIGHTNING_PROPERTIES()
{
	const auto forkmax = _lhvm->Popf();
	const auto forkmin = _lhvm->Popf();
	const auto sheetmax = _lhvm->Popf();
	const auto sheetmin = _lhvm->Popf();
	const auto storm = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native125_CHANGE_TIME_FADE_PROPERTIES()
{
	const auto fadeTime = _lhvm->Popf();
	const auto duration = _lhvm->Popf();
	const auto storm = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native126_CHANGE_CLOUD_PROPERTIES()
{
	const auto elevation = _lhvm->Popf();
	const auto blackness = _lhvm->Popf();
	const auto numClouds = _lhvm->Popf();
	const auto storm = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native127_SET_HEADING_AND_SPEED()
{
	const auto speed = _lhvm->Popf();
	const auto position = PopVec();
	const auto unk0 = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native128_START_GAME_SPEED()
{
	// TODO
}

void Game::Native129_END_GAME_SPEED()
{
	// TODO
}

void Game::Native130_BUILD_BUILDING()
{
	const auto desire = _lhvm->Popf();
	const auto position = PopVec();
	// TODO
}

void Game::Native131_SET_AFFECTED_BY_WIND()
{
	const auto object = _lhvm->Pop().uintVal;
	const auto enabled = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native132_WIDESCREEN_TRANSISTION_FINISHED()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native133_GET_RESOURCE()
{
	const auto container = _lhvm->Pop().uintVal;
	const auto resource = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native134_ADD_RESOURCE()
{
	const auto container = _lhvm->Pop().uintVal;
	const auto quantity = _lhvm->Popf();
	const auto resource = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native135_REMOVE_RESOURCE()
{
	const auto container = _lhvm->Pop().uintVal;
	const auto quantity = _lhvm->Popf();
	const auto resource = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native136_GET_TARGET_RELATIVE_POS()
{
	const auto angle = _lhvm->Popf();
	const auto distance = _lhvm->Popf();
	const auto to = PopVec();
	const auto from = PopVec();
	// TODO
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::Native137_STOP_POINTING()
{
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native138_STOP_LOOKING()
{
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native139_LOOK_AT_POSITION()
{
	const auto position = PopVec();
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native140_PLAY_SPIRIT_ANIM()
{
	const auto unk4 = _lhvm->Pop().intVal;
	const auto unk3 = _lhvm->Pop().intVal;
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native141_CALL_IN_NOT_NEAR()
{
	const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	const auto radius = _lhvm->Popf();
	const auto pos = PopVec();
	const auto container = _lhvm->Pop().uintVal;
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native142_SET_CAMERA_ZONE()
{
	const auto filename = PopString();
	// TODO
}

void Game::Native143_GET_OBJECT_STATE()
{
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native144_REVEAL_COUNTDOWN_TIMER()
{
	// TODO
}

void Game::Native145_SET_TIMER_TIME()
{
	const auto time = _lhvm->Popf();
	const auto timer = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native146_CREATE_TIMER()
{
	const auto timeout = _lhvm->Popf();
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native147_GET_TIMER_TIME_REMAINING()
{
	const auto timer = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native148_GET_TIMER_TIME_SINCE_SET()
{
	const auto timer = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native149_MOVE_MUSIC()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native150_GET_INCLUSION_DISTANCE()
{
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native151_GET_LAND_HEIGHT()
{
	const auto position = PopVec();
	const auto& island = Locator::terrainSystem::value();
	const auto elevation = island.GetHeightAt(glm::vec2(position.x, position.z));
	_lhvm->Pushf(elevation);
}

void Game::Native152_LOAD_MAP()
{
	const auto path = PopString();

	auto& fileSystem = Locator::filesystem::value();
	auto mapPath = fileSystem.GetGamePath() / path;
	LoadMap(mapPath);
}

void Game::Native153_STOP_ALL_SCRIPTS_EXCLUDING()
{
	const auto scriptNames = PopString();

	const auto names = GetUniqueWords(scriptNames);
	_lhvm->StopScripts([&names](const std::string& name, [[maybe_unused]] const std::string& filename) -> bool {
		return !names.contains(name);
	});
}

void Game::Native154_STOP_ALL_SCRIPTS_IN_FILES_EXCLUDING()
{
	const auto sourceFilenames = PopString();

	const auto filenames = GetUniqueWords(sourceFilenames);
	_lhvm->StopScripts([&filenames]([[maybe_unused]] const std::string& name, const std::string& filename) -> bool {
		return !filenames.contains(filename);
	});
}

void Game::Native155_STOP_SCRIPT()
{
	const auto scriptName = PopString();
	_lhvm->StopScripts([&scriptName](const std::string& name, [[maybe_unused]] const std::string& filename) -> bool {
		return name == scriptName;
	});
}

void Game::Native156_CLEAR_CLICKED_OBJECT()
{
	// TODO
}

void Game::Native157_CLEAR_CLICKED_POSITION()
{
	// TODO
}

void Game::Native158_POSITION_CLICKED()
{
	const auto unk3 = _lhvm->Pop().intVal;
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native159_RELEASE_FROM_SCRIPT()
{
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native160_GET_OBJECT_HAND_IS_OVER()
{
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native161_ID_POISONED_SIZE()
{
	const auto container = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native162_IS_POISONED()
{
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native163_CALL_POISONED_IN()
{
	const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	const auto container = _lhvm->Pop().uintVal;
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native164_CALL_NOT_POISONED_IN()
{
	const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	const auto container = _lhvm->Pop().uintVal;
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native165_SPIRIT_PLAYED()
{
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native166_CLING_SPIRIT()
{
	const auto yPercent = _lhvm->Popf();
	const auto xPercent = _lhvm->Popf();
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native167_FLY_SPIRIT()
{
	const auto yPercent = _lhvm->Popf();
	const auto xPercent = _lhvm->Popf();
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native168_SET_ID_MOVEABLE()
{
	const auto obj = _lhvm->Pop().uintVal;
	const auto moveable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native169_SET_ID_PICKUPABLE()
{
	const auto obj = _lhvm->Pop().uintVal;
	const auto pickupable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native170_IS_ON_FIRE()
{
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native171_IS_FIRE_NEAR()
{
	const auto radius = _lhvm->Popf();
	const auto position = PopVec();
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native172_STOP_SCRIPTS_IN_FILES()
{
	const auto sourceFilenames = PopString();

	const auto names = GetUniqueWords(sourceFilenames);
	_lhvm->StopScripts([&names]([[maybe_unused]] const std::string& name, const std::string& filename) -> bool {
		return names.contains(filename);
	});
}

void Game::Native173_SET_POISONED()
{
	const auto obj = _lhvm->Pop().uintVal;
	const auto poisoned = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native174_SET_TEMPERATURE()
{
	const auto temperature = _lhvm->Popf();
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native175_SET_ON_FIRE()
{
	const auto burnSpeed = _lhvm->Popf();
	const auto object = _lhvm->Pop().uintVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native176_SET_TARGET()
{
	const auto time = _lhvm->Popf();
	const auto position = PopVec();
	const auto obj = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native177_WALK_PATH()
{
	const auto valTo = _lhvm->Popf();
	const auto valFrom = _lhvm->Popf();
	const auto camera_enum = _lhvm->Pop().intVal;
	const auto forward = static_cast<bool>(_lhvm->Pop().intVal);
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native178_FOCUS_AND_POSITION_FOLLOW()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native179_GET_WALK_PATH_PERCENTAGE()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native180_CAMERA_PROPERTIES()
{
	const auto enableBehind = static_cast<bool>(_lhvm->Pop().intVal);
	const auto angle = _lhvm->Popf();
	const auto speed = _lhvm->Popf();
	const auto distance = _lhvm->Popf();
	// TODO
}

void Game::Native181_ENABLE_DISABLE_MUSIC()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native182_GET_MUSIC_OBJ_DISTANCE()
{
	const auto source = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native183_GET_MUSIC_ENUM_DISTANCE()
{
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native184_SET_MUSIC_PLAY_POSITION()
{
	const auto unk3 = _lhvm->Pop().intVal;
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native185_ATTACH_OBJECT_LEASH_TO_OBJECT()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native186_ATTACH_OBJECT_LEASH_TO_HAND()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native187_DETACH_OBJECT_LEASH()
{
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native188_SET_CREATURE_ONLY_DESIRE()
{
	const auto value = _lhvm->Popf();
	const auto desire = _lhvm->Pop().intVal;
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native189_SET_CREATURE_ONLY_DESIRE_OFF()
{
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native190_RESTART_MUSIC()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native191_MUSIC_PLAYED()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native192_IS_OF_TYPE()
{
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = _lhvm->Pop().intVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native193_CLEAR_HIT_OBJECT()
{
	// TODO
}

void Game::Native194_GAME_THING_HIT()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native195_SPELL_AT_THING()
{
	const auto curl = _lhvm->Popf();
	const auto duration = _lhvm->Popf();
	const auto radius = _lhvm->Popf();
	const auto from = PopVec();
	const auto target = _lhvm->Pop().uintVal;
	const auto spell = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native196_SPELL_AT_POS()
{
	const auto curl = _lhvm->Popf();
	const auto duration = _lhvm->Popf();
	const auto radius = _lhvm->Popf();
	const auto from = PopVec();
	const auto target = PopVec();
	const auto spell = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native197_CALL_PLAYER_CREATURE()
{
	const auto player = _lhvm->Popf();
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native198_GET_SLOWEST_SPEED()
{
	const auto flock = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native199_GET_OBJECT_HELD()
{
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native200_HELP_SYSTEM_ON()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native201_SHAKE_CAMERA()
{
	const auto duration = _lhvm->Popf();
	const auto amplitude = _lhvm->Popf();
	const auto radius = _lhvm->Popf();
	const auto position = PopVec();
	// TODO
}

void Game::Native202_SET_ANIMATION_MODIFY()
{
	const auto creature = _lhvm->Pop().uintVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native203_SET_AVI_SEQUENCE()
{
	const auto aviSequence = _lhvm->Pop().intVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native204_PLAY_GESTURE()
{
	const auto unk4 = _lhvm->Pop().intVal;
	const auto unk3 = _lhvm->Pop().intVal;
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native205_DEV_FUNCTION()
{
	const auto func = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native206_HAS_MOUSE_WHEEL()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native207_NUM_MOUSE_BUTTONS()
{
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native208_SET_CREATURE_DEV_STAGE()
{
	const auto stage = _lhvm->Pop().intVal;
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native209_SET_FIXED_CAM_ROTATION()
{
	const auto unk3 = _lhvm->Pop().intVal;
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native210_SWAP_CREATURE()
{
	const auto toCreature = _lhvm->Pop().uintVal;
	const auto fromCreature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native211_GET_ARENA()
{
	const auto unk4 = _lhvm->Pop().intVal;
	const auto unk3 = _lhvm->Pop().intVal;
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native212_GET_FOOTBALL_PITCH()
{
	const auto town = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native213_STOP_ALL_GAMES()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native214_ATTACH_TO_GAME()
{
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native215_DETACH_FROM_GAME()
{
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native216_DETACH_UNDEFINED_FROM_GAME()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native217_SET_ONLY_FOR_SCRIPTS()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native218_START_MATCH_WITH_REFEREE()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native219_GAME_TEAM_SIZE()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native220_GAME_TYPE()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native221_GAME_SUB_TYPE()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native222_IS_LEASHED()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native223_SET_CREATURE_HOME()
{
	const auto position = PopVec();
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native224_GET_HIT_OBJECT()
{
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native225_GET_OBJECT_WHICH_HIT()
{
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native226_GET_NEAREST_TOWN_OF_PLAYER()
{
	const auto unk4 = _lhvm->Pop().intVal;
	const auto unk3 = _lhvm->Pop().intVal;
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native227_SPELL_AT_POINT()
{
	const auto radius = _lhvm->Popf();
	const auto position = PopVec();
	const auto spell = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native228_SET_ATTACK_OWN_TOWN()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native229_IS_FIGHTING()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native230_SET_MAGIC_RADIUS()
{
	const auto radius = _lhvm->Popf();
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native231_TEMP_TEXT_WITH_NUMBER()
{
	const auto withInteraction = _lhvm->Pop().intVal;
	const auto value = _lhvm->Popf();
	const auto format = PopString();
	const auto singleLine = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native232_RUN_TEXT_WITH_NUMBER()
{
	const auto withInteraction = _lhvm->Pop().intVal;
	const auto number = _lhvm->Popf();
	const auto string = _lhvm->Pop().intVal;
	const auto singleLine = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native233_CREATURE_SPELL_REVERSION()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native234_GET_DESIRE()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native235_GET_EVENTS_PER_SECOND()
{
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native236_GET_TIME_SINCE()
{
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native237_GET_TOTAL_EVENTS()
{
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native238_UPDATE_SNAPSHOT()
{
	const auto challengeId = _lhvm->Pop().intVal;
	const auto argc = _lhvm->Pop().intVal;
	const auto argv = PopVarArg(argc);
	const auto reminderScript = PopString();
	const auto titleStrID = _lhvm->Pop().intVal;
	const auto alignment = _lhvm->Popf();
	const auto success = _lhvm->Popf();
	// TODO
}

void Game::Native239_CREATE_REWARD()
{
	const auto fromSky = static_cast<bool>(_lhvm->Pop().intVal);
	const auto position = PopVec();
	const auto reward = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native240_CREATE_REWARD_IN_TOWN()
{
	const auto fromSky = static_cast<bool>(_lhvm->Pop().intVal);
	const auto position = PopVec();
	const auto town = _lhvm->Pop().uintVal;
	const auto reward = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native241_SET_FADE()
{
	const auto time = _lhvm->Popf();
	const auto blue = _lhvm->Popf();
	const auto green = _lhvm->Popf();
	const auto red = _lhvm->Popf();
	// TODO
}

void Game::Native242_SET_FADE_IN()
{
	const auto duration = _lhvm->Popf();
	// TODO
}

void Game::Native243_FADE_FINISHED()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native244_SET_PLAYER_MAGIC()
{
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native245_HAS_PLAYER_MAGIC()
{
	const auto player = _lhvm->Popf();
	const auto spell = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native246_SPIRIT_SPEAKS()
{
	const auto textID = _lhvm->Pop().intVal;
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native247_BELIEF_FOR_PLAYER()
{
	const auto player = _lhvm->Popf();
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native248_GET_HELP()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native249_SET_LEASH_WORKS()
{
	const auto creature = _lhvm->Pop().uintVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native250_LOAD_MY_CREATURE()
{
	const auto position = PopVec();
	// TODO
}

void Game::Native251_OBJECT_RELATIVE_BELIEF()
{
	const auto belief = _lhvm->Popf();
	const auto player = _lhvm->Popf();
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native252_CREATE_WITH_ANGLE_AND_SCALE()
{
	const auto position = PopVec();
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = static_cast<SCRIPT_OBJECT_TYPE>(_lhvm->Pop().intVal);
	const auto scale = _lhvm->Popf();
	const auto angle = _lhvm->Popf();
	
	entt::entity object = CreateScriptObject(type, subtype, position, 0.0f, 0.0f, angle, 0.0f, scale);
	const auto objId = _refManager.GetReference(object);

	_lhvm->Pusho(objId);
}

void Game::Native253_SET_HELP_SYSTEM()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native254_SET_VIRTUAL_INFLUENCE()
{
	const auto player = _lhvm->Popf();
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native255_SET_ACTIVE()
{
	const auto object = _lhvm->Pop().uintVal;
	const auto active = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native256_THING_VALID()
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

void Game::Native257_VORTEX_FADE_OUT()
{
	const auto vortex = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native258_REMOVE_REACTION_OF_TYPE()
{
	const auto reaction = _lhvm->Pop().intVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native259_CREATURE_LEARN_EVERYTHING_EXCLUDING()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native260_PLAYED_PERCENTAGE()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native261_OBJECT_CAST_BY_OBJECT()
{
	const auto caster = _lhvm->Pop().uintVal;
	const auto spellInstance = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native262_IS_WIND_MAGIC_AT_POS()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native263_CREATE_MIST()
{
	const auto heightRatio = _lhvm->Popf();
	const auto transparency = _lhvm->Popf();
	const auto b = _lhvm->Popf();
	const auto g = _lhvm->Popf();
	const auto r = _lhvm->Popf();
	const auto scale = _lhvm->Popf();
	const auto pos = PopVec();
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native264_SET_MIST_FADE()
{
	const auto duration = _lhvm->Popf();
	const auto endTransparency = _lhvm->Popf();
	const auto startTransparency = _lhvm->Popf();
	const auto endScale = _lhvm->Popf();
	const auto startScale = _lhvm->Popf();
	const auto mist = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native265_GET_OBJECT_FADE()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native266_PLAY_HAND_DEMO()
{
	const auto withoutHandModify = static_cast<bool>(_lhvm->Pop().intVal);
	const auto withPause = static_cast<bool>(_lhvm->Pop().intVal);
	const auto string = PopString();
	// TODO
}

void Game::Native267_IS_PLAYING_HAND_DEMO()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native268_GET_ARSE_POSITION()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::Native269_IS_LEASHED_TO_OBJECT()
{
	const auto target = _lhvm->Pop().uintVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native270_GET_INTERACTION_MAGNITUDE()
{
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native271_IS_CREATURE_AVAILABLE()
{
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native272_CREATE_HIGHLIGHT()
{
	const auto challengeID = _lhvm->Pop().intVal;
	const auto position = PopVec();
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native273_GET_OBJECT_HELD()
{
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native274_GET_ACTION_COUNT()
{
	const auto creature = _lhvm->Pop().uintVal;
	const auto action = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native275_GET_OBJECT_LEASH_TYPE()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native276_SET_FOCUS_FOLLOW()
{
	const auto target = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native277_SET_POSITION_FOLLOW()
{
	const auto target = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native278_SET_FOCUS_AND_POSITION_FOLLOW()
{
	const auto distance = _lhvm->Popf();
	const auto target = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native279_SET_CAMERA_LENS()
{
	const auto lens = _lhvm->Popf();
	// TODO
}

void Game::Native280_MOVE_CAMERA_LENS()
{
	const auto time = _lhvm->Popf();
	const auto lens = _lhvm->Popf();
	// TODO
}

void Game::Native281_CREATURE_REACTION()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native282_CREATURE_IN_DEV_SCRIPT()
{
	const auto creature = _lhvm->Pop().uintVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native283_STORE_CAMERA_DETAILS()
{
	// TODO
}

void Game::Native284_RESTORE_CAMERA_DETAILS()
{
	// TODO
}

void Game::Native285_START_ANGLE_SOUND()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native286_SET_CAMERA_POS_FOC_LENS()
{
	const auto unk6 = _lhvm->Pop().intVal;
	const auto unk5 = _lhvm->Pop().intVal;
	const auto unk4 = _lhvm->Pop().intVal;
	const auto unk3 = _lhvm->Pop().intVal;
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native287_MOVE_CAMERA_POS_FOC_LENS()
{
	const auto unk7 = _lhvm->Pop().intVal;
	const auto unk6 = _lhvm->Pop().intVal;
	const auto unk5 = _lhvm->Pop().intVal;
	const auto unk4 = _lhvm->Pop().intVal;
	const auto unk3 = _lhvm->Pop().intVal;
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native288_GAME_TIME_ON_OFF()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native289_MOVE_GAME_TIME()
{
	const auto duration = _lhvm->Popf();
	const auto hourOfTheDay = _lhvm->Popf();
	// TODO
}

void Game::Native290_SET_HIGH_GRAPHICS_DETAIL()
{
	const auto object = _lhvm->Pop().uintVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native291_SET_SKELETON()
{
	const auto object = _lhvm->Pop().uintVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native292_IS_SKELETON()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native293_PLAYER_SPELL_CAST_TIME()
{
	const auto player = _lhvm->Popf();
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native294_PLAYER_SPELL_LAST_CAST()
{
	const auto player = _lhvm->Popf();
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native295_GET_LAST_SPELL_CAST_POS()
{
	const auto player = _lhvm->Popf();
	// TODO
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::Native296_ADD_SPOT_VISUAL_TARGET_POS()
{
	const auto position = PopVec();
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native297_ADD_SPOT_VISUAL_TARGET_OBJECT()
{
	const auto target = _lhvm->Pop().uintVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native298_SET_INDESTRUCTABLE()
{
	const auto object = _lhvm->Pop().uintVal;
	const auto indestructible = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native299_SET_GRAPHICS_CLIPPING()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native300_SPIRIT_APPEAR()
{
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native301_SPIRIT_DISAPPEAR()
{
	const auto spirit = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native302_SET_FOCUS_ON_OBJECT()
{
	const auto target = _lhvm->Pop().uintVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native303_RELEASE_OBJECT_FOCUS()
{
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native304_IMMERSION_EXISTS()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native305_SET_DRAW_LEASH()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native306_SET_DRAW_HIGHLIGHT()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native307_SET_OPEN_CLOSE()
{
	const auto object = _lhvm->Pop().uintVal;
	const auto open = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native308_SET_INTRO_BUILDING()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native309_CREATURE_FORCE_FRIENDS()
{
	const auto targetCreature = _lhvm->Pop().uintVal;
	const auto creature = _lhvm->Pop().uintVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native310_MOVE_COMPUTER_PLAYER_POSITION()
{
	const auto withFixedHeight = static_cast<bool>(_lhvm->Pop().intVal);
	const auto speed = _lhvm->Popf();
	const auto position = PopVec();
	const auto player = _lhvm->Popf();
	// TODO
}

void Game::Native311_ENABLE_DISABLE_COMPUTER_PLAYER()
{
	const auto player = _lhvm->Popf();
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native312_GET_COMPUTER_PLAYER_POSITION()
{
	const auto player = _lhvm->Popf();
	// TODO
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::Native313_SET_COMPUTER_PLAYER_POSITION()
{
	const auto withFixedHeight = static_cast<bool>(_lhvm->Pop().intVal);
	const auto position = PopVec();
	const auto player = _lhvm->Popf();
	// TODO
}

void Game::Native314_GET_STORED_CAMERA_POSITION()
{
	// TODO
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::Native315_GET_STORED_CAMERA_FOCUS()
{
	// TODO
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::Native316_CALL_NEAR_IN_STATE()
{
	const auto excludingScripted = static_cast<bool>(_lhvm->Pop().intVal);
	const auto radius = _lhvm->Popf();
	const auto position = PopVec();
	const auto state = _lhvm->Pop().intVal;
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native317_SET_CREATURE_SOUND()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native318_CREATURE_INTERACTING_WITH()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native319_SET_SUN_DRAW()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native320_OBJECT_INFO_BITS()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native321_SET_HURT_BY_FIRE()
{
	const auto object = _lhvm->Pop().uintVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native322_CONFINED_OBJECT()
{
	const auto unk4 = _lhvm->Pop().intVal;
	const auto unk3 = _lhvm->Pop().intVal;
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native323_CLEAR_CONFINED_OBJECT()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native324_GET_OBJECT_FLOCK()
{
	const auto member = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native325_SET_PLAYER_BELIEF()
{
	const auto belief = _lhvm->Popf();
	const auto player = _lhvm->Popf();
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native326_PLAY_JC_SPECIAL()
{
	const auto feature = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native327_IS_PLAYING_JC_SPECIAL()
{
	const auto feature = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native328_VORTEX_PARAMETERS()
{
	const auto flock = _lhvm->Pop().uintVal;
	const auto radius = _lhvm->Popf();
	const auto distance = _lhvm->Popf();
	const auto position = PopVec();
	const auto town = _lhvm->Pop().uintVal;
	const auto vortex = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native329_LOAD_CREATURE()
{
	const auto position = PopVec();
	const auto player = _lhvm->Popf();
	const auto mindFilename = PopString();
	const auto type = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native330_IS_SPELL_CHARGING()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native331_IS_THAT_SPELL_CHARGING()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native332_OPPOSING_CREATURE()
{
	const auto god = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native333_FLOCK_WITHIN_LIMITS()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native334_HIGHLIGHT_PROPERTIES()
{
	const auto category = _lhvm->Pop().intVal;
	const auto text = _lhvm->Pop().intVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native335_LAST_MUSIC_LINE()
{
	const auto line = _lhvm->Popf();
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native336_HAND_DEMO_TRIGGER()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native337_GET_BELLY_POSITION()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::Native338_SET_CREATURE_CREED_PROPERTIES()
{
	const auto time = _lhvm->Popf();
	const auto power = _lhvm->Popf();
	const auto scale = _lhvm->Popf();
	const auto handGlow = _lhvm->Pop().intVal;
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native339_GAME_THING_CAN_VIEW_CAMERA()
{
	const auto degrees = _lhvm->Popf();
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native340_GAME_PLAY_SAY_SOUND_EFFECT()
{
	const auto withPosition = static_cast<bool>(_lhvm->Pop().intVal);
	const auto position = PopVec();
	const auto sound = _lhvm->Pop().intVal;
	const auto extra = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native341_SET_TOWN_DESIRE_BOOST()
{
	const auto boost = _lhvm->Popf();
	const auto desire = _lhvm->Pop().intVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native342_IS_LOCKED_INTERACTION()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native343_SET_CREATURE_NAME()
{
	const auto textID = _lhvm->Pop().intVal;
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native344_COMPUTER_PLAYER_READY()
{
	const auto player = _lhvm->Popf();
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native345_ENABLE_DISABLE_COMPUTER_PLAYER()
{
	const auto player = _lhvm->Popf();
	const auto pause = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native346_CLEAR_ACTOR_MIND()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native347_ENTER_EXIT_CITADEL()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native348_START_ANGLE_SOUND()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native349_THING_JC_SPECIAL()
{
	const auto target = _lhvm->Pop().uintVal;
	const auto feature = _lhvm->Pop().intVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native350_MUSIC_PLAYED()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native351_UPDATE_SNAPSHOT_PICTURE()
{
	const auto challengeID = _lhvm->Pop().intVal;
	const auto takingPicture = static_cast<bool>(_lhvm->Pop().intVal);
	const auto titleStrID = _lhvm->Pop().intVal;
	const auto alignment = _lhvm->Popf();
	const auto success = _lhvm->Popf();
	const auto focus = PopVec();
	const auto position = PopVec();
	// TODO
}

void Game::Native352_STOP_SCRIPTS_IN_FILES_EXCLUDING()
{
	const auto scriptNames = PopString();
	const auto sourceFilenames = PopString();

	const auto names = GetUniqueWords(scriptNames);
	const auto filenames = GetUniqueWords(sourceFilenames);
	_lhvm->StopScripts([&names, &filenames](const std::string& name, const std::string& filename) -> bool {
					   return filenames.contains(filename) && !names.contains(name);
	});
}

void Game::Native353_CREATE_RANDOM_VILLAGER_OF_TRIBE()
{
	const auto position = PopVec();
	const auto tribe = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native354_TOGGLE_LEASH()
{
	const auto player = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native355_GAME_SET_MANA()
{
	const auto mana = _lhvm->Popf();
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native356_SET_MAGIC_PROPERTIES()
{
	const auto duration = _lhvm->Popf();
	const auto magicType = _lhvm->Pop().intVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native357_SET_GAME_SOUND()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native358_SEX_IS_MALE()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native359_GET_FIRST_HELP()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native360_GET_LAST_HELP()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native361_IS_ACTIVE()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native362_SET_BOOKMARK_POSITION()
{
	const auto unk3 = _lhvm->Pop().intVal;
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native363_SET_SCAFFOLD_PROPERTIES()
{
	const auto destroy = static_cast<bool>(_lhvm->Pop().intVal);
	const auto size = _lhvm->Popf();
	const auto type = _lhvm->Pop().intVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native364_SET_COMPUTER_PLAYER_PERSONALITY()
{
	const auto probability = _lhvm->Popf();
	const auto aspect = PopString();
	const auto player = _lhvm->Popf();
	// TODO
}

void Game::Native365_SET_COMPUTER_PLAYER_SUPPRESSION()
{
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native366_FORCE_COMPUTER_PLAYER_ACTION()
{
	const auto obj2 = _lhvm->Pop().uintVal;
	const auto obj1 = _lhvm->Pop().uintVal;
	const auto action = PopString();
	const auto player = _lhvm->Popf();
	// TODO
}

void Game::Native367_QUEUE_COMPUTER_PLAYER_ACTION()
{
	const auto obj2 = _lhvm->Pop().uintVal;
	const auto obj1 = _lhvm->Pop().uintVal;
	const auto action = PopString();
	const auto player = _lhvm->Popf();
	// TODO
}

void Game::Native368_GET_TOWN_WITH_ID()
{
	const auto id = _lhvm->Popf();
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native369_SET_DISCIPLE()
{
	const auto withSound = static_cast<bool>(_lhvm->Pop().intVal);
	const auto discipleType = _lhvm->Pop().intVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native370_RELEASE_COMPUTER_PLAYER()
{
	const auto player = _lhvm->Popf();
	// TODO
}

void Game::Native371_SET_COMPUTER_PLAYER_SPEED()
{
	const auto speed = _lhvm->Popf();
	const auto player = _lhvm->Popf();
	// TODO
}

void Game::Native372_SET_FOCUS_FOLLOW_COMPUTER_PLAYER()
{
	const auto player = _lhvm->Popf();
	// TODO
}

void Game::Native373_SET_POSITION_FOLLOW_COMPUTER_PLAYER()
{
	const auto player = _lhvm->Popf();
	// TODO
}

void Game::Native374_CALL_COMPUTER_PLAYER()
{
	const auto player = _lhvm->Popf();
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native375_CALL_BUILDING_IN_TOWN()
{
	const auto unk3 = _lhvm->Pop().intVal;
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native376_SET_CAN_BUILD_WORSHIPSITE()
{
	const auto object = _lhvm->Pop().uintVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native377_GET_FACING_CAMERA_POSITION()
{
	const auto distance = _lhvm->Popf();
	// TODO
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::Native378_SET_COMPUTER_PLAYER_ATTITUDE()
{
	const auto attitude = _lhvm->Popf();
	const auto player2 = _lhvm->Popf();
	const auto player1 = _lhvm->Popf();
	// TODO
}

void Game::Native379_GET_COMPUTER_PLAYER_ATTITUDE()
{
	const auto player2 = _lhvm->Popf();
	const auto player1 = _lhvm->Popf();
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native380_LOAD_COMPUTER_PLAYER_PERSONALITY()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native381_SAVE_COMPUTER_PLAYER_PERSONALITY()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native382_SET_PLAYER_ALLY()
{
	const auto percentage = _lhvm->Popf();
	const auto player2 = _lhvm->Popf();
	const auto player1 = _lhvm->Popf();
	// TODO
}

void Game::Native383_CALL_FLYING()
{
	const auto excluding = static_cast<bool>(_lhvm->Pop().intVal);
	const auto radius = _lhvm->Popf();
	const auto position = PopVec();
	const auto subtype = _lhvm->Pop().intVal;
	const auto type = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native384_SET_OBJECT_FADE_IN()
{
	const auto time = _lhvm->Popf();
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native385_IS_AFFECTED_BY_SPELL()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native386_SET_MAGIC_IN_OBJECT()
{
	const auto object = _lhvm->Pop().uintVal;
	const auto MAGIC_TYPE = _lhvm->Pop().intVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native387_ID_ADULT_SIZE()
{
	const auto container = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native388_OBJECT_CAPACITY()
{
	const auto container = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native389_OBJECT_ADULT_CAPACITY()
{
	const auto container = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native390_SET_CREATURE_AUTO_FIGHTING()
{
	const auto creature = _lhvm->Pop().uintVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native391_IS_AUTO_FIGHTING()
{
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native392_SET_CREATURE_QUEUE_FIGHT_MOVE()
{
	const auto move = _lhvm->Pop().intVal;
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native393_SET_CREATURE_QUEUE_FIGHT_SPELL()
{
	const auto spell = _lhvm->Pop().intVal;
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native394_SET_CREATURE_QUEUE_FIGHT_STEP()
{
	const auto step = _lhvm->Pop().intVal;
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native395_GET_CREATURE_FIGHT_ACTION()
{
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native396_CREATURE_FIGHT_QUEUE_HITS()
{
	const auto creature = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native397_SQUARE_ROOT()
{
	const auto value = _lhvm->Popf();
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native398_GET_PLAYER_ALLY()
{
	const auto player2 = _lhvm->Popf();
	const auto player1 = _lhvm->Popf();
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native399_SET_PLAYER_WIND_RESISTANCE()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native400_GET_PLAYER_WIND_RESISTANCE()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native401_PAUSE_UNPAUSE_CLIMATE_SYSTEM()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native402_PAUSE_UNPAUSE_STORM_CREATION_IN_CLIMATE_SYSTEM()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native403_GET_MANA_FOR_SPELL()
{
	const auto spell = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native404_KILL_STORMS_IN_AREA()
{
	const auto radius = _lhvm->Popf();
	const auto position = PopVec();
	// TODO
}

void Game::Native405_INSIDE_TEMPLE()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native406_RESTART_OBJECT()
{
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native407_SET_GAME_TIME_PROPERTIES()
{
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native408_RESET_GAME_TIME_PROPERTIES()
{
	// TODO
}

void Game::Native409_SOUND_EXISTS()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native410_GET_TOWN_WORSHIP_DEATHS()
{
	const auto town = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native411_GAME_CLEAR_DIALOGUE()
{
	// TODO
}

void Game::Native412_GAME_CLOSE_DIALOGUE()
{
	// TODO
}

void Game::Native413_GET_HAND_STATE()
{
	// TODO
	_lhvm->Pushi(0);
}

void Game::Native414_SET_INTERFACE_CITADEL()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native415_MAP_SCRIPT_FUNCTION()
{
	const auto command = PopString();
	// TODO
}

void Game::Native416_WITHIN_ROTATION()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native417_GET_PLAYER_TOWN_TOTAL()
{
	const auto player = _lhvm->Popf();
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native418_SPIRIT_SCREEN_POINT()
{
	const auto unk2 = _lhvm->Pop().intVal;
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native419_KEY_DOWN()
{
	const auto key = _lhvm->Pop().intVal;
	// TODO translate key to physical key code
	_lhvm->Pushb(_keysdown.contains(key));
}

void Game::Native420_SET_FIGHT_EXIT()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native421_GET_OBJECT_CLICKED()
{
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native422_GET_MANA()
{
	const auto worshipSite = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native423_CLEAR_PLAYER_SPELL_CHARGING()
{
	const auto player = _lhvm->Popf();
	// TODO
}

void Game::Native424_STOP_SOUND_EFFECT()
{
	const auto soundbank = _lhvm->Pop().intVal;
	const auto sound = _lhvm->Pop().intVal;
	const auto alwaysFalse = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native425_GET_TOTEM_STATUE()
{
	const auto town = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native426_SET_SET_ON_FIRE()
{
	const auto object = _lhvm->Pop().uintVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native427_SET_LAND_BALANCE()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native428_SET_OBJECT_BELIEF_SCALE()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native429_START_IMMERSION()
{
	const auto effect = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native430_STOP_IMMERSION()
{
	const auto effect = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native431_STOP_ALL_IMMERSION()
{
	// TODO
}

void Game::Native432_SET_CREATURE_IN_TEMPLE()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native433_GAME_DRAW_TEXT()
{
	const auto fade = _lhvm->Popf();
	const auto size = _lhvm->Popf();
	const auto height = _lhvm->Popf();
	const auto width = _lhvm->Popf();
	const auto down = _lhvm->Popf();
	const auto across = _lhvm->Popf();
	const auto textID = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native434_GAME_DRAW_TEMP_TEXT()
{
	const auto fade = _lhvm->Popf();
	const auto size = _lhvm->Popf();
	const auto height = _lhvm->Popf();
	const auto width = _lhvm->Popf();
	const auto down = _lhvm->Popf();
	const auto across = _lhvm->Popf();
	const auto string = PopString();
	// TODO
}

void Game::Native435_FADE_ALL_DRAW_TEXT()
{
	const auto time = _lhvm->Popf();
	// TODO
}

void Game::Native436_SET_DRAW_TEXT_COLOUR()
{
	const auto blue = _lhvm->Popf();
	const auto green = _lhvm->Popf();
	const auto red = _lhvm->Popf();
	// TODO
}

void Game::Native437_SET_CLIPPING_WINDOW()
{
	const auto time = _lhvm->Popf();
	const auto height = _lhvm->Popf();
	const auto width = _lhvm->Popf();
	const auto down = _lhvm->Popf();
	const auto across = _lhvm->Popf();
	// TODO
}

void Game::Native438_CLEAR_CLIPPING_WINDOW()
{
	const auto time = _lhvm->Popf();
	// TODO
}

void Game::Native439_SAVE_GAME_IN_SLOT()
{
	const auto slot = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native440_SET_OBJECT_CARRYING()
{
	const auto carriedObj = _lhvm->Pop().intVal;
	const auto object = _lhvm->Pop().uintVal;
	// TODO
}

void Game::Native441_POS_VALID_FOR_CREATURE()
{
	const auto position = PopVec();
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native442_GET_TIME_SINCE_OBJECT_ATTACKED()
{
	const auto town = _lhvm->Pop().uintVal;
	const auto player = _lhvm->Popf();
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native443_GET_TOWN_AND_VILLAGER_HEALTH_TOTAL()
{
	const auto town = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native444_GAME_ADD_FOR_BUILDING()
{
	const auto unk1 = _lhvm->Pop().intVal;
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native445_ENABLE_DISABLE_ALIGNMENT_MUSIC()
{
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native446_GET_DEAD_LIVING()
{
	const auto radius = _lhvm->Popf();
	const auto position = PopVec();
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native447_ATTACH_SOUND_TAG()
{
	const auto target = _lhvm->Pop().uintVal;
	const auto soundbank = _lhvm->Pop().intVal;
	const auto sound = _lhvm->Pop().intVal;
	const auto threeD = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native448_DETACH_SOUND_TAG()
{
	const auto target = _lhvm->Pop().uintVal;
	const auto soundbank = _lhvm->Pop().intVal;
	const auto sound = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native449_GET_SACRIFICE_TOTAL()
{
	const auto worshipSite = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pushf(0.0f);
}

void Game::Native450_GAME_SOUND_PLAYING()
{
	const auto soundbank = _lhvm->Pop().intVal;
	const auto sound = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native451_GET_TEMPLE_POSITION()
{
	const auto player = _lhvm->Popf();
	// TODO
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::Native452_CREATURE_AUTOSCALE()
{
	const auto size = _lhvm->Popf();
	const auto creature = _lhvm->Pop().uintVal;
	const auto enable = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
}

void Game::Native453_GET_SPELL_ICON_IN_TEMPLE()
{
	const auto temple = _lhvm->Pop().uintVal;
	const auto spell = _lhvm->Pop().intVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native454_GAME_CLEAR_COMPUTER_PLAYER_ACTIONS()
{
	const auto player = _lhvm->Popf();
	// TODO
}

void Game::Native455_GET_FIRST_IN_CONTAINER()
{
	const auto container = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native456_GET_NEXT_IN_CONTAINER()
{
	const auto after = _lhvm->Pop().uintVal;
	const auto container = _lhvm->Pop().uintVal;
	// TODO
	_lhvm->Pusho(0);
}

void Game::Native457_GET_TEMPLE_ENTRANCE_POSITION()
{
	const auto height = _lhvm->Popf();
	const auto radius = _lhvm->Popf();
	const auto player = _lhvm->Popf();
	// TODO
	_lhvm->Pushv(0.0f); // x
	_lhvm->Pushv(0.0f); // y
	_lhvm->Pushv(0.0f); // z
}

void Game::Native458_SAY_SOUND_EFFECT_PLAYING()
{
	const auto sound = _lhvm->Pop().intVal;
	const auto alwaysFalse = static_cast<bool>(_lhvm->Pop().intVal);
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native459_SET_HAND_DEMO_KEYS()
{
	const auto unk0 = _lhvm->Pop().intVal;
	// TODO
}

void Game::Native460_CAN_SKIP_TUTORIAL()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native461_CAN_SKIP_CREATURE_TRAINING()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native462_IS_KEEPING_OLD_CREATURE()
{
	// TODO
	_lhvm->Pushb(false);
}

void Game::Native463_CURRENT_PROFILE_HAS_CREATURE()
{
	// TODO
	_lhvm->Pushb(false);
}
#pragma warning(pop)

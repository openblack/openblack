/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Game.h"

#include <cstdint>

#include <string>

#include <LHVM/LHVM.h>
#include <Serializer/FotFile.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/sinks/android_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "3D/Camera.h"
#include "3D/CreatureBody.h"
#include "3D/LandIslandInterface.h"
#include "3D/Sky.h"
#include "3D/Water.h"
#include "Audio/AudioManagerInterface.h"
#include "Common/EventManager.h"
#include "Common/RandomNumberManager.h"
#include "Common/StringUtils.h"
#include "Debug/Gui.h"
#include "ECS/Archetypes/HandArchetype.h"
#include "ECS/Archetypes/PlayerArchetype.h"
#include "ECS/Components/Fixed.h"
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
#include "GameWindow.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/Texture2D.h"
#include "LHScriptX/Script.h"
#include "Locator.h"
#include "PackFile.h"
#include "Parsers/InfoFile.h"
#include "Profiler.h"
#include "Renderer.h"
#include "Resources/Loaders.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::lhscriptx;
using namespace std::chrono_literals;

const std::string k_WindowTitle = "openblack";

Game* Game::sInstance = nullptr;

Game::Game(Arguments&& args)
    : _gamePath(args.gamePath)
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
		_window =
		    std::make_unique<GameWindow>(k_WindowTitle, args.windowWidth, args.windowHeight, args.displayMode, extraFlags);
	}
	try
	{
		_renderer = std::make_unique<Renderer>(_window.get(), args.rendererType, args.vsync);
	}
	catch (std::runtime_error& exception)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to create renderer", exception.what(), nullptr);
		throw exception;
	}

	_gui = debug::gui::Gui::Create(_window.get(), graphics::RenderPass::ImGui, args.scale);

	_eventManager->AddHandler(std::function([this](const SDL_Event& event) {
		// If gui captures this input, do not propagate
		if (!this->_gui->ProcessEvents(event))
		{
			this->_camera->ProcessSDLEvent(event);
			this->_config.running = this->ProcessEvents(event);
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

	_water.reset();
	_sky.reset();
	_gui.reset();
	_renderer.reset();
	_window.reset();
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

	switch (event.type)
	{
	case SDL_QUIT:
		return false;
	case SDL_WINDOWEVENT:
		if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == _window->GetID())
		{
			return false;
		}
		else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
		{
			const auto width = static_cast<uint16_t>(event.window.data1);
			const auto height = static_cast<uint16_t>(event.window.data2);
			_renderer->Reset(width, height);
			_renderer->ConfigureView(graphics::RenderPass::Main, width, height);

			auto aspect = _window->GetAspectRatio();
			_camera->SetProjectionMatrixPerspective(_config.cameraXFov, aspect, _config.cameraNearClip, _config.cameraFarClip);
		}
		break;
	case SDL_KEYDOWN:
		switch (event.key.keysym.sym)
		{
		case SDLK_ESCAPE:
			return false;
		case SDLK_f:
			_window->SetFullscreen(true);
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
				auto handPosition = _handPose * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				const auto index = static_cast<uint8_t>(event.key.keysym.sym - SDLK_1);
				Locator::cameraBookmarkSystem::value().SetBookmark(index, handPosition);
			}
			else if (event.key.keysym.mod == 0)
			{
				// TODO(#348): Fly-to the bookmark
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
			glm::ivec2 screenSize;
			_window->GetSize(screenSize.x, screenSize.y);
			SDL_SetRelativeMouseMode((event.type == SDL_MOUSEBUTTONDOWN) ? SDL_TRUE : SDL_FALSE);
			SDL_WarpMouseInWindow(_window->GetHandle(), screenSize.x / 2, screenSize.y / 2);
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
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0)
		{
			_eventManager->Create<SDL_Event>(e);
		}
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
			glm::ivec2 screenSize {};
			if (_window)
			{
				_window->GetSize(screenSize.x, screenSize.y);
			}

			const auto scale = glm::vec3(50.0f, 50.0f, 50.0f);
			if (screenSize.x > 0 && screenSize.y > 0)
			{
				glm::vec3 rayOrigin;
				glm::vec3 rayDirection;
				_camera->DeprojectScreenToWorld(_mousePosition, screenSize, rayOrigin, rayDirection);
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
				_renderer->UpdateDebugCrossUniforms(_handPose);
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
			auto cameraRotation = _camera->GetRotation();
			handTransform.rotation = glm::eulerAngleY(-cameraRotation.y) * modelRotationCorrection;
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
	ecs::systems::InitializeGame();
	auto& fileSystem = Locator::filesystem::value();
	auto& resources = Locator::resources::value();
	auto& meshManager = resources.GetMeshes();
	auto& textureManager = resources.GetTextures();
	auto& animationManager = resources.GetAnimations();
	auto& levelManager = resources.GetLevels();
	auto& soundManager = resources.GetSounds();
	auto& glowManager = resources.GetGlows();

	fileSystem.SetGamePath(_gamePath);

	if (fileSystem.GetGamePath().empty())
	{
		// no key, don't guess, let the user know to set the command param
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Game Path missing",
		                         "Game path was not supplied, use the -g "
		                         "command parameter to set it.",
		                         nullptr);
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to find the GameDir.");
		return false;
	}

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

	fileSystem.Iterate(fileSystem.GetPath<filesystem::Path::Citadel>() / "engine", false,
	                   [&meshManager, &glowManager](const std::filesystem::path& f) {
		                   if (f.extension() == ".zzz")
		                   {
			                   SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading interior temple mesh: {}", f.stem().string());
			                   try
			                   {
				                   meshManager.Load(fmt::format("temple/interior/{}", f.stem().string()),
				                                    resources::L3DLoader::FromDiskTag {}, f);
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
				                                    resources::GlowLoader::FromDiskTag {}, f);
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
	pack.Open(fileSystem.ReadAll(fileSystem.GetPath<Path::Data>(true) / "AllMeshes.g3d"));
#else
	pack.Open(fileSystem.GetPath<Path::Data>(true) / "AllMeshes.g3d");
#endif
	const auto& meshes = pack.GetMeshes();
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
	animationPack.Open(fileSystem.ReadAll(fileSystem.GetPath<Path::Data>(true) / "AllAnims.anm"));
#else
	animationPack.Open(fileSystem.GetPath<Path::Data>(true) / "AllAnims.anm");
#endif
	const auto& animations = animationPack.GetAnimations();
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
	fileSystem.Iterate(fileSystem.GetPath<Path::Audio>(), true, [&audioManager, &soundManager](const std::filesystem::path& f) {
		if (f.extension() != ".sad")
		{
			return;
		}

		pack::PackFile soundPack;
		SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Opening sound pack {}", f.filename().string());
		soundPack.Open(f);
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
					SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Empty sound buffer found for {}. Skipping", soundName.string());
					return;
				}

				const entt::id_type id = entt::hashed_string(fmt::format("{}/{}", groupName, i).c_str());
				const std::vector<std::vector<uint8_t>> buffer = {audioData[i]};
				SPDLOG_LOGGER_DEBUG(spdlog::get("audio"), "Loading sound {}/{}", groupName, i);
				soundManager.Load(id, resources::SoundLoader::FromBufferTag {}, audioHeaders[i], buffer);
				audioManager.AddToSoundGroup(groupName, id);
			}
		}
	});

	// create our camera
	_camera = std::make_unique<Camera>();
	auto aspect = _window ? _window->GetAspectRatio() : 1.0f;
	_camera->SetProjectionMatrixPerspective(_config.cameraXFov, aspect, _config.cameraNearClip, _config.cameraFarClip);

	_camera->SetPosition(glm::vec3(1441.56f, 24.764f, 2081.76f));
	_camera->SetRotation(glm::radians(glm::vec3(0.0f, -45.0f, 0.0f)));

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
	return true;
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
#if __ANDROID__
		//  Android has a complicated permissions API, must call java code to read contents.
		_lhvm->Open(fileSystem.ReadAll(fileSystem.FindPath(challengePath)));
#else
		_lhvm->Open(fileSystem.FindPath(challengePath));
#endif
	}
	else
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Challenge file not found at {}",
		                    (fileSystem.GetGamePath() / challengePath).generic_string());
		return false;
	}

	// Initialize the Acceleration Structure
	Locator::entitiesMap::value().Rebuild();

	if (_window)
	{
		int width;
		int height;
		_window->GetSize(width, height);
		_renderer->ConfigureView(graphics::RenderPass::Main, static_cast<uint16_t>(width), static_cast<uint16_t>(height));
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

	auto data = fileSystem.ReadAll(path);
	std::string source(reinterpret_cast<const char*>(data.data()), data.size());

	// Reset everything. Deletes all entities and their components
	Locator::entitiesRegistry::value().Reset();

	// We need a hand for the player
	_handEntity = ecs::archetypes::HandArchetype::Create(glm::vec3(0.0f), glm::half_pi<float>(), 0.0f, glm::half_pi<float>(),
	                                                     0.01f, false);

	Script script;
	script.Load(source);

	// Each released map comes with an optional .fot file which contains the footpath information for the map
	auto stem = string_utils::LowerCase(path.stem().generic_string());
	auto fotPath = fileSystem.GetPath<filesystem::Path::Landscape>() / fmt::format("{}.fot", stem);

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
	ecs::systems::InitializeLevel(fixedName);

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

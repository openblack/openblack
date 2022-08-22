/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

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
#include "3D/LandIsland.h"
#include "3D/Sky.h"
#include "3D/Water.h"
#include "Common/EventManager.h"
#include "Common/FileSystem.h"
#include "Common/RandomNumberManagerProduction.h"
#include "Common/StringUtils.h"
#include "Debug/Gui.h"
#include "ECS/Archetypes/HandArchetype.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Transform.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CameraBookmarkSystemInterface.h"
#include "ECS/Systems/CameraPathSystemInterface.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/PathfindingSystemInterface.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "ECS/Systems/TownSystemInterface.h"
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
#include "Resources/ResourceManager.h"
#include "Resources/Resources.h"

#ifdef WIN32
#include <Windows.h>
#endif

using namespace openblack;
using namespace openblack::lhscriptx;
using namespace std::chrono_literals;

const std::string k_WindowTitle = "openblack";

Game* Game::sInstance = nullptr;

Game::Game(Arguments&& args)
    : _eventManager(std::make_unique<EventManager>())
    , _fileSystem(std::make_unique<FileSystem>())
    , _entityRegistry(std::make_unique<ecs::Registry>())
    , _entityMap(std::make_unique<ecs::Map>())
    , _config()
    , _handPose()
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
	SetGamePath(args.gamePath);
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
	_fileSystem->SetGamePath(GetGamePath());
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "The GamePath is \"{}\".", _fileSystem->GetGamePath().generic_string());

	if (std::filesystem::path(args.startLevel).is_absolute())
	{
		_startMap = args.startLevel;
		if (std::find(_startMap.begin(), _startMap.end(), "Scripts") != _startMap.end())
		{
			auto p = _startMap;
			while (p.filename() != "Scripts" && p != p.parent_path())
			{
				p = p.parent_path();
			}
			_fileSystem->AddAdditionalPath(p.parent_path());
		}
		else
		{
			_fileSystem->AddAdditionalPath(_startMap.parent_path());
		}
	}
	else
	{
		_startMap = _fileSystem->ScriptsPath() / args.startLevel;
	}

	_gui = debug::gui::Gui::Create(_window.get(), graphics::RenderPass::ImGui, args.scale);

	_eventManager->AddHandler(std::function([this](const SDL_Event& event) {
		// If gui captures this input, do not propagate
		if (!this->_gui->ProcessEventSdl2(event))
		{
			this->_camera->ProcessSDLEvent(event);
			this->_config.running = this->ProcessEvents(event);
		}
	}));
}

Game::~Game()
{
	// Manually delete the assets here before BGFX renderer clears its buffers resulting in invalid handles in our assets
	if (Locator::resources::has_value())
	{
		auto& resources = Locator::resources::value();
		resources.GetMeshes().Clear();
		resources.GetTextures().Clear();
		resources.GetAnimations().Clear();
	}

	Locator::rendereringSystem::reset();
	Locator::dynamicsSystem::reset();
	Locator::cameraBookmarkSystem::reset();
	Locator::livingActionSystem::reset();
	Locator::townSystem::reset();
	Locator::pathfindingSystem::reset();

	_water.reset();
	_sky.reset();
	_landIsland.reset();
	_entityRegistry.reset();
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
	_entityMap->Rebuild();

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
	Locator::cameraPathSystem::value().Update(deltaTime);

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

		// Update Hand
		if (!_handGripping)
		{
			const glm::vec3 handOffset(0, 1.5f, 0);
			const glm::mat4 modelRotationCorrection = glm::eulerAngleX(glm::radians(90.0f));
			auto& handTransform = _entityRegistry->Get<ecs::components::Transform>(_handEntity);
			// TODO(#480): move using velocity rather than snapping hand to intersectionTransform
			handTransform.position = intersectionTransform.position;
			auto cameraRotation = _camera->GetRotation();
			handTransform.rotation = glm::eulerAngleY(glm::radians(-cameraRotation.y)) * modelRotationCorrection;
			handTransform.rotation = intersectionTransform.rotation * handTransform.rotation;
			handTransform.position += intersectionTransform.rotation * handOffset;
			_entityRegistry->SetDirty();
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

	return _config.numFramesToSimulate == 0 || _frameCount < _config.numFramesToSimulate;
}

bool Game::Initialize()
{
	Locator::resources::emplace<resources::Resources>();
	Locator::rng::emplace<RandomNumberManagerProduction>();
	Locator::temple::emplace<TempleInterior>();
	ecs::systems::InitializeGame();
	auto& resources = Locator::resources::value();
	auto& meshManager = resources.GetMeshes();
	auto& textureManager = resources.GetTextures();
	auto& animationManager = resources.GetAnimations();
	auto& levelManager = resources.GetLevels();
	auto& camManager = resources.GetCameraPaths();
	const auto citadelExteriors = _fileSystem->FindPath(_fileSystem->CitadelPath() / "OutsideMeshes");
	for (const auto& f : std::filesystem::directory_iterator {citadelExteriors})
	{
		if (f.path().extension() == ".zzz")
		{
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading exterior temple mesh: {}", f.path().stem().string());
			try
			{
				meshManager.Load(fmt::format("temple/{}", f.path().stem().string()), resources::L3DLoader::FromDiskTag {}, f);
			}
			catch (std::runtime_error& err)
			{
				SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
			}
		}
	}

	meshManager.Load("hand", resources::L3DLoader::FromDiskTag {},
	                 _fileSystem->FindPath(_fileSystem->CreatureMeshPath() / "Hand_Boned_Base2.l3d"));
	meshManager.Load("coffre", resources::L3DLoader::FromDiskTag {},
	                 _fileSystem->FindPath(_fileSystem->MiscPath() / "coffre.l3d"));
	pack::PackFile pack;
	pack.Open(_fileSystem->FindPath(_fileSystem->DataPath() / "AllMeshes.g3d"));
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
	animationPack.Open(_fileSystem->FindPath(_fileSystem->DataPath() / "AllAnims.anm"));
	const auto& animations = animationPack.GetAnimations();
	for (size_t i = 0; i < animations.size(); i++)
	{
		animationManager.Load(i, resources::L3DAnimLoader::FromBufferTag {}, animations[i]);
	}

	const auto creatures = _fileSystem->FindPath(_fileSystem->CreatureMeshPath());
	for (const auto& f : std::filesystem::directory_iterator {creatures})
	{
		const auto& fileName = f.path().stem().string();
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading creature mesh: {}", fileName);
		try
		{
			if (string_utils::BeginsWith(fileName, "Hand"))
			{
				continue;
			}

			const auto meshId = creature::GetIdFromMeshName(fileName);
			meshManager.Load(meshId, resources::L3DLoader::FromDiskTag {}, f);
		}
		catch (std::runtime_error& err)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
		}
	}

	const auto citadelInteriors = _fileSystem->FindPath(_fileSystem->CitadelPath() / "engine");
	for (const auto& f : std::filesystem::directory_iterator {citadelInteriors})
	{
		if (f.path().extension() == ".zzz")
		{
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading interior temple mesh: {}", f.path().stem().string());
			try
			{
				auto meshId = fmt::format("temple/interior/{}", f.path().stem().string());
				meshManager.Load(meshId, resources::L3DLoader::FromDiskTag {}, f);
			}
			catch (std::runtime_error& err)
			{
				SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
			}
		}
		else if (f.path().extension() == ".cam")
		{
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading interior temple cam: {}", f.path().stem().string());
			try
			{
				camManager.Load(fmt::format("temple/{}", f.path().stem().string()), resources::CameraPathLoader::FromDiskTag {},
				                f);
			}
			catch (std::runtime_error& err)
			{
				SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
			}
		}
	}

	const auto symbolsCamPaths = _fileSystem->FindPath(_fileSystem->SymbolsPath());
	for (const auto& f : std::filesystem::directory_iterator {symbolsCamPaths})
	{
		if (f.path().extension() == ".cam")
		{
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading symbol cam: {}", f.path().stem().string());
			try
			{
				camManager.Load(fmt::format("symbol/{}", f.path().stem().string()), resources::CameraPathLoader::FromDiskTag {},
				                f);
			}
			catch (std::runtime_error& err)
			{
				SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
			}
		}
	}

	// Load loose one-off assets
	animationManager.Load("coffre", resources::L3DAnimLoader::FromDiskTag {},
	                      _fileSystem->FindPath(_fileSystem->MiscPath() / "coffre.anm"));
	meshManager.Load("hand", resources::L3DLoader::FromDiskTag {},
	                 _fileSystem->FindPath(_fileSystem->CreatureMeshPath() / "Hand_Boned_Base2.l3d"));
	meshManager.Load("coffre", resources::L3DLoader::FromDiskTag {},
	                 _fileSystem->FindPath(_fileSystem->MiscPath() / "coffre.l3d"));
	meshManager.Load("cone", resources::L3DLoader::FromDiskTag {}, _fileSystem->FindPath(_fileSystem->DataPath() / "cone.l3d"));
	meshManager.Load("marker", resources::L3DLoader::FromDiskTag {},
	                 _fileSystem->FindPath(_fileSystem->DataPath() / "marker.l3d"));
	meshManager.Load("river", resources::L3DLoader::FromDiskTag {},
	                 _fileSystem->FindPath(_fileSystem->DataPath() / "river.l3d"));
	meshManager.Load("river2", resources::L3DLoader::FromDiskTag {},
	                 _fileSystem->FindPath(_fileSystem->DataPath() / "river2.l3d"));
	meshManager.Load("metre_sphere", resources::L3DLoader::FromDiskTag {},
	                 _fileSystem->FindPath(_fileSystem->DataPath() / "metre_sphere.l3d"));
	camManager.Load("cam", resources::CameraPathLoader::FromDiskTag {},
	                _fileSystem->FindPath(_fileSystem->DataPath() / "cam.cam"));
	camManager.Load("flying", resources::CameraPathLoader::FromDiskTag {},
	                _fileSystem->FindPath(_fileSystem->DataPath() / "flying.cam"));

	// TODO(raffclar): #400: Parse level files within the resource loader
	// TODO(raffclar): #405: Determine campaign levels from the challenge script file
	// Load the campaign levels
	const auto scriptsPath = _fileSystem->FindPath(_fileSystem->ScriptsPath());
	for (const auto& f : std::filesystem::directory_iterator {scriptsPath})
	{
		const auto& name = f.path().stem().string();
		if (f.path().extension() != ".txt" || name.rfind("InfoScript", 0) != std::string::npos)
		{
			continue;
		}
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading campaign level: {}", f.path().stem().string());
		try
		{
			levelManager.Load(fmt::format("campaign/{}", name), resources::LevelLoader::FromDiskTag {}, name, f, true);
		}
		catch (std::runtime_error& err)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
		}
	}
	// Load Playgrounds
	levelManager.Load("playgrounds/TwoGods", resources::LevelLoader::FromDiskTag {}, "Two Gods",
	                  scriptsPath / "Playgrounds" / "TwoGods.txt", false);
	levelManager.Load("playgrounds/ThreeGods", resources::LevelLoader::FromDiskTag {}, "Three Gods",
	                  scriptsPath / "Playgrounds" / "ThreeGods.txt", false);
	levelManager.Load("playgrounds/FourGods", resources::LevelLoader::FromDiskTag {}, "Four Gods",
	                  scriptsPath / "Playgrounds" / "FourGods.txt", false);
	// Attempt to load additional levels as playgrounds
	for (const auto& f : std::filesystem::directory_iterator {scriptsPath / "Playgrounds"})
	{
		if (f.path().extension() != ".txt")
		{
			continue;
		}
		const auto& name = f.path().stem().string();
		if (levelManager.Contains(fmt::format("playgrounds/{}", name)))
		{
			// Already added
			continue;
		}

		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading custom level: {}", f.path().stem().string());
		try
		{
			levelManager.Load(fmt::format("playgrounds/{}", name), resources::LevelLoader::FromDiskTag {}, name, f, false);
		}
		catch (std::runtime_error& err)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
		}
	}

	// Create profiler
	_profiler = std::make_unique<Profiler>();

	// create our camera
	_camera = std::make_unique<Camera>();
	auto aspect = _window ? _window->GetAspectRatio() : 1.0f;
	_camera->SetProjectionMatrixPerspective(_config.cameraXFov, aspect, _config.cameraNearClip, _config.cameraFarClip);

	_camera->SetPosition(glm::vec3(1441.56f, 24.764f, 2081.76f));
	_camera->SetRotation(glm::vec3(0.0f, -45.0f, 0.0f));

	if (!LoadVariables())
	{
		return false;
	}

	for (const auto& f : std::filesystem::directory_iterator {_fileSystem->FindPath(_fileSystem->TexturePath())})
	{
		if (f.path().extension() == ".raw")
		{
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading raw texture: {}", f.path().stem().string());
			try
			{
				textureManager.Load(fmt::format("raw/{}", f.path().stem().string()), resources::Texture2DLoader::FromDiskTag {},
				                    f);
			}
			catch (std::runtime_error& err)
			{
				SPDLOG_LOGGER_ERROR(spdlog::get("game"), "{}", err.what());
			}
		}
	}

	_sky = std::make_unique<Sky>();
	_water = std::make_unique<Water>();
	return true;
}

bool Game::Run()
{
	LoadMap(_startMap);
	Locator::dynamicsSystem::value().RegisterRigidBodies();

	auto challengePath = _fileSystem->QuestsPath() / "challenge.chl";
	if (_fileSystem->Exists(challengePath))
	{
		_lhvm = std::make_unique<LHVM::LHVM>();
		_lhvm->LoadBinary((_fileSystem->GetGamePath() / challengePath).generic_string());
	}
	else
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Challenge file not found at {}",
		                    (_fileSystem->GetGamePath() / challengePath).generic_string());
		return false;
	}

	// Initialize the Acceleration Structure
	_entityMap->Rebuild();

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
		_landIsland->GetFootprintFramebuffer().GetSize(width, height);
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
			    /*island =*/*_landIsland,
			    /*entities =*/*_entityRegistry,
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
	if (!_fileSystem->Exists(path))
	{
		throw std::runtime_error("Could not find script " + path.generic_string());
	}

	auto data = _fileSystem->ReadAll(path);
	std::string source(reinterpret_cast<const char*>(data.data()), data.size());

	// Reset everything. Deletes all entities and their components
	_entityRegistry->Reset();

	// We need a hand for the player
	_handEntity = ecs::archetypes::HandArchetype::Create(glm::vec3(0.0f), glm::half_pi<float>(), 0.0f, glm::half_pi<float>(),
	                                                     0.01f, false);

	Script script(this);
	script.Load(source);

	// Each released map comes with an optional .fot file which contains the footpath information for the map
	auto stem = string_utils::LowerCase(path.stem().generic_string());
	auto fotPath = _fileSystem->LandscapePath() / fmt::format("{}.fot", stem);

	if (_fileSystem->Exists(fotPath))
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
	ecs::systems::InitializeLevel();
	if (_landIsland)
	{
		_landIsland.reset();
	}

	auto fixedName = Game::Instance()->GetFileSystem().FindPath(FileSystem::FixPath(path));

	if (!_fileSystem->Exists(fixedName))
	{
		throw std::runtime_error("Could not find landscape " + path.generic_string());
	}

	_landIsland = std::make_unique<LandIsland>();
	_landIsland->LoadFromFile(fixedName);

	Locator::cameraBookmarkSystem::value().Initialize();
	Locator::dynamicsSystem::value().RegisterIslandRigidBodies(*_landIsland);
}

bool Game::LoadVariables()
{
	InfoFile infoFile;
	return infoFile.LoadFromFile(_fileSystem->ScriptsPath() / "info.dat", _infoConstants);
}

void Game::SetGamePath(std::filesystem::path gamePath)
{
	if (gamePath.empty())
	{
		return;
	}

#if defined(unix) || defined(__unix__) || defined(__unix)
	if (gamePath.string().size() >= 2 && gamePath.string().c_str()[0] == '~' && gamePath.string().c_str()[1] == '/')
	{
		gamePath = std::getenv("HOME") + gamePath.string().substr(1);
	}
#endif

	if (!std::filesystem::exists(gamePath))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "GamePath does not exist: '{}'", gamePath.generic_string());
		return;
	}
	_gamePath = gamePath;
}

const std::filesystem::path& Game::GetGamePath()
{
	if (_gamePath.empty())
	{
#ifdef _WIN32
		DWORD dataLen = 0;
		LSTATUS status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir",
		                             RRF_RT_REG_SZ, nullptr, nullptr, &dataLen);
		if (status == ERROR_SUCCESS)
		{
			std::vector<char> path(dataLen);
			status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir", RRF_RT_REG_SZ,
			                     nullptr, path.data(), &dataLen);

			_gamePath = std::filesystem::path(path.data());
			return _gamePath;
		}

		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to find the GameDir registry value, game not installed.");
#endif // _WIN32

		// no key, don't guess, let the user know to set the command param
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Game Path missing",
		                         "Game path was not supplied, use the -g "
		                         "command parameter to set it.",
		                         nullptr);
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Failed to find the GameDir.");
		exit(1);
	}

	return _gamePath;
}

void Game::SetTime(float time)
{
	GetSky().SetTime(time);
}

void Game::RequestScreenshot(const std::filesystem::path& path)
{
	_requestScreenshot = std::make_pair(_frameCount, path);
}

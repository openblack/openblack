/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Game.h"

#include "3D/AnimationPack.h"
#include "3D/Camera.h"
#include "3D/L3DAnim.h"
#include "3D/L3DMesh.h"
#include "3D/LandIsland.h"
#include "3D/MeshPack.h"
#include "3D/Sky.h"
#include "3D/Water.h"
#include "Common/EventManager.h"
#include "Common/FileSystem.h"
#include "Entities/Components/Hand.h"
#include "Entities/Components/Mesh.h"
#include "Entities/Components/Transform.h"
#include "Entities/Registry.h"
#include "GameWindow.h"
#include "GitSHA1.h"
#include "Graphics/FrameBuffer.h"
#include "Gui/Gui.h"
#include "LHScriptX/Script.h"
#include "Parsers/InfoFile.h"
#include "Profiler.h"
#include "Renderer.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/intersect.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <Serializer/FotFile.h>
#include <cstdint>
#include <string>

#ifdef WIN32
#include <Windows.h>
#endif

using namespace openblack;
using namespace openblack::lhscriptx;
using namespace std::chrono_literals;

const std::string kBuildStr(kGitSHA1Hash, 8);
const std::string kWindowTitle = "openblack";

Game* Game::sInstance = nullptr;

Game::Game(Arguments&& args)
    : _eventManager(std::make_unique<EventManager>())
    , _fileSystem(std::make_unique<FileSystem>())
    , _entityRegistry(std::make_unique<entities::Registry>())
    , _config()
    , _gameSpeedMultiplier(1.0f)
    , _frameCount(0)
    , _turnCount(0)
    , _intersection()
{
	std::function<std::shared_ptr<spdlog::logger>(const std::string&)> CreateLogger;
	if (!args.logFile.empty() && args.logFile != "stdout")
	{
		CreateLogger = [&args](const std::string& name) { return spdlog::basic_logger_mt(name, args.logFile); };
	}
	else
	{
		CreateLogger = [](const std::string& name) { return spdlog::stdout_color_mt(name); };
	}
	for (size_t i = 0; i < LoggingSubsystemStrs.size(); ++i)
	{
		auto logger = CreateLogger(LoggingSubsystemStrs[i].data());
		logger->set_level(args.logLevels[i]);
	}
	sInstance = this;

	std::string binaryPath = fs::path {args.executablePath}.parent_path().generic_string();
	_config.numFramesToSimulate = args.numFramesToSimulate;
	SPDLOG_LOGGER_INFO(spdlog::get("game"), "current binary path: {}", binaryPath);
	SetGamePath(args.gamePath);
	if (args.rendererType != bgfx::RendererType::Noop)
	{
		_window = std::make_unique<GameWindow>(kWindowTitle + " [" + kBuildStr + "]", args.windowWidth, args.windowHeight,
		                                       args.displayMode);
	}
	_renderer = std::make_unique<Renderer>(_window.get(), args.rendererType, args.vsync);
	_fileSystem->SetGamePath(GetGamePath());
	_handModel = std::make_unique<L3DMesh>();
	_handModel->LoadFromFile(_fileSystem->CreatureMeshPath() / "Hand_Boned_Base2.l3d");
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "The GamePath is \"{}\".", _fileSystem->GetGamePath().generic_string());

	_gui = gui::Gui::create(_window.get(), graphics::RenderPass::ImGui, args.scale);

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
	_water.reset();
	_sky.reset();
	_testModel.reset();
	_handModel.reset();
	_animationPack.reset();
	_meshPack.reset();
	_landIsland.reset();
	_entityRegistry.reset();
	_gui.reset();
	_renderer.reset();
	_window.reset();
	_eventManager.reset();
	SDL_Quit(); // todo: move to GameWindow
}

const entities::components::Transform& Game::GetHandTransform() const
{
	return _entityRegistry->Get<entities::components::Transform>(_handEntity);
}

entities::components::Transform& Game::GetHandTransform()
{
	return _entityRegistry->Get<entities::components::Transform>(_handEntity);
}

bool Game::ProcessEvents(const SDL_Event& event)
{
	static bool leftMouseButton = false;

	if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == SDL_BUTTON_LEFT)
		leftMouseButton = !leftMouseButton;

	_handGripping = leftMouseButton;

	switch (event.type)
	{
	case SDL_QUIT:
		return false;
	case SDL_WINDOWEVENT:
		if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == _window->GetID())
		{
			return false;
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
		case SDLK_F1:
			_config.bgfxDebug = !_config.bgfxDebug;
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
	const auto currentTime = std::chrono::steady_clock::now();
	const auto delta = currentTime - _lastGameLoopTime;
	if (delta < kTurnDuration * _gameSpeedMultiplier)
	{
		return false;
	}

	// TODO: update entities
	// const auto& registry = GetEntityRegistry();

	_lastGameLoopTime = currentTime;
	_turnDeltaTime = delta;
	++_turnCount;

	return false;
}

bool Game::Update()
{
	_profiler->Frame();
	auto deltaTime =
	    std::chrono::duration_cast<std::chrono::microseconds>(_profiler->_entries[_profiler->GetEntryIndex(0)]._frameStart -
	                                                          _profiler->_entries[_profiler->GetEntryIndex(-1)]._frameStart);

	// Input events
	{
		auto sdlInput = _profiler->BeginScoped(Profiler::Stage::SdlInput);
		SDL_Event e;
		while (SDL_PollEvent(&e))
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

	// Update Camera
	{
		_camera->Update(deltaTime);
	}

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
		{
			glm::ivec2 screenSize {};
			if (_window)
			{
				_window->GetSize(screenSize.x, screenSize.y);
			}

			glm::vec3 rayOrigin, rayDirection;
			_camera->DeprojectScreenToWorld(_mousePosition, screenSize, rayOrigin, rayDirection);

			float intersectDistance = 0.0f;
			bool intersects = glm::intersectRayPlane(rayOrigin, rayDirection, glm::vec3(0.0f, 0.0f, 0.0f), // plane origin
			                                         glm::vec3(0.0f, 1.0f, 0.0f),                          // plane normal
			                                         intersectDistance);

			if (intersects)
				_intersection = rayOrigin + rayDirection * intersectDistance;

			_intersection.y = _landIsland->GetHeightAt(glm::vec2(_intersection.x, _intersection.z));

			_renderer->UpdateDebugCrossUniforms(_intersection, 50.0f);
		}

		// Update Hand
		if (!_handGripping)
		{
			const glm::mat4 modelRotationCorrection = glm::eulerAngleX(glm::radians(90.0f));
			auto& handTransform = _entityRegistry->Get<entities::components::Transform>(_handEntity);
			handTransform.position = _intersection;
			auto cameraRotation = _camera->GetRotation();

			auto handHeight = GetLandIsland().GetHeightAt(glm::vec2(handTransform.position.x, handTransform.position.z)) + 4.0f;

			handTransform.rotation = glm::eulerAngleY(glm::radians(-cameraRotation.y)) * modelRotationCorrection;
			handTransform.position = glm::vec3(handTransform.position.x, handHeight, handTransform.position.z);
			_entityRegistry->Context().renderContext.dirty = true;
		}

		// Update Entities
		{
			auto updateEntities = _profiler->BeginScoped(Profiler::Stage::UpdateEntities);
			if (_config.drawEntities)
			{
				_entityRegistry->PrepareDraw(_config.drawBoundingBoxes, _config.drawFootpaths, _config.drawStreams);
			}
		}
	} // Update Uniforms

	return _config.numFramesToSimulate == 0 || _frameCount < _config.numFramesToSimulate;
}

bool Game::Run()
{
	// Create profiler
	_profiler = std::make_unique<Profiler>();

	// create our camera
	_camera = std::make_unique<Camera>();
	auto aspect = _window ? _window->GetAspectRatio() : 1.0f;
	_camera->SetProjectionMatrixPerspective(70.0f, aspect, 1.0f, 65536.0f);

	_camera->SetPosition(glm::vec3(1441.56f, 24.764f, 2081.76f));
	_camera->SetRotation(glm::vec3(0.0f, -45.0f, 0.0f));

	_meshPack = std::make_unique<MeshPack>();
	if (!_meshPack->LoadFromFile(_fileSystem->DataPath() / "AllMeshes.g3d"))
	{
		return false;
	}

	_animationPack = std::make_unique<AnimationPack>();
	if (!_animationPack->LoadFromFile(_fileSystem->DataPath() / "AllAnims.anm"))
	{
		return false;
	}

	_testModel = std::make_unique<L3DMesh>();
	if (!_testModel->LoadFromFile(_fileSystem->MiscPath() / "coffre.l3d"))
	{
		return false;
	}

	_testAnimation = std::make_unique<L3DAnim>();
	if (!_testAnimation->LoadFromFile(_fileSystem->MiscPath() / "coffre.anm"))
	{
		return false;
	}

	_handModel = std::make_unique<L3DMesh>();
	if (!_handModel->LoadFromFile(_fileSystem->CreatureMeshPath() / "Hand_Boned_Base2.l3d"))
	{
		return false;
	}

	_sky = std::make_unique<Sky>();
	_water = std::make_unique<Water>();

	if (!LoadVariables())
	{
		return false;
	}
	LoadMap(_fileSystem->ScriptsPath() / "Land1.txt");

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

	if (_window)
	{
		int width, height;
		_window->GetSize(width, height);
		_renderer->ConfigureView(graphics::RenderPass::Main, static_cast<uint16_t>(width), static_cast<uint16_t>(height));
	}

	{
		uint16_t waterWidth, waterHeight;
		_water->GetFrameBuffer().GetSize(waterWidth, waterHeight);
		_renderer->ConfigureView(graphics::RenderPass::Reflection, waterWidth, waterHeight);
	}

	_frameCount = 0;
	auto last_time = std::chrono::high_resolution_clock::now();
	while (Update())
	{
		auto duration = std::chrono::high_resolution_clock::now() - last_time;
		auto milliseconds = std::chrono::duration_cast<std::chrono::duration<uint32_t, std::milli>>(duration);
		{
			auto section = _profiler->BeginScoped(Profiler::Stage::SceneDraw);

			Renderer::DrawSceneDesc drawDesc {
			    /*time =*/milliseconds.count(), // TODO get actual time
			    /*viewId =*/graphics::RenderPass::Main,
			    /*profiler =*/*_profiler,
			    /*camera =*/_camera.get(),
			    /*frameBuffer =*/nullptr,
			    /*drawSky =*/_config.drawSky,
			    /*sky =*/*_sky,
			    /*drawWater =*/_config.drawWater,
			    /*water =*/*_water,
			    /*drawIsland =*/_config.drawIsland,
			    /*island =*/*_landIsland,
			    /*drawEntities =*/_config.drawEntities,
			    /*entities =*/*_entityRegistry,
			    /*drawTestModel =*/_config.drawEntities,
			    /*testModel =*/*_testModel,
			    /*testAnimation =*/*_testAnimation,
			    /*drawDebugCross =*/_config.drawDebugCross,
			    /*drawBoundingBoxes =*/_config.drawBoundingBoxes,
			    /*cullBack =*/false,
			    /*bgfxDebug =*/_config.bgfxDebug,
			    /*bgfxProfile =*/_config.bgfxProfile,
			    /*wireframe =*/_config.wireframe,
			    /*timeOfDay =*/_config.timeOfDay,
			    /*bumpMapStrength =*/_config.bumpMapStrength,
			    /*smallBumpMapStrength =*/_config.smallBumpMapStrength,
			};

			_renderer->DrawScene(*_meshPack, drawDesc);
		}

		{
			auto section = _profiler->BeginScoped(Profiler::Stage::GuiDraw);
			_gui->Draw();
		}

		{
			auto section = _profiler->BeginScoped(Profiler::Stage::RendererFrame);
			_renderer->Frame();
		}
		_frameCount++;
	}

	return true;
}

void Game::LoadMap(const fs::path& path)
{
	if (!_fileSystem->Exists(path))
		throw std::runtime_error("Could not find script " + path.generic_string());

	auto data = _fileSystem->ReadAll(path);
	std::string source(reinterpret_cast<const char*>(data.data()), data.size());

	// Reset everything. Deletes all entities and their components
	_entityRegistry->Reset();

	// We need a hand for the player
	_handEntity = _entityRegistry->Create();
	_entityRegistry->Assign<entities::components::Hand>(_handEntity);
	const auto rotation = glm::mat3(glm::eulerAngleXZ(glm::half_pi<float>(), glm::half_pi<float>()));
	_entityRegistry->Assign<entities::components::Transform>(_handEntity, glm::vec3(0.0f), rotation, glm::vec3(0.02f));
	_entityRegistry->Assign<entities::components::Mesh>(_handEntity, entities::components::Hand::meshId, static_cast<int8_t>(0),
	                                                    static_cast<int8_t>(0));

	Script script(this);
	script.Load(source);

	// Each released map comes with an optional .fot file which contains the footpath information for the map
	auto stem = path.stem().generic_string();
	std::transform(stem.begin(), stem.end(), stem.begin(), [](auto c) { return std::tolower(c); });
	auto fot_path = _fileSystem->LandscapePath() / fmt::format("{}.fot", stem);

	if (_fileSystem->Exists(fot_path))
	{
		FotFile fotFile(*this);
		fotFile.Load(fot_path);
	}
	else
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "The map at {} does not come with a footpath file. Expected {}",
		                   path.generic_string(), fot_path.generic_string());
	}

	_lastGameLoopTime = std::chrono::steady_clock::now();
	_turnDeltaTime = 0ns;
	SetGameSpeed(Game::kTurnDurationMultiplierNormal);
	_turnCount = 0;
}

void Game::LoadLandscape(const fs::path& path)
{
	if (_landIsland)
		_landIsland.reset();

	auto fixedName = Game::instance()->GetFileSystem().FindPath(FileSystem::FixPath(path));

	if (!_fileSystem->Exists(fixedName))
		throw std::runtime_error("Could not find landscape " + path.generic_string());

	_landIsland = std::make_unique<LandIsland>();
	_landIsland->LoadFromFile(fixedName.u8string());
}

bool Game::LoadVariables()
{
	InfoFile infoFile;

	if (!infoFile.LoadFromFile(_fileSystem->ScriptsPath() / "info.dat", _infoConstants))
	{
		return false;
	}

	for (const auto& m : _infoConstants.magic)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "MAGIC_TYPE {}, PerceivedPower={}", m.typeEnum, m.perceivedPower);
	}

	// GMagicInfo: 0x48 bytes // DETAIL_MAGIC_GENERAL_INFO
	// 10

	// GMagicHealInfo: 0x48, 0x8 bytes // DETAIL_MAGIC_HEAL_INFO
	// GMagicTeleportInfo: 0x48, 0x8 bytes // DETAIL_MAGIC_TELEPORT_INFO

	// DETAIL_MAGIC_GENERAL_INFO
	// DETAIL_MAGIC_HEAL_INFO
	// DETAIL_MAGIC_TELEPORT_INFO
	// DETAIL_MAGIC_FOREST_INFO
	// DETAIL_MAGIC_FOOD_INFO
	// DETAIL_MAGIC_STORM_AND_TORNADO_INFO
	// DETAIL_MAGIC_SHIELD_ONE_INFO
	// DETAIL_MAGIC_WOOD_INFO
	// DETAIL_MAGIC_WATER_INFO
	// DETAIL_MAGIC_FLOCK_FLYING_INFO
	// DETAIL_MAGIC_FLOCK_GROUND_INFO
	// DETAIL_MAGIC_CREATURE_SPELL_INFO

	return true;
}

void Game::SetGamePath(const fs::path& gamePath)
{
	if (gamePath.empty())
	{
		return;
	}
	if (!fs::exists(gamePath))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "GamePath does not exist: '{}'", gamePath.generic_string());
		return;
	}
	_gamePath = gamePath;
}

const fs::path& Game::GetGamePath()
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

			_gamePath = fs::path(path.data());
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

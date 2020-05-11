/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Game.h"

#include "3D/Camera.h"
#include "3D/L3DMesh.h"
#include "3D/LandIsland.h"
#include "3D/MeshPack.h"
#include "3D/Sky.h"
#include "3D/Water.h"
#include "Common/EventManager.h"
#include "Common/FileSystem.h"
#include "Entities/Registry.h"
#include "GameWindow.h"
#include "GitSHA1.h"
#include "Gui.h"
#include "LHScriptX/Script.h"
#include "MeshViewer.h"
#include "Profiler.h"
#include "Renderer.h"
#include <Entities/Components/Hand.h>
#include <Entities/Components/Transform.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtc/constants.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <string>

#ifdef WIN32
#include <Windows.h>
#endif

using namespace openblack;
using namespace openblack::lhscriptx;

const std::string kBuildStr(kGitSHA1Hash, 8);
const std::string kWindowTitle = "openblack";

Game* Game::sInstance = nullptr;

Game::Game(Arguments&& args)
    : _fileSystem(std::make_unique<FileSystem>())
    , _entityRegistry(std::make_unique<entities::Registry>())
    , _config()
    , _frameCount(0)
    , _intersection()
    , _eventManager(std::make_unique<EventManager>())
{
	if (!args.logFile.empty() && args.logFile != "stdout")
	{
		auto logger = spdlog::basic_logger_mt("default_logger", args.logFile);
		spdlog::set_default_logger(logger);
	}
	spdlog::set_level(static_cast<spdlog::level::level_enum>(spdlog::level::debug + args.logLevel));
	sInstance = this;

	std::string binaryPath = fs::path {args.executablePath}.parent_path().generic_string();
	_config.numFramesToSimulate = args.numFramesToSimulate;
	spdlog::info("current binary path: {}", binaryPath);
	SetGamePath(args.gamePath);
	if (args.rendererType != bgfx::RendererType::Noop)
	{
		_window = std::make_unique<GameWindow>(kWindowTitle + " [" + kBuildStr + "]", args.windowWidth, args.windowHeight,
		                                       args.displayMode);
	}
	_renderer = std::make_unique<Renderer>(_window.get(), args.rendererType, args.vsync);
	_fileSystem->SetGamePath(GetGamePath());
	_handModel = std::make_unique<L3DMesh>();
	_handModel->LoadFromFile("Data/CreatureMesh/Hand_Boned_Base2.l3d");
	spdlog::debug("The GamePath is \"{}\".", _fileSystem->GetGamePath().generic_string());

	_gui = Gui::create(_window.get(), graphics::RenderPass::ImGui, args.scale);

	_eventManager->AddHandler(std::function([this](const SDL_Event& event) {
		this->_camera->ProcessSDLEvent(event);
		this->_gui->ProcessEventSdl2(event);
		this->_config.running = this->ProcessEvents(event);
	}));
}

Game::~Game()
{
	_water.reset();
	_sky.reset();
	_testModel.reset();
	_meshPack.reset();
	_landIsland.reset();
	_entityRegistry.reset();
	_gui.reset();
	_renderer.reset();
	_window.reset();
	_eventManager.reset();
	SDL_Quit(); // todo: move to GameWindow
}

bool Game::ProcessEvents(const SDL_Event &event)
{
	static bool leftMouseButton = false;

	if ((event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == SDL_BUTTON_LEFT)
		leftMouseButton = !leftMouseButton;

	if (!leftMouseButton)
	{
		auto& handTransform = _entityRegistry->Get<Transform>(_handEntity);
		handTransform.position = _intersection;
		auto cameraRotation = _camera->GetRotation();
		auto handHeight = GetLandIsland().GetHeightAt(glm::vec2(handTransform.position.x, handTransform.position.z)) + 4.0f;
		handTransform.rotation = glm::vec3(handTransform.rotation.x, handTransform.rotation.y, glm::radians(cameraRotation.y));
		handTransform.position = glm::vec3(handTransform.position.x, handHeight, handTransform.position.z);
		_entityRegistry->Context().renderContext.dirty = true;
	}

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

		// Update Entities
		{
			auto updateEntities = _profiler->BeginScoped(Profiler::Stage::UpdateEntities);
			if (_config.drawEntities)
			{
				_entityRegistry->PrepareDraw(_config.drawBoundingBoxes, _config.drawStreams);
			}
		}
	} // Update Uniforms

	return _config.numFramesToSimulate == 0 || _frameCount < _config.numFramesToSimulate;
}

void Game::Run()
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
	_meshPack->LoadFromFile("Data/AllMeshes.g3d");

	 _testModel = std::make_unique<L3DMesh>();
	 _testModel->LoadFromFile("Data/CreatureMesh/C_Tortoise_Base.l3d");
	_handModel = std::make_unique<L3DMesh>();
	_handModel->LoadFromFile("Data/CreatureMesh/Hand_Boned_Base2.l3d");

	_sky = std::make_unique<Sky>();
	_water = std::make_unique<Water>();

	LoadVariables();
	LoadMap("./Scripts/Land1.txt");

	// _lhvm = std::make_unique<LHVM::LHVM>();
	// _lhvm->LoadBinary(GetGamePath() + "/Scripts/Quests/challenge.chl");

	if (_window)
	{
		int width, height;
		_window->GetSize(width, height);
		_renderer->ConfigureView(graphics::RenderPass::Main, width, height);
	}
	{
		uint16_t width, height;
		_water->GetFrameBuffer().GetSize(width, height);
		_renderer->ConfigureView(graphics::RenderPass::Reflection, width, height);
	}

	_frameCount = 0;
	while (Update())
	{
		{
			auto section = _profiler->BeginScoped(Profiler::Stage::SceneDraw);

			Renderer::DrawSceneDesc drawDesc {
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
			    /*drawDebugCross =*/_config.drawDebugCross,
			    /*drawBoundingBoxes =*/_config.drawBoundingBoxes,
			    /*cullBack =*/false,
			    /*bgfxDebug =*/_config.bgfxDebug,
			    /*wireframe =*/_config.wireframe,
			    /*profile =*/_config.showProfiler,
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
}

void Game::LoadMap(const std::string& name)
{
	if (!_fileSystem->Exists(name))
		throw std::runtime_error("Could not find script " + name);

	auto data = _fileSystem->ReadAll(name);
	std::string source(reinterpret_cast<const char*>(data.data()), data.size());

	// Reset everything. Deletes all entities and their components
	_entityRegistry->Reset();

	// We need a hand for the player
	_handEntity = _entityRegistry->Create();
	_entityRegistry->Assign<Hand>(_handEntity);
	auto pi = glm::pi<float>();
	_entityRegistry->Assign<Transform>(_handEntity, glm::vec3(0), glm::vec3(pi / 2, 0, pi / 2), glm::vec3(0.02));

	Script script(this);
	script.Load(source);
}

void Game::LoadLandscape(const std::string& name)
{
	if (_landIsland)
		_landIsland.reset();

	auto fixedName = Game::instance()->GetFileSystem().FindPath(FileSystem::FixPath(name));

	if (!_fileSystem->Exists(fixedName))
		throw std::runtime_error("Could not find landscape " + name);

	_landIsland = std::make_unique<LandIsland>();
	_landIsland->LoadFromFile(fixedName.u8string());
}

void Game::LoadVariables()
{
	return;

	auto file = _fileSystem->Open("Scripts/info.dat", FileMode::Read);

	// check magic header
	constexpr char kLionheadMagic[] = "LiOnHeAd";

	struct
	{
		char blockName[32];
		uint32_t blockSize;
		uint32_t position;
	} header;

	char magic[8];
	file->Read(magic, 8);
	if (!std::equal(kLionheadMagic, kLionheadMagic + 8, magic))
		throw std::runtime_error("invalid Lionhead file magic");

	char blockName[32];
	file->Read(blockName, 32);
	uint32_t size = file->ReadValue<uint32_t>();

	spdlog::debug("info.dat: block name = {} = {} bytes", blockName, size);

	struct MagicInfo
	{
		uint32_t typeEnum;          // ENUM_MAGIC_TYPE
		uint32_t immersionTypeEnum; // ENUM_IMMERSION_EFFECT_TYPE
		uint32_t stopImmersion;
		float perceivedPower;
		uint32_t particleTypeEnum;   // ENUM_PARTICLE_TYPE
		uint32_t impressiveTypeEnum; // ENUM_IMPRESSIVE_TYPE
		uint32_t spellSeedTypeEnum;  // ENUM_SPELL_SEED_TYPE
		uint32_t gestureType;        // ENUM_GESTURE_TYPE
		uint32_t powerupType;        // ENUM_POWER_UP_TYPE
		uint32_t castRuleType;       // ENUM_CAST_RULE_TYPE
		uint32_t IsSpellSeedDrawnInHand;
		uint32_t IsSpellRecharged;
		uint32_t IsCreatureCastFromAbove;
		uint32_t OneOffSpellIsPlayful;
		uint32_t OneOffSpellIsAggressive;
		uint32_t OneOffSpellIsCompassionate;
		uint32_t OneOffSpellIsToRestoreHealth;
	};

	for (auto i = 0; i < 10; i++)
	{
		MagicInfo info;
		file->Read(&info, 0x48);

		spdlog::debug("MAGIC_TYPE {}, PerceivedPower={}", info.typeEnum, info.perceivedPower);
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
}

void Game::SetGamePath(const std::string& gamePath)
{
	if (gamePath.empty())
	{
		return;
	}
	if (!fs::exists(gamePath))
	{
		spdlog::error("GamePath does not exist: '{}'", gamePath);
		return;
	}
	sGamePath = gamePath;
}

const std::string& Game::GetGamePath()
{
	if (sGamePath.empty())
	{
#ifdef _WIN32
		DWORD dataLen = 0;
		LSTATUS status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir",
		                             RRF_RT_REG_SZ, nullptr, nullptr, &dataLen);
		if (status == ERROR_SUCCESS)
		{
			char* path = new char[dataLen];
			status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir", RRF_RT_REG_SZ,
			                     nullptr, path, &dataLen);

			sGamePath = std::string(path);
			return sGamePath;
		}

		spdlog::error("Failed to find the GameDir registry value, game not installed.");
#endif // _WIN32

		// no key, don't guess, let the user know to set the command param
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Game Path missing",
		                         "Game path was not supplied, use the -g "
		                         "command parameter to set it.",
		                         nullptr);
		spdlog::error("Failed to find the GameDir.");
		exit(1);
	}

	return sGamePath;
}

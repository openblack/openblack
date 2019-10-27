/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <Game.h>

#include <string>

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/intersect.hpp>

#include "GitSHA1.h"
#include "Gui.h"
#include <GameWindow.h>
#include <3D/Camera.h>
#include <3D/L3DMesh.h>
#include <3D/LandIsland.h>
#include <3D/MeshPack.h>
#include <3D/Sky.h>
#include <3D/Water.h>
#include <Entities/Components/Transform.h>
#include <Common/CmdLineArgs.h>
#include <Common/FileSystem.h>
#include <Entities/Registry.h>
#include <LHScriptX/Script.h>
#include <MeshViewer.h>
#include <Profiler.h>
#include <Renderer.h>

#ifdef WIN32
#include <Windows.h>
#endif

#include <spdlog/spdlog.h>

using namespace openblack;
using namespace openblack::LHScriptX;

const std::string kBuildStr(kGitSHA1Hash, 8);
const std::string kWindowTitle = "openblack";

Game* Game::sInstance = nullptr;

Game::Game(int argc, char** argv)
	: _fileSystem(std::make_unique<FileSystem>())
	, _entityRegistry(std::make_unique<Entities::Registry>())
	, _config()
	, _intersection()
{
	spdlog::set_level(spdlog::level::debug);
	sInstance = this;

	int windowWidth = 1280, windowHeight = 1024;
	bool vsync = false;
	bool fullscreen = false;
	bool borderless = false;

	auto args = CmdLineArgs(argc, argv);
	args.Get("w", windowWidth);
	args.Get("h", windowHeight);
	args.Get("v", vsync);
	args.Get("f", fullscreen);
	args.Get("b", borderless);

	DisplayMode displayMode = borderless ? DisplayMode::Borderless : (fullscreen ? DisplayMode::Fullscreen : DisplayMode::Windowed);

	std::string binaryPath = fs::path{argv[0]}.parent_path().generic_string();
	spdlog::info("current binary path: {}", binaryPath);
	{
		std::string gamePath;
		args.Get("g", gamePath);
		SetGamePath(gamePath);
	}

	_window = std::make_unique<GameWindow>(kWindowTitle + " [" + kBuildStr + "]", windowWidth, windowHeight, displayMode);

	_renderer = std::make_unique<Renderer>(*_window, vsync);

	_fileSystem->SetGamePath(GetGamePath());
	spdlog::debug("The GamePath is \"{}\".", _fileSystem->GetGamePath().generic_string());

	float scale = 1.0f;
	args.Get<float>("scale", scale);
	_gui = Gui::create(*_window, graphics::RenderPass::ImGui, scale);
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
	SDL_Quit(); // todo: move to GameWindow
}

glm::mat4 Game::GetModelMatrix() const
{
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix           = glm::translate(modelMatrix, _modelPosition);

	modelMatrix = glm::rotate(modelMatrix, glm::radians(_modelRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(_modelRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(_modelRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	return glm::scale(modelMatrix, _modelScale);
}

bool Game::ProcessEvents()
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
			case SDL_QUIT:
				return true;
			case SDL_WINDOWEVENT:
				if (e.window.event == SDL_WINDOWEVENT_CLOSE && e.window.windowID == _window->GetID())
				{
					return true;
				}
				break;
			case SDL_KEYDOWN:
				switch (e.key.keysym.sym)
				{
					case SDLK_ESCAPE:
						return true;
					case SDLK_f:
						_window->SetFullscreen(true);
						break;
					case SDLK_F1:
						_config.bgfxDebug = !_config.bgfxDebug;
						break;
				}
				break;
			case SDL_MOUSEMOTION:
				SDL_GetMouseState(&_mousePosition.x, &_mousePosition.y);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				switch (e.button.button)
				{
					case SDL_BUTTON_MIDDLE:
					{
						glm::ivec2 screenSize;
						_window->GetSize(screenSize.x, screenSize.y);
						SDL_SetRelativeMouseMode((e.type == SDL_MOUSEBUTTONDOWN) ? SDL_TRUE : SDL_FALSE);
						SDL_WarpMouseInWindow(_window->GetHandle(), screenSize.x / 2, screenSize.y / 2);
					} break;
				}
				break;
		}

		_camera->ProcessSDLEvent(e);
		_gui->ProcessEventSdl2(e);
	}
	return false;
}

bool Game::Update()
{
	_profiler->Frame();
	auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(
		_profiler->_entries[_profiler->GetEntryIndex(0)]._frameStart -
			_profiler->_entries[_profiler->GetEntryIndex(-1)]._frameStart);

	// Input events
	{
		auto sdlInput = _profiler->BeginScoped(Profiler::Stage::SdlInput);
		if (ProcessEvents())
		{
			return false;  // Quit event
		}
	}

	// ImGui events + prepare
	{
		auto guiLoop = _profiler->BeginScoped(Profiler::Stage::GuiLoop);
		if (_gui->Loop(*this))
		{
			return false; // Quit event
		}
	}

	// Update Camera
	{
		_camera->Update(deltaTime);
		_modelRotation.y = fmod(_modelRotation.y + float(deltaTime.count())*.0001f, 360.f);
	}

	// Update Uniforms
	{
		auto profilerScopedUpdateUniforms = _profiler->BeginScoped(Profiler::Stage::UpdateUniforms);

		// Update Debug Cross
		{
			glm::ivec2 screenSize;
			_window->GetSize(screenSize.x, screenSize.y);

			glm::vec3 rayOrigin, rayDirection;
			_camera->DeprojectScreenToWorld(_mousePosition, screenSize, rayOrigin, rayDirection);

			float intersectDistance = 0.0f;
			bool intersects = glm::intersectRayPlane(
				rayOrigin,
				rayDirection,
				glm::vec3(0.0f, 0.0f, 0.0f), // plane origin
				glm::vec3(0.0f, 1.0f, 0.0f), // plane normal
				intersectDistance);

			if (intersects)
				_intersection = rayOrigin + rayDirection*intersectDistance;

			_intersection.y = _landIsland->GetHeightAt(glm::vec2(_intersection.x, _intersection.z));

			_renderer->UpdateDebugCrossUniforms(_intersection, 50.0f);
		}

		// Update Terrain
		{
			_renderer->UpdateTerrainUniforms(_config.timeOfDay, _config.bumpMapStrength, _config.smallBumpMapStrength);
		}

		// Update Entities
		{
			auto updateEntities = _profiler->BeginScoped(Profiler::Stage::UpdateEntities);
			if (_config.drawEntities)
			{
				_entityRegistry->PrepareDraw(_config.drawBoundingBoxes);
			}
		}
	}  // Update Uniforms

	return true;
}

void Game::Run()
{
	// Create profiler
	_profiler = std::make_unique<Profiler>();

	// create our camera
	_camera = std::make_unique<Camera>();
	_camera->SetProjectionMatrixPerspective(70.0f, _window->GetAspectRatio(), 1.0f, 65536.0f);

	_camera->SetPosition(glm::vec3(1441.56f, 24.764f, 2081.76f));
	_camera->SetRotation(glm::vec3(0.0f, -45.0f, 0.0f));

	_modelPosition = glm::vec3(2485.0f, 50.0f, 1907.0f);
	_modelRotation = glm::vec3(180.0f, 111.0f, 0.0f);
	_modelScale    = glm::vec3(0.5f);

	auto file = _fileSystem->Open("Data/AllMeshes.g3d", FileMode::Read);
	_meshPack = std::make_unique<MeshPack>();
	_meshPack->Load(*file);

	_testModel = std::make_unique<L3DMesh>();
	_testModel->LoadFromFile("Data/CreatureMesh/C_Tortoise_Base.l3d");

	_sky   = std::make_unique<Sky>();
	_water = std::make_unique<Water>();

	LoadVariables();
	LoadLandscape("./Data/Landscape/Land1.lnd");
	LoadMap("./Scripts/Land1.txt");

	// _lhvm = std::make_unique<LHVM::LHVM>();
	// _lhvm->LoadBinary(GetGamePath() + "/Scripts/Quests/challenge.chl");

	{
		int width, height;
		_window->GetDrawableSize(width, height);
		_renderer->ConfigureView(graphics::RenderPass::Main, width, height);
	}
	{
		uint16_t width, height;
		_water->GetFrameBuffer().GetSize(width, height);
		_renderer->ConfigureView(graphics::RenderPass::Reflection, width, height);
	}

	while (Update())
	{
		{
			auto section = _profiler->BeginScoped(Profiler::Stage::SceneDraw);

			Renderer::DrawSceneDesc drawDesc{
				/*viewId =*/ graphics::RenderPass::Main,
				/*profiler =*/ *_profiler,
				/*camera =*/ _camera.get(),
				/*frameBuffer =*/ nullptr,
				/*drawSky =*/ _config.drawSky,
				/*sky =*/ *_sky,
				/*drawWater =*/ _config.drawWater,
				/*water =*/ *_water,
				/*drawIsland =*/ _config.drawIsland,
				/*island =*/ *_landIsland,
				/*drawEntities =*/ _config.drawEntities,
				/*entities =*/ *_entityRegistry,
				/*drawDebugCross =*/ _config.drawDebugCross,
				/*drawBoundingBoxes =*/ _config.drawBoundingBoxes,
				/*cullBack =*/ false,
				/*bgfxDebug =*/ _config.bgfxDebug,
				/*wireframe =*/ _config.wireframe,
				/*profile =*/ _config.showProfiler,
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
	}
}

void Game::LoadMap(const std::string& name)
{
	if (!_fileSystem->Exists(name))
		throw std::runtime_error("Could not find script " + name);

	auto data = _fileSystem->ReadAll(name);
	std::string source(reinterpret_cast<const char*>(data.data()), data.size());

	_entityRegistry->Reset();
	Script script(this);
	script.Load(source);
}

void Game::LoadLandscape(const std::string& name)
{
	if (_landIsland)
		_landIsland.reset();

	if (!_fileSystem->Exists(name))
		throw std::runtime_error("Could not find landscape " + name);

	auto file = _fileSystem->Open(name, FileMode::Read);

	_landIsland = std::make_unique<LandIsland>();
	_landIsland->LoadFromFile(*file);
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
		DWORD dataLen  = 0;
		LSTATUS status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir", RRF_RT_REG_SZ, nullptr, nullptr, &dataLen);
		if (status == ERROR_SUCCESS)
		{
			char* path = new char[dataLen];
			status     = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir", RRF_RT_REG_SZ, nullptr, path, &dataLen);

			sGamePath = std::string(path);
			return sGamePath;
		}

		spdlog::error("Failed to find the GameDir registry value, game not installed.");
#endif // _WIN32

		// no key, don't guess, let the user know to set the command param
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                 "Game Path missing",
                                 "Game path was not supplied, use the -g command parameter to set it.",
                                 nullptr);
        spdlog::error("Failed to find the GameDir.");
        exit(1);
	}

	return sGamePath;
}

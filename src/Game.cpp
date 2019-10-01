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
	,_running(true)
{
	spdlog::set_level(spdlog::level::debug);
	sInstance = this;

	int windowWidth = 1280, windowHeight = 1024;
	DisplayMode displayMode = DisplayMode::Windowed;

	auto args = CmdLineArgs(argc, argv);
	args.Get("w", windowWidth);
	args.Get("h", windowHeight);

	std::string binaryPath = fs::path{argv[0]}.parent_path().generic_string();
	spdlog::info("current binary path: {}", binaryPath);
	{
		std::string gamePath;
		args.Get("g", gamePath);
		SetGamePath(gamePath);
	}

	_window = std::make_unique<GameWindow>(kWindowTitle + " [" + kBuildStr + "]", windowWidth, windowHeight, displayMode);

	_renderer = std::make_unique<Renderer>(*_window, binaryPath);

	_fileSystem->SetGamePath(GetGamePath());
	spdlog::debug("The GamePath is \"{}\".", _fileSystem->GetGamePath().generic_string());

	float scale = 1.0f;
	args.Get<float>("scale", scale);
	_gui = Gui::create(*_window, 255, scale);
}

Game::~Game()
{
	_water.reset();
	_sky.reset();
	_testModel.reset();
	_meshPack.reset();
	_landIsland.reset();
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

void Game::Run()
{
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

	// measure our delta time
	using namespace std::chrono_literals;
	uint64_t now                        = SDL_GetPerformanceCounter();
	uint64_t last                       = 0;
	std::chrono::microseconds deltaTime = 0us;

	_running = true;
	SDL_Event e;
	while (_running)
	{
		last = now;
		now  = SDL_GetPerformanceCounter();

		deltaTime = std::chrono::microseconds((now - last) * 1000000 / SDL_GetPerformanceFrequency());

		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
				_running = false;
			if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE && e.window.windowID == _window->GetID())
				_running = false;
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
				_running = false;
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_f)
				_window->SetFullscreen(true);
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F1)
				_config.bgfxDebug = !_config.bgfxDebug;
			if (e.type == SDL_MOUSEMOTION)
			{
				SDL_GetMouseState(&_mousePosition.x, &_mousePosition.y);
				glm::ivec2 screenSize;
				_window->GetSize(screenSize.x, screenSize.y);

				glm::vec3 rayOrigin, rayDirection;
				_camera->DeprojectScreenToWorld(_mousePosition, screenSize, rayOrigin, rayDirection);

				float intersectDistance = 0.0f;
				bool intersects         = glm::intersectRayPlane(
                    rayOrigin,
                    rayDirection,
                    glm::vec3(0.0f, 0.0f, 0.0f), // plane origin
                    glm::vec3(0.0f, 1.0f, 0.0f), // plane normal
                    intersectDistance);

				if (intersects)
					_intersection = rayOrigin + rayDirection * intersectDistance;

				float height    = _landIsland->GetHeightAt(glm::vec2(_intersection.x, _intersection.z));
				_intersection.y = height;
			}
			// rotation mode on middle mouse button
			else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
			{
				if (e.button.button == SDL_BUTTON_MIDDLE)
				{
					glm::ivec2 screenSize;
					_window->GetSize(screenSize.x, screenSize.y);
					SDL_SetRelativeMouseMode((e.type == SDL_MOUSEBUTTONDOWN) ? SDL_TRUE : SDL_FALSE);
					SDL_WarpMouseInWindow(_window->GetHandle(), screenSize.x / 2, screenSize.y / 2);
				}
			}

			_camera->ProcessSDLEvent(e);

			_gui->ProcessEventSdl2(e);
		}

		_camera->Update(deltaTime);
		_modelRotation.y = fmod(_modelRotation.y + float(deltaTime.count()) * .0001f, 360.f);

		_renderer->UpdateDebugCrossPose(deltaTime, _intersection, 50.0f);

		_gui->Loop(*this);

		// Reflection Pass
		bgfx::setViewName(0, "Reflection Pass");
		_water->BeginReflection(0,*_camera);
		{
			// TODO(bwrsandman): The setting of viewport and clearing should probably be done in framebuffer bind
			uint16_t width, height;
			_water->GetFramebufferSize(width, height);
			_renderer->ClearScene(0, width, height);
		}
		auto reflectionCamera = _water->GetReflectionCamera();
		_renderer->UploadUniforms(deltaTime, 0, *this, reflectionCamera);
		_renderer->DrawScene(*this, 0, false, false, true);
//		_water->EndReflection(1);

		// Main Draw Pass
		bgfx::setViewName(1, "Main Pass");
		{
			int width, height;
			_window->GetDrawableSize(width, height);
			_renderer->ClearScene(1, width, height);
		}
		_renderer->UploadUniforms(deltaTime, 1, *this, *_camera);
		_renderer->DrawScene(*this, 1, true, true, false);

		_gui->Draw();
		_renderer->Frame();
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

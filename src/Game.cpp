/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Game.h"

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include "GitSHA1.h"

#include <3D/Camera.h>
#include <3D/LandIsland.h>
#include <3D/MeshPack.h>
#include <3D/L3DModel.h>
#include <3D/Sky.h>
#include <3D/Water.h>
#include <Common/CmdLineArgs.h>
#include <Common/FileSystem.h>
#include <Entities/Registry.h>
#include <Entities/Components/Model.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>
#include <LHScriptX/Script.h>
#include <LHVMViewer.h>
#include <Renderer.h>
#include <glm/gtx/intersect.hpp>

#ifdef WIN32
#include <Windows.h>
#endif

#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_opengl3.h>
#include <imgui/examples/imgui_impl_sdl.h>

#include <spdlog/spdlog.h>

using namespace OpenBlack;
using namespace OpenBlack::LHScriptX;

const std::string kBuildStr(kGitSHA1Hash, 8);
const std::string kWindowTitle = "OpenBlack";

Game* Game::sInstance = nullptr;

Game::Game(int argc, char** argv):
    _running(true), _wireframe(false), _waterDebug(false), _timeOfDay(1.0f), _bumpmapStrength(1.0f), _smallBumpmapStrength(1.0f),
    _fileSystem(std::make_unique<FileSystem>()),
    _entityRegistry(std::make_unique<Entities::Registry>())
{
	spdlog::set_level(spdlog::level::debug);
	sInstance = this;

	int windowWidth = 1280, windowHeight = 1024;
	DisplayMode displayMode = DisplayMode::Windowed;

	auto args = CmdLineArgs(argc, argv);
	args.Get("w", windowWidth);
	args.Get("h", windowHeight);

	{
		std::string gamePath;
		args.Get("g", gamePath);
		SetGamePath(gamePath);
	}
	_window = std::make_unique<GameWindow>(kWindowTitle + " [" + kBuildStr + "]", windowWidth, windowHeight, displayMode);
	_window->SetSwapInterval(1);

	_renderer = std::make_unique<Renderer>(_window);

	_fileSystem->SetGamePath(GetGamePath());
	spdlog::debug("The GamePath is \"{}\".", _fileSystem->GetGamePath().generic_string());

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io    = ImGui::GetIO();
	io.IniFilename = NULL;

	ImGui::StyleColorsLight();

	ImGuiStyle& style     = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;

	ImGui_ImplSDL2_InitForOpenGL(_window->GetHandle(), _renderer->GetGLContext());
	ImGui_ImplOpenGL3_Init("#version 130");
}

Game::~Game()
{
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

	_camera->SetPosition(glm::vec3(2441.865f, 24.764f, 1887.351f));
	_camera->SetRotation(glm::vec3(12.0f, 117.0f, 0.0f));

	_modelPosition = glm::vec3(2485.0f, 50.0f, 1907.0f);
	_modelRotation = glm::vec3(180.0f, 111.0f, 0.0f);
	_modelScale    = glm::vec3(0.5f);

	File file(GetGamePath() + "/Data/AllMeshes.g3d", FileMode::Read);
	_meshPack = std::make_unique<MeshPack>();
	_meshPack->LoadFromFile(file);
	file.Close();

	//_videoPlayer = std::make_unique<Video::VideoPlayer>(GetGamePath() + "/Data/logo.bik");

	_testModel = std::make_unique<L3DModel>();
	_testModel->LoadFromFile(GetGamePath() + "/Data/CreatureMesh/C_Tortoise_Base.l3d");

	_sky   = std::make_unique<Sky>();
	_water = std::make_unique<Water>();

	LoadLandscape("./Data/Landscape/Land1.lnd");
	LoadMap("./Scripts/Land1.txt");

	// _lhvm = std::make_unique<LHVM::LHVM>();
	// _lhvm->LoadBinary(GetGamePath() + "/Scripts/Quests/challenge.chl");

	// measure our delta time
	using namespace std::chrono_literals;
	uint64_t now     = SDL_GetPerformanceCounter();
	uint64_t last    = 0;
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

				// rotation mode on middle mouse button
				if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
				{
					if (e.button.button == SDL_BUTTON_MIDDLE)
					{
						SDL_SetRelativeMouseMode((e.type == SDL_MOUSEBUTTONDOWN) ? SDL_TRUE : SDL_FALSE);
						SDL_WarpMouseInWindow(_window->GetHandle(), screenSize.x/2, screenSize.y/2);
					}
				}
			}

			_camera->ProcessSDLEvent(e);

			ImGui_ImplSDL2_ProcessEvent(&e);
		}

		_camera->Update(deltaTime);
		_modelRotation.y = fmod(_modelRotation.y + float(deltaTime.count()) * .0001f, 360.f);

		this->guiLoop();

		ImGuiIO& io = ImGui::GetIO();
		_renderer->ClearScene((int)io.DisplaySize.x, (int)io.DisplaySize.y);

		_water->BeginReflection(*_camera);
		_renderer->DrawScene(deltaTime, *this, _water->GetReflectionCamera(), false);
		_water->EndReflection();

		// reset viewport here, should be done in EndReflection
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);

		_renderer->DrawScene(deltaTime, *this, *_camera, true);

		_renderer->DebugDraw(deltaTime, *this, _intersection, 50.0f);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		_window->SwapWindow();
	}
}

void Game::guiLoop()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(_window->GetHandle());
	ImGui::NewFrame();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Quit", "Esc")) { _running = false; }
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::Checkbox("Wireframe", &_wireframe);
			ImGui::Checkbox("Water Debug", &_waterDebug);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools"))
		{
			if (ImGui::MenuItem("Dump Land Textures")) { _landIsland->DumpTextures(); }
			ImGui::EndMenu();
		}

		ImGui::Text("%d, %d", _mousePosition.x, _mousePosition.y);

		ImGui::SameLine(ImGui::GetWindowWidth() - 154.0f);
		ImGui::Text("%.2f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

		ImGui::EndMainMenuBar();
	}

	ImGui::Begin("Mesh Pack");

	int i = 0;
	for (const auto& tex : _meshPack->GetTextures())
	{
		ImGui::Image((ImTextureID)tex->GetNativeHandle(), ImVec2(128, 128));

		if (++i % 4 != 0)
			ImGui::SameLine();
	}

	ImGui::End();

	//ImGui::ShowDemoWindow();

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 8.0f, io.DisplaySize.y - 8.0f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
	ImGui::SetNextWindowBgAlpha(0.35f);

	if (ImGui::Begin("Camera position overlay", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
	{
		auto camPos = _camera->GetPosition();
		auto camRot = _camera->GetRotation();
		ImGui::Text("Camera Position: (%.1f,%.1f, %.1f)", camPos.x, camPos.y, camPos.z);
		ImGui::Text("Camera Rotation: (%.1f,%.1f, %.1f)", camRot.x, camRot.y, camRot.z);

		if (ImGui::IsMousePosValid())
			ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
		else
			ImGui::Text("Mouse Position: <invalid>");
	}
	ImGui::End();

	if (_lhvm != nullptr)
		LHVMViewer::Draw(_lhvm.get());

	ImGui::Begin("Land Island");

	ImGui::Text("Load Land Island:");
	ImGui::BeginGroup();
	if (ImGui::Button("1"))
	{
		LoadLandscape("./Data/Landscape/Land1.lnd");
		LoadMap("./Scripts/Land1.txt");
	}
	ImGui::SameLine();
	if (ImGui::Button("2"))
	{
		LoadLandscape("./Data/Landscape/Land2.lnd");
		LoadMap("./Scripts/Land2.txt");
	}
	ImGui::SameLine();
	if (ImGui::Button("3"))
	{
		LoadLandscape("./Data/Landscape/Land3.lnd");
		LoadMap("./Scripts/Land3.txt");
	}
	ImGui::SameLine();
	if (ImGui::Button("4"))
	{
		LoadLandscape("./Data/Landscape/Land4.lnd");
		LoadMap("./Scripts/Land4.txt");
	}
	ImGui::SameLine();
	if (ImGui::Button("5"))
	{
		LoadLandscape("./Data/Landscape/Land5.lnd");
		LoadMap("./Scripts/Land5.txt");
	}

	ImGui::SameLine();
	if (ImGui::Button("T"))
	{
		LoadLandscape("./Data/Landscape/LandT.lnd");
		LoadMap("./Scripts/LandT.txt");
	}

	if (ImGui::Button("2P"))
	{
		LoadLandscape("./Data/Landscape/Multi_Player/MPM_2P_1.lnd");
		LoadMap("./Scripts/Playgrounds/TwoGods.txt");
	}

	ImGui::SameLine();
	if (ImGui::Button("3P"))
	{
		LoadLandscape("./Data/Landscape/Multi_Player/MPM_3P_1.lnd");
		LoadMap("./Scripts/Playgrounds/ThreeGods.txt");
	}
	ImGui::SameLine();
	if (ImGui::Button("4P"))
	{
		LoadLandscape("./Data/Landscape/Multi_Player/MPM_4P_1.lnd");
		LoadMap("./Scripts/Playgrounds/FourGods.txt");
	}

	ImGui::EndGroup();

	ImGui::SliderFloat("Day", &_timeOfDay, 0.0f, 1.0f, "%.3f");
	ImGui::SliderFloat("Bump", &_bumpmapStrength, 0.0f, 1.0f, "%.3f");
	ImGui::SliderFloat("Small Bump", &_smallBumpmapStrength, 0.0f, 1.0f, "%.3f");

	ImGui::Separator();

	ImGui::Text("Block Count: %zu", _landIsland->GetBlocks().size());
	ImGui::Text("Country Count: %zu", _landIsland->GetCountries().size());

	ImGui::Separator();

	if (ImGui::Button("Dump Textures"))
		_landIsland->DumpTextures();

	if (ImGui::Button("Dump Heightmap"))
		_landIsland->DumpMaps();

	ImGui::End();

	if (_waterDebug)
		_water->DebugGUI();

	ImGui::Render();
}

void Game::LoadMap(const std::string& name)
{
	if (!_fileSystem->Exists(name))
		throw std::runtime_error("Could not find script " + name);

	auto file = _fileSystem->Open(name, FileMode::Read);

	_entityRegistry->Reset();
	Script script(this);
	script.LoadFromFile(*file);
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

void Game::SetGamePath(const std::string &gamePath)
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

		// no key? guess
#ifdef _WIN32
		sGamePath = std::string("C:\\Program Files (x86)\\Lionhead Studios Ltd\\Black & White");
#else
		sGamePath = std::string("/mnt/windows/Program Files (x86/Lionhead Studios Ltd/Black & White");
#endif // _WIN32

		spdlog::debug("Guessing the GamePath using \"{}\".", sGamePath);
	}

	return sGamePath;
}

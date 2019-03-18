/* OpenBlack - A reimplementation of Lionheads's Black & White engine
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

#include <iostream>
#include <sstream>
#include <string>

#include <SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <3D/Camera.h>
#include <3D/LandIsland.h>

#include <Common/OSFile.h>

#include <Graphics/ShaderProgram.h>
#include <Graphics/Texture2D.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

#include <3D/MeshPack.h>
#include <3D/Sky.h>
#include <3D/SkinnedModel.h>
#include <3D/Water.h>

#include <Video/VideoPlayer.h>

//#include <Script/LHScriptX.h>
#include <LHScriptX/Script.h>
#include <LHScriptX/Command.h>
#include <LHScriptX/Impl_LandCommands.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>

#include <Common/CmdLineArgs.h>

#include "GitSHA1.h"

using namespace OpenBlack;
using namespace OpenBlack::Graphics;
using namespace OpenBlack::LHScriptX;

const std::string kBuildStr(kGitSHA1Hash, 8);
const std::string kWindowTitle = "OpenBlack";

Game* Game::sInstance = nullptr;

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
		(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
		type, severity, message);
}

Game::Game(int argc, char **argv)
	: _running(true), _wireframe(false), _timeOfDay(1.0f), _bumpmapStrength(1.0f)
{
	int windowWidth = 1280, windowHeight = 1024;
	DisplayMode displayMode = DisplayMode::Windowed;

	auto args = CmdLineArgs(argc, argv);
	args.Get("w", windowWidth);
	args.Get("h", windowHeight);

	_window = std::make_unique<GameWindow>(kWindowTitle + " [" + kBuildStr + "]", windowWidth, windowHeight, displayMode);
	_window->SetSwapInterval(1);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
	glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);

	sInstance = this;
}

Game::~Game()
{
    SDL_Quit(); // todo: move to GameWindow
}

void Game::Run()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;

	ImGui::StyleColorsLight();

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;

	ImGui_ImplSDL2_InitForOpenGL(_window->GetHandle(), _window->GetGLContext());
	ImGui_ImplOpenGL3_Init("#version 130");

	// create our camera
	_camera = std::make_unique<Camera>();
	_camera->SetProjectionMatrixPerspective(60.0f, _window->GetAspectRatio(), 0.1f, 65536.0f);
	_camera->SetPosition(glm::vec3(2441.865f, 56.764f, 1887.351f));
	_camera->SetRotation(glm::vec3(117.0f, 12.0f, 0.0f));

	_modelPosition = glm::vec3(2485.0f, 39.0f, 1907.0f);
	_modelRotation = glm::vec3(0.0f, 24.0f, 90.0f);
	_modelScale = glm::vec3(0.5f);

	_videoPlayer = std::make_unique<Video::VideoPlayer>(GetGamePath() + "/Data/logo.bik");

	_worldObjectShader = std::make_unique<ShaderProgram>("shaders/skin.vert", "shaders/skin.frag");
	_testModel = std::make_unique<SkinnedModel>();
	_testModel->LoadFromFile(GetGamePath() + "/Data/CreatureMesh/C_Tortoise_Base.l3d");

	_sky = std::make_unique<Sky>();
	_water = std::make_unique<Water>();

	LoadMap(GetGamePath() + "/Data/Landscape/Land1.lnd");
	//_landIsland->DumpMaps();

	ShaderProgram* terrainShader = new ShaderProgram("shaders/terrain.vert", "shaders/terrain.frag");

	// measure our delta time
	uint64_t now = SDL_GetPerformanceCounter();
	uint64_t last = 0;
	double deltaTime = 0.0;

	_running = true;
	SDL_Event e;
	while (_running)
	{
		last = now;
		now = SDL_GetPerformanceCounter();

		deltaTime = ((now - last) * 1000 / (double)SDL_GetPerformanceFrequency());

		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				_running = false;
			if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE && e.window.windowID == _window->GetID())
				_running = false;
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
				_running = false;
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_f)
				_window->SetFullscreen(true);

			_camera->ProcessSDLEvent(&e);

			ImGui_ImplSDL2_ProcessEvent(&e);
		}

		_camera->Update(deltaTime);

		this->guiLoop();

		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(39.0f / 255.0f, 70.0f / 255.0f, 89.0f / 255.0f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		_sky->Draw();
		//_water->Draw();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (_wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		terrainShader->Bind();
		terrainShader->SetUniformValue("viewProj", _camera->GetViewProjectionMatrix());
		terrainShader->SetUniformValue("timeOfDay", _timeOfDay);
		terrainShader->SetUniformValue("bumpmapStrength", _bumpmapStrength);
		terrainShader->SetUniformValue("sMaterials", 0);
		terrainShader->SetUniformValue("sBumpMap", 1);

		_landIsland->GetMaterialArray()->Bind(0);
		_landIsland->GetBumpMap()->Bind(1);

		_landIsland->Draw(terrainShader);

		if (_wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glm::mat4 modelMatrix = glm::mat4(1.0f);
		modelMatrix = glm::translate(modelMatrix, _modelPosition);

		modelMatrix = glm::rotate(modelMatrix, glm::radians(_modelRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(_modelRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		modelMatrix = glm::rotate(modelMatrix, glm::radians(_modelRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

		modelMatrix = glm::scale(modelMatrix, _modelScale);

		_worldObjectShader->Bind();
		_worldObjectShader->SetUniformValue("u_viewProjection", _camera->GetViewProjectionMatrix());
		_worldObjectShader->SetUniformValue("u_modelTransform", modelMatrix);

		_testModel->Draw(_worldObjectShader.get());

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		_window->SwapWindow();
	}

	delete terrainShader;
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
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools"))
		{
			if (ImGui::MenuItem("Dump Land Textures")) { _landIsland->DumpTextures(); }
			ImGui::EndMenu();
		}

		ImGui::SameLine(ImGui::GetWindowWidth() - 154.0f);
		ImGui::Text("%.2f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

		ImGui::EndMainMenuBar();
	}

	//ImGui::ShowDemoWindow();

	//m_MeshViewer->GUI();

	int width, height;
	_window->GetSize(width, height);
	ImGui::SetNextWindowPos(ImVec2(width - 300.0f, 32.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Camera", NULL, ImGuiWindowFlags_AlwaysAutoResize);

	glm::vec3 pos = _camera->GetPosition();
	glm::vec3 rot = _camera->GetRotation();

	ImGui::DragFloat3("Position", &pos[0]);
	ImGui::DragFloat3("Rotation", &rot[0]);

	_camera->SetPosition(pos);
	_camera->SetRotation(rot);

	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2(width - 300.0f, 128.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Model", NULL, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::DragFloat3("Position", glm::value_ptr(_modelPosition));
	ImGui::DragFloat3("Rotation", glm::value_ptr(_modelRotation));
	ImGui::SliderFloat3("Scale", glm::value_ptr(_modelScale), 0.0f, 2.0f, "%.1f");

	ImGui::End();

	ImGui::Begin("Bones");

	ImGui::BeginChild("Scrolling");

	auto &bones = _testModel->GetBones();
	for (int b = 0; b < bones.size(); b++)
	{
		auto &bone = bones[b];
		ImGui::DragFloat3(std::to_string(b).c_str(), &bone.position[0]);
	}

	ImGui::EndChild();

	ImGui::End();

	ImGui::Begin("Land Island");

	ImGui::Text("Load Land Island:");
	ImGui::BeginGroup();
	if (ImGui::Button("1"))
		LoadMap(GetGamePath() + "/Data/Landscape/Land1.lnd");
	ImGui::SameLine();
	if (ImGui::Button("2"))
		LoadMap(GetGamePath() + "/Data/Landscape/Land2.lnd");
	ImGui::SameLine();
	if (ImGui::Button("3"))
		LoadMap(GetGamePath() + "/Data/Landscape/Land3.lnd");
	ImGui::SameLine();
	if (ImGui::Button("4"))
		LoadMap(GetGamePath() + "/Data/Landscape/Land4.lnd");
	ImGui::SameLine();
	if (ImGui::Button("5"))
		LoadMap(GetGamePath() + "/Data/Landscape/Land5.lnd");
	ImGui::SameLine();
	if (ImGui::Button("T"))
		LoadMap(GetGamePath() + "/Data/Landscape/LandT.lnd");
	ImGui::SameLine();
	ImGui::EndGroup();

	ImGui::SliderFloat("Day", &_timeOfDay, 0.0f, 1.0f, "%.3f");
	ImGui::SliderFloat("Bump", &_bumpmapStrength, 0.0f, 1.0f, "%.3f");
	
	/*auto noisemap = _landIsland->GetNoiseMap();
	auto bumpmap = _landIsland->GetBumpMap();
	ImGui::Text("Noisemap");
	ImGui::Image((void*)(intptr_t)noisemap->GetHandle(), ImVec2(noisemap->GetWidth() / 2, noisemap->GetHeight() / 2));
	ImGui::Text("Bumpmap");
	ImGui::Image((void*)(intptr_t)bumpmap->GetHandle(), ImVec2(bumpmap->GetWidth() / 2, bumpmap->GetHeight() / 2));*/
	ImGui::End();

	ImGui::Render();
}

void Game::LoadMap(std::string name)
{
	if (_landIsland)
		_landIsland.reset();

	_landIsland = std::make_unique<LandIsland>();
	_landIsland->LoadFromDisk(name);
}

std::string Game::GetGamePath()
{
	static std::string sGamePath;

	if (sGamePath.empty())
	{
#ifdef _WIN32
		DWORD dataLen = 0;
		LSTATUS status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir", RRF_RT_REG_SZ, nullptr, nullptr, &dataLen);
		if (status == ERROR_SUCCESS)
		{
			char* path = new char[dataLen];
			status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir", RRF_RT_REG_SZ, nullptr, path, &dataLen);

			sGamePath = std::string(path);
			return sGamePath;
		}

		std::cerr << "Failed to find GameDir registry value, game not installed" << std::endl;
#endif // _WIN32

		// no key? guess
#ifdef _WIN32
		sGamePath = std::string("C:\\Program Files (x86)\\Lionhead Studios Ltd\\Black & White");
#else
		sGamePath = std::string("/mnt/windows/Program Files (x86/Lionhead Studios Ltd/Black & White");
#endif // _WIN32

		std::clog << "Guessing GamePath: " << sGamePath << std::endl;
	}

	return sGamePath;
}

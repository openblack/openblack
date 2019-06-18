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

#include <iostream>
#include <sstream>
#include <string>

#include <SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/intersect.hpp>

#include <3D/Camera.h>
#include <3D/LandIsland.h>

#include <Common/OSFile.h>

#include <Graphics/ShaderProgram.h>
#include <Graphics/ShaderManager.h>
#include <Graphics/Texture2D.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

#include <3D/MeshPack.h>
#include <3D/Sky.h>
#include <3D/SkinnedModel.h>
#include <3D/Water.h>

//#include <Video/VideoPlayer.h>

#include <LHScriptX/Script.h>
#include <LHVMViewer.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>

#include <Common/CmdLineArgs.h>

#include "GitSHA1.h"

#include <Graphics/DebugDraw.h>

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
	: _running(true), _wireframe(false), _timeOfDay(1.0f), _bumpmapStrength(1.0f), _shaderManager(std::make_unique<ShaderManager>())
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

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;

	ImGui::StyleColorsLight();

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;

	ImGui_ImplSDL2_InitForOpenGL(_window->GetHandle(), _window->GetGLContext());
	ImGui_ImplOpenGL3_Init("#version 130");

	_shaderManager->LoadShader("DebugLine", "shaders/line.vert", "shaders/line.frag");
	_shaderManager->LoadShader("Terrain", "shaders/terrain.vert", "shaders/terrain.frag");
	_shaderManager->LoadShader("SkinnedMesh", "shaders/skin.vert", "shaders/skin.frag");

	DebugDraw::Init();

	sInstance = this;
}

Game::~Game()
{
	DebugDraw::Shutdown();
    SDL_Quit(); // todo: move to GameWindow
}

void Game::Run()
{
	// create our camera
	_camera = std::make_unique<Camera>();
	_camera->SetProjectionMatrixPerspective(60.0f, _window->GetAspectRatio(), 0.1f, 65536.0f);
	_camera->SetPosition(glm::vec3(2441.865f, 56.764f, 1887.351f));
	_camera->SetRotation(glm::vec3(117.0f, 12.0f, 0.0f));

	_modelPosition = glm::vec3(2485.0f, 50.0f, 1907.0f);
	_modelRotation = glm::vec3(180.0f, 111.0f, 0.0f);
	_modelScale = glm::vec3(0.5f);

	//_videoPlayer = std::make_unique<Video::VideoPlayer>(GetGamePath() + "/Data/logo.bik");

	_testModel = std::make_unique<SkinnedModel>();
	_testModel->LoadFromFile(GetGamePath() + "/Data/CreatureMesh/C_Tortoise_Base.l3d");

	_sky = std::make_unique<Sky>();
	_water = std::make_unique<Water>();

	LoadMap(GetGamePath() + "/Data/Landscape/Land1.lnd");

	Script script;
	script.LoadFile(GetGamePath() + "/Scripts/Land1.txt");

	// _lhvm = std::make_unique<LHVM::LHVM>();
	// _lhvm->LoadBinary(GetGamePath() + "/Scripts/Quests/challenge.chl");

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
			if (e.type == SDL_MOUSEMOTION)
			{
				SDL_GetMouseState(&_mousePosition.x, &_mousePosition.y);
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
					intersectDistance
				);

				if (intersects)
					_intersection = rayOrigin + rayDirection * intersectDistance;
			}

			_camera->ProcessSDLEvent(&e);

			ImGui_ImplSDL2_ProcessEvent(&e);
		}

		DebugDraw::Cross(_intersection, 50.0f);

		_camera->Update(deltaTime);
		_modelRotation.y = fmod(_modelRotation.y + deltaTime * .1f, 360.f);

		this->guiLoop();

		ImGuiIO& io = ImGui::GetIO();
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

		ShaderProgram* terrainShader = _shaderManager->GetShader("Terrain");
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

		ShaderProgram* objectShader = _shaderManager->GetShader("SkinnedMesh");
		objectShader->Bind();
		objectShader->SetUniformValue("u_viewProjection", _camera->GetViewProjectionMatrix());
		objectShader->SetUniformValue("u_modelTransform", modelMatrix);
		_testModel->Draw(objectShader);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		ShaderProgram* debugShader = _shaderManager->GetShader("DebugLine");
		debugShader->Bind();
		debugShader->SetUniformValue("u_viewProjection", _camera->GetViewProjectionMatrix());
		DebugDraw::DrawDebugLines();

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

	// ImGui::ShowDemoWindow();

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

	if (_lhvm != nullptr)
		LHVMViewer::Draw(_lhvm.get());

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

	ImGui::End();

	ImGui::Render();
}

void Game::LoadMap(const std::string &name)
{
	if (_landIsland)
		_landIsland.reset();

	_landIsland = std::make_unique<LandIsland>();
	_landIsland->LoadFromDisk(name);
}

const std::string& Game::GetGamePath()
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

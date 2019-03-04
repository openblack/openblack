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

#include <Graphics/GameShaders.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

#include <3D/MeshPack.h>

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
	: _running(true), _wireframe(false)
{
	int windowWidth = 1280, windowHeight = 768;
	DisplayMode displayMode = DisplayMode::Windowed;

	auto args = CmdLineArgs(argc, argv);
	args.Get("w", windowWidth);
	args.Get("h", windowHeight);

	_window = std::make_unique<GameWindow>(kWindowTitle + " [" + kBuildStr + "]", windowWidth, windowHeight, displayMode);
	_window->SetSwapInterval(0);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

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

	ImGui_ImplSDL2_InitForOpenGL(_window->GetHandle(), _window->GetGLContext());
	ImGui_ImplOpenGL3_Init("#version 130");

	// create our camera
	_camera = std::make_unique<Camera>();
	_camera->SetProjectionMatrixPerspective(60.0f, _window->GetAspectRatio(), 0.1f, 8192.f);
	_camera->SetPosition(glm::vec3(2458.0f, 169.0f, 1743.0f));
	_camera->SetRotation(glm::vec3(104.0f, 15.0f, 0.0f));

#ifdef _WIN32
	LoadMap(GetGamePath() + "\\Data\\Landscape\\Land1.lnd");
#else
	LoadMap(GetGamePath() + "/Data/Landscape/Land1.lnd");
#endif // _WIN32
	//LoadMap("Land1.lnd");

	/* we pass the unique_ptr straight to the Script, so do not reuse this */
	auto commands = std::make_unique<ScriptCommands>();
	commands->RegisterCommands(
		Impl_LandCommands::Definitions,
		sizeof(Impl_LandCommands::Definitions) / sizeof(*Impl_LandCommands::Definitions)
	);

	/*ScriptParameters parameters{ ScriptParameter(2.3000f) };
	ScriptCommandContext ctx(this, &parameters);
	_commands->Call("VERSION", ctx);*/

	_scriptx = std::make_unique<Script>();
	_scriptx->SetCommands(commands);

	//_scriptx->ScanLine("VERSION(\"LOL\")");
#ifdef _WIN32
	_scriptx->LoadFile(GetGamePath() + "\\Scripts\\Land1.txt");
#else
	_scriptx->LoadFile(GetGamePath() + "/Scripts/Land1.txt");
#endif // _WIN32

	//LHScriptX* script = new LHScriptX();
	//script->LoadFile(GetGamePath() + "\\Scripts\\Land1.txt");

	Shader* terrainShader = new Shader();
	terrainShader->Create(OpenBlack::Shaders::Terrain::VertexShader, OpenBlack::Shaders::Terrain::FragmentShader);

	GLint uniTerrainView = glGetUniformLocation(terrainShader->GetHandle(), "viewProj");
	GLint uniTerrainMaterialArray = glGetUniformLocation(terrainShader->GetHandle(), "sMaterials");
	GLint uniTerrainBumpMap = glGetUniformLocation(terrainShader->GetHandle(), "sBumpMap");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

		glUseProgram(terrainShader->GetHandle());
		glUniformMatrix4fv(uniTerrainView, 1, GL_FALSE, glm::value_ptr(_camera->GetViewProjectionMatrix()));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, _landIsland->GetMaterialArray()->GetHandle());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, _landIsland->GetBumpMap()->GetHandle());

		glUniform1i(uniTerrainMaterialArray, 0);
		glUniform1i(uniTerrainBumpMap, 1);

		if (_wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		_landIsland->Draw();

		if (_wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

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

		ImGui::SameLine(ImGui::GetWindowWidth() - 154.0f);
		ImGui::Text("%.2f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

		ImGui::EndMainMenuBar();
	}

	//m_MeshViewer->GUI();

	int width, height;
	_window->GetSize(width, height);
	ImGui::SetNextWindowPos(ImVec2(width - 300.0f, 32.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Camera", NULL, ImGuiWindowFlags_AlwaysAutoResize);

	glm::vec3 pos = _camera->GetPosition();
	glm::vec3 rot = _camera->GetRotation();

	ImGui::DragFloat3("Position", &pos[0]);
	ImGui::DragFloat3("Rotation", &rot[0]);
	ImGui::DragFloat3("Forward", &_camera->GetForward()[0]);

	_camera->SetPosition(pos);
	_camera->SetRotation(rot);

	ImGui::End();

	ImGui::Begin("Land Island");
	auto noisemap = _landIsland->GetNoiseMap();
	auto bumpmap = _landIsland->GetBumpMap();
	ImGui::Text("Noisemap");
	ImGui::Image((void*)(intptr_t)noisemap->GetHandle(), ImVec2(noisemap->GetWidth(), noisemap->GetHeight()));
	ImGui::Text("Bumpmap");
	ImGui::Image((void*)(intptr_t)bumpmap->GetHandle(), ImVec2(bumpmap->GetWidth(), bumpmap->GetHeight()));
	ImGui::End();

	if (_scriptx) {
		ImGui::Begin("LHScriptX", NULL, ImVec2(300, 200), -1.0f, NULL);

		ImGui::TextColored(ImVec4(1, 1, 0, 1), "Commands");
		ImGui::BeginChild("Scrolling");

		for (const auto& kv : _scriptx->GetCommands().GetCommands()) {
			ImGui::Text("%s", kv.first.c_str());
		}

		ImGui::EndChild();
		ImGui::End();
	}

	ImGui::Render();
}

void Game::LoadMap(std::string name)
{
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

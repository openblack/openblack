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

#include <Script/LHScriptX.h>

// #include <AllMeshes.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>

#include "GitSHA1.h"

using namespace OpenBlack;
using namespace OpenBlack::Graphics;

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
{
	// todo: parse arg

	sInstance = this;

	// create the game window
	_window = std::make_unique<GameWindow>(kWindowTitle + " [" + kBuildStr + "]", 1024, 768, DisplayMode::Windowed);
	_window->SetSwapInterval(1);

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);

	GetGamePath(); // lazy precache the game path todo: better
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
	_camera->SetProjectionMatrixPerspective(60.0f, (float)1280 / (float)960, 0.1f, 8192.f);
	_camera->SetPosition(glm::vec3(2174.0f, 185.0f, 1679.0f));
	_camera->SetRotation(glm::vec3(20.0f, 114.0f, 0.0f));

	//LoadMap(GetGamePath() + "\\Data\\Landscape\\Land1.lnd");
	LoadMap("Land1.lnd");

	Shader* terrainShader = new Shader();
	terrainShader->Create(OpenBlack::Shaders::Terrain::VertexShader, OpenBlack::Shaders::Terrain::FragmentShader);

	GLint uniTerrainView = glGetUniformLocation(terrainShader->GetHandle(), "viewProj");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	_running = true;
	SDL_Event e;
	while (_running)
	{
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT)
				_running = false;
			if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE && e.window.windowID == _window->GetID())
				_running = false;
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
				_running = false;
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_f)
				_window->SetFullscreen(true);

			ImGui_ImplSDL2_ProcessEvent(&e);
		}

		this->guiLoop();

		glClearColor(39.0f / 255.0f, 70.0f / 255.0f, 89.0f / 255.0f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(terrainShader->GetHandle());
		glUniformMatrix4fv(uniTerrainView, 1, GL_FALSE, glm::value_ptr(_camera->GetViewProjectionMatrix()));

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		_landIsland->Draw();
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
			if (ImGui::MenuItem("Dump Land Textures")) { _landIsland->DumpTextures(); }
			if (ImGui::MenuItem("Quit", "Alt+F4")) { _running = false; }
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	//m_MeshViewer->GUI();

	ImGui::Begin("Camera");

	glm::vec3 pos = _camera->GetPosition();
	glm::vec3 rot = _camera->GetRotation();

	ImGui::DragFloat3("Position", &pos[0]);
	ImGui::DragFloat3("Rotation", &rot[0]);

	_camera->SetPosition(pos);
	_camera->SetRotation(rot);

	ImGui::End();

	ImGui::Render();
}

void Game::LoadMap(std::string name)
{
	_landIsland = std::make_unique<LandIsland>();
	_landIsland->LoadFromDisk(name);
}

std::string Game::GetGamePath()
{
#ifdef _WIN32
	// todo: cache this
	DWORD dataLen;
	LSTATUS status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir", RRF_RT_REG_SZ, nullptr, nullptr, &dataLen);
	if (status == ERROR_SUCCESS)
	{
		char* path = new char[dataLen];
		status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir", RRF_RT_REG_SZ, nullptr, path, &dataLen);
		return path;
	}

	std::cerr << "Failed to find GameDir registry value, game not installed" << std::endl;
#endif // _WIN32

	// no key? guess
#ifdef _WIN32
	std::string guessPath = std::string("C:\\Program Files (x86)\\Lionhead Studios Ltd\\Black & White");
#else
	std::string guessPath = std::string("/mnt/windows/Program Files (x86/Lionhead Studios Ltd/Black & White");
#endif // _WIN32

	std::clog << "Guessing GamePath: " << guessPath << std::endl;

	return guessPath;
}

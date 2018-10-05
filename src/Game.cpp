#include "Game.h"

#include <iostream>
#include <sstream>

#include <SDL.h>
#define GLEW_STATIC
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <3D/Camera.h>
#include <3D/LandIsland.h>
#include <3D/oldLH3DIsland.h>

#include <Common/OSFile.h>

#include <Graphics/GameShaders.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/VertexBuffer.h>

#include <3D/MeshPack.h>

// #include <AllMeshes.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>

using namespace OpenBlack;
using namespace OpenBlack::Graphics;

void GLAPIENTRY OpenGLMsgCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
		return;

    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

void checkSDLError(int ret)
{
    if (ret != 0)
        std::cerr << "SDL error: " << SDL_GetError() << std::endl;
}

Game* Game::sInstance = nullptr;

Game::Game(int argc, char **argv)
    : m_Window(NULL),
    m_GLContext(NULL)
{
	sInstance = this;

    uint32_t flags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER;
    if (SDL_WasInit(flags) == 0)
    {
        SDL_SetMainReady();
        if (SDL_Init(flags) != 0)
            throw std::runtime_error("Could not initialize SDL! " + std::string(SDL_GetError()));
    }

    createWindow(1280, 960);
}

Game::~Game()
{
    SDL_GL_DeleteContext(m_GLContext);

    if (m_Window)
    {
        SDL_DestroyWindow(m_Window);
		m_Window = NULL;
    }

    SDL_Quit();
}

void Game::Run()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;

	ImGui_ImplSDL2_InitForOpenGL(m_Window, m_GLContext);
	ImGui_ImplOpenGL3_Init("#version 130");

	ImGui::StyleColorsDark();

    m_Camera = new Camera;
    m_Camera->SetProjectionMatrixPerspective(60.0f, (float)1280 / (float)960, 0.1f, 8192.f);
	//m_Camera->SetPosition(glm::vec3(0, 0, 0));
	//m_Camera->SetRotation(glm::vec3(0, 0, 0));
	m_Camera->SetPosition(glm::vec3(2742.0f, 61.0f, 1730.0f));
	//m_Camera->SetPosition(glm::vec3(1441.56f, 240.0f, 2081.76));
	m_Camera->SetRotation(glm::vec3(14.0f, 122.0f, 0.0f));

	m_meshPos = glm::vec3(2695.0f, 70.0f, 1761.0f);
	m_meshRot = glm::vec3(0.0f, 0.0f, 0.0f);

	LoadMap(GetGamePath() + "\\Data\\Landscape\\Land1.lnd");

	OSFile* allMeshesFile = new OSFile();
	allMeshesFile->Open((GetGamePath() + "\\Data\\AllMeshes.g3d").c_str(), LH_FILE_MODE::Read);
	m_MeshPack = new MeshPack(allMeshesFile);
	allMeshesFile->Close();

	m_MeshViewer = new MeshViewer();

	Shader* modelShader = new Shader();
	modelShader->Create(OpenBlack::Shaders::WorldObject::VertexShader, OpenBlack::Shaders::WorldObject::FragmentShader);

	Shader* terrainShader = new Shader();
	terrainShader->Create(OpenBlack::Shaders::Terrain::VertexShader, OpenBlack::Shaders::Terrain::FragmentShader);

	GLint uniMVP = glGetUniformLocation(modelShader->GetHandle(), "MVP");
	GLint uniTerrainView = glGetUniformLocation(terrainShader->GetHandle(), "viewProj");
	
	SDL_Event event;

	glEnable(GL_DEPTH_TEST);
	glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1);

	//glm::mat4 modelMatrix = glm::translate(glm::mat4(), glm::vec3(5.0f, 0.0f, 0.0f));
	//glm::mat4 modelMatrix = glm::mat4(1.0f);

	m_Running = true;
	while (m_Running)
	{
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				m_Running = false;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(m_Window))
				m_Running = false;
		}

		this->guiLoop();

		SDL_GL_MakeCurrent(m_Window, m_GLContext);
		//glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(terrainShader->GetHandle());
		glUniformMatrix4fv(uniTerrainView, 1, GL_FALSE, glm::value_ptr(m_Camera->GetViewProjectionMatrix()));
		m_LandIsland->Draw();

		glUseProgram(modelShader->GetHandle());

		glm::mat4 model = glm::mat4(1.0f);

		model = glm::translate(model, m_meshPos);
		model = glm::rotate(model, glm::radians(m_meshRot.x), glm::vec3(1, 0, 0));
		model = glm::rotate(model, glm::radians(m_meshRot.y), glm::vec3(0, 1, 0));
		model = glm::rotate(model, glm::radians(m_meshRot.z), glm::vec3(0, 0, 1));
		model = glm::scale(model, glm::vec3(1, 1, 1));

		glUniformMatrix4fv(uniMVP, 1, GL_FALSE, glm::value_ptr(m_Camera->GetViewProjectionMatrix() * model));
		m_MeshViewer->Render();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(m_Window);
	}
}

void Game::guiLoop()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(m_Window);
	ImGui::NewFrame();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Quit", "Alt+F4")) { m_Running = false; }
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	m_MeshViewer->GUI();

	ImGui::Begin("Camera");

	glm::vec3 pos = m_Camera->GetPosition();
	glm::vec3 rot = m_Camera->GetRotation();

	ImGui::DragFloat3("Position", &pos[0]);
	ImGui::DragFloat3("Rotation", &rot[0]);
	ImGui::DragFloat3("Mesh Pos", &m_meshPos[0]);
	ImGui::DragFloat3("Mesh Rot", &m_meshRot[0]);

	m_Camera->SetPosition(pos);
	m_Camera->SetRotation(rot);

	ImGui::End();

	ImGui::Render();
}

void Game::loop()
{

}

void Game::createWindow(int width, int height)
{
	int pos_x = SDL_WINDOWPOS_CENTERED,
		pos_y = SDL_WINDOWPOS_CENTERED;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

	m_Window = SDL_CreateWindow("OpenBlack", pos_x, pos_y, width, height, flags);
	if (!m_Window)
	{
		std::stringstream error;
		error << "Failed to create SDL window: " << SDL_GetError() << std::endl;
		throw std::runtime_error(error.str());
	}

	m_GLContext = SDL_GL_CreateContext(m_Window);
	if (!m_GLContext)
		throw std::runtime_error("Failed to create a GL context\n");

	SDL_GL_SetSwapInterval(1);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		std::stringstream error;
		error << "glewInit failed: " << glewGetErrorString(err) << std::endl;
		throw std::runtime_error(error.str());
	}

	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(OpenGLMsgCallback, 0);
}

void Game::LoadMap(std::string name)
{
	m_LandIsland = new LandIsland();
	m_LandIsland->LoadFromDisk(name);

	// GGame::ClearMap()
	// LH3DLandscape::Release()
	// LH3DIsland::Release()
	// GSetup::LoadMapFeatures(name)
	// Town::AsssignTownFeature()
}

std::string Game::GetGamePath()
{
	// todo: cache this
	DWORD dataLen;
	LSTATUS status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir", RRF_RT_REG_SZ, nullptr, nullptr, &dataLen);
	if (status == ERROR_SUCCESS)
	{
		char* path = new char[dataLen];
		status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir", RRF_RT_REG_SZ, nullptr, path, &dataLen);

		return path;
	}

	// no key? guess
    return std::string("C:\\Program Files (x86)\\Lionhead Studios Ltd\\Black & White");
}

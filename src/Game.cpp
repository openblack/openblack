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

#include <3D/Mesh.h>
#include <3D/MeshPack.h>

#include <AllMeshes.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>

using namespace OpenBlack;
using namespace OpenBlack::Graphics;

void GLAPIENTRY OpenGLMsgCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

void checkSDLError(int ret)
{
    if (ret != 0)
        std::cerr << "SDL error: " << SDL_GetError() << std::endl;
}

Game::Game(int argc, char **argv)
    : mWindow(NULL),
    mGLContext(NULL)
{
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
    SDL_GL_DeleteContext(mGLContext);

    if (mWindow)
    {
        SDL_DestroyWindow(mWindow);
        mWindow = NULL;
    }

    SDL_Quit();
}

void Game::Run()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui_ImplSDL2_InitForOpenGL(mWindow, mGLContext);
	ImGui_ImplOpenGL3_Init("#version 130");

	ImGui::StyleColorsDark();

    mCamera = new Camera;
    mCamera->SetProjectionMatrixPerspective(60.0f, (float)1280 / (float)960, 0.1f, 8192.f);
	//mCamera->SetPosition(glm::vec3(2500.0f, 240.0f, 1600.0f));
	//mCamera->SetPosition(glm::vec3(1441.56f, 240.0f, 2081.76));
	mCamera->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    mCamera->SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));

	std::string allMeshesFilePath = GetGamePath() + "\\Data\\AllMeshes.g3d";

	OSFile* allMeshesFile = new OSFile();
	allMeshesFile->Open(allMeshesFilePath.c_str(), LH_FILE_MODE::Read);
	MeshPack* pack = new MeshPack(allMeshesFile);
	allMeshesFile->Close();

    LandIsland* island = new LandIsland();
    island->LoadFromDisk(GetGamePath() + "\\Data\\Landscape\\Land1.lnd");

	Shader* modelShader = new Shader();
	modelShader->Create(OpenBlack::Shaders::WorldObject::VertexShader, OpenBlack::Shaders::WorldObject::FragmentShader);

	GLint uniView = glGetUniformLocation(modelShader->GetHandle(), "viewProj");
	
	SDL_Event event;

	glEnable(GL_DEPTH_TEST);
	glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1);

	mCurrentMesh = 0;
	mRunning = true;
	while (mRunning)
	{
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				mRunning = false;
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(mWindow))
				mRunning = false;
		}

		this->guiLoop();

		SDL_GL_MakeCurrent(mWindow, mGLContext);
		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		glClearColor(100.0f / 255.0f, 149.0f / 255.0f, 237.0f / 255.0f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(modelShader->GetHandle());
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(mCamera->GetViewProjectionMatrix()));

		pack->Meshes[mCurrentMesh]->Draw();

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		SDL_GL_SwapWindow(mWindow);
	}
}

void Game::guiLoop()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(mWindow);
	ImGui::NewFrame();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			ImGui::MenuItem("(dummy menu)", NULL, false, false);
			if (ImGui::MenuItem("New")) {}
			if (ImGui::MenuItem("Open", "Ctrl+O")) {}
			ImGui::Separator();
			if (ImGui::MenuItem("Quit", "Alt+F4")) { mRunning = false; }
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	{
		ImGui::Begin("Mesh Viewer");

		glm::vec3 v = mCamera->GetPosition();
		ImGui::SliderFloat3("Camera", glm::value_ptr(v), -50.0f, 50.0f);
		mCamera->SetPosition(v);

		ImGui::Separator();
		ImGui::PushItemWidth(-1);
		ImGui::ListBox("", &mCurrentMesh, gG3DStringList, IM_ARRAYSIZE(gG3DStringList), 8);
		ImGui::Separator();

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

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

	mWindow = SDL_CreateWindow("OpenBlack", pos_x, pos_y, width, height, flags);
	if (!mWindow)
	{
		std::stringstream error;
		error << "Failed to create SDL window: " << SDL_GetError() << std::endl;
		throw std::runtime_error(error.str());
	}

	mGLContext = SDL_GL_CreateContext(mWindow);
	if (!mGLContext)
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

	//glEnable(GL_DEBUG_OUTPUT);
	//glDebugMessageCallback(OpenGLMsgCallback, 0);
}

void Game::LoadMap(std::string name)
{
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

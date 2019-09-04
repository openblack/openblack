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
#include <sstream>
#include <string>
#include "GitSHA1.h"

#include <3D/Camera.h>
#include <3D/LandIsland.h>
#include <3D/MeshPack.h>
#include <3D/SkinnedModel.h>
#include <3D/Sky.h>
#include <3D/Water.h>
#include <Common/CmdLineArgs.h>
#include <Common/FileSystem.h>
#include <Entities/Registry.h>
#include <Entities/Components/Model.h>
#include <Entities/Components/Transform.h>
#include <Graphics/DebugDraw.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/ShaderManager.h>
#include <Graphics/ShaderProgram.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>
#include <LHScriptX/Script.h>
#include <LHVMViewer.h>
#include <glm/gtx/intersect.hpp>
#include <iostream>
#include <sstream>
#include <string>

#ifdef WIN32
#include <Windows.h>
#endif

#include <imgui/imgui.h>
#include <imgui/examples/imgui_impl_opengl3.h>
#include <imgui/examples/imgui_impl_sdl.h>

#include <spdlog/spdlog.h>

// About OpenGL function loaders: modern OpenGL doesn't have a standard header file and requires individual function pointers to be loaded manually.
// Helper libraries are often used for this purpose! Here we are supporting a few common ones: gl3w, glew, glad.
// You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif


using namespace OpenBlack;
using namespace OpenBlack::Graphics;
using namespace OpenBlack::LHScriptX;

const std::string kBuildStr(kGitSHA1Hash, 8);
const std::string kWindowTitle = "OpenBlack";

Game* Game::sInstance = nullptr;

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	spdlog::debug("GL CALLBACK: {} type = {0:x}, severity = {0:x}, message = {}\n",
	        (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
	        type, severity, message);
}

Game::Game(int argc, char** argv):
	_shaderManager(std::make_unique<ShaderManager>()),
	_fileSystem(std::make_unique<FileSystem>()),
	_entityRegistry(std::make_unique<Entities::Registry>())
{
	spdlog::set_level(spdlog::level::debug);
	sInstance = this;

	// get parameter
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

	_fileSystem->SetGamePath(GetGamePath());
	spdlog::debug("The GamePath is \"{}\".", _fileSystem->GetGamePath().generic_string());

	// setup all that is needed for the GUI
	std::string window_title =  kWindowTitle + " [" + kBuildStr + "]";

	// Setup SDL
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
	{
		throw std::runtime_error(std::string("SDL_Init failed with Error: ") + SDL_GetError());
	}
	{
		// print SDL version
		SDL_version compiledVersion, linkedVersion;
		SDL_VERSION(&compiledVersion)
		SDL_GetVersion(&linkedVersion);

		spdlog::info("Initializing SDL...");
		spdlog::info("SDL Version/Compiled {}.{}.{}", compiledVersion.major, compiledVersion.minor, compiledVersion.patch);
		spdlog::info("SDL Version/Linked {}.{}.{}", linkedVersion.major, linkedVersion.minor, linkedVersion.patch);
	}

	// Decide GL+GLSL versions
	// GL 3.0 + GLSL 130
	const char* glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

	SDL_ShowCursor(SDL_DISABLE);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	// Create window with graphics context
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	uint32_t window_flags =
				SDL_WINDOW_OPENGL |
				SDL_WINDOW_INPUT_FOCUS |
				SDL_WINDOW_RESIZABLE |
				SDL_WINDOW_ALLOW_HIGHDPI;
	SDL_Window* window = SDL_CreateWindow(
				window_title.c_str(),
				SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED,
				windowWidth, windowHeight, window_flags);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	// Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
	bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
	bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
	bool err = gladLoadGL() == 0;
#else
	bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
	if (err)
	{
#if defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
		std::stringstream error;
		error << "glewInit failed: " << glewGetErrorString(err) << std::endl;
		throw std::runtime_error(error.str());
#else
		throw std::runtime_error("Failed to initialize OpenGL loader!");
#endif
	}
	{
		int majorVersion, minorVersion;
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &majorVersion);
		SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minorVersion);
		spdlog::info("OpenGL version: {}.{}", majorVersion, minorVersion);
	}

	// debug outputs
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(MessageCallback, 0);
	glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	//ImGui::StyleColorsLight();
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'misc/fonts/README.txt' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// create GameWindow handle
	_window = std::make_unique<GameWindow>(window);

	// load shader
	_shaderManager->LoadShader("DebugLine", "shaders/line.vert", "shaders/line.frag");
	_shaderManager->LoadShader("Terrain", "shaders/terrain.vert", "shaders/terrain.frag");
	_shaderManager->LoadShader("SkinnedMesh", "shaders/skin.vert", "shaders/skin.frag");
	_shaderManager->LoadShader("Water", "shaders/water.vert", "shaders/water.frag");

	// allocate vertex buffers for our debug draw
	DebugDraw::Init();
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

	_testModel = std::make_unique<SkinnedModel>();
	_testModel->LoadFromFile(GetGamePath() + "/Data/CreatureMesh/C_Tortoise_Base.l3d");

	_sky   = std::make_unique<Sky>();
	_water = std::make_unique<Water>();

	LoadLandscape("./Data/Landscape/Land1.lnd");
	LoadMap("./Scripts/Land1.txt");

	// _lhvm = std::make_unique<LHVM::LHVM>();
	// _lhvm->LoadBinary(GetGamePath() + "/Scripts/Quests/challenge.chl");

	// measure our delta time
	uint64_t now     = SDL_GetPerformanceCounter();
	uint64_t last    = 0;
	double deltaTime = 0.0;

	_running = true;
	SDL_Event e;
	while (_running)
	{
		last = now;
		now  = SDL_GetPerformanceCounter();

		deltaTime = ((now - last) * 1000 / (double)SDL_GetPerformanceFrequency());

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

		DebugDraw::Cross(_intersection, 50.0f);

		_camera->Update(deltaTime);
		_modelRotation.y = fmod(_modelRotation.y + float(deltaTime) * .1f, 360.f);

		this->guiLoop();

		ImGuiIO& io = ImGui::GetIO();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(39.0f / 255.0f, 70.0f / 255.0f, 89.0f / 255.0f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		_water->BeginReflection(*_camera);
		drawScene(_water->GetReflectionCamera(), false);
		_water->EndReflection();

		// reset viewport here, should be done in EndReflection
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);

		drawScene(*_camera, true);

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

void Game::drawScene(const Camera& camera, bool drawWater)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	_sky->Draw(camera);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	if (drawWater)
	{
		ShaderProgram* waterShader = _shaderManager->GetShader("Water");
		waterShader->Bind();
		waterShader->SetUniformValue("viewProj", camera.GetViewProjectionMatrix());
		_water->Draw(*waterShader);
	}

	if (_wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	ShaderProgram* terrainShader = _shaderManager->GetShader("Terrain");
	terrainShader->Bind();
	terrainShader->SetUniformValue("viewProj", camera.GetViewProjectionMatrix());
	terrainShader->SetUniformValue("timeOfDay", _timeOfDay);
	terrainShader->SetUniformValue("bumpmapStrength", _bumpmapStrength);
	terrainShader->SetUniformValue("smallBumpmapStrength", _smallBumpmapStrength);

	_landIsland->Draw(*terrainShader);

	if (_wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix           = glm::translate(modelMatrix, _modelPosition);

	modelMatrix = glm::rotate(modelMatrix, glm::radians(_modelRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(_modelRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	modelMatrix = glm::rotate(modelMatrix, glm::radians(_modelRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

	modelMatrix = glm::scale(modelMatrix, _modelScale);

	ShaderProgram* objectShader = _shaderManager->GetShader("SkinnedMesh");
	objectShader->Bind();
	objectShader->SetUniformValue("u_viewProjection", camera.GetViewProjectionMatrix());
	objectShader->SetUniformValue("u_modelTransform", modelMatrix);
	_testModel->Draw(objectShader);
	_entityRegistry->DrawModels(camera, *_shaderManager);

	glDisable(GL_CULL_FACE);
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
		{
			ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
		}
		else
		{
			ImGui::Text("Mouse Position: <invalid>");
		}

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

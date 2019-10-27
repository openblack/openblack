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

#include "Gui.h"

#include <bx/math.h>
#include <bx/timer.h>
#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#include <imgui.h>
#include <imgui_widget_flamegraph.h>
#ifdef _WIN32
#include <SDL2/SDL_syswm.h>
#endif

#include <Game.h>
#include <GameWindow.h>
#include <LHVMViewer.h>
#include <MeshViewer.h>
#include <3D/Camera.h>
#include <3D/LandIsland.h>
#include <3D/MeshPack.h>
#include <3D/Sky.h>
#include <3D/Water.h>
#include <Entities/Components/Transform.h>
#include <Entities/Components/Tree.h>

#include "Entities/Registry.h"
#include "Graphics/Shaders/imgui/vs_ocornut_imgui.bin.h"
#include "Graphics/Shaders/imgui/fs_ocornut_imgui.bin.h"
#include "Graphics/Shaders/imgui/vs_imgui_image.bin.h"
#include "Graphics/Shaders/imgui/fs_imgui_image.bin.h"
#include "Profiler.h"

using namespace openblack;

#define IMGUI_FLAGS_NONE        UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

namespace
{
const bgfx::EmbeddedShader s_embeddedShaders[] =
	{
		BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
		BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
		BGFX_EMBEDDED_SHADER(vs_imgui_image),
		BGFX_EMBEDDED_SHADER(fs_imgui_image),

		BGFX_EMBEDDED_SHADER_END()
	};
}  // namespace

std::unique_ptr<Gui> Gui::create(const GameWindow &window, bgfx::ViewId viewId, float scale)
{
	IMGUI_CHECKVERSION();
	auto imgui = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsLight();

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;
	style.ScaleAllSizes(scale);
	io.FontGlobalScale = scale;

	io.BackendRendererName = "imgui_impl_bgfx";
	auto meshViewer = std::make_unique<MeshViewer>(viewId - 1);

	auto gui = std::unique_ptr<Gui>(new Gui(imgui, viewId, std::move(meshViewer)));

	if (!gui->InitSdl2(window.GetHandle()))
	{
		return nullptr;
	}

	return gui;
}

Gui::Gui(ImGuiContext* imgui, bgfx::ViewId viewId, std::unique_ptr<MeshViewer> &&meshViewer)
	: _imgui(imgui)
	, _time(0)
	, _vertexBuffer(BGFX_INVALID_HANDLE)
	, _indexBuffer(BGFX_INVALID_HANDLE)
	, _program(BGFX_INVALID_HANDLE)
	, _imageProgram(BGFX_INVALID_HANDLE)
	, _texture(BGFX_INVALID_HANDLE)
	, _s_tex(BGFX_INVALID_HANDLE)
	, _u_imageLodEnabled(BGFX_INVALID_HANDLE)
	, _mousePressed { false, false, false }
	, _mouseCursors { 0 }
	, _clipboardTextData(nullptr)
	, _last(bx::getHPCounter())
	, _lastScroll(0)
	, _viewId(viewId)
	, _meshViewer(std::move(meshViewer))
{
	CreateDeviceObjectsBgfx();
}

Gui::~Gui()
{
	ImGui::DestroyContext(_imgui);

	if (bgfx::isValid(_vertexBuffer))
		bgfx::destroy(_vertexBuffer);
	if (bgfx::isValid(_indexBuffer))
		bgfx::destroy(_indexBuffer);
	if (bgfx::isValid(_u_imageLodEnabled))
		bgfx::destroy(_u_imageLodEnabled);
	if (bgfx::isValid(_s_tex))
		bgfx::destroy(_s_tex);
	if (bgfx::isValid(_texture))
		bgfx::destroy(_texture);
	if (bgfx::isValid(_imageProgram))
		bgfx::destroy(_imageProgram);
	if (bgfx::isValid(_program))
		bgfx::destroy(_program);
}

bool Gui::ProcessEventSdl2(const SDL_Event& event)
{
	ImGui::SetCurrentContext(_imgui);

	ImGuiIO& io = ImGui::GetIO();
	switch (event.type)
	{
		case SDL_MOUSEWHEEL:
		{
			if (event.wheel.x > 0) io.MouseWheelH += 1;
			if (event.wheel.x < 0) io.MouseWheelH -= 1;
			if (event.wheel.y > 0) io.MouseWheel += 1;
			if (event.wheel.y < 0) io.MouseWheel -= 1;
			return true;
		}
		case SDL_MOUSEBUTTONDOWN:
		{
			if (event.button.button == SDL_BUTTON_LEFT) _mousePressed[0] = true;
			if (event.button.button == SDL_BUTTON_RIGHT) _mousePressed[1] = true;
			if (event.button.button == SDL_BUTTON_MIDDLE) _mousePressed[2] = true;
			return true;
		}
		case SDL_TEXTINPUT:
		{
			io.AddInputCharactersUTF8(event.text.text);
			return true;
		}
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			int key = event.key.keysym.scancode;
			IM_ASSERT(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
			io.KeysDown[key] = (event.type == SDL_KEYDOWN);
			io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
			io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
			io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
			io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
			return true;
		}
	}
	return false;
}

const char* Gui::GetClipboardText()
{
	if (_clipboardTextData)
		SDL_free(_clipboardTextData);
	_clipboardTextData = SDL_GetClipboardText();
	return _clipboardTextData;
}

void Gui::SetClipboardText(const char* text)
{
	SDL_SetClipboardText(text);
}

bool Gui::InitSdl2(SDL_Window* window)
{
	_window = window;

	// Setup back-end capabilities flags
	ImGuiIO& io = ImGui::GetIO();
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;       // We can honor GetMouseCursor() values (optional)
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;        // We can honor io.WantSetMousePos requests (optional, rarely used)
	io.BackendPlatformName = "imgui_impl_sdl";

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
	io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
	io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
	io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
	io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
	io.KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_RETURN2;
	io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
	io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
	io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
	io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
	io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
	io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;

	io.SetClipboardTextFn = StaticSetClipboardText;
	io.GetClipboardTextFn = StaticGetClipboardText;
	io.ClipboardUserData = this;

	_mouseCursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	_mouseCursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	_mouseCursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	_mouseCursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
	_mouseCursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	_mouseCursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	_mouseCursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
	_mouseCursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

#ifdef _WIN32
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	io.ImeWindowHandle = wmInfo.info.win.window;
#else
	(void)window;
#endif

	return true;
}

bool Gui::CreateFontsTextureBgfx()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

	_texture = bgfx::createTexture2D(
		static_cast<uint16_t>(width),
		static_cast<uint16_t>(height),
		false,
		1,
		bgfx::TextureFormat::BGRA8,
		0,
		bgfx::copy(pixels, width * height * 4));

	return true;
}

bool Gui::CreateDeviceObjectsBgfx()
{
	// Create shaders
	bgfx::RendererType::Enum type = bgfx::getRendererType();
	_program = bgfx::createProgram(
		bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui"),
		bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui"),
		true
	);
	_imageProgram = bgfx::createProgram(
		bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_imgui_image"),
		bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_imgui_image"),
		true
	);

	// Create buffers
	_u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);

	_layout
		.begin()
		.add(bgfx::Attrib::Position,  2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
		.end();

	_s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

	CreateFontsTextureBgfx();

	return true;
}

void Gui::UpdateMousePosAndButtons()
{
	ImGuiIO& io = ImGui::GetIO();

	// Set OS mouse position if requested (rarely used, only when ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
	if (io.WantSetMousePos)
		SDL_WarpMouseInWindow(_window, (int)io.MousePos.x, (int)io.MousePos.y);
	else
		io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

	int mx, my;
	Uint32 mouse_buttons = SDL_GetMouseState(&mx, &my);
	io.MouseDown[0] = _mousePressed[0] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;  // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
	io.MouseDown[1] = _mousePressed[1] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	io.MouseDown[2] = _mousePressed[2] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	_mousePressed[0] = _mousePressed[1] = _mousePressed[2] = false;

#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE && !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) && !(defined(__APPLE__) && TARGET_OS_IOS)
	SDL_Window* focused_window = SDL_GetKeyboardFocus();
    if (_window == focused_window)
    {
        // SDL_GetMouseState() gives mouse position seemingly based on the last window entered/focused(?)
        // The creation of a new windows at runtime and SDL_CaptureMouse both seems to severely mess up with that, so we retrieve that position globally.
        int wx, wy;
        SDL_GetWindowPosition(focused_window, &wx, &wy);
        SDL_GetGlobalMouseState(&mx, &my);
        mx -= wx;
        my -= wy;
        io.MousePos = ImVec2((float)mx, (float)my);
    }

    // SDL_CaptureMouse() let the OS know e.g. that our imgui drag outside the SDL window boundaries shouldn't e.g. trigger the OS window resize cursor.
    // The function is only supported from SDL 2.0.4 (released Jan 2016)
    bool any_mouse_button_down = ImGui::IsAnyMouseDown();
    SDL_CaptureMouse(any_mouse_button_down ? SDL_TRUE : SDL_FALSE);
#else
	if (SDL_GetWindowFlags(_window) & SDL_WINDOW_INPUT_FOCUS)
		io.MousePos = ImVec2((float)mx, (float)my);
#endif
}

void Gui::UpdateMouseCursor()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
		return;

	ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
	if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
	{
		// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
		SDL_ShowCursor(SDL_FALSE);
	}
	else
	{
		// Show OS mouse cursor
		SDL_SetCursor(_mouseCursors[imgui_cursor] ? _mouseCursors[imgui_cursor] : _mouseCursors[ImGuiMouseCursor_Arrow]);
		SDL_ShowCursor(SDL_TRUE);
	}
}

void Gui::UpdateGamepads()
{
	ImGuiIO& io = ImGui::GetIO();
	memset(io.NavInputs, 0, sizeof(io.NavInputs));
	if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
		return;

	// Get gamepad
	SDL_GameController* game_controller = SDL_GameControllerOpen(0);
	if (!game_controller)
	{
		io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
		return;
	}

	// Update gamepad inputs
#define MAP_BUTTON(NAV_NO, BUTTON_NO)       { io.NavInputs[NAV_NO] = (SDL_GameControllerGetButton(game_controller, BUTTON_NO) != 0) ? 1.0f : 0.0f; }
#define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float vn = (float)(SDL_GameControllerGetAxis(game_controller, AXIS_NO) - V0) / (float)(V1 - V0); if (vn > 1.0f) vn = 1.0f; if (vn > 0.0f && io.NavInputs[NAV_NO] < vn) io.NavInputs[NAV_NO] = vn; }
	const int thumb_dead_zone = 8000;           // SDL_gamecontroller.h suggests using this value.
	MAP_BUTTON(ImGuiNavInput_Activate,      SDL_CONTROLLER_BUTTON_A);               // Cross / A
	MAP_BUTTON(ImGuiNavInput_Cancel,        SDL_CONTROLLER_BUTTON_B);               // Circle / B
	MAP_BUTTON(ImGuiNavInput_Menu,          SDL_CONTROLLER_BUTTON_X);               // Square / X
	MAP_BUTTON(ImGuiNavInput_Input,         SDL_CONTROLLER_BUTTON_Y);               // Triangle / Y
	MAP_BUTTON(ImGuiNavInput_DpadLeft,      SDL_CONTROLLER_BUTTON_DPAD_LEFT);       // D-Pad Left
	MAP_BUTTON(ImGuiNavInput_DpadRight,     SDL_CONTROLLER_BUTTON_DPAD_RIGHT);      // D-Pad Right
	MAP_BUTTON(ImGuiNavInput_DpadUp,        SDL_CONTROLLER_BUTTON_DPAD_UP);         // D-Pad Up
	MAP_BUTTON(ImGuiNavInput_DpadDown,      SDL_CONTROLLER_BUTTON_DPAD_DOWN);       // D-Pad Down
	MAP_BUTTON(ImGuiNavInput_FocusPrev,     SDL_CONTROLLER_BUTTON_LEFTSHOULDER);    // L1 / LB
	MAP_BUTTON(ImGuiNavInput_FocusNext,     SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);   // R1 / RB
	MAP_BUTTON(ImGuiNavInput_TweakSlow,     SDL_CONTROLLER_BUTTON_LEFTSHOULDER);    // L1 / LB
	MAP_BUTTON(ImGuiNavInput_TweakFast,     SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);   // R1 / RB
	MAP_ANALOG(ImGuiNavInput_LStickLeft,    SDL_CONTROLLER_AXIS_LEFTX, -thumb_dead_zone, -32768);
	MAP_ANALOG(ImGuiNavInput_LStickRight,   SDL_CONTROLLER_AXIS_LEFTX, +thumb_dead_zone, +32767);
	MAP_ANALOG(ImGuiNavInput_LStickUp,      SDL_CONTROLLER_AXIS_LEFTY, -thumb_dead_zone, -32767);
	MAP_ANALOG(ImGuiNavInput_LStickDown,    SDL_CONTROLLER_AXIS_LEFTY, +thumb_dead_zone, +32767);

	io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
#undef MAP_BUTTON
#undef MAP_ANALOG
}

void Gui::NewFrameSdl2(SDL_Window* window)
{
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

	// Setup display size (every frame to accommodate for window resizing)
	int w, h;
	int display_w, display_h;
	SDL_GetWindowSize(window, &w, &h);
	SDL_GL_GetDrawableSize(window, &display_w, &display_h);
	io.DisplaySize = ImVec2((float)w, (float)h);
	if (w > 0 && h > 0)
		io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);

	// Setup time step (we don't use SDL_GetTicks() because it is using millisecond resolution)
	static Uint64 frequency = SDL_GetPerformanceFrequency();
	Uint64 current_time = SDL_GetPerformanceCounter();
	io.DeltaTime = _time > 0 ? (float)((double)(current_time - _time) / frequency) : (float)(1.0f / 60.0f);
	_time = current_time;

	UpdateMousePosAndButtons();
	UpdateMouseCursor();

	// Update game controllers (if enabled and available)
	UpdateGamepads();
}

void Gui::NewFrame(GameWindow& window)
{
	ImGui::SetCurrentContext(_imgui);

	NewFrameSdl2(window.GetHandle());
	ImGui::NewFrame();
}

void Gui::Loop(Game& game)
{
	_meshViewer->DrawScene();

	NewFrame(game.GetWindow());

	auto& config = game.GetConfig();

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Quit", "Esc"))
			{
				game.ScheduleQuit();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::Checkbox("Wireframe", &config.wireframe);
			ImGui::Checkbox("Water Debug", &config.waterDebug);
			ImGui::Checkbox("Bounding Boxes", &config.drawBoundingBoxes);
			ImGui::Checkbox("Profiler", &config.showProfiler);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools"))
		{
			if (ImGui::MenuItem("Dump Land Textures"))
			{
				game.GetLandIsland().DumpTextures();
			}
			if (ImGui::MenuItem("Mesh Viewer"))
			{
				_meshViewer->Open();
			}
			ImGui::EndMenu();
		}

		const auto& mousePosition = game.GetMousePosition();
		ImGui::Text("%d, %d", mousePosition.x, mousePosition.y);

		ImGui::SameLine(ImGui::GetWindowWidth() - 154.0f);
		ImGui::Text("%.2f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

		ImGui::EndMainMenuBar();
	}

	_meshViewer->DrawWindow();

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 8.0f, io.DisplaySize.y - 8.0f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
	ImGui::SetNextWindowBgAlpha(0.35f);

	static const auto cameraPositionOverlayFlags = 0u |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing |
		ImGuiWindowFlags_NoNav;
	if (ImGui::Begin("Camera position overlay", nullptr, cameraPositionOverlayFlags))
	{
		auto camPos = game.GetCamera().GetPosition();
		auto camRot = game.GetCamera().GetRotation();
		ImGui::Text("Camera Position: (%.1f,%.1f, %.1f)", camPos.x, camPos.y, camPos.z);
		ImGui::Text("Camera Rotation: (%.1f,%.1f, %.1f)", camRot.x, camRot.y, camRot.z);

		if (ImGui::IsMousePosValid())
			ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
		else
			ImGui::Text("Mouse Position: <invalid>");
	}
	ImGui::End();

	if (game.GetLhvm() != nullptr)
		LHVMViewer::Draw(game.GetLhvm());

	if (ImGui::Begin("Land Island"))
	{
		ImGui::Text("Load Land Island:");

		{
			ImGui::BeginGroup();
			if (ImGui::Button("1"))
			{
				game.LoadLandscape("./Data/Landscape/Land1.lnd");
				game.LoadMap("./Scripts/Land1.txt");
			}
			ImGui::SameLine();
			if (ImGui::Button("2"))
			{
				game.LoadLandscape("./Data/Landscape/Land2.lnd");
				game.LoadMap("./Scripts/Land2.txt");
			}
			ImGui::SameLine();
			if (ImGui::Button("3"))
			{
				game.LoadLandscape("./Data/Landscape/Land3.lnd");
				game.LoadMap("./Scripts/Land3.txt");
			}
			ImGui::SameLine();
			if (ImGui::Button("4"))
			{
				game.LoadLandscape("./Data/Landscape/Land4.lnd");
				game.LoadMap("./Scripts/Land4.txt");
			}
			ImGui::SameLine();
			if (ImGui::Button("5"))
			{
				game.LoadLandscape("./Data/Landscape/Land5.lnd");
				game.LoadMap("./Scripts/Land5.txt");
			}

			ImGui::SameLine();
			if (ImGui::Button("T"))
			{
				game.LoadLandscape("./Data/Landscape/LandT.lnd");
				game.LoadMap("./Scripts/LandT.txt");
			}

			if (ImGui::Button("2P"))
			{
				game.LoadLandscape("./Data/Landscape/Multi_Player/MPM_2P_1.lnd");
				game.LoadMap("./Scripts/Playgrounds/TwoGods.txt");
			}

			ImGui::SameLine();
			if (ImGui::Button("3P"))
			{
				game.LoadLandscape("./Data/Landscape/Multi_Player/MPM_3P_1.lnd");
				game.LoadMap("./Scripts/Playgrounds/ThreeGods.txt");
			}
			ImGui::SameLine();
			if (ImGui::Button("4P"))
			{
				game.LoadLandscape("./Data/Landscape/Multi_Player/MPM_4P_1.lnd");
				game.LoadMap("./Scripts/Playgrounds/FourGods.txt");
			}

			ImGui::EndGroup();
		}

		if (ImGui::SliderFloat("Day", &config.timeOfDay, 0.0f, 1.0f, "%.3f"))
			Game::instance()->GetSky().SetTime(config.timeOfDay);
		ImGui::SliderFloat("Bump", &config.bumpMapStrength, 0.0f, 1.0f, "%.3f");
		ImGui::SliderFloat("Small Bump", &config.smallBumpMapStrength, 0.0f, 1.0f, "%.3f");

		ImGui::Separator();

		ImGui::Text("Block Count: %zu", game.GetLandIsland().GetBlocks().size());
		ImGui::Text("Country Count: %zu", game.GetLandIsland().GetCountries().size());

		ImGui::Separator();

		if (ImGui::Button("Dump Textures"))
			game.GetLandIsland().DumpTextures();

		if (ImGui::Button("Dump Heightmap"))
			game.GetLandIsland().DumpMaps();

	}
	ImGui::End();

	if (config.showProfiler)
		ShowProfilerWindow(game);

	if (config.waterDebug)
		game.GetWater().DebugGUI();

	ImGui::Render();
}

void Gui::RenderDrawDataBgfx(ImDrawData* drawData)
{
	const ImGuiIO& io = ImGui::GetIO();
	const float width  = io.DisplaySize.x;
	const float height = io.DisplaySize.y;

	bgfx::setViewName(_viewId, "ImGui");
	bgfx::setViewMode(_viewId, bgfx::ViewMode::Sequential);

	const bgfx::Caps* caps = bgfx::getCaps();
	{
		float ortho[16];
		bx::mtxOrtho(ortho, 0.0f, width, height, 0.0f, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
		bgfx::setViewTransform(_viewId, NULL, ortho);
		bgfx::setViewRect(_viewId, 0, 0, uint16_t(width), uint16_t(height) );
	}

	// Render command lists
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;
	for (int32_t ii = 0, num = drawData->CmdListsCount; ii < num; ++ii)
	{
		const ImDrawList* drawList = drawData->CmdLists[ii];
		vertexCount += (uint32_t)drawList->VtxBuffer.size();
		indexCount += (uint32_t)drawList->IdxBuffer.size();
	}

	if (!bgfx::isValid(_vertexBuffer) || vertexCount > _vertexCount)
	{
		if (bgfx::isValid(_vertexBuffer))
		{
			bgfx::destroy(_vertexBuffer);
		}
		_vertexBuffer = bgfx::createDynamicVertexBuffer(vertexCount, _layout);
		_vertexCount = vertexCount;
	}
	if (!bgfx::isValid(_indexBuffer) || indexCount > _indexCount)
	{
		if (bgfx::isValid(_indexBuffer))
		{
			bgfx::destroy(_indexBuffer);
		}
		_indexBuffer = bgfx::createDynamicIndexBuffer(indexCount);
		_indexCount = indexCount;
	}

	uint32_t vertexBufferOffset = 0;
	uint32_t indexBufferOffset = 0;
	for (int32_t ii = 0, num = drawData->CmdListsCount; ii < num; ++ii)
	{
		const ImDrawList* drawList = drawData->CmdLists[ii];
		uint32_t numVertices = (uint32_t)drawList->VtxBuffer.size();
		uint32_t numIndices  = (uint32_t)drawList->IdxBuffer.size();

		bgfx::update(_vertexBuffer, vertexBufferOffset, bgfx::makeRef(drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert)));
		bgfx::update(_indexBuffer, indexBufferOffset, bgfx::makeRef(drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx)));

		uint32_t offset = indexBufferOffset;
		for (const ImDrawCmd* cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd)
		{
			if (cmd->UserCallback)
			{
				cmd->UserCallback(drawList, cmd);
			}
			else if (0 != cmd->ElemCount)
			{
				uint64_t state = 0u
					| BGFX_STATE_WRITE_RGB
					| BGFX_STATE_WRITE_A
					| BGFX_STATE_MSAA
				;

				bgfx::TextureHandle th = _texture;
				bgfx::ProgramHandle program = _program;

				if (cmd->TextureId != nullptr)
				{
					union { ImTextureID ptr; struct { bgfx::TextureHandle handle; uint8_t flags; uint8_t mip; } s; } texture = { cmd->TextureId };
					state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.s.flags)
							 ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
							 : BGFX_STATE_NONE
						;
					th = texture.s.handle;
					if (0 != texture.s.mip)
					{
						const float lodEnabled[4] = { float(texture.s.mip), 1.0f, 0.0f, 0.0f };
						bgfx::setUniform(_u_imageLodEnabled, lodEnabled);
						program = _imageProgram;
					}
				}
				else
				{
					state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
				}

				const uint16_t xx = uint16_t(bx::max(cmd->ClipRect.x, 0.0f) );
				const uint16_t yy = uint16_t(bx::max(cmd->ClipRect.y, 0.0f) );
				bgfx::setScissor(xx, yy
					, uint16_t(bx::min(cmd->ClipRect.z, 65535.0f)-xx)
					, uint16_t(bx::min(cmd->ClipRect.w, 65535.0f)-yy)
				);

				bgfx::setState(state);
				bgfx::setTexture(0, _s_tex, th);
				bgfx::setVertexBuffer(0, _vertexBuffer, vertexBufferOffset, numVertices);
				bgfx::setIndexBuffer(_indexBuffer, offset, cmd->ElemCount);
				bgfx::submit(_viewId, program);
			}

			offset += cmd->ElemCount;
		}
		vertexBufferOffset += numVertices;
		indexBufferOffset += numIndices;
	}
}

void Gui::Draw()
{
	ImGui::SetCurrentContext(_imgui);

	RenderDrawDataBgfx(ImGui::GetDrawData());
}

void Gui::ShowProfilerWindow(Game& game)
{
	if (ImGui::Begin("Profiler"))
	{
		const bgfx::Stats* stats = bgfx::getStats();
		const double toMsCpu     = 1000.0 / stats->cpuTimerFreq;
		const double toMsGpu     = 1000.0 / stats->gpuTimerFreq;
		const double frameMs     = double(stats->cpuTimeFrame) * toMsCpu;
		_times.pushBack(frameMs);
		_fps.pushBack(1000.0f / frameMs);

		char frameTextOverlay[256];
		std::snprintf(frameTextOverlay, sizeof(frameTextOverlay), "%.3fms, %.1f FPS", _times.back(), _fps.back());

		ImGui::Text("Submit CPU %0.3f, GPU %0.3f (Max GPU Latency: %d)", double(stats->cpuTimeEnd - stats->cpuTimeBegin) * toMsCpu, double(stats->gpuTimeEnd - stats->gpuTimeBegin) * toMsGpu, stats->maxGpuLatency);
		ImGui::Text("Wait Submit %0.3f, Wait Render %0.3f", stats->waitSubmit * toMsCpu, stats->waitRender * toMsCpu);

		ImGui::Columns(5);
		ImGui::Checkbox("Sky", &game.GetConfig().drawSky); ImGui::NextColumn();
		ImGui::Checkbox("Water", &game.GetConfig().drawWater); ImGui::NextColumn();
		ImGui::Checkbox("Island", &game.GetConfig().drawIsland); ImGui::NextColumn();
		ImGui::Checkbox("Entities", &game.GetConfig().drawEntities); ImGui::NextColumn();
		ImGui::Checkbox("Debug Cross", &game.GetConfig().drawDebugCross);
		ImGui::Columns(1);

		auto width = ImGui::GetColumnWidth() - ImGui::CalcTextSize("Frame").x;
		ImGui::PlotHistogram("Frame", _times._values, decltype(_times)::_bufferSize, _times._offset, frameTextOverlay, 0.0f, FLT_MAX, ImVec2(width, 45.0f));

		ImGui::Text("Primitives Triangles %u, Triangle Strips %u, Lines %u Line Strips %u, Points %u", stats->numPrims[0], stats->numPrims[1], stats->numPrims[2], stats->numPrims[3], stats->numPrims[4]);
		ImGui::Columns(2);
		ImGui::Text("Num Entities %u, Trees %u", static_cast<uint32_t>(game.GetEntityRegistry().Size<const Transform>()), static_cast<uint32_t>(game.GetEntityRegistry().Size<const Tree>()));
		ImGui::Text("Num Draw %u, Num Compute %u, Num Blit %u", stats->numDraw, stats->numCompute, stats->numBlit);
		ImGui::Text("Num Buffers Index %u, Vertex %u", stats->numIndexBuffers, stats->numVertexBuffers);
		ImGui::Text("Num Dynamic Buffers Index %u, Vertex %u", stats->numDynamicIndexBuffers, stats->numDynamicVertexBuffers);
		ImGui::Text("Num Transient Buffers Index %u, Vertex %u", stats->transientIbUsed, stats->transientVbUsed);
		ImGui::NextColumn();
		ImGui::Text("Num Vertex Layouts %u", stats->numVertexLayouts);
		ImGui::Text("Num Textures %u, FrameBuffers %u", stats->numTextures, stats->numFrameBuffers);
		ImGui::Text("Memory Texture %ld, RenderTarget %ld", stats->textureMemoryUsed, stats->rtMemoryUsed);
		ImGui::Text("Num Programs %u, Num Shaders %u, Uniforms %u", stats->numPrograms, stats->numShaders, stats->numUniforms);
		ImGui::Text("Num Occlusion Queries %u", stats->numOcclusionQueries);

		ImGui::Columns(1);

		auto& entry = game.GetProfiler()._entries[game.GetProfiler().GetEntryIndex(-1)];

		ImGuiWidgetFlameGraph::PlotFlame("CPU",
			[](float* startTimestamp, float* endTimestamp, ImU8* level, const char** caption, const void* data, int idx) -> void {
				auto entry = reinterpret_cast<const Profiler::Entry*>(data);
				auto& stage = entry->_stages[idx];
				if (startTimestamp)
				{
					std::chrono::duration<float, std::milli> fltStart = stage._start - entry->_frameStart;
					*startTimestamp = fltStart.count();
				}
				if (endTimestamp)
				{
					*endTimestamp = stage._end.time_since_epoch().count() / 1e6f;

					std::chrono::duration<float, std::milli> fltEnd = stage._end - entry->_frameStart;
					*endTimestamp = fltEnd.count();
				}
				if (level)
				{
					*level = stage._level;
				}
				if (caption)
				{
					*caption = Profiler::stageNames[idx].data();
				}
			}, &entry, static_cast<uint8_t>(Profiler::Stage::_count), 0, "Main Thread", 0, FLT_MAX, ImVec2(width, 0));

		ImGuiWidgetFlameGraph::PlotFlame("GPU",
			[](float* startTimestamp, float* endTimestamp, ImU8* level, const char** caption, const void* data, int idx) -> void {
				auto stats = reinterpret_cast<const bgfx::Stats*>(data);
				if (startTimestamp)
				{
					*startTimestamp = 1000.0f * (stats->viewStats[idx].gpuTimeBegin - stats->gpuTimeBegin) / (double)stats->gpuTimerFreq;
				}
				if (endTimestamp)
				{
					*endTimestamp = 1000.0f * (stats->viewStats[idx].gpuTimeEnd - stats->gpuTimeBegin) / (double)stats->gpuTimerFreq;
				}
				if (level)
				{
					*level = 0;
				}
				if (caption)
				{
					*caption = stats->viewStats[idx].name;
				}
			}, stats, stats->numViews, 0, "GPU Frame",
            0, 1000.0f * (stats->gpuTimeEnd - stats->gpuTimeBegin) / (double)stats->gpuTimerFreq, ImVec2(width, 0));

		ImGui::Columns(2);
		if (ImGui::CollapsingHeader("Details (CPU)", ImGuiTreeNodeFlags_DefaultOpen))
		{
			std::chrono::duration<float, std::milli> frameDuration = entry._frameEnd - entry._frameStart;
			ImGui::Text("Full Frame: %0.3f", frameDuration.count());
			auto cursorX = ImGui::GetCursorPosX();
			auto indentSize = ImGui::CalcTextSize("    ").x;

			for (uint8_t i = 0; i < static_cast<uint8_t>(Profiler::Stage::_count); ++i)
			{
				std::chrono::duration<float, std::milli> duration = entry._stages[i]._end - entry._stages[i]._start;
				ImGui::SetCursorPosX(cursorX + indentSize * entry._stages[i]._level);
				ImGui::Text("    %s: %0.3f", Profiler::stageNames[i].data(), duration.count());
				if (entry._stages[i]._level == 0)
					frameDuration -= duration;
			}
			ImGui::Text("    Unaccounted: %0.3f", frameDuration.count());
		}
		ImGui::NextColumn();
		if (ImGui::CollapsingHeader("Details (GPU)", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto frameDuration = stats->gpuTimeEnd - stats->gpuTimeBegin;
			ImGui::Text("Full Frame: %0.3f", 1000.0f * frameDuration / (double)stats->gpuTimerFreq);

			for (uint8_t i = 0; i < stats->numViews; ++i)
			{
				ImGui::Text("    %s: %0.3f", stats->viewStats[i].name, 1000.0f * stats->viewStats[i].gpuTimeElapsed  / (double)stats->gpuTimerFreq);
				frameDuration -= stats->viewStats[i].gpuTimeElapsed;
			}
			ImGui::Text("    Unaccounted: %0.3f", 1000.0f * frameDuration / (double)stats->gpuTimerFreq);
		}
		ImGui::Columns(1);
	}
	ImGui::End();
}

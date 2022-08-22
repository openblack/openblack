/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Gui.h"

#include <cinttypes>
#include <cmath>

#include <bgfx/embedded_shader.h>
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <bx/math.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#include <SDL.h>
#include <bx/timer.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <imgui_user.h>
#ifdef _WIN32
#include <SDL2/SDL_syswm.h>
#endif

#include <3D/Camera.h>
#include <3D/Sky.h>
#include <Common/FileSystem.h>
#include <ECS/Components/LivingAction.h>
#include <ECS/Components/Transform.h>
#include <ECS/Components/Villager.h>
#include <ECS/Registry.h>
#include <Game.h>
#include <GameWindow.h>
#include <Locator.h>
#include <Resources/Resources.h>

#include "Camera.h"
#include "Console.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "LHVMViewer.h"
#include "LandIsland.h"
#include "MeshViewer.h"
#include "PathFinding.h"
#include "Profiler.h"
#include "Temple.h"
#include "TextureViewer.h"

// Turn off formatting because it adds spaces which break the stringifying
// clang-format off
// NOLINTNEXTLINE(bugprone-macro-parentheses)
#define IMGUI_SHADER_DIR imgui/
// clang-format on
#define SHADER_NAME vs_imgui_image
#define SHADER_DIR IMGUI_SHADER_DIR
#include "Graphics/ShaderIncluder.h"
#define SHADER_DIR IMGUI_SHADER_DIR
#define SHADER_NAME fs_imgui_image
#include "Graphics/ShaderIncluder.h"

#define SHADER_NAME vs_ocornut_imgui
#define SHADER_DIR IMGUI_SHADER_DIR
#include "Graphics/ShaderIncluder.h"
#define SHADER_NAME fs_ocornut_imgui
#define SHADER_DIR IMGUI_SHADER_DIR
#include "Graphics/ShaderIncluder.h"

using namespace openblack;
using namespace openblack::debug::gui;

namespace
{
const std::array<bgfx::EmbeddedShader, 5> k_EmbeddedShaders = {{
    BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(fs_ocornut_imgui),
    BGFX_EMBEDDED_SHADER(vs_imgui_image),
    BGFX_EMBEDDED_SHADER(fs_imgui_image),

    BGFX_EMBEDDED_SHADER_END(),
}};
} // namespace

// https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_sdl.cpp
static ImGuiKey GetKeycode(int keycode)
{
	switch (keycode)
	{
	case SDLK_TAB:
		return ImGuiKey_Tab;
	case SDLK_LEFT:
		return ImGuiKey_LeftArrow;
	case SDLK_RIGHT:
		return ImGuiKey_RightArrow;
	case SDLK_UP:
		return ImGuiKey_UpArrow;
	case SDLK_DOWN:
		return ImGuiKey_DownArrow;
	case SDLK_PAGEUP:
		return ImGuiKey_PageUp;
	case SDLK_PAGEDOWN:
		return ImGuiKey_PageDown;
	case SDLK_HOME:
		return ImGuiKey_Home;
	case SDLK_END:
		return ImGuiKey_End;
	case SDLK_INSERT:
		return ImGuiKey_Insert;
	case SDLK_DELETE:
		return ImGuiKey_Delete;
	case SDLK_BACKSPACE:
		return ImGuiKey_Backspace;
	case SDLK_SPACE:
		return ImGuiKey_Space;
	case SDLK_RETURN:
		return ImGuiKey_Enter;
	case SDLK_ESCAPE:
		return ImGuiKey_Escape;
	case SDLK_QUOTE:
		return ImGuiKey_Apostrophe;
	case SDLK_COMMA:
		return ImGuiKey_Comma;
	case SDLK_MINUS:
		return ImGuiKey_Minus;
	case SDLK_PERIOD:
		return ImGuiKey_Period;
	case SDLK_SLASH:
		return ImGuiKey_Slash;
	case SDLK_SEMICOLON:
		return ImGuiKey_Semicolon;
	case SDLK_EQUALS:
		return ImGuiKey_Equal;
	case SDLK_LEFTBRACKET:
		return ImGuiKey_LeftBracket;
	case SDLK_BACKSLASH:
		return ImGuiKey_Backslash;
	case SDLK_RIGHTBRACKET:
		return ImGuiKey_RightBracket;
	case SDLK_BACKQUOTE:
		return ImGuiKey_GraveAccent;
	case SDLK_CAPSLOCK:
		return ImGuiKey_CapsLock;
	case SDLK_SCROLLLOCK:
		return ImGuiKey_ScrollLock;
	case SDLK_NUMLOCKCLEAR:
		return ImGuiKey_NumLock;
	case SDLK_PRINTSCREEN:
		return ImGuiKey_PrintScreen;
	case SDLK_PAUSE:
		return ImGuiKey_Pause;
	case SDLK_KP_0:
		return ImGuiKey_Keypad0;
	case SDLK_KP_1:
		return ImGuiKey_Keypad1;
	case SDLK_KP_2:
		return ImGuiKey_Keypad2;
	case SDLK_KP_3:
		return ImGuiKey_Keypad3;
	case SDLK_KP_4:
		return ImGuiKey_Keypad4;
	case SDLK_KP_5:
		return ImGuiKey_Keypad5;
	case SDLK_KP_6:
		return ImGuiKey_Keypad6;
	case SDLK_KP_7:
		return ImGuiKey_Keypad7;
	case SDLK_KP_8:
		return ImGuiKey_Keypad8;
	case SDLK_KP_9:
		return ImGuiKey_Keypad9;
	case SDLK_KP_PERIOD:
		return ImGuiKey_KeypadDecimal;
	case SDLK_KP_DIVIDE:
		return ImGuiKey_KeypadDivide;
	case SDLK_KP_MULTIPLY:
		return ImGuiKey_KeypadMultiply;
	case SDLK_KP_MINUS:
		return ImGuiKey_KeypadSubtract;
	case SDLK_KP_PLUS:
		return ImGuiKey_KeypadAdd;
	case SDLK_KP_ENTER:
		return ImGuiKey_KeypadEnter;
	case SDLK_KP_EQUALS:
		return ImGuiKey_KeypadEqual;
	case SDLK_LCTRL:
		return ImGuiKey_LeftCtrl;
	case SDLK_LSHIFT:
		return ImGuiKey_LeftShift;
	case SDLK_LALT:
		return ImGuiKey_LeftAlt;
	case SDLK_LGUI:
		return ImGuiKey_LeftSuper;
	case SDLK_RCTRL:
		return ImGuiKey_RightCtrl;
	case SDLK_RSHIFT:
		return ImGuiKey_RightShift;
	case SDLK_RALT:
		return ImGuiKey_RightAlt;
	case SDLK_RGUI:
		return ImGuiKey_RightSuper;
	case SDLK_APPLICATION:
		return ImGuiKey_Menu;
	case SDLK_0:
		return ImGuiKey_0;
	case SDLK_1:
		return ImGuiKey_1;
	case SDLK_2:
		return ImGuiKey_2;
	case SDLK_3:
		return ImGuiKey_3;
	case SDLK_4:
		return ImGuiKey_4;
	case SDLK_5:
		return ImGuiKey_5;
	case SDLK_6:
		return ImGuiKey_6;
	case SDLK_7:
		return ImGuiKey_7;
	case SDLK_8:
		return ImGuiKey_8;
	case SDLK_9:
		return ImGuiKey_9;
	case SDLK_a:
		return ImGuiKey_A;
	case SDLK_b:
		return ImGuiKey_B;
	case SDLK_c:
		return ImGuiKey_C;
	case SDLK_d:
		return ImGuiKey_D;
	case SDLK_e:
		return ImGuiKey_E;
	case SDLK_f:
		return ImGuiKey_F;
	case SDLK_g:
		return ImGuiKey_G;
	case SDLK_h:
		return ImGuiKey_H;
	case SDLK_i:
		return ImGuiKey_I;
	case SDLK_j:
		return ImGuiKey_J;
	case SDLK_k:
		return ImGuiKey_K;
	case SDLK_l:
		return ImGuiKey_L;
	case SDLK_m:
		return ImGuiKey_M;
	case SDLK_n:
		return ImGuiKey_N;
	case SDLK_o:
		return ImGuiKey_O;
	case SDLK_p:
		return ImGuiKey_P;
	case SDLK_q:
		return ImGuiKey_Q;
	case SDLK_r:
		return ImGuiKey_R;
	case SDLK_s:
		return ImGuiKey_S;
	case SDLK_t:
		return ImGuiKey_T;
	case SDLK_u:
		return ImGuiKey_U;
	case SDLK_v:
		return ImGuiKey_V;
	case SDLK_w:
		return ImGuiKey_W;
	case SDLK_x:
		return ImGuiKey_X;
	case SDLK_y:
		return ImGuiKey_Y;
	case SDLK_z:
		return ImGuiKey_Z;
	case SDLK_F1:
		return ImGuiKey_F1;
	case SDLK_F2:
		return ImGuiKey_F2;
	case SDLK_F3:
		return ImGuiKey_F3;
	case SDLK_F4:
		return ImGuiKey_F4;
	case SDLK_F5:
		return ImGuiKey_F5;
	case SDLK_F6:
		return ImGuiKey_F6;
	case SDLK_F7:
		return ImGuiKey_F7;
	case SDLK_F8:
		return ImGuiKey_F8;
	case SDLK_F9:
		return ImGuiKey_F9;
	case SDLK_F10:
		return ImGuiKey_F10;
	case SDLK_F11:
		return ImGuiKey_F11;
	case SDLK_F12:
		return ImGuiKey_F12;
	}
	return ImGuiKey_None;
}

std::unique_ptr<Gui> Gui::Create(const GameWindow* window, graphics::RenderPass viewId, float scale)
{
	IMGUI_CHECKVERSION();
	auto* imgui = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(scale);
	io.FontGlobalScale = scale;
	io.BackendRendererName = "imgui_impl_bgfx";

	std::vector<std::unique_ptr<Window>> debugWindows;
	debugWindows.emplace_back(new Profiler);
	debugWindows.emplace_back(new MeshViewer);
	debugWindows.emplace_back(new TextureViewer);
	debugWindows.emplace_back(new Console);
	debugWindows.emplace_back(new LandIsland);
	debugWindows.emplace_back(new LHVMViewer);
	debugWindows.emplace_back(new PathFinding);
	debugWindows.emplace_back(new TempleInterior);
	debugWindows.emplace_back(new Camera);

	auto gui = std::unique_ptr<Gui>(new Gui(imgui, static_cast<bgfx::ViewId>(viewId), std::move(debugWindows)));

	if (window != nullptr)
	{
		if (!gui->InitSdl2(window->GetHandle()))
		{
			return nullptr;
		}
	}

	return gui;
}

Gui::Gui(ImGuiContext* imgui, bgfx::ViewId viewId, std::vector<std::unique_ptr<Window>>&& debugWindows)
    : _imgui(imgui)
    , _vertexBuffer(BGFX_INVALID_HANDLE)
    , _indexBuffer(BGFX_INVALID_HANDLE)
    , _program(BGFX_INVALID_HANDLE)
    , _imageProgram(BGFX_INVALID_HANDLE)
    , _texture(BGFX_INVALID_HANDLE)
    , _s_tex(BGFX_INVALID_HANDLE)
    , _u_imageLodEnabled(BGFX_INVALID_HANDLE)
    , _mousePressed {false, false, false}
    , _mouseCursors {0}
    , _last(bx::getHPCounter())
    , _viewId(viewId)
    , _debugWindows(std::move(debugWindows))
{
	CreateDeviceObjectsBgfx();
}

Gui::~Gui()
{
	ImGui::DestroyContext(_imgui);

	if (bgfx::isValid(_vertexBuffer))
	{
		bgfx::destroy(_vertexBuffer);
	}
	if (bgfx::isValid(_indexBuffer))
	{
		bgfx::destroy(_indexBuffer);
	}
	if (bgfx::isValid(_u_imageLodEnabled))
	{
		bgfx::destroy(_u_imageLodEnabled);
	}
	if (bgfx::isValid(_s_tex))
	{
		bgfx::destroy(_s_tex);
	}
	if (bgfx::isValid(_texture))
	{
		bgfx::destroy(_texture);
	}
	if (bgfx::isValid(_imageProgram))
	{
		bgfx::destroy(_imageProgram);
	}
	if (bgfx::isValid(_program))
	{
		bgfx::destroy(_program);
	}
}

bool Gui::ProcessEventSdl2(const SDL_Event& event)
{
	ImGui::SetCurrentContext(_imgui);

	for (auto& window : _debugWindows)
	{
		window->WindowProcessEvent(event);
	}

	ImGuiIO& io = ImGui::GetIO();
	switch (event.type)
	{
	case SDL_QUIT:
		return false;
	case SDL_MOUSEWHEEL:
	{
		if (event.wheel.x > 0)
		{
			io.MouseWheelH += 1;
		}
		if (event.wheel.x < 0)
		{
			io.MouseWheelH -= 1;
		}
		if (event.wheel.y > 0)
		{
			io.MouseWheel += 1;
		}
		if (event.wheel.y < 0)
		{
			io.MouseWheel -= 1;
		}
		return io.WantCaptureMouse;
	}
	case SDL_MOUSEBUTTONDOWN:
	{
		if (event.button.button == SDL_BUTTON_LEFT)
		{
			_mousePressed[0] = true;
		}
		if (event.button.button == SDL_BUTTON_RIGHT)
		{
			_mousePressed[1] = true;
		}
		if (event.button.button == SDL_BUTTON_MIDDLE)
		{
			_mousePressed[2] = true;
		}
		return io.WantCaptureMouse;
	}
	case SDL_TEXTINPUT:
	{
		io.AddInputCharactersUTF8(event.text.text);
		return io.WantTextInput;
	}
	case SDL_KEYDOWN:
	case SDL_KEYUP:
	{
		int key = event.key.keysym.scancode;
		IM_ASSERT(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
		int guiKey = GetKeycode(key);
		io.AddKeyEvent(guiKey, event.type == SDL_KEYDOWN);
		io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
		io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
		io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
		io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
		return io.WantCaptureKeyboard;
	}
	case SDL_WINDOWEVENT:
		if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
		{
			return false;
		}
		break;
	}
	return io.WantCaptureMouse;
}

const char* Gui::GetClipboardText()
{
	if (_clipboardTextData != nullptr)
	{
		SDL_free(_clipboardTextData);
	}
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
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor()
	                                                      // values (optional)
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;  // We can honor io.WantSetMousePos
	                                                      // requests (optional, rarely used)
	io.BackendPlatformName = "imgui_impl_sdl";
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
	int width;
	int height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width,
	                             &height); // Load as RGBA 32-bits (75% of the memory is wasted, but
	                                       // default font is so small) because it is more likely to
	                                       // be compatible with user's existing shaders. If your
	                                       // ImTextureId represent a higher-level concept than just
	                                       // a GL texture id, consider calling GetTexDataAsAlpha8()
	                                       // instead to save on GPU memory.

	_texture = bgfx::createTexture2D(static_cast<uint16_t>(width), static_cast<uint16_t>(height), false, 1,
	                                 bgfx::TextureFormat::BGRA8, 0, bgfx::copy(pixels, width * height * 4));

	return true;
}

bool Gui::CreateDeviceObjectsBgfx()
{
	// Create shaders
	bgfx::RendererType::Enum type = bgfx::getRendererType();
	_program = bgfx::createProgram(bgfx::createEmbeddedShader(k_EmbeddedShaders.data(), type, "vs_ocornut_imgui"),
	                               bgfx::createEmbeddedShader(k_EmbeddedShaders.data(), type, "fs_ocornut_imgui"), true);
	_imageProgram = bgfx::createProgram(bgfx::createEmbeddedShader(k_EmbeddedShaders.data(), type, "vs_imgui_image"),
	                                    bgfx::createEmbeddedShader(k_EmbeddedShaders.data(), type, "fs_imgui_image"), true);

	// Create buffers
	_u_imageLodEnabled = bgfx::createUniform("u_imageLodEnabled", bgfx::UniformType::Vec4);

	_layout.begin()
	    .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();

	_s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

	CreateFontsTextureBgfx();

	return true;
}

void Gui::UpdateMousePosAndButtons()
{
	ImGuiIO& io = ImGui::GetIO();

	// Set OS mouse position if requested (rarely used, only when
	// ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
	if (io.WantSetMousePos)
	{
		SDL_WarpMouseInWindow(_window, static_cast<int>(io.MousePos.x), static_cast<int>(io.MousePos.y));
	}
	else
	{
		io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	}

	int mx;
	int my;
	Uint32 mouseButtons = SDL_GetMouseState(&mx, &my);
	io.MouseDown[0] = _mousePressed[0] ||
	                  (mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0; // If a mouse press event came, always pass it as
	                                                                     // "mouse held this frame", so we don't miss
	                                                                     // click-release events that are shorter than 1 frame.
	io.MouseDown[1] = _mousePressed[1] || (mouseButtons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	io.MouseDown[2] = _mousePressed[2] || (mouseButtons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	_mousePressed[0] = _mousePressed[1] = _mousePressed[2] = false;

#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE && !defined(__EMSCRIPTEN__) && !defined(__ANDROID__) && \
    !(defined(__APPLE__) && TARGET_OS_IOS)
	SDL_Window* focused_window = SDL_GetKeyboardFocus();
	if (_window == focused_window)
	{
		// SDL_GetMouseState() gives mouse position seemingly based on the last
		// window entered/focused(?) The creation of a new windows at runtime
		// and SDL_CaptureMouse both seems to severely mess up with that, so we
		// retrieve that position globally.
		int wx, wy;
		SDL_GetWindowPosition(focused_window, &wx, &wy);
		SDL_GetGlobalMouseState(&mx, &my);
		mx -= wx;
		my -= wy;
		io.MousePos = ImVec2((float)mx, (float)my);
	}

	// SDL_CaptureMouse() let the OS know e.g. that our imgui drag outside the
	// SDL window boundaries shouldn't e.g. trigger the OS window resize cursor.
	// The function is only supported from SDL 2.0.4 (released Jan 2016)
	bool any_mouse_button_down = ImGui::IsAnyMouseDown();
	SDL_CaptureMouse(any_mouse_button_down ? SDL_TRUE : SDL_FALSE);
#else
	if ((SDL_GetWindowFlags(_window) & SDL_WINDOW_INPUT_FOCUS) != 0)
	{
		io.MousePos = ImVec2(static_cast<float>(mx), static_cast<float>(my));
	}
#endif
}

void Gui::UpdateMouseCursor()
{
	ImGuiIO& io = ImGui::GetIO();
	if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) != 0)
	{
		return;
	}

	ImGuiMouseCursor imguiCursor = ImGui::GetMouseCursor();
	if (io.MouseDrawCursor || imguiCursor == ImGuiMouseCursor_None)
	{
		// Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
		SDL_ShowCursor(SDL_FALSE);
	}
	else
	{
		// Show OS mouse cursor
		auto* const cursor = _mouseCursors.at(imguiCursor);
		SDL_SetCursor(cursor != nullptr ? cursor : _mouseCursors[ImGuiMouseCursor_Arrow]);
		SDL_ShowCursor(SDL_TRUE);
	}
}

void Gui::UpdateGamepads()
{
	ImGuiIO& io = ImGui::GetIO();
	memset(io.NavInputs, 0, sizeof(io.NavInputs));
	if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
	{
		return;
	}

	// Get gamepad
	SDL_GameController* game_controller = SDL_GameControllerOpen(0);
	if (game_controller == nullptr)
	{
		io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
		return;
	}

	// Update gamepad inputs
#define MAP_BUTTON(NAV_NO, BUTTON_NO)                                                                            \
	{                                                                                                            \
		io.NavInputs[(NAV_NO)] = (SDL_GameControllerGetButton(game_controller, (BUTTON_NO)) != 0) ? 1.0f : 0.0f; \
	}
#define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1)                                                                      \
	{                                                                                                            \
		float vn = (float)(SDL_GameControllerGetAxis(game_controller, (AXIS_NO)) - (V0)) / (float)((V1) - (V0)); \
		if (vn > 1.0f)                                                                                           \
			vn = 1.0f;                                                                                           \
		if (vn > 0.0f && io.NavInputs[(NAV_NO)] < vn)                                                            \
			io.NavInputs[(NAV_NO)] = vn;                                                                         \
	}
	const int thumbDeadZone = 8000;                              // SDL_gamecontroller.h suggests using this value.
	MAP_BUTTON(ImGuiNavInput_Activate, SDL_CONTROLLER_BUTTON_A); // Cross / A
	MAP_BUTTON(ImGuiNavInput_Cancel, SDL_CONTROLLER_BUTTON_B);   // Circle / B
	MAP_BUTTON(ImGuiNavInput_Menu, SDL_CONTROLLER_BUTTON_X);     // Square / X
	MAP_BUTTON(ImGuiNavInput_Input, SDL_CONTROLLER_BUTTON_Y);    // Triangle / Y
	MAP_BUTTON(ImGuiNavInput_DpadLeft,
	           SDL_CONTROLLER_BUTTON_DPAD_LEFT); // D-Pad Left
	MAP_BUTTON(ImGuiNavInput_DpadRight,
	           SDL_CONTROLLER_BUTTON_DPAD_RIGHT);                    // D-Pad Right
	MAP_BUTTON(ImGuiNavInput_DpadUp, SDL_CONTROLLER_BUTTON_DPAD_UP); // D-Pad Up
	MAP_BUTTON(ImGuiNavInput_DpadDown,
	           SDL_CONTROLLER_BUTTON_DPAD_DOWN); // D-Pad Down
	MAP_BUTTON(ImGuiNavInput_FocusPrev,
	           SDL_CONTROLLER_BUTTON_LEFTSHOULDER); // L1 / LB
	MAP_BUTTON(ImGuiNavInput_FocusNext,
	           SDL_CONTROLLER_BUTTON_RIGHTSHOULDER); // R1 / RB
	MAP_BUTTON(ImGuiNavInput_TweakSlow,
	           SDL_CONTROLLER_BUTTON_LEFTSHOULDER); // L1 / LB
	MAP_BUTTON(ImGuiNavInput_TweakFast,
	           SDL_CONTROLLER_BUTTON_RIGHTSHOULDER); // R1 / RB
	MAP_ANALOG(ImGuiNavInput_LStickLeft, SDL_CONTROLLER_AXIS_LEFTX, -thumbDeadZone, -32768);
	MAP_ANALOG(ImGuiNavInput_LStickRight, SDL_CONTROLLER_AXIS_LEFTX, +thumbDeadZone, +32767);
	MAP_ANALOG(ImGuiNavInput_LStickUp, SDL_CONTROLLER_AXIS_LEFTY, -thumbDeadZone, -32767);
	MAP_ANALOG(ImGuiNavInput_LStickDown, SDL_CONTROLLER_AXIS_LEFTY, +thumbDeadZone, +32767);

	io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
#undef MAP_BUTTON
#undef MAP_ANALOG
}

void Gui::NewFrameSdl2(SDL_Window* window)
{
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer "
	                                 "back-end. Missing call to renderer _NewFrame() function? e.g. "
	                                 "ImGui_ImplOpenGL3_NewFrame().");

	// Setup display size (every frame to accommodate for window resizing)
	if (window != nullptr)
	{
		int w;
		int h;
		int displayW;
		int displayH;
		SDL_GetWindowSize(window, &w, &h);
		SDL_GL_GetDrawableSize(window, &displayW, &displayH);
		io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));
		if (w > 0 && h > 0)
		{
			io.DisplayFramebufferScale = ImVec2(static_cast<float>(displayW) / w, static_cast<float>(displayH) / h);
		}
	}
	else
	{
		io.DisplaySize = ImVec2(1.0f, 1.0f);
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	}

	// Setup time step (we don't use SDL_GetTicks() because it is using
	// millisecond resolution)
	static Uint64 frequency = SDL_GetPerformanceFrequency();
	Uint64 currentTime = SDL_GetPerformanceCounter();
	io.DeltaTime =
	    _time > 0 ? static_cast<float>(static_cast<double>(currentTime - _time) / frequency) : static_cast<float>(1.0f / 60.0f);
	_time = currentTime;

	UpdateMousePosAndButtons();
	UpdateMouseCursor();

	// Update game controllers (if enabled and available)
	UpdateGamepads();
}

void Gui::NewFrame(GameWindow* window)
{
	ImGui::SetCurrentContext(_imgui);

	NewFrameSdl2(window != nullptr ? window->GetHandle() : nullptr);
	ImGui::NewFrame();
}

bool Gui::Loop(Game& game, const Renderer& renderer)
{
	for (auto& window : _debugWindows)
	{
		window->WindowUpdate(game, renderer);
	}
	NewFrame(game.GetWindow());
	if (ShowMenu(game))
	{
		// Exit option selected
		return true;
	}
	for (auto& window : _debugWindows)
	{
		window->WindowDraw(game);
	}
	ShowVillagerNames(game);
	ShowCameraPositionOverlay(game);

	ImGui::Render();

	return false;
}

void Gui::RenderDrawDataBgfx(ImDrawData* drawData)
{
	const ImGuiIO& io = ImGui::GetIO();
	const float width = io.DisplaySize.x;
	const float height = io.DisplaySize.y;

	bgfx::setViewMode(_viewId, bgfx::ViewMode::Sequential);

	const bgfx::Caps* caps = bgfx::getCaps();
	{
		glm::mat4 ortho;
		// NOLINTNEXTLINE(readability-suspicious-call-argument): width -> _right, height -> bottom is correct
		bx::mtxOrtho(glm::value_ptr(ortho), 0.0f, width, height, 0.0f, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
		bgfx::setViewTransform(_viewId, nullptr, glm::value_ptr(ortho));
		bgfx::setViewRect(_viewId, 0, 0, uint16_t(width), uint16_t(height));
	}

	// Render command lists
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;
	for (int32_t ii = 0, num = drawData->CmdListsCount; ii < num; ++ii)
	{
		const ImDrawList* drawList = drawData->CmdLists[ii];
		vertexCount += static_cast<uint32_t>(drawList->VtxBuffer.size());
		indexCount += static_cast<uint32_t>(drawList->IdxBuffer.size());
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
		auto numVertices = static_cast<uint32_t>(drawList->VtxBuffer.size());
		auto numIndices = static_cast<uint32_t>(drawList->IdxBuffer.size());

		bgfx::update(_vertexBuffer, vertexBufferOffset,
		             bgfx::makeRef(drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert)));
		bgfx::update(_indexBuffer, indexBufferOffset,
		             bgfx::makeRef(drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx)));

		uint32_t offset = indexBufferOffset;
		for (const ImDrawCmd *cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd)
		{
			if (cmd->UserCallback != nullptr)
			{
				cmd->UserCallback(drawList, cmd);
			}
			else if (0 != cmd->ElemCount)
			{
				uint64_t state = 0u | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;

				bgfx::TextureHandle th = _texture;
				bgfx::ProgramHandle program = _program;

				if (cmd->TextureId != nullptr)
				{
					union
					{
						ImTextureID ptr;
						struct
						{
							bgfx::TextureHandle handle;
							uint8_t flags;
							uint8_t mip;
						} s;
					} texture = {cmd->TextureId};
					state |= 0 != (IMGUI_FLAGS_ALPHA_BLEND & texture.s.flags)
					             ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA)
					             : BGFX_STATE_NONE;
					th = texture.s.handle;
					if (0 != texture.s.mip)
					{
						const glm::vec4 lodEnabled = {static_cast<float>(texture.s.mip), 1.0f, 0.0f, 0.0f};
						bgfx::setUniform(_u_imageLodEnabled, glm::value_ptr(lodEnabled));
						program = _imageProgram;
					}
				}
				else
				{
					state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
				}

				const uint16_t xx = uint16_t(bx::max(cmd->ClipRect.x, 0.0f));
				const uint16_t yy = uint16_t(bx::max(cmd->ClipRect.y, 0.0f));
				bgfx::setScissor(xx, yy, uint16_t(bx::min(cmd->ClipRect.z, 65535.0f) - xx),
				                 uint16_t(bx::min(cmd->ClipRect.w, 65535.0f) - yy));

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

bool Gui::ShowMenu(Game& game)
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("Load Island"))
		{
			auto& resources = Locator::resources::value();
			auto& levelsManager = resources.GetLevels();
			auto campaigns = std::vector<const Level*>();
			auto playgrounds = std::vector<const Level*>();
			levelsManager.Each([&campaigns, &playgrounds](entt::id_type /*id*/, const Level& level) {
				if (level.IsCampaign())
				{
					campaigns.emplace_back(&level);
				}
				else
				{
					playgrounds.emplace_back(&level);
				}
			});

			auto menuItem = [&game](const auto& label, const std::filesystem::path& path) {
				if (ImGui::MenuItem(label.data()))
				{
					game.LoadMap(path);
				}
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%s", path.generic_string().c_str());
				}
			};

			ImGui::MenuItem("Story Islands", nullptr, false, false);
			for (auto& level : campaigns)
			{
				menuItem(level->GetName(), level->GetScriptPath());
			}
			ImGui::Separator();
			ImGui::MenuItem("Playground Islands", nullptr, false, false);
			for (auto& level : playgrounds)
			{
				menuItem(level->GetName(), level->GetScriptPath());
			}

			ImGui::EndMenu();
		}

		auto& config = game.GetConfig();

		if (ImGui::BeginMenu("World"))
		{
			if (ImGui::SliderFloat("Time of Day", &config.timeOfDay, 0.0f, 24.0f, "%.3f"))
			{
				Game::Instance()->SetTime(fmodf(config.timeOfDay, 24.0f));
			}

			ImGui::Text("Sky Type Index %f", game.GetSky().GetCurrentSkyType());
			ImGui::SliderFloat("Sky alignment", &config.skyAlignment, -1.0f, 1.0f, "%.3f");

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Debug"))
		{
			if (ImGui::BeginMenu("Windows"))
			{
				for (auto& window : _debugWindows)
				{
					if (ImGui::MenuItem(window->GetName().c_str()))
					{
						window->Open();
					}
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Villager Names"))
			{
				ImGui::Checkbox("Show", &config.showVillagerNames);
				ImGui::Checkbox("Show States", &config.debugVillagerStates);
				ImGui::Checkbox("Debug", &config.debugVillagerNames);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				ImGui::Checkbox("Sky", &config.drawSky);
				ImGui::Checkbox("Water", &config.drawWater);
				ImGui::Checkbox("Island", &config.drawIsland);
				ImGui::Checkbox("Entities", &config.drawEntities);
				ImGui::Checkbox("Sprites", &config.drawSprites);
				ImGui::Checkbox("TestModel", &config.drawTestModel);
				ImGui::Checkbox("Debug Cross", &config.drawDebugCross);
				ImGui::Checkbox("Wireframe", &config.wireframe);
				ImGui::Checkbox("Bounding Boxes", &config.drawBoundingBoxes);
				ImGui::Checkbox("Footpaths", &config.drawFootpaths);
				ImGui::Checkbox("Streams", &config.drawStreams);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Game Speed"))
			{
				float multiplier = game.GetGameSpeed();
				ImGui::Text("Scaled game duration: %.3fms (%.3f Hz)", multiplier * Game::k_TurnDuration.count(),
				            1000.0f / (multiplier * Game::k_TurnDuration.count()));
				if (ImGui::MenuItem("Slow"))
				{
					game.SetGameSpeed(Game::k_TurnDurationMultiplierSlow);
				}
				if (ImGui::MenuItem("Normal"))
				{
					game.SetGameSpeed(Game::k_TurnDurationMultiplierNormal);
				}
				if (ImGui::MenuItem("Fast"))
				{
					game.SetGameSpeed(Game::k_TurnDurationMultiplierFast);
				}
				if (ImGui::SliderFloat("Multiplier", &multiplier, 1.0f / 10.0f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic))
				{
					game.SetGameSpeed(multiplier);
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Capture"))
		{
			if (ImGui::Button("Capture"))
			{
				game.RequestScreenshot(_screenshotFilename);
			}
			ImGui::SameLine();
			ImGui::InputText("Screenshot", &_screenshotFilename);
			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("Quit", "Esc"))
		{
			return true;
		}

		ImGui::SameLine(ImGui::GetWindowWidth() - 154.0f);
		ImGui::Text("%.2f FPS (%.2f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

		_menuBarSize = ImGui::GetWindowSize();

		ImGui::EndMainMenuBar();
	}
	return false;
}

void Gui::RenderArrow(const std::string& name, const ImVec2& pos, const ImVec2& size) const
{
	// clang-format off
	static const auto boxOverlayFlags =
		0u
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_NoInputs
	;
	// clang-format on
	ImGui::SetNextWindowPos(ImVec2(pos.x + size.x / 2, pos.y + size.y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::SetNextWindowSize(size);
	if (ImGui::Begin((name + " Frame").c_str(), nullptr, boxOverlayFlags))
	{
		std::string strId = name + " Arrow";
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (!window->SkipItems)
		{
			const ImGuiID id = window->GetID(strId.c_str());
			const ImRect bb(window->DC.CursorPos, ImVec2(window->DC.CursorPos.x + size.x, window->DC.CursorPos.y + size.y));
			if (ImGui::ItemAdd(bb, id))
			{
				// Render
				const ImU32 color = ImGui::GetColorU32(ImGuiCol_WindowBg, 0.35f);
				const ImVec2 tipDimensions(size.x / 2.0f, size.x / 3.0f * 2.0f);
				const float shaftCenterX = window->DC.CursorPos.x + size.x / 2.0f;
				const ImVec2 shaftSize(size.x / 5.0f, bb.Max.y - bb.Min.y - tipDimensions.y);
				const ImRect shaft(shaftCenterX - shaftSize.x, bb.Min.y, shaftCenterX + shaftSize.x, bb.Min.y + shaftSize.y);
				ImGui::RenderRectFilledRangeH(window->DrawList, shaft, color, 0.0f, 1.0f, 0.0f);
				ImGui::RenderArrowPointingAt(window->DrawList, ImVec2(bb.Max.x - size.x / 2.0f, bb.Max.y), tipDimensions,
				                             ImGuiDir_Down, color);
			}
		}
	}
	ImGui::End();
	ImGui::PopStyleVar(); // ImGuiStyleVar_WindowBorderSize
	ImGui::PopStyleVar(); // ImGuiStyleVar_WindowRounding
	ImGui::PopStyleVar(); // ImGuiStyleVar_WindowPadding
}

bool boxIntersect(const glm::vec4& box1, const glm::vec4& box2)
{
	// where z is width and y is height
	return box1.x - box2.x < box2.z && //
	       box2.x - box1.x < box1.z && //
	       box1.y - box2.y < box2.w && //
	       box2.y - box1.y < box1.w;
}

bool fitBox(float minY, const std::vector<glm::vec4>& coveredAreas, glm::vec4& box)
{
	// Where z is width and y is height
	bool restart = true;
	while (restart)
	{
		restart = false;
		for (const auto area : coveredAreas)
		{
			// If there is an overlap, try moving it up
			while (boxIntersect(area, box))
			{
				restart = true; // Restart because we now might intersect with earlier boxes
				box.y = area.y - 1.0f - box.w;
				if (box.y - box.w < minY)
				{
					return false;
				}
			}
			if (restart)
			{
				break;
			}
		}
	}
	return true;
}

std::optional<glm::uvec4> Gui::RenderVillagerName(const std::vector<glm::vec4>& coveredAreas, const std::string& name,
                                                  const std::string& text, const glm::vec4& color, const ImVec2& pos,
                                                  float arrowLength, std::function<void(void)> debugCallback) const
{
	// clang-format off
	const auto boxOverlayFlags =
		0u
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoResize
		| (debugCallback ? ImGuiWindowFlags_AlwaysVerticalScrollbar : ImGuiWindowFlags_NoScrollbar)
		| ImGuiWindowFlags_NoCollapse
		| (debugCallback ? 0 : ImGuiWindowFlags_NoInputs)
	;
	// clang-format on

	const auto& style = ImGui::GetStyle();

	ImVec4 textColor;
	{
		glm::vec4 adjustedColor = color;
		if (adjustedColor.r < 0.04f && adjustedColor.g < 0.04f && adjustedColor.b < 0.04f)
		{
			adjustedColor = glm::vec4(1.0f, 1.0f, 1.0f, adjustedColor.a);
		}
		adjustedColor = glm::vec4((glm::vec3(adjustedColor) + glm::vec3(1.0f)) / 2.0f, 1.0f);
		textColor = ImVec4(adjustedColor.r, adjustedColor.g, adjustedColor.b, adjustedColor.a);
	}

	const std::string fullText = name + "\n" + text;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	glm::vec4 boxExtent;
	{
		const auto textSize = ImGui::CalcTextSize(fullText.c_str());
		ImVec2 boxSize(textSize.x + 2 * style.WindowPadding.x, textSize.y + 2 * style.WindowPadding.y);
		if (debugCallback)
		{
			boxSize.x += style.ScrollbarSize;
			boxSize.y += ImGui::CalcTextSize("Debug").y + 2 * style.FramePadding.y + style.ItemSpacing.y;
		}
		const float halfWidth = boxSize.x / 2.0f;
		const auto clampedX = std::clamp(pos.x, halfWidth, ImGui::GetIO().DisplaySize.x - halfWidth);
		const ImVec2 boxPos(clampedX + halfWidth, std::max(pos.y - arrowLength, .0f));
		boxExtent = glm::uvec4(boxPos.x, boxPos.y, boxSize.x, boxSize.y);
	}

	float originalY = boxExtent.y;
	bool fits = fitBox(_menuBarSize.y, coveredAreas, boxExtent);

	if (fits)
	{
		ImGui::SetNextWindowPos(ImVec2(boxExtent.x, boxExtent.y), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
		if (!debugCallback)
		{
			ImGui::SetNextWindowSize(ImVec2(boxExtent.z, boxExtent.w));
		}
		ImGui::SetNextWindowBgAlpha(0.35f);

		if (ImGui::Begin(("Villager overlay #" + name).c_str(), nullptr, boxOverlayFlags))
		{
			ImGui::TextColored(textColor, "%s", fullText.c_str());
			if (debugCallback)
			{
				if (ImGui::CollapsingHeader("Debug"))
				{
					debugCallback();
				}
			}
		}
		ImGui::End();
	}
	ImGui::PopStyleVar();

	if (!fits)
	{
		return std::nullopt;
	}

	if (originalY == boxExtent.y)
	{
		RenderArrow("Villager overlay arrow #" + name, ImVec2(pos.x, pos.y - arrowLength), ImVec2(15, arrowLength));
	}

	return std::make_optional<glm::uvec4>(boxExtent);
}

void Gui::ShowVillagerNames(const Game& game)
{
	using namespace ecs::components;
	using namespace ecs::systems;

	const auto& config = game.GetConfig();
	if (!config.showVillagerNames)
	{
		return;
	}

	const auto& displaySize = ImGui::GetIO().DisplaySize;

	uint32_t i = 0;
	const auto& camera = game.GetCamera();
	const glm::vec4 viewport =
	    glm::vec4(ImGui::GetStyle().WindowPadding.x, 0, displaySize.x - ImGui::GetStyle().WindowPadding.x, displaySize.y);
	std::vector<glm::vec4> coveredAreas;
	coveredAreas.reserve(game.GetEntityRegistry().Size<Villager>());
	game.GetEntityRegistry().Each<const Transform, Villager, LivingAction>([this, &i, &coveredAreas, &camera, config, viewport](
	                                                                           const Transform& transform, Villager& villager,
	                                                                           LivingAction& action) {
		++i;
		const float height = 2.0f * transform.scale.y; // TODO(bwrsandman): get from bounding box max y
		glm::vec3 screenPoint;
		if (!camera.ProjectWorldToScreen(transform.position + glm::vec3(0.0f, height, 0.0f), viewport, screenPoint))
		{
			return;
		}

		// 3.5 was measured in vanilla but it is possible that it is configurable
		float maxDistance = 3.5f;
		const glm::vec3 relativePosition = (camera.GetPosition() - transform.position) / 100.0f;
		if (glm::dot(relativePosition, relativePosition) > maxDistance * maxDistance)
		{
			return;
		}

		// TODO(bwrsandman): Get owner player and associated color
		glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		// Female villagers have a lighter colour
		if (villager.sex == Villager::Sex::FEMALE)
		{
			color += glm::vec4((glm::vec3(1.0f) - glm::vec3(color)) * glm::vec3(144.0f / 255.0f), color.a);
			color = glm::saturate(color);
		}

		const std::string name = "Villager #" + std::to_string(i);
		const std::string stateHelpText = "TODO: STATE HELP TEXT";
		std::string details =
		    fmt::format("{}\nA:{} L:{}%, H:{}%", stateHelpText, villager.age, villager.health, villager.hunger);
		const auto& actionSystem = Locator::livingActionSystem::value();
		if (config.debugVillagerStates)
		{
			details += fmt::format(
			    "\n"
			    "Top State:      {}\n"
			    "Final State:    {}\n"
			    "Previous State: {}\n"
			    "Turns until next state change: {}\n"
			    "Turns since last state change: {}",
			    k_VillagerStateStrings.at(static_cast<size_t>(actionSystem.VillagerGetState(action, LivingAction::Index::Top))),
			    k_VillagerStateStrings.at(
			        static_cast<size_t>(actionSystem.VillagerGetState(action, LivingAction::Index::Final))),
			    k_VillagerStateStrings.at(
			        static_cast<size_t>(actionSystem.VillagerGetState(action, LivingAction::Index::Previous))),
			    action.turnsUntilStateChange, action.turnsSinceStateChange);
		}

		std::function<void(void)> debugCallback;
		if (config.debugVillagerNames)
		{
			debugCallback = [&villager, &action, &actionSystem] {
				if (villager.abode == entt::null)
				{
					ImGui::Text("Homeless");
				}
				ImGui::InputInt("Health", reinterpret_cast<int*>(&villager.health));
				ImGui::InputInt("Age", reinterpret_cast<int*>(&villager.age));
				ImGui::InputInt("Hunger", reinterpret_cast<int*>(&villager.hunger));
				ImGui::Combo("Life Stage", &villager.lifeStage, Villager::k_LifeStageStrs);
				ImGui::Combo("Sex", &villager.sex, Villager::k_SexStrs);
				ImGui::Combo("Tribe", &villager.tribe, k_TribeStrs);
				ImGui::Combo("Villager Number", &villager.number, k_VillagerRoleStrs);
				ImGui::Combo("Task", &villager.task, Villager::k_TaskStrs);

				size_t index = 0;
				for (const auto& str : LivingAction::k_IndexStrings)
				{
					if (ImGui::BeginCombo(str.data(), k_VillagerStateStrings
					                                      .at(static_cast<size_t>(actionSystem.VillagerGetState(
					                                          action, static_cast<LivingAction::Index>(index))))
					                                      .data()))
					{
						size_t n = 0;
						for (const auto& stateStr : k_VillagerStateStrings)
						{
							const bool isSelected = static_cast<size_t>(actionSystem.VillagerGetState(
							                            action, static_cast<LivingAction::Index>(index))) == n;
							if (ImGui::Selectable(stateStr.data(), isSelected))
							{
								actionSystem.VillagerSetState(action, static_cast<LivingAction::Index>(index),
								                              static_cast<VillagerStates>(n), true);
							}

							// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
							if (isSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
							++n;
						}
						ImGui::EndCombo();
					}
					++index;
				}
			};
		}

		const auto area = RenderVillagerName(coveredAreas, name, details, color,
		                                     ImVec2(screenPoint.x, viewport.w - screenPoint.y), 100.0f, debugCallback);
		if (area.has_value())
		{
			coveredAreas.emplace_back(area.value());
		}
	});
}

void Gui::ShowCameraPositionOverlay(const Game& game)
{
	// clang-format off
	static const auto cameraPositionOverlayFlags =
		0u
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoDecoration
		| ImGuiWindowFlags_AlwaysAutoResize
		| ImGuiWindowFlags_NoSavedSettings
		| ImGuiWindowFlags_NoFocusOnAppearing
		| ImGuiWindowFlags_NoNav
		;
	// clang-format on

	const auto& displaySize = ImGui::GetIO().DisplaySize;
	ImGui::SetNextWindowPos(ImVec2(displaySize.x - 8.0f, displaySize.y - 8.0f), ImGuiCond_Always, ImVec2(1.0f, 1.0f));
	ImGui::SetNextWindowBgAlpha(0.35f);

	if (ImGui::Begin("Camera position overlay", nullptr, cameraPositionOverlayFlags))
	{
		const auto camPos = game.GetCamera().GetPosition();
		const auto camRot = game.GetCamera().GetRotation();
		ImGui::Text("Camera Position: (%.1f,%.1f, %.1f)", camPos.x, camPos.y, camPos.z);
		ImGui::Text("Camera Rotation: (%.1f,%.1f, %.1f)", camRot.x, camRot.y, camRot.z);

		if (ImGui::IsMousePosValid())
		{
			const auto& mousePos = ImGui::GetIO().MousePos;
			ImGui::Text("Mouse Position: (%.1f,%.1f)", mousePos.x, mousePos.y);
		}
		else
		{
			ImGui::Text("Mouse Position: <invalid>");
		}

		const auto& handPosition = game.GetEntityRegistry().Get<ecs::components::Transform>(game.GetHand()).position;
		ImGui::Text("Hand Position: (%.1f,%.1f,%.1f)", handPosition.x, handPosition.y, handPosition.z);

		ImGui::Text("Game Turn: %u (%.3f ms)%s", game.GetTurn(), game.GetDeltaTime().count(),
		            game.IsPaused() ? " - paused" : "");
	}
	ImGui::End();
}

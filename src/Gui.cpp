/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Gui.h"

#include <bgfx/bgfx.h>
#include <bgfx/embedded_shader.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#include <bx/math.h>
#pragma GCC diagnostic pop
#include <bx/timer.h>
#include <glm/gtx/compatibility.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_widget_flamegraph.h>
#ifdef _WIN32
#include <SDL2/SDL_syswm.h>
#endif

#include <LNDFile.h>

#include "3D/Camera.h"
#include "3D/LandIsland.h"
#include "3D/MeshPack.h"
#include "3D/Sky.h"
#include "3D/Water.h"
#include "Common/FileSystem.h"
#include "Console.h"
#include "Entities/Components/Transform.h"
#include "Entities/Components/Tree.h"
#include "Entities/Registry.h"
#include "Game.h"
#include "GameWindow.h"
#include "Graphics/Shaders/imgui/fs_imgui_image.bin.h"
#include "Graphics/Shaders/imgui/fs_ocornut_imgui.bin.h"
#include "Graphics/Shaders/imgui/vs_imgui_image.bin.h"
#include "Graphics/Shaders/imgui/vs_ocornut_imgui.bin.h"
#include "LHVMViewer.h"
#include "MeshViewer.h"
#include "Profiler.h"
#include "Sound/AudioDebug.h"
#include "Sound/SoundHandler.h"
#include "Sound/SoundPack.h"

using namespace openblack;

#define IMGUI_FLAGS_NONE UINT8_C(0x00)
#define IMGUI_FLAGS_ALPHA_BLEND UINT8_C(0x01)

namespace
{
const bgfx::EmbeddedShader s_embeddedShaders[] = {BGFX_EMBEDDED_SHADER(vs_ocornut_imgui),
                                                  BGFX_EMBEDDED_SHADER(fs_ocornut_imgui), BGFX_EMBEDDED_SHADER(vs_imgui_image),
                                                  BGFX_EMBEDDED_SHADER(fs_imgui_image),

                                                  BGFX_EMBEDDED_SHADER_END()};
} // namespace

std::unique_ptr<Gui> Gui::create(const GameWindow* window, graphics::RenderPass viewId, float scale)
{
	IMGUI_CHECKVERSION();
	auto imgui = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(scale);
	io.FontGlobalScale = scale;

	io.BackendRendererName = "imgui_impl_bgfx";
	auto meshViewer = std::make_unique<MeshViewer>();
	auto terminal = std::make_unique<Console>();

	auto gui =
	    std::unique_ptr<Gui>(new Gui(imgui, static_cast<bgfx::ViewId>(viewId), std::move(meshViewer), std::move(terminal)));

	if (window)
	{
		if (!gui->InitSdl2(window->GetHandle()))
		{
			return nullptr;
		}
	}

	return gui;
}

Gui::Gui(ImGuiContext* imgui, bgfx::ViewId viewId, std::unique_ptr<MeshViewer>&& meshViewer,
         std::unique_ptr<Console>&& terminal)
    : _imgui(imgui)
    , _time(0)
    , _vertexBuffer(BGFX_INVALID_HANDLE)
    , _indexBuffer(BGFX_INVALID_HANDLE)
    , _program(BGFX_INVALID_HANDLE)
    , _imageProgram(BGFX_INVALID_HANDLE)
    , _texture(BGFX_INVALID_HANDLE)
    , _s_tex(BGFX_INVALID_HANDLE)
    , _u_imageLodEnabled(BGFX_INVALID_HANDLE)
    , _mousePressed {false, false, false}
    , _mouseCursors {0}
    , _clipboardTextData(nullptr)
    , _last(bx::getHPCounter())
    , _lastScroll(0)
    , _viewId(viewId)
    , _meshViewer(std::move(meshViewer))
    , _console(std::move(terminal))
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

	_console->ProcessEventSdl2(event);

	ImGuiIO& io = ImGui::GetIO();
	switch (event.type)
	{
	case SDL_QUIT:
		return false;
	case SDL_MOUSEWHEEL:
	{
		if (event.wheel.x > 0)
			io.MouseWheelH += 1;
		if (event.wheel.x < 0)
			io.MouseWheelH -= 1;
		if (event.wheel.y > 0)
			io.MouseWheel += 1;
		if (event.wheel.y < 0)
			io.MouseWheel -= 1;
		return io.WantCaptureMouse;
	}
	case SDL_MOUSEBUTTONDOWN:
	{
		if (event.button.button == SDL_BUTTON_LEFT)
			_mousePressed[0] = true;
		if (event.button.button == SDL_BUTTON_RIGHT)
			_mousePressed[1] = true;
		if (event.button.button == SDL_BUTTON_MIDDLE)
			_mousePressed[2] = true;
		return io.WantCaptureMouse;
	}
	case SDL_TEXTINPUT:
	{
		io.AddInputCharactersUTF8(event.text.text);
		return io.WantTextInput;
	}
	case SDL_KEYDOWN:
		if (event.key.keysym.sym == SDLK_BACKQUOTE)
		{
			_console->Toggle();
		}
		[[fallthrough]];
	case SDL_KEYUP:
	{
		int key = event.key.keysym.scancode;
		IM_ASSERT(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
		io.KeysDown[key] = (event.type == SDL_KEYDOWN);
		io.KeyShift = ((SDL_GetModState() & KMOD_SHIFT) != 0);
		io.KeyCtrl = ((SDL_GetModState() & KMOD_CTRL) != 0);
		io.KeyAlt = ((SDL_GetModState() & KMOD_ALT) != 0);
		io.KeySuper = ((SDL_GetModState() & KMOD_GUI) != 0);
		return io.WantCaptureKeyboard;
	}
	}
	return io.WantCaptureMouse;
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
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor()
	                                                      // values (optional)
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;  // We can honor io.WantSetMousePos
	                                                      // requests (optional, rarely used)
	io.BackendPlatformName = "imgui_impl_sdl";

	// Keyboard mapping. ImGui will use those indices to peek into the
	// io.KeysDown[] array.
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
	_program = bgfx::createProgram(bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_ocornut_imgui"),
	                               bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_ocornut_imgui"), true);
	_imageProgram = bgfx::createProgram(bgfx::createEmbeddedShader(s_embeddedShaders, type, "vs_imgui_image"),
	                                    bgfx::createEmbeddedShader(s_embeddedShaders, type, "fs_imgui_image"), true);

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
		SDL_WarpMouseInWindow(_window, (int)io.MousePos.x, (int)io.MousePos.y);
	else
		io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

	int mx, my;
	Uint32 mouse_buttons = SDL_GetMouseState(&mx, &my);
	io.MouseDown[0] = _mousePressed[0] ||
	                  (mouse_buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0; // If a mouse press event came, always pass it as
	                                                                      // "mouse held this frame", so we don't miss
	                                                                      // click-release events that are shorter than 1 frame.
	io.MouseDown[1] = _mousePressed[1] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
	io.MouseDown[2] = _mousePressed[2] || (mouse_buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
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
#define MAP_BUTTON(NAV_NO, BUTTON_NO)                                                                        \
	{                                                                                                        \
		io.NavInputs[NAV_NO] = (SDL_GameControllerGetButton(game_controller, BUTTON_NO) != 0) ? 1.0f : 0.0f; \
	}
#define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1)                                                              \
	{                                                                                                    \
		float vn = (float)(SDL_GameControllerGetAxis(game_controller, AXIS_NO) - V0) / (float)(V1 - V0); \
		if (vn > 1.0f)                                                                                   \
			vn = 1.0f;                                                                                   \
		if (vn > 0.0f && io.NavInputs[NAV_NO] < vn)                                                      \
			io.NavInputs[NAV_NO] = vn;                                                                   \
	}
	const int thumb_dead_zone = 8000;                            // SDL_gamecontroller.h suggests using this value.
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
	MAP_ANALOG(ImGuiNavInput_LStickLeft, SDL_CONTROLLER_AXIS_LEFTX, -thumb_dead_zone, -32768);
	MAP_ANALOG(ImGuiNavInput_LStickRight, SDL_CONTROLLER_AXIS_LEFTX, +thumb_dead_zone, +32767);
	MAP_ANALOG(ImGuiNavInput_LStickUp, SDL_CONTROLLER_AXIS_LEFTY, -thumb_dead_zone, -32767);
	MAP_ANALOG(ImGuiNavInput_LStickDown, SDL_CONTROLLER_AXIS_LEFTY, +thumb_dead_zone, +32767);

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
	if (window)
	{
		int w, h;
		int display_w, display_h;
		SDL_GetWindowSize(window, &w, &h);
		SDL_GL_GetDrawableSize(window, &display_w, &display_h);
		io.DisplaySize = ImVec2((float)w, (float)h);
		if (w > 0 && h > 0)
			io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);
	}
	else
	{
		io.DisplaySize = ImVec2(1.0f, 1.0f);
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
	}

	// Setup time step (we don't use SDL_GetTicks() because it is using
	// millisecond resolution)
	static Uint64 frequency = SDL_GetPerformanceFrequency();
	Uint64 current_time = SDL_GetPerformanceCounter();
	io.DeltaTime = _time > 0 ? (float)((double)(current_time - _time) / frequency) : (float)(1.0f / 60.0f);
	_time = current_time;

	UpdateMousePosAndButtons();
	UpdateMouseCursor();

	// Update game controllers (if enabled and available)
	UpdateGamepads();
}

void Gui::NewFrame(GameWindow* window)
{
	ImGui::SetCurrentContext(_imgui);

	NewFrameSdl2(window ? window->GetHandle() : nullptr);
	ImGui::NewFrame();
}

bool Gui::Loop(Game& game, const Renderer& renderer)
{
	_meshViewer->DrawScene(renderer);
	NewFrame(game.GetWindow());
	if (ShowMenu(game))
	{
		// Exit option selected
		return true;
	}
	_meshViewer->DrawWindow();
	_console->DrawWindow(game);
	ShowVillagerNames(game);
	ShowCameraPositionOverlay(game);
	LHVMViewer::Draw(game.GetLhvm());
	ShowLandIslandWindow(game);
	ShowAudioDebuggerWindow(game);
	ShowProfilerWindow(game);
	ShowWaterFramebuffer(game);

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
		float ortho[16];
		bx::mtxOrtho(ortho, 0.0f, width, height, 0.0f, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
		bgfx::setViewTransform(_viewId, NULL, ortho);
		bgfx::setViewRect(_viewId, 0, 0, uint16_t(width), uint16_t(height));
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
		uint32_t numIndices = (uint32_t)drawList->IdxBuffer.size();

		bgfx::update(_vertexBuffer, vertexBufferOffset,
		             bgfx::makeRef(drawList->VtxBuffer.begin(), numVertices * sizeof(ImDrawVert)));
		bgfx::update(_indexBuffer, indexBufferOffset,
		             bgfx::makeRef(drawList->IdxBuffer.begin(), numIndices * sizeof(ImDrawIdx)));

		uint32_t offset = indexBufferOffset;
		for (const ImDrawCmd *cmd = drawList->CmdBuffer.begin(), *cmdEnd = drawList->CmdBuffer.end(); cmd != cmdEnd; ++cmd)
		{
			if (cmd->UserCallback)
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
						const float lodEnabled[4] = {float(texture.s.mip), 1.0f, 0.0f, 0.0f};
						bgfx::setUniform(_u_imageLodEnabled, lodEnabled);
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
			constexpr std::array<std::pair<std::string_view, std::string_view>, 6> RegularIslands = {
			    // clang-format off
			    std::pair {"Land 1", "Land1.txt"},
			    std::pair {"Land 2", "Land2.txt"},
			    std::pair {"Land 3", "Land3.txt"},
			    std::pair {"Land 4", "Land4.txt"},
			    std::pair {"Land 5", "Land5.txt"},
			    std::pair {"God's Playground", "LandT.txt"},
			    // clang-format on
			};
			constexpr std::array<std::pair<std::string_view, std::string_view>, 3> PlaygroundIslands = {
			    std::pair {"Two Gods", "TwoGods.txt"},
			    std::pair {"Three Gods", "ThreeGods.txt"},
			    std::pair {"Four Gods", "FourGods.txt"},
			};

			auto menu_item = [&game](const auto& label, const fs::path& path) {
				if (ImGui::MenuItem(label.data()))
					game.LoadMap(path);
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", path.generic_string().c_str());
			};

			auto& filesystem = Game::instance()->GetFileSystem();

			ImGui::MenuItem("Story Islands", nullptr, false, false);
			for (auto& [label, path] : RegularIslands)
			{
				menu_item(label, filesystem.ScriptsPath() / path);
			}
			ImGui::Separator();
			ImGui::MenuItem("Playground Islands", nullptr, false, false);
			for (auto& [label, path] : PlaygroundIslands)
			{
				menu_item(label, filesystem.PlaygroundPath() / path);
			}

			ImGui::EndMenu();
		}

		auto& config = game.GetConfig();

		if (ImGui::BeginMenu("World"))
		{
			if (ImGui::SliderFloat("Time of Day", &config.timeOfDay, 0.0f, 1.0f, "%.3f"))
				Game::instance()->GetSky().SetTime(config.timeOfDay);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Debug"))
		{
			if (ImGui::MenuItem("Open Profiler"))
			{
				config.showProfiler = true;
			}

			if (ImGui::MenuItem("Open Audio Debugger"))
			{
				config.showAudioDebugger = true;
			}

			if (ImGui::MenuItem("Console"))
			{
				_console->Open();
			}

			if (ImGui::MenuItem("Mesh Viewer"))
			{
				_meshViewer->Open();
			}

			if (ImGui::MenuItem("Land Island"))
			{
				config.showLandIsland = true;
			}

			if (ImGui::BeginMenu("Villager Names"))
			{
				ImGui::Checkbox("Show", &config.showVillagerNames);
				ImGui::Checkbox("Debug", &config.debugVillagerNames);

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				ImGui::Checkbox("Sky", &config.drawSky);
				ImGui::Checkbox("Water", &config.drawWater);
				ImGui::Checkbox("Island", &config.drawIsland);
				ImGui::Checkbox("Entities", &config.drawEntities);
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
				ImGui::Text("Scaled game duration: %.3fms (%.3f Hz)", multiplier * Game::kTurnDuration.count(),
				            1000.0f / (multiplier * Game::kTurnDuration.count()));
				if (ImGui::MenuItem("Slow"))
				{
					game.SetGameSpeed(Game::kTurnDurationMultiplierSlow);
				}
				if (ImGui::MenuItem("Normal"))
				{
					game.SetGameSpeed(Game::kTurnDurationMultiplierNormal);
				}
				if (ImGui::MenuItem("Fast"))
				{
					game.SetGameSpeed(Game::kTurnDurationMultiplierFast);
				}
				if (ImGui::SliderFloat("Multiplier", &multiplier, 1.0f / 10.0f, 10.0f, "%.3f", 2.0f))
				{
					game.SetGameSpeed(multiplier);
				}

				ImGui::EndMenu();
			}

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
		std::string str_id = name + " Arrow";
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (!window->SkipItems)
		{
			const ImGuiID id = window->GetID(str_id.c_str());
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

bool boxIntersect(const glm::vec4& box_1, const glm::vec4& box_2)
{
	// where z is width and y is height
	return box_1.x - box_2.x < box_2.z && //
	       box_2.x - box_1.x < box_1.z && //
	       box_1.y - box_2.y < box_2.w && //
	       box_2.y - box_1.y < box_1.w;
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
		const ImVec2 boxPos(std::clamp(pos.x, halfWidth, ImGui::GetIO().DisplaySize.x - halfWidth) + halfWidth,
		                    pos.y - arrowLength);

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
		RenderArrow(("Villager overlay arrow #" + name).c_str(), ImVec2(pos.x, pos.y - arrowLength), ImVec2(15, arrowLength));
	}

	return std::make_optional<glm::uvec4>(boxExtent);
}

void Gui::ShowVillagerNames(const Game& game)
{
	using namespace entities::components;

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
	game.GetEntityRegistry().Each<Villager, const Transform>([this, &i, &coveredAreas, camera, config,
	                                                          viewport](Villager& entity, const Transform& transform) {
		++i;
		const float height = 2.0f * transform.scale.y; // TODO(bwrsandman): get from bounding box max y
		glm::vec3 screenPoint;
		if (!camera.ProjectWorldToScreen(transform.position + glm::vec3(0.0f, height, 0.0f), viewport, screenPoint))
		{
			return;
		}

		// 3.5 was measured in vanilla but it is possible that it is configurable
		float max_distance = 3.5f;
		const glm::vec3 relativePosition = (camera.GetPosition() - transform.position) / 100.0f;
		if (glm::dot(relativePosition, relativePosition) > max_distance * max_distance)
		{
			return;
		}

		// TODO(bwrsandman): Get owner player and associated color
		glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		// Female villagers have a lighter colour
		if (entity.sex == Villager::Sex::FEMALE)
		{
			color += glm::vec4((glm::vec3(1.0f) - glm::vec3(color)) * glm::vec3(144.0f / 255.0f), color.a);
			color = glm::saturate(color);
		}

		std::function<void(void)> debugCallback;
		if (config.debugVillagerNames)
		{
			debugCallback = [&entity] {
				ImGui::InputInt("Health", reinterpret_cast<int*>(&entity.health));
				ImGui::InputInt("Age", reinterpret_cast<int*>(&entity.age));
				ImGui::InputInt("Hunger", reinterpret_cast<int*>(&entity.hunger));
				ImGui::InputInt("Life Stage", reinterpret_cast<int*>(&entity.lifeStage));
				ImGui::InputInt("Sex", reinterpret_cast<int*>(&entity.sex));
				ImGui::InputInt("Tribe", reinterpret_cast<int*>(&entity.tribe));
				ImGui::InputInt("Role", reinterpret_cast<int*>(&entity.role));
				ImGui::InputInt("Task", reinterpret_cast<int*>(&entity.task));
			};
		}

		const auto area = RenderVillagerName(coveredAreas, "Villager #" + std::to_string(i),
		                                     fmt::format("TODO: STATE HELP TEXT"
		                                                 "\nA:{} L:{}%, H:{}%",
		                                                 entity.age, entity.health, entity.hunger),
		                                     color, ImVec2(screenPoint.x, viewport.w - screenPoint.y), 100.0f, debugCallback);
		if (area.has_value())
		{
			coveredAreas.emplace_back(area.value());
		}
	});
}

void Gui::ShowProfilerWindow(Game& game)
{
	auto& config = game.GetConfig();

	if (!config.showProfiler)
	{
		return;
	}

	if (ImGui::Begin("Profiler", &config.showProfiler))
	{
		using namespace entities::components;

		const bgfx::Stats* stats = bgfx::getStats();
		const double toMsCpu = 1000.0 / stats->cpuTimerFreq;
		const double toMsGpu = 1000.0 / stats->gpuTimerFreq;
		const double frameMs = double(stats->cpuTimeFrame) * toMsCpu;
		_times.pushBack(frameMs);
		_fps.pushBack(1000.0f / frameMs);

		char frameTextOverlay[256];
		std::snprintf(frameTextOverlay, sizeof(frameTextOverlay), "%.3fms, %.1f FPS", _times.back(), _fps.back());

		ImGui::Text("Submit CPU %0.3f, GPU %0.3f (Max GPU Latency: %d)",
		            double(stats->cpuTimeEnd - stats->cpuTimeBegin) * toMsCpu,
		            double(stats->gpuTimeEnd - stats->gpuTimeBegin) * toMsGpu, stats->maxGpuLatency);
		ImGui::Text("Wait Submit %0.3f, Wait Render %0.3f", stats->waitSubmit * toMsCpu, stats->waitRender * toMsCpu);

		ImGui::Columns(5);
		ImGui::Checkbox("Sky", &config.drawSky);
		ImGui::NextColumn();
		ImGui::Checkbox("Water", &config.drawWater);
		ImGui::NextColumn();
		ImGui::Checkbox("Island", &config.drawIsland);
		ImGui::NextColumn();
		ImGui::Checkbox("Entities", &config.drawEntities);
		ImGui::NextColumn();
		ImGui::Checkbox("TestModel", &config.drawTestModel);
		ImGui::NextColumn();
		ImGui::Checkbox("Debug Cross", &config.drawDebugCross);
		ImGui::Columns(1);

		auto width = ImGui::GetColumnWidth() - ImGui::CalcTextSize("Frame").x;
		ImGui::PlotHistogram("Frame", _times._values, decltype(_times)::_bufferSize, _times._offset, frameTextOverlay, 0.0f,
		                     FLT_MAX, ImVec2(width, 45.0f));

		ImGui::Text("Primitives Triangles %u, Triangle Strips %u, Lines %u "
		            "Line Strips %u, Points %u",
		            stats->numPrims[0], stats->numPrims[1], stats->numPrims[2], stats->numPrims[3], stats->numPrims[4]);
		ImGui::Columns(2);
		ImGui::Text("Num Entities %u, Trees %u", static_cast<uint32_t>(game.GetEntityRegistry().Size<Transform>()),
		            static_cast<uint32_t>(game.GetEntityRegistry().Size<Tree>()));
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

		ImGuiWidgetFlameGraph::PlotFlame(
		    "CPU",
		    [](float* startTimestamp, float* endTimestamp, ImU8* level, const char** caption, const void* data,
		       int idx) -> void {
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
		    },
		    &entry, static_cast<uint8_t>(Profiler::Stage::_count), 0, "Main Thread", 0, FLT_MAX, ImVec2(width, 0));

		ImGuiWidgetFlameGraph::PlotFlame(
		    "GPU",
		    [](float* startTimestamp, float* endTimestamp, ImU8* level, const char** caption, const void* data,
		       int idx) -> void {
			    auto stats = reinterpret_cast<const bgfx::Stats*>(data);
			    if (startTimestamp)
			    {
				    *startTimestamp =
				        1000.0f * (stats->viewStats[idx].gpuTimeBegin - stats->gpuTimeBegin) / (double)stats->gpuTimerFreq;
			    }
			    if (endTimestamp)
			    {
				    *endTimestamp =
				        1000.0f * (stats->viewStats[idx].gpuTimeEnd - stats->gpuTimeBegin) / (double)stats->gpuTimerFreq;
			    }
			    if (level)
			    {
				    *level = 0;
			    }
			    if (caption)
			    {
				    *caption = stats->viewStats[idx].name;
			    }
		    },
		    stats, stats->numViews, 0, "GPU Frame", 0,
		    1000.0f * (stats->gpuTimeEnd - stats->gpuTimeBegin) / (double)stats->gpuTimerFreq, ImVec2(width, 0));

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
				auto const& viewStat = stats->viewStats[i];
				int64_t gpuTimeElapsed = viewStat.gpuTimeEnd - viewStat.gpuTimeBegin;

				ImGui::Text("    %s: %0.3f", viewStat.name, 1000.0f * gpuTimeElapsed / (double)stats->gpuTimerFreq);
				frameDuration -= gpuTimeElapsed;
			}
			ImGui::Text("    Unaccounted: %0.3f", 1000.0f * frameDuration / (double)stats->gpuTimerFreq);
		}
		ImGui::Columns(1);
	}
	ImGui::End();
}

void Gui::ShowWaterFramebuffer(const Game& game)
{
	auto& config = game.GetConfig();

	if (!config.waterDebug)
	{
		return;
	}

	const auto& water = game.GetWater();

	ImGui::Begin("Water Debug");
	ImGui::Image(water.GetFrameBuffer().GetColorAttachment().GetNativeHandle(), ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
	ImGui::End();
}

void Gui::ShowLandIslandWindow(Game& game)
{
	auto& config = game.GetConfig();

	if (!config.showLandIsland)
	{
		return;
	}

	if (ImGui::Begin("Land Island", &config.showLandIsland))
	{
		const auto& landIsland = game.GetLandIsland();

		ImGui::SliderFloat("Bump", &config.bumpMapStrength, 0.0f, 1.0f, "%.3f");
		ImGui::SliderFloat("Small Bump", &config.smallBumpMapStrength, 0.0f, 1.0f, "%.3f");

		ImGui::Separator();

		ImGui::Text("Block Count: %zu", landIsland.GetBlocks().size());
		ImGui::Text("Country Count: %zu", landIsland.GetCountries().size());

		ImGui::Separator();

		if (ImGui::Button("Dump Textures"))
		{
			landIsland.DumpTextures();
		}

		if (ImGui::Button("Dump Heightmap"))
		{
			landIsland.DumpMaps();
		}

		ImGui::End();
	}
}

void Gui::ShowAudioDebuggerWindow(Game& game)
{
	auto& config = game.GetConfig();

	if (!config.showAudioDebugger)
	{
		return;
	}

	audio::AudioDebug::ShowDebugGui(game);
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

		const auto& handPosition = game.GetHandTransform().position;
		ImGui::Text("Hand Position: (%.1f,%.1f,%.1f)", handPosition.x, handPosition.y, handPosition.z);

		ImGui::Text("Game Turn: %u (%.3f ms)", game.GetTurn(), game.GetDeltaTime().count());
	}
	ImGui::End();
}

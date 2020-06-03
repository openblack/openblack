/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "GameWindow.h"

#include "Renderer.h"

#include <SDL_syswm.h>
#include <spdlog/spdlog.h>
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
#include <wayland-egl.h>
#endif // defined(SDL_VIDEO_DRIVER_WAYLAND)

#include <iostream>

using namespace openblack;

void GameWindow::SDLDestroyer::operator()(SDL_Window* window) const
{
	SDL_DestroyWindow(window);
}

GameWindow::GameWindow(const std::string& title, int width, int height, DisplayMode displayMode)
{
	SDL_version compiledVersion, linkedVersion;
	SDL_VERSION(&compiledVersion);
	SDL_GetVersion(&linkedVersion);

	spdlog::info("Initializing SDL...");
	spdlog::info("SDL Version/Compiled {}.{}.{}", compiledVersion.major, compiledVersion.minor, compiledVersion.patch);
	spdlog::info("SDL Version/Linked {}.{}.{}", linkedVersion.major, linkedVersion.minor, linkedVersion.patch);

	// Initialize SDL
	if (SDL_WasInit(0) == 0)
	{
		SDL_SetMainReady();
		if (SDL_Init(0) != 0)
			throw std::runtime_error("Could not initialize SDL: " + std::string(SDL_GetError()));

		if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0)
			throw std::runtime_error("Could not initialize SDL Video Subsystem: " + std::string(SDL_GetError()));

		if (SDL_InitSubSystem(SDL_INIT_TIMER) != 0)
			throw std::runtime_error("Could not initialize SDL Timer Subsystem: " + std::string(SDL_GetError()));
	}

	SDL_ShowCursor(SDL_DISABLE);

	uint32_t flags = SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
	if (displayMode == DisplayMode::Fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN;
	else if (displayMode == DisplayMode::Borderless)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	// Get SDL Window requirements from Renderer
	const int x = SDL_WINDOWPOS_UNDEFINED;
	const int y = SDL_WINDOWPOS_UNDEFINED;

	auto window = std::unique_ptr<SDL_Window, SDLDestroyer>(SDL_CreateWindow(title.c_str(), x, y, width, height, flags));

	if (window == nullptr)
	{
		spdlog::error("Failed to create SDL2 window: '{}'", SDL_GetError());
		throw std::runtime_error("Failed creating SDL2 window: " + std::string(SDL_GetError()));
	}

	_window = std::move(window);
}

SDL_Window* GameWindow::GetHandle() const
{
	return _window.get();
}

void GameWindow::GetNativeHandles(void*& native_window, void*& native_display) const
{
	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	if (!SDL_GetWindowWMInfo(_window.get(), &wmi))
	{
		throw std::runtime_error("Failed getting native window handles: " + std::string(SDL_GetError()));
	}

	// Linux
#if defined(SDL_VIDEO_DRIVER_WAYLAND)
	if (wmi.subsystem == SDL_SYSWM_WAYLAND)
	{
		auto win_impl = static_cast<wl_egl_window*>(SDL_GetWindowData(_window.get(), "wl_egl_window"));
		if (!win_impl)
		{
			int width, height;
			SDL_GetWindowSize(_window.get(), &width, &height);
			struct wl_surface* surface = wmi.info.wl.surface;
			if (!surface)
			{
				throw std::runtime_error("Failed getting native window handles: " + std::string(SDL_GetError()));
			}
			win_impl = wl_egl_window_create(surface, width, height);
			SDL_SetWindowData(_window.get(), "wl_egl_window", win_impl);
		}
		native_window = reinterpret_cast<void*>(win_impl);
		native_display = wmi.info.wl.display;
	}
	else
#endif // defined(SDL_VIDEO_DRIVER_WAYLAND)
#if defined(SDL_VIDEO_DRIVER_X11)
	    if (wmi.subsystem == SDL_SYSWM_X11)
	{
		native_window = reinterpret_cast<void*>(wmi.info.x11.window);
		native_display = wmi.info.x11.display;
	}
	else
#endif // defined(SDL_VIDEO_DRIVER_X11) \
       // Mac
#if defined(SDL_VIDEO_DRIVER_COCOA)
	    if (wmi.subsystem == SDL_SYSWM_COCOA)
	{
		native_window = wmi.info.cocoa.window;
		native_display = nullptr;
	}
	else
#endif // defined(SDL_VIDEO_DRIVER_COCOA) \
       // Windows
#if defined(SDL_VIDEO_DRIVER_WINDOWS)
	    if (wmi.subsystem == SDL_SYSWM_WINDOWS)
	{
		native_window = wmi.info.win.window;
		native_display = nullptr;
	}
	else
#endif // defined(SDL_VIDEO_DRIVER_WINDOWS) \
       // Steam Link
#if defined(SDL_VIDEO_DRIVER_VIVANTE)
	    if (wmi.subsystem == SDL_SYSWM_VIVANTE)
	{
		native_window = wmi.info.vivante.window;
		native_display = wmi.info.vivante.display;
	}
	else
#endif // defined(SDL_VIDEO_DRIVER_VIVANTE)
	{
		throw std::runtime_error("Unsupported platform or window manager: " + std::to_string(wmi.subsystem));
	}
}

bool GameWindow::IsOpen() const
{
	return _window != nullptr;
}

float GameWindow::GetBrightness() const
{
	return SDL_GetWindowBrightness(_window.get());
}

void GameWindow::SetBrightness(float brightness)
{
	if (SDL_SetWindowBrightness(_window.get(), brightness))
	{
		throw std::runtime_error("SDL_SetWindowBrightness Error: " + std::string(SDL_GetError()));
	}
}

uint32_t GameWindow::GetID() const
{
	return SDL_GetWindowID(_window.get());
}

uint32_t GameWindow::GetFlags() const
{
	return SDL_GetWindowFlags(_window.get());
}

void GameWindow::GrabInput(bool b)
{
	SDL_SetWindowGrab(_window.get(), b ? SDL_TRUE : SDL_FALSE);
}

void GameWindow::SetMousePosition(int x, int y)
{
	SDL_WarpMouseInWindow(_window.get(), x, y);
}

bool GameWindow::IsInputGrabbed() const
{
	return SDL_GetWindowGrab(_window.get()) != SDL_FALSE;
}

std::string GameWindow::GetTitle() const
{
	return SDL_GetWindowTitle(_window.get());
}

void GameWindow::SetTitle(const std::string& str)
{
	SDL_SetWindowTitle(_window.get(), str.c_str());
}

float GameWindow::GetAspectRatio() const
{
	int width, height;
	SDL_GetWindowSize(_window.get(), &width, &height);

	return (float)width / (float)height;
}

void GameWindow::SetPosition(int x, int y)
{
	SDL_SetWindowPosition(_window.get(), x, y);
}

void GameWindow::GetPosition(int& x, int& y) const
{
	SDL_GetWindowPosition(_window.get(), &x, &y);
}

void GameWindow::SetMinimumSize(int width, int height)
{
	SDL_SetWindowMinimumSize(_window.get(), width, height);
}

void GameWindow::GetMinimumSize(int& width, int& height) const
{
	SDL_GetWindowMinimumSize(_window.get(), &width, &height);
}

void GameWindow::SetMaximumSize(int width, int height)
{
	SDL_SetWindowMaximumSize(_window.get(), width, height);
}

void GameWindow::GetMaximumSize(int& width, int& height) const
{
	SDL_GetWindowMaximumSize(_window.get(), &width, &height);
}

void GameWindow::SetSize(int width, int height)
{
	SDL_SetWindowSize(_window.get(), width, height);
}

void GameWindow::GetSize(int& width, int& height) const
{
	SDL_GetWindowSize(_window.get(), &width, &height);
}

void GameWindow::Show()
{
	SDL_ShowWindow(_window.get());
}

void GameWindow::Hide()
{
	SDL_HideWindow(_window.get());
}

void GameWindow::Maximise()
{
	SDL_MaximizeWindow(_window.get());
}

void GameWindow::Minimise()
{
	SDL_MinimizeWindow(_window.get());
}

void GameWindow::Restore()
{
	SDL_RestoreWindow(_window.get());
}

void GameWindow::Raise()
{
	SDL_RaiseWindow(_window.get());
}

void GameWindow::SetBordered(bool b)
{
	SDL_SetWindowBordered(_window.get(), b ? SDL_TRUE : SDL_FALSE);
}

void GameWindow::SetFullscreen(bool b)
{
	// todo: use DisplayMode
	if (SDL_SetWindowFullscreen(_window.get(), b ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0))
	{
		throw std::runtime_error("SDL_SetWindowFullscreen Error: " + std::string(SDL_GetError()));
	}
}

void GameWindow::Close()
{
	_window.reset(nullptr);
}

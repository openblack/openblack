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

#include "GameWindow.h"

#include <iostream>
#include <sstream>
#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#include <spdlog/spdlog.h>

using namespace OpenBlack;

GameWindow::GameWindow(const std::string& title, const SDL_DisplayMode& display, DisplayMode displaymode):
    GameWindow::GameWindow(title, display.w, display.h, displaymode)
{
}

GameWindow::GameWindow(const std::string& title, int width, int height, DisplayMode displaymode)
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

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_ALLOW_HIGHDPI;

	if (displaymode == DisplayMode::Fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN;
	else if (displaymode == DisplayMode::Borderless)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	auto window = std::unique_ptr<SDL_Window, SDLDestroyer>(SDL_CreateWindow(
	    title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	    width, height, flags));

	if (window == nullptr)
	{
		throw std::runtime_error("Failed creating window: " + std::string(SDL_GetError()));
	}
	_window = std::move(window);

	// Create a debug context?
	bool useDebug = true;
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, useDebug ? SDL_GL_CONTEXT_DEBUG_FLAG : 0);

	auto context = SDL_GL_CreateContext(_window.get());

	if (context == nullptr)
	{
		throw std::runtime_error("Could not create OpenGL context: " + std::string(SDL_GetError()));
	}
	else
	{
		_glcontext = std::unique_ptr<SDL_GLContext, SDLDestroyer>(&context);
	}

	spdlog::info("OpenGL context successfully created.");
	
	int majorVersion, minorVersion;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &majorVersion);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minorVersion);

	spdlog::info("OpenGL version: {}.{}", majorVersion, minorVersion);

	// initalize glew
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	#ifdef GLEW_ERROR_NO_GLX_DISPLAY
	if (GLEW_ERROR_NO_GLX_DISPLAY == err)
	{
		spdlog::warn("GLEW couldn't open GLX display");
	}
	else
	#endif
	if (GLEW_OK != err)
	{
		std::stringstream error;
		error << "glewInit failed: " << glewGetErrorString(err) << std::endl;
		throw std::runtime_error(error.str());
	}

	spdlog::info("Using GLEW {0}", glewGetString(GLEW_VERSION));
}

SDL_Window* GameWindow::GetHandle() const
{
	return _window.get();
}

SDL_GLContext* GameWindow::GetGLContext() const
{
	return _glcontext.get();
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

bool GameWindow::IsInputGrabbed()
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

int GameWindow::GetSwapInterval()
{
	return SDL_GL_GetSwapInterval();
}

void GameWindow::SetSwapInterval(int interval)
{
	// todo: throw on error
	SDL_GL_SetSwapInterval(interval);
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

void GameWindow::GetPosition(int& x, int& y)
{
	SDL_GetWindowPosition(_window.get(), &x, &y);
}

void GameWindow::SetMinimumSize(int width, int height)
{
	SDL_SetWindowMinimumSize(_window.get(), width, height);
}

void GameWindow::GetMinimumSize(int& width, int& height)
{
	SDL_GetWindowMinimumSize(_window.get(), &width, &height);
}

void GameWindow::SetMaximumSize(int width, int height)
{
	SDL_SetWindowMaximumSize(_window.get(), width, height);
}

void GameWindow::GetMaximumSize(int& width, int& height)
{
	SDL_GetWindowMaximumSize(_window.get(), &width, &height);
}

void GameWindow::SetSize(int width, int height)
{
	SDL_SetWindowSize(_window.get(), width, height);
}

void GameWindow::GetSize(int& width, int& height)
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
	_glcontext.reset(nullptr);
	_window.reset(nullptr);
}

void GameWindow::SwapWindow()
{
	SDL_GL_SwapWindow(_window.get());
}

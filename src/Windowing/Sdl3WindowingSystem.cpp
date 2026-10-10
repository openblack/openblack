/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "Sdl3WindowingSystem.h"

#include <utility>

#include <SDL3/SDL.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_video.h>
#include <spdlog/spdlog.h>

using namespace openblack::windowing;

void Sdl3WindowingSystem::SDLDestroyer::operator()(SDL_Window* window) const
{
	SDL_DestroyWindow(window);
}

Sdl3WindowingSystem::Sdl3WindowingSystem(const std::string& title, int width, int height, DisplayMode displayMode,
                                         uint32_t extraFlags)
{
	const int linkedVersion = SDL_GetVersion();

	SPDLOG_LOGGER_INFO(spdlog::get("graphics"), "Initializing SDL...");
	SPDLOG_LOGGER_INFO(spdlog::get("graphics"), "SDL Version/Compiled {}.{}.{}", //
	                   SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);
	SPDLOG_LOGGER_INFO(spdlog::get("graphics"), "SDL Version/Linked {}.{}.{}", //
	                   SDL_VERSIONNUM_MAJOR(linkedVersion), SDL_VERSIONNUM_MINOR(linkedVersion),
	                   SDL_VERSIONNUM_MICRO(linkedVersion));

	// Initialize SDL
	SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "x11");
	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		throw std::runtime_error("Could not initialize SDL: " + std::string(SDL_GetError()));
	}

	SDL_HideCursor();
	// SDL_SetHint(SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN, "1");

	uint32_t flags = SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | extraFlags;
	if (displayMode == DisplayMode::Fullscreen)
	{
		flags |= SDL_WINDOW_FULLSCREEN;
	}
	else if (displayMode == DisplayMode::Borderless)
	{
		flags |= SDL_WINDOW_BORDERLESS;
	}

	auto window = std::unique_ptr<SDL_Window, SDLDestroyer>(SDL_CreateWindow(title.c_str(), width, height, flags));

	if (window == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create SDL3 window: '{}'", SDL_GetError());
		throw std::runtime_error("Failed creating SDL3 window: " + std::string(SDL_GetError()));
	}

	_window = std::move(window);
}

void* Sdl3WindowingSystem::GetHandle() const
{
	return _window.get();
}

bool Sdl3WindowingSystem::IsOpen() const
{
	return _window != nullptr;
}

uint32_t Sdl3WindowingSystem::GetID() const
{
	return SDL_GetWindowID(_window.get());
}

uint32_t Sdl3WindowingSystem::GetFlags() const
{
	return static_cast<uint32_t>(SDL_GetWindowFlags(_window.get()));
}

Sdl3WindowingSystem& Sdl3WindowingSystem::GrabInput(bool b)
{
	SDL_SetWindowMouseGrab(_window.get(), b);
	SDL_SetWindowKeyboardGrab(_window.get(), b);
	return *this;
}

Sdl3WindowingSystem& Sdl3WindowingSystem::SetMousePosition(glm::ivec2 position)
{
	SDL_WarpMouseInWindow(_window.get(), static_cast<float>(position.x), static_cast<float>(position.y));
	return *this;
}

bool Sdl3WindowingSystem::IsInputGrabbed() const
{
	return SDL_GetWindowMouseGrab(_window.get()) || SDL_GetWindowKeyboardGrab(_window.get());
}

std::string Sdl3WindowingSystem::GetTitle() const
{
	return SDL_GetWindowTitle(_window.get());
}

Sdl3WindowingSystem& Sdl3WindowingSystem::SetTitle(const std::string& str)
{
	SDL_SetWindowTitle(_window.get(), str.c_str());
	return *this;
}

float Sdl3WindowingSystem::GetAspectRatio() const
{
	int width;
	int height;
	SDL_GetWindowSize(_window.get(), &width, &height);

	return static_cast<float>(width) / static_cast<float>(height);
}

Sdl3WindowingSystem& Sdl3WindowingSystem::SetPosition(glm::ivec2 position)
{
	SDL_SetWindowPosition(_window.get(), position.x, position.y);
	return *this;
}

glm::ivec2 Sdl3WindowingSystem::GetPosition() const
{
	glm::ivec2 result;
	SDL_GetWindowPosition(_window.get(), &result.x, &result.y);
	return result;
}

Sdl3WindowingSystem& Sdl3WindowingSystem::SetMinimumSize(glm::ivec2 size)
{
	SDL_SetWindowMinimumSize(_window.get(), size.x, size.y);
	return *this;
}

glm::ivec2 Sdl3WindowingSystem::GetMinimumSize() const
{
	glm::ivec2 result;
	SDL_GetWindowMinimumSize(_window.get(), &result.x, &result.y);
	return result;
}

Sdl3WindowingSystem& Sdl3WindowingSystem::SetMaximumSize(glm::ivec2 size)
{
	SDL_SetWindowMaximumSize(_window.get(), size.x, size.y);
	return *this;
}

glm::ivec2 Sdl3WindowingSystem::GetMaximumSize() const
{
	glm::ivec2 result;
	SDL_GetWindowMaximumSize(_window.get(), &result.x, &result.y);
	return result;
}

Sdl3WindowingSystem& Sdl3WindowingSystem::SetSize(glm::ivec2 size)
{
	SDL_SetWindowSize(_window.get(), size.x, size.y);
	return *this;
}

glm::ivec2 Sdl3WindowingSystem::GetSize() const
{
	glm::ivec2 result;
	SDL_GetWindowSize(_window.get(), &result.x, &result.y);
	return result;
}

Sdl3WindowingSystem& Sdl3WindowingSystem::Show()
{
	SDL_ShowWindow(_window.get());
	return *this;
}

Sdl3WindowingSystem& Sdl3WindowingSystem::Hide()
{
	SDL_HideWindow(_window.get());
	return *this;
}

Sdl3WindowingSystem& Sdl3WindowingSystem::Maximise()
{
	SDL_MaximizeWindow(_window.get());
	return *this;
}

Sdl3WindowingSystem& Sdl3WindowingSystem::Minimise()
{
	SDL_MinimizeWindow(_window.get());
	return *this;
}

Sdl3WindowingSystem& Sdl3WindowingSystem::Restore()
{
	SDL_RestoreWindow(_window.get());
	return *this;
}

Sdl3WindowingSystem& Sdl3WindowingSystem::Raise()
{
	SDL_RaiseWindow(_window.get());
	return *this;
}

WindowingInterface& Sdl3WindowingSystem::SetDisplayMode(DisplayMode mode)
{
	switch (mode)
	{
	case DisplayMode::Windowed:
		SDL_SetWindowFullscreen(_window.get(), false);
		SDL_SetWindowBordered(_window.get(), true);
		break;
	case DisplayMode::Fullscreen:
		SDL_SetWindowFullscreen(_window.get(), true);
		SDL_SetWindowBordered(_window.get(), true);
		break;
	case DisplayMode::Borderless:
		SDL_SetWindowFullscreen(_window.get(), true);
		SDL_SetWindowBordered(_window.get(), false);
		break;
	default:
		assert(false);
		std::unreachable();
	}
	return *this;
}

Sdl3WindowingSystem& Sdl3WindowingSystem::Close()
{
	_window.reset(nullptr);
	return *this;
}

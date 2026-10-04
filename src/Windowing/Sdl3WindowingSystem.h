/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

// TODO: Put a header guard against direct access

#include <cstdint>

#include <memory>
#include <string>

#include <glm/vec2.hpp>

#include "WindowingInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp"
#endif

struct SDL_Window;

namespace openblack::windowing
{

class Sdl3WindowingSystem final: public WindowingInterface
{
	struct SDLDestroyer
	{
		void operator()(SDL_Window* window) const;
	};

public:
	Sdl3WindowingSystem(const std::string& title, int width, int height, DisplayMode displayMode, uint32_t extraFlags);

	[[nodiscard]] void* GetHandle() const final;
	[[nodiscard]] NativeHandles GetNativeHandles() const final;
	[[nodiscard]] uint32_t GetID() const final;
	[[nodiscard]] glm::ivec2 GetSize() const final;
	[[nodiscard]] float GetAspectRatio() const final;

	WindowingInterface& SetDisplayMode(DisplayMode mode) final;

private:
	[[nodiscard]] bool IsOpen() const;
	[[nodiscard]] uint32_t GetFlags() const;
	Sdl3WindowingSystem& GrabInput(bool b = true);
	Sdl3WindowingSystem& SetMousePosition(glm::ivec2 position);
	[[nodiscard]] bool IsInputGrabbed() const;

	Sdl3WindowingSystem& SetTitle(const std::string& str);
	[[nodiscard]] std::string GetTitle() const;

	Sdl3WindowingSystem& Show();
	Sdl3WindowingSystem& Hide();

	Sdl3WindowingSystem& SetPosition(glm::ivec2 position);
	[[nodiscard]] glm::ivec2 GetPosition() const;
	Sdl3WindowingSystem& SetSize(glm::ivec2 size);
	Sdl3WindowingSystem& SetMinimumSize(glm::ivec2 size);
	[[nodiscard]] glm::ivec2 GetMinimumSize() const;
	Sdl3WindowingSystem& SetMaximumSize(glm::ivec2 size);
	[[nodiscard]] glm::ivec2 GetMaximumSize() const;

	Sdl3WindowingSystem& Minimise();
	Sdl3WindowingSystem& Maximise();
	Sdl3WindowingSystem& Restore();
	Sdl3WindowingSystem& Raise();

	Sdl3WindowingSystem& Close();

	std::unique_ptr<SDL_Window, SDLDestroyer> _window;
};
} // namespace openblack::windowing

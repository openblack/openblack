/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
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
#pragma message("Locator interface implementations should only be included in Locator.cpp")
#endif

struct SDL_Window;

namespace openblack::windowing
{

class Sdl2WindowingSystem final: public WindowingInterface
{
	struct SDLDestroyer
	{
		void operator()(SDL_Window* window) const;
	};

public:
	Sdl2WindowingSystem(const std::string& title, int width, int height, DisplayMode displayMode, uint32_t extraFlags);

	[[nodiscard]] void* GetHandle() const final;
	[[nodiscard]] NativeHandles GetNativeHandles() const final;
	[[nodiscard]] uint32_t GetID() const final;
	[[nodiscard]] glm::ivec2 GetSize() const final;
	[[nodiscard]] float GetAspectRatio() const final;

	WindowingInterface& SetDisplayMode(DisplayMode mode) final { return SetFullscreen(mode != DisplayMode::Windowed); }

private:
	[[nodiscard]] bool IsOpen() const;
	[[nodiscard]] float GetBrightness() const;
	Sdl2WindowingSystem& SetBrightness(float bright);
	[[nodiscard]] uint32_t GetFlags() const;
	Sdl2WindowingSystem& GrabInput(bool b = true);
	Sdl2WindowingSystem& SetMousePosition(glm::ivec2 position);
	[[nodiscard]] bool IsInputGrabbed() const;

	Sdl2WindowingSystem& SetTitle(const std::string& str);
	[[nodiscard]] std::string GetTitle() const;

	Sdl2WindowingSystem& Show();
	Sdl2WindowingSystem& Hide();

	Sdl2WindowingSystem& SetPosition(glm::ivec2 position);
	[[nodiscard]] glm::ivec2 GetPosition() const;
	Sdl2WindowingSystem& SetSize(glm::ivec2 size);
	Sdl2WindowingSystem& SetMinimumSize(glm::ivec2 size);
	[[nodiscard]] glm::ivec2 GetMinimumSize() const;
	Sdl2WindowingSystem& SetMaximumSize(glm::ivec2 size);
	[[nodiscard]] glm::ivec2 GetMaximumSize() const;

	Sdl2WindowingSystem& Minimise();
	Sdl2WindowingSystem& Maximise();
	Sdl2WindowingSystem& Restore();
	Sdl2WindowingSystem& Raise();
	Sdl2WindowingSystem& SetBordered(bool b = true);
	Sdl2WindowingSystem& SetFullscreen(bool f = true);

	Sdl2WindowingSystem& Close();

	std::unique_ptr<SDL_Window, SDLDestroyer> _window;
};
} // namespace openblack::windowing

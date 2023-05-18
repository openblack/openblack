/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <memory>
#include <string>

#include <glm/vec2.hpp>

struct SDL_Window;

namespace openblack
{
enum DisplayMode
{
	Windowed,
	Fullscreen,
	Borderless
};

class GameWindow
{
	struct SDLDestroyer
	{
		void operator()(SDL_Window* window) const;
	};

public:
	GameWindow(const std::string& title, int width, int height, DisplayMode displayMode, uint32_t extraFlags);

	[[nodiscard]] SDL_Window* GetHandle() const;
	struct NativeHandles
	{
		void* nativeWindow;
		void* nativeDisplay;
	};
	[[nodiscard]] NativeHandles GetNativeHandles() const;

	[[nodiscard]] bool IsOpen() const;
	[[nodiscard]] float GetBrightness() const;
	GameWindow& SetBrightness(float bright);
	[[nodiscard]] uint32_t GetID() const;
	[[nodiscard]] uint32_t GetFlags() const;
	GameWindow& GrabInput(bool b = true);
	GameWindow& SetMousePosition(glm::ivec2 position);
	[[nodiscard]] bool IsInputGrabbed() const;
	[[nodiscard]] float GetAspectRatio() const;

	GameWindow& SetTitle(const std::string& str);
	[[nodiscard]] std::string GetTitle() const;

	GameWindow& Show();
	GameWindow& Hide();

	GameWindow& SetPosition(glm::ivec2 position);
	[[nodiscard]] glm::ivec2 GetPosition() const;
	GameWindow& SetSize(glm::ivec2 size);
	[[nodiscard]] glm::ivec2 GetSize() const;
	GameWindow& SetMinimumSize(glm::ivec2 size);
	[[nodiscard]] glm::ivec2 GetMinimumSize() const;
	GameWindow& SetMaximumSize(glm::ivec2 size);
	[[nodiscard]] glm::ivec2 GetMaximumSize() const;

	GameWindow& Minimise();
	GameWindow& Maximise();
	GameWindow& Restore();
	GameWindow& Raise();
	GameWindow& SetBordered(bool b = true);
	GameWindow& SetFullscreen(bool f = true);

	GameWindow& Close();

private:
	std::unique_ptr<SDL_Window, SDLDestroyer> _window;
};
} // namespace openblack

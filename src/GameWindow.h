/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <cstdint>
#include <memory>
#include <string>

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
	GameWindow(const std::string& title, int width, int height, DisplayMode displayMode);

	[[nodiscard]] SDL_Window* GetHandle() const;
	void GetNativeHandles(void*& nativeWindow, void*& nativeDisplay) const;

	[[nodiscard]] bool IsOpen() const;
	[[nodiscard]] float GetBrightness() const;
	void SetBrightness(float bright);
	[[nodiscard]] uint32_t GetID() const;
	[[nodiscard]] uint32_t GetFlags() const;
	void GrabInput(bool b = true);
	void SetMousePosition(int x, int y);
	[[nodiscard]] bool IsInputGrabbed() const;
	[[nodiscard]] float GetAspectRatio() const;

	void SetTitle(const std::string& str);
	[[nodiscard]] std::string GetTitle() const;

	void Show();
	void Hide();

	void SetPosition(int x, int y);
	void GetPosition(int& x, int& y) const;
	void SetSize(int width, int height);
	void GetSize(int& width, int& height) const;
	void SetMinimumSize(int width, int height);
	void GetMinimumSize(int& width, int& height) const;
	void SetMaximumSize(int width, int height);
	void GetMaximumSize(int& width, int& height) const;

	void Minimise();
	void Maximise();
	void Restore();
	void Raise();
	void SetBordered(bool b = true);
	void SetFullscreen(bool f = true);

	void Close();

private:
	std::unique_ptr<SDL_Window, SDLDestroyer> _window;
};
} // namespace openblack

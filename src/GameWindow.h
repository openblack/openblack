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

#pragma once

#include <SDL.h>
#include <cstdint>
#include <memory>
#include <string>

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
		void operator()(SDL_Window* window) const { SDL_DestroyWindow(window); }
	};

public:
	GameWindow(const std::string& title, const SDL_DisplayMode& display, DisplayMode displaymode);
	GameWindow(const std::string& title, int width, int height, DisplayMode displaymode);

	[[nodiscard]] SDL_Window* GetHandle() const;
	void GetNativeHandles(void*& native_window, void*& native_display) const;

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

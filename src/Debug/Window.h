/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <string>

#include <imgui.h>

union SDL_Event;

namespace openblack::debug::gui
{

class Window
{
public:
	Window(std::string name, ImVec2 defaultSize) noexcept;
	Window(const Window&) = delete;
	virtual ~Window() noexcept;

	void WindowUpdate() noexcept;
	void WindowDraw() noexcept;
	/// Hands the event to the window, and whether the window takes it for itself, keeping it from the game
	bool WindowProcessEvent(const SDL_Event& event) noexcept;
	[[nodiscard]] bool IsOpen() const noexcept { return _open; }
	[[nodiscard]] const std::string& GetName() const noexcept { return _name; }
	virtual void Open() noexcept;
	virtual void Close() noexcept;
	virtual void Toggle() noexcept;

protected:
	virtual void Draw() noexcept = 0;
	virtual void Update() noexcept = 0;
	/// Once a frame whether the window is open or not, for what goes on with it closed
	virtual void UpdateAlways() noexcept {}
	virtual void ProcessEventOpen(const SDL_Event& event) noexcept = 0;
	virtual void ProcessEventAlways(const SDL_Event& event) noexcept = 0;
	/// Whether the open window takes the event for itself, such as a click on the land it acts on
	[[nodiscard]] virtual bool TakesEvent([[maybe_unused]] const SDL_Event& event) const noexcept { return false; }

private:
	const std::string _name;
	const ImVec2 _defaultSize;
	bool _open {false};
};

} // namespace openblack::debug::gui

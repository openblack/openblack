/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <string>

#include <imgui.h>

union SDL_Event;

namespace openblack
{

class Game;
class Renderer;

namespace gui
{

class DebugWindow
{
public:
	DebugWindow(std::string name, ImVec2 defaultSize);
	DebugWindow(const DebugWindow&) = delete;
	virtual ~DebugWindow();

	void WindowUpdate(Game& game, const Renderer& renderer);
	void WindowDraw(Game& game);
	void WindowProcessEvent(const SDL_Event& event);
	bool IsOpen() const { return _open; }
	const std::string& GetName() const { return _name; }
	virtual void Open();
	virtual void Close();
	virtual void Toggle();

protected:
	virtual void Draw(Game& game) = 0;
	virtual void Update(Game& game, const Renderer& renderer) = 0;
	virtual void ProcessEventOpen(const SDL_Event& event) = 0;
	virtual void ProcessEventAlways(const SDL_Event& event) = 0;

private:
	const std::string _name;
	const ImVec2 _defaultSize;
	bool _open;
};

} // namespace gui

} // namespace openblack

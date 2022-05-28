/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "DebugWindow.h"

#include <utility>

using namespace openblack::gui;

DebugWindow::DebugWindow(std::string name, ImVec2 defaultSize)
    : _name(std::move(name))
    , _defaultSize(defaultSize)
    , _open(false)
{
}

DebugWindow::~DebugWindow() = default;

void DebugWindow::Open()
{
	_open = true;
}

void DebugWindow::Close()
{
	_open = false;
}

void DebugWindow::Toggle()
{
	if (IsOpen())
	{
		Close();
	}
	else
	{
		Open();
	}
}

void DebugWindow::WindowUpdate(openblack::Game& game, const openblack::Renderer& renderer)
{
	if (_open)
	{
		Update(game, renderer);
	}
}

void DebugWindow::WindowDraw(Game& game)
{
	if (_open)
	{
		ImGui::SetNextWindowSize(_defaultSize, ImGuiCond_FirstUseEver);
		ImGui::Begin(_name.c_str(), &_open);
		Draw(game);
		ImGui::End();
	}
}

void DebugWindow::WindowProcessEvent(const SDL_Event& event)
{
	ProcessEventAlways(event);
	if (_open)
	{
		ProcessEventOpen(event);
	}
}

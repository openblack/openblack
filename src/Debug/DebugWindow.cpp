/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <utility>

#include "Window.h"

using namespace openblack::debug::gui;

Window::Window(std::string name, ImVec2 defaultSize)
    : _name(std::move(name))
    , _defaultSize(defaultSize)
{
}

Window::~Window() = default;

void Window::Open()
{
	_open = true;
}

void Window::Close()
{
	_open = false;
}

void Window::Toggle()
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

void Window::WindowUpdate(openblack::Game& game, const openblack::Renderer& renderer)
{
	if (_open)
	{
		Update(game, renderer);
	}
}

void Window::WindowDraw(Game& game)
{
	if (_open)
	{
		ImGui::SetNextWindowSize(_defaultSize, ImGuiCond_FirstUseEver);
		ImGui::Begin(_name.c_str(), &_open);
		Draw(game);
		ImGui::End();
	}
}

void Window::WindowProcessEvent(const SDL_Event& event)
{
	ProcessEventAlways(event);
	if (_open)
	{
		ProcessEventOpen(event);
	}
}

/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Window.h"

#include <utility>

using namespace openblack::debug::gui;

Window::Window(std::string name, ImVec2 defaultSize) noexcept
    : _name(std::move(name))
    , _defaultSize(defaultSize)
{
}

Window::~Window() noexcept = default;

void Window::Open() noexcept
{
	_open = true;
}

void Window::Close() noexcept
{
	_open = false;
}

void Window::Toggle() noexcept
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

void Window::WindowUpdate() noexcept
{
	if (_open)
	{
		Update();
	}
}

void Window::WindowDraw() noexcept
{
	if (_open)
	{
		ImGui::SetNextWindowSize(_defaultSize, ImGuiCond_FirstUseEver);
		ImGui::Begin(_name.c_str(), &_open);
		Draw();
		ImGui::End();
	}
}

void Window::WindowProcessEvent(const SDL_Event& event) noexcept
{
	ProcessEventAlways(event);
	if (_open)
	{
		ProcessEventOpen(event);
	}
}

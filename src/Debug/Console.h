/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <imgui.h>

#include "Window.h"

struct ImGuiInputTextCallbackData;

namespace openblack::debug::gui
{

class Console final: public Window
{
public:
	Console() noexcept;
	void Open() noexcept override;

protected:
	void Draw() noexcept override;
	void Update() noexcept override;
	void ProcessEventOpen(const SDL_Event& event) noexcept override;
	void ProcessEventAlways(const SDL_Event& event) noexcept override;

private:
	void AddLog(const char* fmt, ...) noexcept;
	void ExecCommand(const std::string& commandLine) noexcept;
	int InputTextCallback(ImGuiInputTextCallbackData* data) noexcept;

	bool _reclaimFocus {false};
	bool _insertHandPosition {false};
	int _inputCursorPosition {0};
	std::array<char, 256> _inputBuffer {0};
	std::vector<std::string> _items;
	std::vector<std::pair<std::string, std::string>> _commands;
	std::vector<std::string> _history;
	std::string _partial;
	std::optional<size_t> _historyPos;
};

} // namespace openblack::debug::gui

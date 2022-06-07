/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

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

namespace openblack
{
namespace lhscriptx
{
class Script;
}

namespace gui
{

class Console: public Window
{
public:
	Console();
	void Open() override;

protected:
	void Draw(Game& game) override;
	void Update(Game& game, const Renderer& renderer) override;
	void ProcessEventOpen(const SDL_Event& event) override;
	void ProcessEventAlways(const SDL_Event& event) override;

private:
	void AddLog(const char* fmt, ...);
	void ExecCommand(const std::string& commandLine, Game& game);
	int InputTextCallback(ImGuiInputTextCallbackData* data);

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

} // namespace gui

} // namespace openblack

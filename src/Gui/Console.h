/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "DebugWindow.h"

#include <imgui.h>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

struct ImGuiInputTextCallbackData;

namespace openblack
{
namespace lhscriptx
{
class Script;
}

namespace gui
{

class Console: public DebugWindow
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
	void ExecCommand(const std::string& command_line, Game& game);
	int InputTextCallback(ImGuiInputTextCallbackData* data);

	bool _reclaim_focus;
	bool _insert_hand_position;
	int _input_cursor_position;
	std::array<char, 256> _input_buffer;
	std::vector<std::string> _items;
	std::vector<std::pair<std::string, std::string>> _commands;
	std::vector<std::string> _history;
	std::string _partial;
	std::optional<size_t> _history_pos;
};

} // namespace gui

} // namespace openblack

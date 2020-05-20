/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <imgui.h>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

struct ImGuiInputTextCallbackData;
union SDL_Event;

namespace openblack
{

class Game;

namespace lhscriptx
{
class Script;
}

class Console
{
public:
	Console();
	virtual ~Console();
	void Open();
	void Toggle();
	void DrawWindow(Game& game);

	void ProcessEventSdl2(const SDL_Event& event);

private:
	void AddLog(const char* fmt, ...);
	void ExecCommand(const std::string& command_line, Game& game);
	int InputTextCallback(ImGuiInputTextCallbackData* data);

	bool _open;
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
} // namespace openblack

/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Console.h"

#include <algorithm>

#include <SDL.h>
#include <imgui.h>

#include "3D/Camera.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/DynamicsSystem.h"
#include "Game.h"
#include "LHScriptX/FeatureScriptCommands.h"
#include "LHScriptX/Script.h"

using namespace openblack;
using namespace openblack::gui;

Console::Console()
    : DebugWindow("Console", ImVec2(520, 600))
    // , _items {"Welcome message goes here"}
    , _commands {
          {"help", "    - Display list of possible commands."},
          {"history", " - Display previously executed commands."},
          {"clear", "   - Clear the output log."},
      }
{
	// TODO: Add custom spdlog sink here
	for (const auto& signature : lhscriptx::FeatureScriptCommands::Signatures)
	{
		std::string details = "";
		for (bool skip = true; const auto param : signature.parameters)
		{
			if (!skip)
			{
				details += ", ";
				skip = false;
			}
			switch (param)
			{
			case lhscriptx::ParameterType::None:
				details += "none";
				break;
			case lhscriptx::ParameterType::String:
				details += "string";
				break;
			case lhscriptx::ParameterType::Float:
				details += "float";
				break;
			case lhscriptx::ParameterType::Number:
				details += "number";
				break;
			case lhscriptx::ParameterType::Vector:
				details += "vector";
				break;
			}
		}
		details += ")";
		_commands.emplace_back(std::string(signature.name.data(), signature.name.size()) + "(", details);
	}
}

void Console::Open()
{
	DebugWindow::Open();
	_reclaimFocus = true;
}

int Console::InputTextCallback(ImGuiInputTextCallbackData* data)
{
	switch (data->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackCompletion:
	{
		// Example of TEXT COMPLETION

		// Locate beginning of current word
		const char* word_end = data->Buf + data->CursorPos;
		const char* word_start = word_end;
		while (word_start > data->Buf)
		{
			const char c = word_start[-1];
			if (c == ' ' || c == '\t' || c == ',' || c == ';')
			{
				break;
			}
			word_start--;
		}

		// Build a list of candidates
		std::vector<std::string> candidates;
		for (auto& Command : _commands)
		{
			if (word_end == word_start ||
			    strncmp(Command.first.c_str(), word_start, static_cast<int>(word_end - word_start)) == 0)
			{
				candidates.push_back(Command.first);
			}
		}

		if (candidates.empty())
		{
			// No match
			AddLog("No match for \"%.*s\"!\n", static_cast<int>(word_end - word_start), word_start);
		}
		else if (candidates.size() == 1)
		{
			// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing
			data->DeleteChars(static_cast<int>(word_start - data->Buf), static_cast<int>(word_end - word_start));
			data->InsertChars(data->CursorPos, candidates[0].c_str());
		}
		else
		{
			// Multiple matches. Complete as much as we can, so inputing "C" will complete to "CL" and display
			// "CLEAR" and "CLASSIFY"
			int match_len = static_cast<int>(word_end - word_start);
			for (;;)
			{
				char c = 0;
				bool all_candidates_matches = true;
				for (size_t i = 0; i < candidates.size() && all_candidates_matches; i++)
				{
					if (i == 0)
					{
						c = candidates[i][match_len];
					}
					else if (c == 0 || c != candidates[i][match_len])
					{
						all_candidates_matches = false;
					}
				}
				if (!all_candidates_matches)
				{
					break;
				}
				match_len++;
			}

			if (match_len > 0)
			{
				data->DeleteChars(static_cast<int>(word_start - data->Buf), static_cast<int>(word_end - word_start));
				data->InsertChars(data->CursorPos, candidates[0].c_str(), candidates[0].c_str() + match_len);
			}

			// List matches
			AddLog("Possible matches:\n");
			for (auto& candidate : candidates)
			{
				AddLog("- %s\n", candidate.c_str());
			}
		}

		break;
	}
	case ImGuiInputTextFlags_CallbackHistory:
	{
		// Example of HISTORY
		const auto prev_historyPos = _historyPos;
		if (data->EventKey == ImGuiKey_UpArrow)
		{
			if (!_historyPos.has_value())
			{
				if (!_history.empty())
				{
					_historyPos = _history.size() - 1;
				}
				_partial = _inputBuffer.data();
			}
			else if (_historyPos.value() > 0)
			{
				_historyPos.value()--;
			}
		}
		else if (data->EventKey == ImGuiKey_DownArrow)
		{
			if (_historyPos.has_value())
			{
				if (++_historyPos.value() >= _history.size())
				{
					_historyPos.reset();
				}
			}
			else
			{
				_partial = _inputBuffer.data();
			}
		}

		// A better implementation would preserve the data on the current input line along with cursor position.
		if (prev_historyPos != _historyPos)
		{
			auto history_str = _historyPos.has_value() ? _history[*_historyPos] : _partial;
			data->DeleteChars(0, data->BufTextLen);
			data->InsertChars(0, history_str.c_str());
		}
		break;
	}
	case ImGuiInputTextFlags_CallbackAlways:
	{
		_inputCursorPosition = data->CursorPos;
		break;
	}
	}
	return 0;
}

void Console::Draw(Game& game)
{
	ImGuiIO& io = ImGui::GetIO();

	glm::ivec2 screenSize {};
	if (game.GetWindow())
	{
		game.GetWindow()->GetSize(screenSize.x, screenSize.y);
	}
	glm::ivec2 mousePosition {};
	SDL_GetMouseState(&mousePosition.x, &mousePosition.y);
	if (!io.WantCaptureMouse && screenSize.x > 0 && screenSize.y > 0)
	{
		glm::vec3 rayOrigin, rayDirection;
		game.GetCamera().DeprojectScreenToWorld(mousePosition, screenSize, rayOrigin, rayDirection);
		if (auto hit = game.GetDynamicsSystem().RayCastClosestHit(rayOrigin, rayDirection, 1e10f))
		{
			if (hit->second.userData)
			{
				switch (hit->second.type)
				{
				case ecs::systems::RigidBodyType::Terrain:
				{
					// auto landIsland = reinterpret_cast<const LandIsland*>(hit->second.userData);
					auto blockIndex = hit->second.id;
					ImGui::SetTooltip("Block Index: %d", blockIndex);
				}
				break;
				case ecs::systems::RigidBodyType::Entity:
				{
					// auto registry = reinterpret_cast<const openblack::ecs::Registry*>(hit->second.userData);
					auto entity = hit->second.id;
					ImGui::SetTooltip("Entity %d", entity);
				}
				break;
				}
			}
		}
	}

	// As a specific feature guaranteed by the library, after calling Begin() the last Item represent the title bar. So e.g.
	// IsItemHovered() will return true when hovering the title bar. Here we create a context menu only available from the
	// title bar.
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Close Console"))
		{
			Close();
		}
		ImGui::EndPopup();
	}

	ImGui::TextWrapped("Enter 'help' for help, press TAB to use text completion.");

	if (ImGui::SmallButton("Clear"))
	{
		_items.clear();
	}
	ImGui::SameLine();
	bool copy_to_clipboard = ImGui::SmallButton("Copy");

	ImGui::Separator();

	const float footer_height_to_reserve =
	    ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing(); // 1 separator, 1 input text
	ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false,
	                  ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText
	if (ImGui::BeginPopupContextWindow())
	{
		if (ImGui::Selectable("Clear"))
		{
			_items.clear();
		}
		ImGui::EndPopup();
	}

	// Display every line as a separate entry so we can change their color or add custom widgets. If you only want raw text
	// you can use ImGui::TextUnformatted(log.begin(), log.end()); NB- if you have thousands of entries this approach may be
	// too inefficient and may require user-side clipping to only process visible items. You can seek and display only the
	// lines that are visible using the ImGuiListClipper helper, if your elements are evenly spaced and you have cheap
	// random access to the elements. To use the clipper we could replace the 'for (int i = 0; i < Items.Size; i++)' loop
	// with:
	//     ImGuiListClipper clipper(Items.Size);
	//     while (clipper.Step())
	//         for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
	// However, note that you can not use this code as is if a filter is active because it breaks the 'cheap random-access'
	// property. We would need random-access on the post-filtered list. A typical application wanting coarse clipping and
	// filtering may want to pre-compute an array of indices that passed the filtering test, recomputing this array when
	// user changes the filter, and appending newly elements as they are inserted. This is left as a task to the user until
	// we can manage to improve this example code! If your items are of variable size you may want to implement code similar
	// to what ImGuiListClipper does. Or split your data into fixed height items to allow random-seeking into your list.
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
	if (copy_to_clipboard)
	{
		ImGui::LogToClipboard();
	}
	for (const auto& item : _items)
	{
		// Normally you would store more information in your item (e.g. make Items[] an array of structure, store color/type
		// etc.)
		bool pop_color = false;
		if (strstr(item.c_str(), "[error]"))
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
			pop_color = true;
		}
		else if (strncmp(item.c_str(), "# ", 2) == 0)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.6f, 1.0f));
			pop_color = true;
		}
		ImGui::TextUnformatted(item.c_str());
		if (pop_color)
		{
			ImGui::PopStyleColor();
		}
	}
	if (copy_to_clipboard)
		ImGui::LogFinish();

	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
	{
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::PopStyleVar();
	ImGui::EndChild();
	ImGui::Separator();

	// Command-line
	ImGui::PushItemWidth(-1);
	if (ImGui::InputText(
	        "##Input", _inputBuffer.data(), _inputBuffer.size(),
	        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion |
	            ImGuiInputTextFlags_CallbackHistory | ImGuiInputTextFlags_CallbackAlways,
	        [](ImGuiInputTextCallbackData* data) -> int {
		        return reinterpret_cast<Console*>(data->UserData)->InputTextCallback(data);
	        },
	        this))
	{
		std::string s = _inputBuffer.data();

		// trim string
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(), s.end());

		if (s[0])
			ExecCommand(s, game);

		_inputBuffer[0] = '\0';

		_reclaimFocus = true;
	}
	ImGui::PopItemWidth();

	if (_insertHandPosition)
	{
		if (_inputCursorPosition >= 0)
		{
			std::string pre(_inputBuffer.data(), _inputBuffer.data() + _inputCursorPosition);
			std::string post(_inputBuffer.data() + _inputCursorPosition);

			const auto& position = game.GetEntityRegistry().Get<ecs::components::Transform>(game.GetHand()).position;
			snprintf(_inputBuffer.data(), _inputBuffer.size(), "%s%.2f,%.2f%s", pre.c_str(), position.x, position.z,
			         post.c_str());
		}
	}
	_insertHandPosition = false;

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (_reclaimFocus)
	{
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
		_reclaimFocus = false;
	}
}

void Console::Update([[maybe_unused]] Game& game, [[maybe_unused]] const Renderer& renderer) {}

void Console::AddLog(const char* fmt, ...)
{
	// FIXME-OPT
	std::array<char, 1024> buf;
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf.data(), buf.size(), fmt, args);
	buf[buf.size() - 1] = 0;
	va_end(args);
	_items.emplace_back(buf.data());
}

void Console::ExecCommand(const std::string& command_line, Game& game)
{
	AddLog("# %s\n", command_line.c_str());

	// Insert into history. First find match and delete it so it can be pushed to the back. This isn't trying to be smart or
	// optimal.
	_historyPos.reset();
	for (int i = static_cast<int>(_history.size()) - 1; i >= 0; i--)
		if (_history[i] == command_line)
		{
			_history.erase(_history.begin() + i);
			break;
		}
	_history.emplace_back(command_line);

	// Process command
	if (command_line == "clear")
	{
		_items.clear();
	}
	else if (command_line == "help")
	{
		AddLog("Commands:");
		for (auto& Command : _commands)
		{
			AddLog("- %s%s", Command.first.c_str(), Command.second.c_str());
		}
	}
	else if (command_line == "history")
	{
		size_t first = _history.size() - 10;
		for (size_t i = first > 0 ? first : 0; i < _history.size(); i++)
		{
			AddLog("%3zu: %s\n", i, _history[i].c_str());
		}
	}
	else
	{
		try
		{
			lhscriptx::Script script(&game);
			script.Load(command_line);
		}
		catch (std::runtime_error& error)
		{
			AddLog("[error]: %s", error.what());
		}
	}
}

void Console::ProcessEventOpen(const SDL_Event& event)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (event.type)
	{
	case SDL_MOUSEBUTTONDOWN:
		if (event.button.clicks == 2 && !io.WantCaptureMouse)
		{
			_insertHandPosition = true;
		}
		break;
	}
}

void Console::ProcessEventAlways(const SDL_Event& event)
{
	// ImGuiIO& io = ImGui::GetIO();

	switch (event.type)
	{
	case SDL_KEYDOWN:
		if (event.key.keysym.sym == SDLK_BACKQUOTE)
		{
			Toggle();
		}
		break;
	}
}

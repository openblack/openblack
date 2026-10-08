/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ScriptsPanel.h"

#include <charconv>
#include <cstdlib>

#include <algorithm>
#include <filesystem>

#include <LHVM.h>
#include <fmt/format.h>
#include <imgui.h>
#include <imgui_memory_editor.h>
#include <imgui_stdlib.h>
#include <imgui_user.h>

#include "Common/FileDialog.h"
#include "ECS/Systems/ScriptObjectsSystemInterface.h"
#include "Editor/EditorOutline.h"
#include "Editor/EditorStyle.h"
#include "Editor/Scripts/Decompiler.h"
#include "FileSystem/FileSystemInterface.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"
#include "generated/scripting/UnimplementedNatives.h"

namespace openblack::editor
{

using namespace scripts;
using lhvm::VMScript;

namespace
{
enum SideTab : int
{
	k_DetailsTab,
	k_TasksTab,
	k_GlobalsTab,
	k_NativesTab,
	k_DataTab,
};

ImVec4 ColourOf(Token::Kind kind)
{
	switch (kind)
	{
	case Token::Kind::Opcode:
		return style::k_Opcode;
	case Token::Kind::Number:
		return style::k_Number;
	case Token::Kind::Global:
		return style::k_Global;
	case Token::Kind::Local:
		return style::k_Local;
	case Token::Kind::Native:
		return style::k_Native;
	case Token::Kind::Script:
		return style::k_Script;
	case Token::Kind::Jump:
		return style::k_Jump;
	case Token::Kind::String:
		return style::k_String;
	case Token::Kind::Comment:
	default:
		return style::k_Comment;
	}
}

ImVec4 ColourOf(SourceToken::Kind kind)
{
	switch (kind)
	{
	case SourceToken::Kind::Keyword:
		return style::k_Opcode;
	case SourceToken::Kind::Number:
		return style::k_Number;
	case SourceToken::Kind::String:
		return style::k_String;
	case SourceToken::Kind::Comment:
		return style::k_Comment;
	case SourceToken::Kind::Text:
	default:
		return style::k_Local;
	}
}

std::optional<uint32_t> ParseAddress(std::string_view text)
{
	int base = 10;
	if (text.starts_with("0x") || text.starts_with("0X"))
	{
		text.remove_prefix(2);
		base = 16;
	}
	uint32_t value = 0;
	const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value, base);
	return error == std::errc {} && end == text.data() + text.size() ? std::optional(value) : std::nullopt;
}

const VMScript* ScriptById(const Program& program, uint32_t id)
{
	return id >= 1 && id <= program.scripts.size() ? &program.scripts[id - 1] : nullptr;
}

/// The folder a program was last opened from, remembered between runs
constexpr std::string_view k_ProgramFolderKey = "script_program_folder";

constexpr ImGuiTableFlags k_ListFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                                        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp;
} // namespace

ScriptsPanel::ScriptsPanel() noexcept = default;
ScriptsPanel::~ScriptsPanel() noexcept = default;

void ScriptsPanel::Draw() noexcept
{
	if (!Locator::vm::has_value())
	{
		ImGui::TextDisabled("The script machine isn't running");
		return;
	}
	auto& vm = Locator::vm::value();
	const auto* natives = vm.GetFunctions();
	const Program program {
	    .code = vm.GetInstructions(),
	    .scripts = vm.GetScripts(),
	    .globals = vm.GetVariables(),
	    .natives =
	        natives != nullptr ? std::span<const lhvm::NativeFunction>(*natives) : std::span<const lhvm::NativeFunction>(),
	    .data = vm.GetData(),
	};
	DrawLoading(vm);
	if (program.scripts.empty())
	{
		ImGui::TextDisabled("No script program is loaded");
		return;
	}
	Refresh(vm, program);
	ImGui::SameLine();

	const auto coverage = CoverageOf(_caches.natives, k_UnimplementedNatives);
	ImGui::Text("%zu scripts, %zu instructions, %zu tasks, %zu globals.", program.scripts.size(), program.code.size(),
	            vm.GetTasks().size(), program.globals.size());
	ImGui::SameLine();
	ImGui::TextColored(coverage.implemented == coverage.used ? style::k_Good : style::k_Warning,
	                   "The program calls %zu natives; openblack has written %zu of them.", coverage.used,
	                   coverage.implemented);

	const auto width = ImGui::GetContentRegionAvail().x;
	ImGui::BeginChild("Browser", ImVec2(width * 0.24f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
	DrawBrowser(vm, program);
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("Code", ImVec2(width * 0.44f, 0.0f), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
	DrawCode(vm, program);
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginChild("Side", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
	DrawSide(vm, program);
	ImGui::EndChild();
}

void ScriptsPanel::DrawLoading(lhvm::LHVM& vm) noexcept
{
	const auto exists = [](const std::filesystem::path& folder) {
		std::error_code error;
		return std::filesystem::is_directory(folder, error);
	};
	std::filesystem::path quests;
	if (Locator::filesystem::has_value())
	{
		quests = Locator::filesystem::value().GetPath<filesystem::Path::Quests>(true);
	}
	if (ImGui::SmallButton("Open program..."))
	{
		const auto outcome = file_dialog::Show({
		    .mode = file_dialog::Mode::Open,
		    .title = "Open a compiled script program",
		    .filters = {{.name = "Script programs (*.chl)", .patterns = {"*.chl"}}, {.name = "All files", .patterns = {"*"}}},
		    .startFolder = file_dialog::StartFolder(file_dialog::RememberedPath(k_ProgramFolderKey), quests, exists),
		    .owner = Locator::windowing::has_value() ? Locator::windowing::value().GetNativeHandles().nativeWindow : nullptr,
		});
		switch (outcome.status)
		{
		case file_dialog::Status::Chosen:
			file_dialog::RememberPath(k_ProgramFolderKey, outcome.path.parent_path());
			// The machine reads it as the game reads its own program, stopping every task first. As when the game restarts
			// its scripts, the old program's objects are let go first: what it made goes, the rest goes back to the game
			try
			{
				Locator::scriptObjects::value().Reset();
				_loadMessage = vm.LoadBinary(outcome.path) == EXIT_SUCCESS
				                   ? fmt::format("Loaded {}", outcome.path.filename().string())
				                   : fmt::format("Couldn't read {}", outcome.path.filename().string());
			}
			catch (const std::exception& error)
			{
				_loadMessage = fmt::format("Couldn't read {}: {}", outcome.path.filename().string(), error.what());
			}
			break;
		case file_dialog::Status::Cancelled:
			break;
		case file_dialog::Status::Unavailable:
			_loadMessage = "No file dialog is available here";
			break;
		}
	}
	ImGui::SetItemTooltip("Loads a .chl into the script machine in place of the game's, stopping every task");
	ImGui::SameLine();
	if (ImGui::SmallButton("Game's program") && Locator::filesystem::has_value())
	{
		auto& fileSystem = Locator::filesystem::value();
		const auto path = fileSystem.GetPath<filesystem::Path::Quests>() / "challenge.chl";
		try
		{
			Locator::scriptObjects::value().Reset();
			_loadMessage = fileSystem.Exists(path) && vm.LoadBinary(fileSystem.ReadAll(path)) == EXIT_SUCCESS
			                   ? "Loaded the game's challenge.chl"
			                   : "Couldn't read the game's challenge.chl";
		}
		catch (const std::exception& error)
		{
			_loadMessage = fmt::format("Couldn't read the game's challenge.chl: {}", error.what());
		}
	}
	ImGui::SetItemTooltip("Loads the game's own challenge.chl again, stopping every task");
	if (!_loadMessage.empty())
	{
		ImGui::SameLine();
		ImGui::TextColored(style::k_Muted, "%s", _loadMessage.c_str());
	}
}

void ScriptsPanel::Refresh(const lhvm::LHVM& vm, const Program& program) noexcept
{
	if (_caches.code != program.code.data() || _caches.size != program.code.size())
	{
		_caches = {};
		_caches.code = program.code.data();
		_caches.size = program.code.size();
		_caches.natives = NativesCalled(program.code, {.begin = 0, .end = static_cast<uint32_t>(program.code.size())});
		_back.clear();
		_address.reset();
		if (ScriptById(program, _scriptId) == nullptr)
		{
			SelectScript(1);
		}
	}
	// The picked task's next instruction, as it runs
	if (_followTask)
	{
		if (const auto address = TaskAddress(vm); address.has_value() && address != _address)
		{
			const auto& task = vm.GetTasks().at(_taskId);
			_scriptId = task.scriptId;
			GoTo(program, *address, false);
		}
	}
}

void ScriptsPanel::SelectScript(uint32_t scriptId) noexcept
{
	if (scriptId == _scriptId)
	{
		return;
	}
	_scriptId = scriptId;
	_address.reset();
	_parameters.clear();
	_scrollToAddress = true;
}

void ScriptsPanel::GoTo(const Program& program, uint32_t address, bool remember) noexcept
{
	if (remember && _address.has_value() && _address != address)
	{
		_back.push_back(*_address);
	}
	if (const auto* script = ScriptAt(program.code, program.scripts, address))
	{
		_scriptId = script->scriptId;
	}
	_address = address;
	_scrollToAddress = true;
}

std::optional<uint32_t> ScriptsPanel::TaskAddress(const lhvm::LHVM& vm) const noexcept
{
	const auto& tasks = vm.GetTasks();
	const auto task = tasks.find(_taskId);
	if (task == tasks.end())
	{
		return std::nullopt;
	}
	return task->second.instructionAddress;
}

void ScriptsPanel::DrawBrowser(const lhvm::LHVM& vm, const Program& program) noexcept
{
	ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() * 3.5f);
	ImGui::InputTextWithHint("##Search", "Search scripts and files", &_filter.text);
	ImGui::SameLine();
	if (ImGui::Button("Kinds"))
	{
		ImGui::OpenPopup("Kinds");
	}
	if (ImGui::BeginPopup("Kinds"))
	{
		for (size_t i = 0; i < k_CategoryCount; ++i)
		{
			bool shown = _filter.categories.at(i);
			if (ImGui::Checkbox(Name(static_cast<Category>(i)).data(), &shown))
			{
				_filter.categories.at(i) = shown;
			}
		}
		ImGui::EndPopup();
	}

	std::map<uint32_t, uint32_t> running;
	for (const auto& [id, task] : vm.GetTasks())
	{
		++running[task.scriptId];
	}

	if (!ImGui::BeginTable("Scripts", 5, k_ListFlags | ImGuiTableFlags_Sortable))
	{
		return;
	}
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort);
	ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 3.0f);
	ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthStretch, 1.5f);
	ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch, 2.0f);
	ImGui::TableSetupColumn("Tasks", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableHeadersRow();

	// The list is sorted and searched again only when the search or the sorting changes
	auto key = _filter.text;
	for (const auto shown : _filter.categories)
	{
		key.push_back(shown ? '1' : '0');
	}
	auto* specs = ImGui::TableGetSortSpecs();
	if (specs != nullptr && specs->SpecsCount > 0 && specs->SpecsDirty)
	{
		_caches.sortColumn = specs->Specs[0].ColumnIndex;
		_caches.sortAscending = specs->Specs[0].SortDirection != ImGuiSortDirection_Descending;
		_caches.filterKey.clear();
		specs->SpecsDirty = false;
	}
	if (key != _caches.filterKey || _caches.visibleScripts.empty())
	{
		_caches.filterKey = key;
		_caches.visibleScripts.clear();
		for (const auto& script : program.scripts)
		{
			if (Matches(script, _filter))
			{
				_caches.visibleScripts.push_back(script.scriptId);
			}
		}
		const auto column = _caches.sortColumn;
		const auto ascending = _caches.sortAscending;
		const auto less = [&program, column](uint32_t a, uint32_t b) {
			const auto& left = program.scripts[a - 1];
			const auto& right = program.scripts[b - 1];
			switch (column)
			{
			case 1:
				return left.name < right.name;
			case 2:
				return CategoryOf(left.type) < CategoryOf(right.type);
			case 3:
				return left.filename < right.filename;
			default:
				return a < b;
			}
		};
		std::ranges::stable_sort(_caches.visibleScripts,
		                         [&less, ascending](uint32_t a, uint32_t b) { return ascending ? less(a, b) : less(b, a); });
	}

	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(_caches.visibleScripts.size()));
	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
		{
			const auto id = _caches.visibleScripts.at(static_cast<size_t>(row));
			const auto& script = program.scripts[id - 1];
			ImGui::PushID(static_cast<int>(id));
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			if (ImGui::Selectable(fmt::format("{}", id).c_str(), id == _scriptId, ImGuiSelectableFlags_SpanAllColumns))
			{
				SelectScript(id);
				_showSideTab = k_DetailsTab;
			}
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(script.name.c_str());
			ImGui::TableNextColumn();
			ImGui::TextColored(style::k_Muted, "%s", Name(CategoryOf(script.type)).data());
			ImGui::TableNextColumn();
			ImGui::TextColored(style::k_Muted, "%s", script.filename.c_str());
			ImGui::TableNextColumn();
			if (const auto count = running.find(id); count != running.end())
			{
				ImGui::TextColored(style::k_Good, "%u", count->second);
			}
			ImGui::PopID();
		}
	}
	ImGui::EndTable();
}

void ScriptsPanel::DrawTokens(const Program& program, const std::vector<Token>& tokens) noexcept
{
	bool first = true;
	for (const auto& token : tokens)
	{
		if (!first)
		{
			ImGui::SameLine(0.0f, ImGui::CalcTextSize(" ").x);
		}
		first = false;
		auto colour = ColourOf(token.kind);
		const bool unwritten = token.kind == Token::Kind::Native && !IsImplemented(token.target, k_UnimplementedNatives);
		if (unwritten)
		{
			colour = style::k_Error;
		}
		switch (token.kind)
		{
		case Token::Kind::Jump:
			if (ImGui::TextButtonColored(colour, token.text.c_str()))
			{
				GoTo(program, token.target);
			}
			ImGui::SetItemTooltip("Go to 0x%04x", token.target);
			break;
		case Token::Kind::Script:
			if (ImGui::TextButtonColored(colour, token.text.c_str()))
			{
				if (const auto* script = ScriptById(program, token.target))
				{
					GoTo(program, script->instructionAddress);
					_showSideTab = k_DetailsTab;
				}
			}
			ImGui::SetItemTooltip("Go to the script");
			break;
		case Token::Kind::Native:
			if (ImGui::TextButtonColored(colour, token.text.c_str()))
			{
				_native = token.target;
				_showSideTab = k_NativesTab;
			}
			ImGui::SetItemTooltip(unwritten ? "Native %u: openblack hasn't written it yet" : "Native %u", token.target);
			break;
		case Token::Kind::Global:
			if (ImGui::TextButtonColored(colour, token.text.c_str()))
			{
				_globalsFilter = token.text;
				_showSideTab = k_GlobalsTab;
			}
			if (token.target < program.globals.size())
			{
				const auto& global = program.globals[token.target];
				ImGui::SetItemTooltip("Global %u = %s", token.target, FormatValue(global.value, global.type).c_str());
			}
			break;
		default:
			ImGui::TextColored(colour, "%s", token.text.c_str());
			break;
		}
	}
}

void ScriptsPanel::DrawCode(lhvm::LHVM& vm, const Program& program) noexcept
{
	const auto* script = ScriptById(program, _scriptId);
	if (script == nullptr)
	{
		ImGui::TextDisabled("Pick a script");
		return;
	}
	const auto range = RangeOf(program.code, *script);

	ImGui::TextColored(style::k_Script, "%s", script->name.c_str());
	ImGui::SameLine();
	ImGui::TextColored(style::k_Muted, "%s, %s, 0x%04x to 0x%04x", Name(CategoryOf(script->type)).data(),
	                   script->filename.c_str(), range.begin, range.end > 0 ? range.end - 1 : 0);

	ImGui::BeginDisabled(_back.empty());
	if (ImGui::ArrowButton("Back", ImGuiDir_Left))
	{
		const auto address = _back.back();
		_back.pop_back();
		GoTo(program, address, false);
	}
	ImGui::EndDisabled();
	ImGui::SetItemTooltip("Back to where the last jump was clicked from");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 12.0f);
	const bool find = ImGui::InputTextWithHint("##Find", "Find in the script", &_search, ImGuiInputTextFlags_EnterReturnsTrue);
	ImGui::SameLine();
	if (ImGui::Button("Next") || find)
	{
		if (const auto found = FindNext(program, script, range, _search, _address))
		{
			GoTo(program, *found);
		}
	}
	ImGui::SameLine();
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 6.0f);
	if (ImGui::InputTextWithHint("##GoTo", "0x0000", &_goTo, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		if (const auto address = ParseAddress(_goTo); address.has_value() && *address < program.code.size())
		{
			GoTo(program, *address);
		}
	}
	ImGui::SetItemTooltip("Go to an address, in hex with 0x or in decimal");
	ImGui::SameLine();
	ImGui::Checkbox("Follow task", &_followTask);
	ImGui::SetItemTooltip("Keeps to the picked task's next instruction as it runs");

	if (ImGui::BeginTabBar("CodeViews"))
	{
		if (ImGui::BeginTabItem("Disassembly"))
		{
			DrawDisassembly(vm, program, *script, range);
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Source"))
		{
			DrawDecompiled(program, *script);
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
}

void ScriptsPanel::DrawDisassembly(lhvm::LHVM& vm, const Program& program, const VMScript& script, CodeRange range) noexcept
{
	ImGui::PushStyleColor(ImGuiCol_TableRowBg, style::k_CodeBackground);
	ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, style::k_CodeBackground);
	const auto flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit;
	if (!ImGui::BeginTable("Disassembly", 3, flags))
	{
		ImGui::PopStyleColor(2);
		return;
	}
	ImGui::TableSetupColumn("##Margin", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize());
	ImGui::TableSetupColumn("##Address", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("##Code", ImGuiTableColumnFlags_WidthStretch);

	const auto rowHeight = ImGui::GetTextLineHeightWithSpacing();
	if (_scrollToAddress)
	{
		const auto at = _address.value_or(range.begin);
		const auto row = at >= range.begin ? static_cast<float>(at - range.begin) : 0.0f;
		ImGui::SetScrollY(std::max(0.0f, (row * rowHeight) - (ImGui::GetWindowHeight() * 0.3f)));
		_scrollToAddress = false;
	}

	const auto taskAddress = TaskAddress(vm);
	const auto& breakpoints = vm.GetBreakpoints();
	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(range.Size()), rowHeight);
	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
		{
			const auto address = range.begin + static_cast<uint32_t>(row);
			ImGui::PushID(static_cast<int>(address));
			ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
			if (address == taskAddress)
			{
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, style::k_TaskLine);
			}
			else if (address == _address)
			{
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, style::k_CurrentLine);
			}

			ImGui::TableNextColumn();
			const auto cell = ImGui::GetCursorScreenPos();
			const bool hasBreakpoint = breakpoints.contains(address);
			if (ImGui::InvisibleButton("Breakpoint", ImVec2(ImGui::GetFontSize(), rowHeight)))
			{
				vm.SetBreakpoint(address, !hasBreakpoint);
			}
			ImGui::SetItemTooltip(hasBreakpoint ? "Remove the breakpoint" : "Hold tasks before this instruction");
			auto* drawList = ImGui::GetWindowDrawList();
			const auto radius = ImGui::GetFontSize() * 0.3f;
			const ImVec2 centre {cell.x + (ImGui::GetFontSize() * 0.5f), cell.y + (rowHeight * 0.5f)};
			if (hasBreakpoint)
			{
				drawList->AddCircleFilled(centre, radius, style::k_Breakpoint);
			}
			else if (ImGui::IsItemHovered())
			{
				drawList->AddCircle(centre, radius, style::k_Breakpoint);
			}
			if (address == taskAddress)
			{
				drawList->AddTriangleFilled(ImVec2(centre.x - radius, centre.y - radius), ImVec2(centre.x + radius, centre.y),
				                            ImVec2(centre.x - radius, centre.y + radius), IM_COL32(240, 200, 70, 255));
			}

			ImGui::TableNextColumn();
			ImGui::TextColored(style::k_Address, "%04x", address);
			if (ImGui::IsItemClicked())
			{
				_address = address;
			}
			ImGui::TableNextColumn();
			DrawTokens(program, Disassemble(program, &script, address));
			ImGui::PopID();
		}
	}
	ImGui::EndTable();
	ImGui::PopStyleColor(2);
}

void ScriptsPanel::DrawDecompiled(const Program& program, const VMScript& script) noexcept
{
	if (!HasDecompiler())
	{
		ImGui::TextWrapped("No decompiler is linked in yet. Once one is, the script's source shows here beside the "
		                   "disassembly, each line leading to its instructions, with the picked task's line marked.");
		return;
	}
	auto cached = _caches.decompiled.find(script.scriptId);
	if (cached == _caches.decompiled.end())
	{
		cached = _caches.decompiled.emplace(script.scriptId, Decompile(program, script)).first;
	}
	if (!cached->second.has_value())
	{
		ImGui::TextDisabled("The decompiler couldn't make source of this script");
		return;
	}
	const auto& source = *cached->second;
	for (const auto& diagnostic : source.diagnostics)
	{
		ImGui::TextColored(style::k_Warning, "%s", diagnostic.c_str());
	}

	// Editing hands the text to the compiler, once there is one
	if (ImGui::Checkbox("Edit", &_editingSource) && _editingSource)
	{
		_sourceText.clear();
		for (const auto& line : source.lines)
		{
			_sourceText += line;
			_sourceText.push_back('\n');
		}
		_compileMessages.clear();
	}
	if (_editingSource)
	{
		ImGui::SameLine();
		ImGui::BeginDisabled(!HasCompiler());
		if (ImGui::Button("Compile"))
		{
			auto result = Compile(program, script, _sourceText);
			_compileMessages = std::move(result.diagnostics);
			if (result.program != nullptr && Locator::vm::value().UpdateProgram(*result.program) == EXIT_SUCCESS)
			{
				// The program's tables have moved: stop drawing from the old ones this frame
				_compileMessages.emplace_back("Compiled: the script runs its new code from its next start");
				ImGui::EndDisabled();
				return;
			}
		}
		ImGui::EndDisabled();
		if (!HasCompiler())
		{
			ImGui::SameLine();
			ImGui::TextColored(style::k_Muted, "No script compiler is linked in yet");
		}
		for (const auto& message : _compileMessages)
		{
			ImGui::TextColored(style::k_Warning, "%s", message.c_str());
		}
		ImGui::InputTextMultiline("##Source", &_sourceText, ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput);
		return;
	}

	const auto taskLine =
	    TaskAddress(Locator::vm::value()).and_then([&source](uint32_t address) { return LineOf(source, address); });
	const auto currentLine = _address.and_then([&source](uint32_t address) { return LineOf(source, address); });
	ImGui::PushStyleColor(ImGuiCol_ChildBg, style::k_CodeBackground);
	ImGui::BeginChild("Source");
	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(source.lines.size()));
	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
		{
			const auto line = static_cast<size_t>(row);
			const auto start = ImGui::GetCursorScreenPos();
			const auto lineSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing());
			if (line == taskLine || line == currentLine)
			{
				ImGui::GetWindowDrawList()->AddRectFilled(start, ImVec2(start.x + lineSize.x, start.y + lineSize.y),
				                                          line == taskLine ? style::k_TaskLine : style::k_CurrentLine);
			}
			ImGui::PushID(row);
			if (ImGui::InvisibleButton("Line", lineSize) && line < source.lineAddresses.size() &&
			    source.lineAddresses.at(line).has_value())
			{
				GoTo(program, *source.lineAddresses.at(line));
			}
			ImGui::PopID();
			ImGui::SetCursorScreenPos(start);
			ImGui::TextColored(style::k_Address, "%4zu ", line + 1);
			for (const auto& token : HighlightSource(source.lines.at(line)))
			{
				ImGui::SameLine(0.0f, 0.0f);
				ImGui::TextColored(ColourOf(token.kind), "%.*s", static_cast<int>(token.text.size()), token.text.data());
			}
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();
}

void ScriptsPanel::DrawSide(lhvm::LHVM& vm, const Program& program) noexcept
{
	if (!ImGui::BeginTabBar("Side"))
	{
		return;
	}
	const auto show = _showSideTab;
	_showSideTab.reset();
	const auto flags = [show](int tab) { return show == tab ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None; };
	if (ImGui::BeginTabItem("Script", nullptr, flags(k_DetailsTab)))
	{
		DrawDetails(vm, program);
		ImGui::EndTabItem();
	}
	const auto taskLabel = fmt::format("Tasks ({})###Tasks", vm.GetTasks().size());
	if (ImGui::BeginTabItem(taskLabel.c_str(), nullptr, flags(k_TasksTab)))
	{
		DrawTasks(vm, program);
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Globals", nullptr, flags(k_GlobalsTab)))
	{
		DrawGlobals(vm, program);
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Natives", nullptr, flags(k_NativesTab)))
	{
		DrawNatives(program);
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Data", nullptr, flags(k_DataTab)))
	{
		DrawData(program);
		ImGui::EndTabItem();
	}
	ImGui::EndTabBar();
}

void ScriptsPanel::DrawDetails(lhvm::LHVM& vm, const Program& program) noexcept
{
	const auto* script = ScriptById(program, _scriptId);
	if (script == nullptr)
	{
		return;
	}
	const auto range = RangeOf(program.code, *script);
	if (ImGui::BeginTable("Facts", 2, ImGuiTableFlags_SizingFixedFit))
	{
		const auto fact = [](const char* name, const std::string& value) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextColored(style::k_Muted, "%s", name);
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(value.c_str());
		};
		fact("Script", fmt::format("{} (id {})", script->name, script->scriptId));
		fact("Kind", std::string(Name(CategoryOf(script->type))));
		fact("File", script->filename);
		fact("Code", fmt::format("0x{:04x} to 0x{:04x}, {} instructions", range.begin, range.end > 0 ? range.end - 1 : 0,
		                         range.Size()));
		fact("Parameters", std::to_string(script->parameterCount));
		fact("Locals", fmt::format("{} (variables from {})", script->variables.size(), script->variablesOffset + 1));
		ImGui::EndTable();
	}

	ImGui::SeparatorText("Start");
	_parameters.resize(script->parameterCount, 0.0f);
	for (size_t i = 0; i < _parameters.size(); ++i)
	{
		const auto label = i < script->variables.size() ? script->variables.at(i) : fmt::format("Parameter {}", i + 1);
		ImGui::InputFloat(label.c_str(), &_parameters.at(i));
	}
	if (ImGui::Button("Start a task"))
	{
		// Parameters go on the stack as a calling script's do, then the script starts as the game starts scripts
		for (const auto value : _parameters)
		{
			vm.Pushf(value);
		}
		const auto task = vm.StartScript(script->name, lhvm::ScriptType::All);
		_lastStart = task != 0 ? fmt::format("Started task {}", task) : "Couldn't start it: see the log";
		if (task != 0)
		{
			_taskId = task;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Stop its tasks"))
	{
		const auto& name = script->name;
		vm.StopScripts([&name](const std::string& taskName, const std::string&) { return taskName == name; });
	}
	if (!_lastStart.empty())
	{
		ImGui::SameLine();
		ImGui::TextColored(style::k_Muted, "%s", _lastStart.c_str());
	}

	if (!script->variables.empty() && ImGui::CollapsingHeader("Locals"))
	{
		for (size_t i = 0; i < script->variables.size(); ++i)
		{
			ImGui::TextColored(style::k_Local, "%s", script->variables.at(i).c_str());
			if (i < script->parameterCount)
			{
				ImGui::SameLine();
				ImGui::TextColored(style::k_Muted, "(parameter)");
			}
		}
	}

	const auto uses = NativesCalled(program.code, range);
	const auto coverage = CoverageOf(uses, k_UnimplementedNatives);
	ImGui::SeparatorText(fmt::format("Natives it calls: {} of {} written", coverage.implemented, coverage.used).c_str());
	if (ImGui::BeginTable("Calls", 3, k_ListFlags))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Native", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Written", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableHeadersRow();
		for (const auto& use : uses)
		{
			const bool written = IsImplemented(use.native, k_UnimplementedNatives);
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			const auto name =
			    use.native < program.natives.size() ? program.natives[use.native].name : fmt::format("NATIVE_{}", use.native);
			if (ImGui::TextButtonColored(written ? style::k_Native : style::k_Error, name.c_str()))
			{
				_native = use.native;
				_showSideTab = k_NativesTab;
			}
			ImGui::TableNextColumn();
			ImGui::Text("%u", use.calls);
			ImGui::TableNextColumn();
			ImGui::TextColored(written ? style::k_Good : style::k_Error, written ? "yes" : "no");
		}
		ImGui::EndTable();
	}
}

void ScriptsPanel::DrawTasks(lhvm::LHVM& vm, const Program& program) noexcept
{
	const auto& tasks = vm.GetTasks();
	if (ImGui::Button("Hold all"))
	{
		for (const auto& [id, task] : tasks)
		{
			vm.HoldTask(id);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Continue all"))
	{
		for (const auto& [id, task] : tasks)
		{
			vm.ContinueTask(id);
		}
	}
	ImGui::SameLine();
	ImGui::TextColored(style::k_Muted, "Held tasks sit out the turns; the game's pause and step drive the rest");

	const auto listHeight = ImGui::GetContentRegionAvail().y * 0.4f;
	if (ImGui::BeginTable("Tasks", 5, k_ListFlags, ImVec2(0.0f, listHeight)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Script", ImGuiTableColumnFlags_WidthStretch, 2.0f);
		ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthStretch, 1.0f);
		ImGui::TableSetupColumn("Next", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Turns", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableHeadersRow();
		for (const auto& [id, task] : tasks)
		{
			const auto state = StateOf(task, vm.IsTaskHeld(id));
			ImGui::PushID(static_cast<int>(id));
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			if (ImGui::Selectable(fmt::format("{}", id).c_str(), id == _taskId, ImGuiSelectableFlags_SpanAllColumns))
			{
				_taskId = id;
				_scriptId = task.scriptId;
				GoTo(program, task.instructionAddress, false);
			}
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(task.name.c_str());
			ImGui::TableNextColumn();
			const auto colour = state == TaskState::Held      ? style::k_Error
			                    : state == TaskState::Running ? style::k_Good
			                                                  : style::k_Warning;
			ImGui::TextColored(colour, "%s", Name(state).data());
			ImGui::TableNextColumn();
			ImGui::Text("%04x", task.instructionAddress);
			ImGui::TableNextColumn();
			ImGui::Text("%u", task.ticks);
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	const auto found = tasks.find(_taskId);
	if (found == tasks.end())
	{
		ImGui::TextDisabled(tasks.empty() ? "No task is running" : "Pick a task");
	}
	else
	{
		const auto& task = found->second;
		const bool held = vm.IsTaskHeld(task.id);
		if (held)
		{
			if (ImGui::Button("Step"))
			{
				vm.StepTask(task.id);
			}
			ImGui::SetItemTooltip("Runs its next instruction at the next turn");
			ImGui::SameLine();
			if (ImGui::Button("Continue"))
			{
				vm.ContinueTask(task.id);
			}
		}
		else if (ImGui::Button("Hold"))
		{
			vm.HoldTask(task.id);
		}
		ImGui::SameLine();
		if (ImGui::Button("Stop"))
		{
			vm.StopTask(task.id);
			_taskId = 0;
		}
		if (_taskId != 0)
		{
			std::string waitedFor;
			if (const auto waited = tasks.find(task.waitingTaskId); waited != tasks.end())
			{
				waitedFor = waited->second.name;
			}
			ImGui::TextWrapped("Task %u, %s: %s.", task.id, task.name.c_str(), Describe(task, held, waitedFor).c_str());
			if (task.waitingTaskId != 0 && tasks.contains(task.waitingTaskId))
			{
				ImGui::SameLine();
				if (ImGui::SmallButton("Go to it"))
				{
					_taskId = task.waitingTaskId;
				}
			}
			if (ImGui::BeginTabBar("TaskParts"))
			{
				if (ImGui::BeginTabItem(fmt::format("Stack ({})###Stack", task.stack.count).c_str()))
				{
					if (ImGui::BeginTable("Stack", 3, k_ListFlags))
					{
						ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed);
						ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
						ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableHeadersRow();
						// The top of the stack first
						for (uint32_t i = task.stack.count; i-- > 0;)
						{
							const auto type = task.stack.types.at(i);
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::Text("%u", i);
							ImGui::TableNextColumn();
							ImGui::TextColored(style::k_Muted, "%s", TypeName(type).data());
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(FormatValue(task.stack.values.at(i), type).c_str());
						}
						ImGui::EndTable();
					}
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem(fmt::format("Locals ({})###Locals", task.localVars.size()).c_str()))
				{
					if (ImGui::BeginTable("Locals", 3, k_ListFlags))
					{
						ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
						ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableHeadersRow();
						for (size_t i = 0; i < task.localVars.size(); ++i)
						{
							const auto& variable = task.localVars.at(i);
							ImGui::PushID(static_cast<int>(i));
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::TextColored(style::k_Local, "%s", variable.name.c_str());
							ImGui::TableNextColumn();
							ImGui::TextColored(style::k_Muted, "%s", TypeName(variable.type).data());
							ImGui::TableNextColumn();
							auto text = FormatValue(variable.value, variable.type);
							ImGui::SetNextItemWidth(-1.0f);
							if (ImGui::InputText("##Value", &text, ImGuiInputTextFlags_EnterReturnsTrue))
							{
								if (const auto value = ParseValue(text, variable.type))
								{
									vm.SetTaskVariable(task.id, i, *value);
								}
							}
							ImGui::PopID();
						}
						ImGui::EndTable();
					}
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Handlers"))
				{
					ImGui::Text("%s an exception handler; it comes back to 0x%04x", task.inExceptionHandler ? "In" : "Not in",
					            task.pevInstructionAddress);
					for (const auto address : task.exceptionHandlerIps)
					{
						if (ImGui::TextButtonColored(style::k_Jump, fmt::format("0x{:04x}", address).c_str()))
						{
							GoTo(program, address);
						}
					}
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
	}

	const auto& breakpoints = vm.GetBreakpoints();
	if (ImGui::CollapsingHeader(fmt::format("Breakpoints ({})###Breakpoints", breakpoints.size()).c_str()))
	{
		std::optional<uint32_t> remove;
		for (const auto address : breakpoints)
		{
			ImGui::PushID(static_cast<int>(address));
			const auto* script = ScriptAt(program.code, program.scripts, address);
			if (ImGui::TextButtonColored(style::k_Jump, fmt::format("0x{:04x}", address).c_str()))
			{
				GoTo(program, address);
			}
			ImGui::SameLine();
			ImGui::TextColored(style::k_Muted, "%s", script != nullptr ? script->name.c_str() : "");
			ImGui::SameLine();
			if (ImGui::SmallButton("Remove"))
			{
				remove = address;
			}
			ImGui::PopID();
		}
		if (remove.has_value())
		{
			vm.SetBreakpoint(*remove, false);
		}
		if (!breakpoints.empty() && ImGui::Button("Remove all"))
		{
			const std::vector<uint32_t> all(breakpoints.begin(), breakpoints.end());
			for (const auto address : all)
			{
				vm.SetBreakpoint(address, false);
			}
		}
	}
}

void ScriptsPanel::DrawGlobals(lhvm::LHVM& vm, const Program& program) noexcept
{
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::InputTextWithHint("##Filter", "Search globals", &_globalsFilter);
	std::vector<uint32_t> shown;
	// The first is the machine's null variable
	for (uint32_t id = 1; id < program.globals.size(); ++id)
	{
		if (MatchesSearch(program.globals[id].name, _globalsFilter))
		{
			shown.push_back(id);
		}
	}
	if (!ImGui::BeginTable("Globals", 4, k_ListFlags))
	{
		return;
	}
	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 2.0f);
	ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed);
	ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 1.0f);
	ImGui::TableHeadersRow();
	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(shown.size()));
	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
		{
			const auto id = shown.at(static_cast<size_t>(row));
			const auto& global = program.globals[id];
			ImGui::PushID(static_cast<int>(id));
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextColored(style::k_Muted, "%u", id);
			ImGui::TableNextColumn();
			ImGui::TextColored(style::k_Global, "%s", global.name.c_str());
			ImGui::TableNextColumn();
			ImGui::TextColored(style::k_Muted, "%s", TypeName(global.type).data());
			ImGui::TableNextColumn();
			if (_editingGlobal == id)
			{
				ImGui::SetNextItemWidth(-1.0f);
				ImGui::SetKeyboardFocusHere();
				if (ImGui::InputText("##Edit", &_editText, ImGuiInputTextFlags_EnterReturnsTrue))
				{
					if (const auto value = ParseValue(_editText, global.type))
					{
						vm.SetVariable(id, *value);
					}
					_editingGlobal.reset();
				}
				if (ImGui::IsKeyPressed(ImGuiKey_Escape))
				{
					_editingGlobal.reset();
				}
			}
			else
			{
				const auto text = FormatValue(global.value, global.type);
				if (ImGui::Selectable(text.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick) &&
				    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					_editingGlobal = id;
					_editText = text;
				}
				ImGui::SetItemTooltip("Double click to change it");
			}
			ImGui::PopID();
		}
	}
	ImGui::EndTable();
}

void ScriptsPanel::DrawNatives(const Program& program) noexcept
{
	const auto written = program.natives.size() - std::min(program.natives.size(), k_UnimplementedNatives.size());
	ImGui::TextColored(style::k_Muted, "openblack has written %zu of the %zu natives.", written, program.natives.size());
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0f);
	ImGui::InputTextWithHint("##Filter", "Search natives", &_nativesFilter);
	ImGui::SameLine();
	ImGui::Checkbox("Used", &_onlyUsedNatives);
	ImGui::SetItemTooltip("Only the natives the program calls");
	ImGui::SameLine();
	ImGui::Checkbox("Unwritten", &_onlyUnwrittenNatives);
	ImGui::SetItemTooltip("Only the natives openblack hasn't written yet");

	std::map<uint32_t, uint32_t> calls;
	for (const auto& use : _caches.natives)
	{
		calls[use.native] = use.calls;
	}
	std::vector<uint32_t> shown;
	for (uint32_t native = 0; native < program.natives.size(); ++native)
	{
		const auto used = calls.contains(native);
		const auto isWritten = IsImplemented(native, k_UnimplementedNatives);
		if ((_onlyUsedNatives && !used) || (_onlyUnwrittenNatives && isWritten) ||
		    !MatchesSearch(program.natives[native].name, _nativesFilter))
		{
			continue;
		}
		shown.push_back(native);
	}

	const auto height = _native.has_value() ? ImGui::GetContentRegionAvail().y * 0.6f : 0.0f;
	if (ImGui::BeginTable("Natives", 5, k_ListFlags | ImGuiTableFlags_Sortable, ImVec2(0.0f, height)))
	{
		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort);
		ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("In/out", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort);
		ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("Written", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableHeadersRow();
		if (const auto* specs = ImGui::TableGetSortSpecs(); specs != nullptr && specs->SpecsCount > 0)
		{
			const auto column = specs->Specs[0].ColumnIndex;
			const bool descending = specs->Specs[0].SortDirection == ImGuiSortDirection_Descending;
			const auto key = [&](uint32_t native) -> int64_t {
				switch (column)
				{
				case 3:
					return calls.contains(native) ? calls.at(native) : 0;
				case 4:
					return IsImplemented(native, k_UnimplementedNatives) ? 1 : 0;
				default:
					return native;
				}
			};
			if (column == 1)
			{
				std::ranges::stable_sort(shown, [&](uint32_t a, uint32_t b) {
					return descending ? program.natives[b].name < program.natives[a].name
					                  : program.natives[a].name < program.natives[b].name;
				});
			}
			else
			{
				std::ranges::stable_sort(
				    shown, [&](uint32_t a, uint32_t b) { return descending ? key(b) < key(a) : key(a) < key(b); });
			}
		}
		ImGuiListClipper clipper;
		clipper.Begin(static_cast<int>(shown.size()));
		while (clipper.Step())
		{
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
			{
				const auto native = shown.at(static_cast<size_t>(row));
				const auto& function = program.natives[native];
				const auto isWritten = IsImplemented(native, k_UnimplementedNatives);
				ImGui::PushID(static_cast<int>(native));
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				if (ImGui::Selectable(fmt::format("{}", native).c_str(), _native == native,
				                      ImGuiSelectableFlags_SpanAllColumns))
				{
					_native = native;
				}
				ImGui::TableNextColumn();
				ImGui::TextColored(isWritten ? style::k_Native : style::k_Error, "%s", function.name.c_str());
				ImGui::TableNextColumn();
				ImGui::TextColored(style::k_Muted, "%d/%d", function.stackIn, function.stackOut);
				ImGui::TableNextColumn();
				if (const auto count = calls.find(native); count != calls.end())
				{
					ImGui::Text("%u", count->second);
				}
				ImGui::TableNextColumn();
				ImGui::TextColored(isWritten ? style::k_Good : style::k_Error, isWritten ? "yes" : "no");
				ImGui::PopID();
			}
		}
		ImGui::EndTable();
	}

	if (_native.has_value() && *_native < program.natives.size())
	{
		const auto native = *_native;
		ImGui::SeparatorText(fmt::format("Scripts calling {}", program.natives[native].name).c_str());
		ImGui::BeginChild("Callers");
		for (const auto& script : program.scripts)
		{
			const auto range = RangeOf(program.code, script);
			for (auto address = range.begin; address < range.end; ++address)
			{
				const auto& instruction = program.code[address];
				if (instruction.code == lhvm::Opcode::Sys && instruction.data.uintVal == native)
				{
					ImGui::PushID(static_cast<int>(address));
					if (ImGui::TextButtonColored(style::k_Script, fmt::format("{} at 0x{:04x}", script.name, address).c_str()))
					{
						GoTo(program, address);
					}
					ImGui::PopID();
				}
			}
		}
		ImGui::EndChild();
	}
}

void ScriptsPanel::DrawData(const Program& program) noexcept
{
	if (_dataView == nullptr)
	{
		_dataView = std::make_unique<MemoryEditor>();
		_dataView->ReadOnly = true;
	}
	// It is read only, so the data is never written through it
	_dataView->DrawContents(const_cast<char*>(program.data.data()), program.data.size(), 0);
}

} // namespace openblack::editor

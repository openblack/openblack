/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LHVMViewer.h"

#include <array>

#include <imgui.h>
#include <imgui_memory_editor.h>
#include <imgui_user.h>

#include "Game.h"

using namespace openblack::debug::gui;

const ImVec4 Disassembly_ColorBG = ImVec4(0.152f, 0.156f, 0.133f, 1.0f);
const ImVec4 Disassembly_ColorFG = ImVec4(0.972f, 0.972f, 0.949f, 1.0f);
const ImVec4 Disassembly_ColorComment = ImVec4(0.458f, 0.443f, 0.368f, 1.0f);
const ImVec4 Disassembly_ColorFuncName = ImVec4(0.650f, 0.886f, 0.180f, 1.0f);
const ImVec4 Disassembly_ColorKeyword = ImVec4(0.976f, 0.149f, 0.447f, 1.0f);
const ImVec4 Disassembly_ColorVariable = ImVec4(0.972f, 0.972f, 0.949f, 1.0f);
const ImVec4 Disassembly_ColorConstant = ImVec4(0.682f, 0.505f, 1.0f, 1.0f);

LHVMViewer::LHVMViewer()
    : Window("LHVM Viewer", ImVec2(720.0f, 612.0f))
{
}

void LHVMViewer::Draw(Game& game)
{
	auto& lhvm = game.GetLhvm();
	if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
	{

		if (ImGui::BeginTabItem("Scripts", nullptr, _openScriptTab ? ImGuiTabItemFlags_SetSelected : 0))
		{
			_openScriptTab = false;
			DrawScriptsTab(lhvm);

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Variables"))
		{
			// left
			static size_t selected = 0;
			ImGui::BeginChild("left pane", ImVec2(240, 0), true);

			auto variables = lhvm.GetVariables();
			for (size_t i = 0; i < variables.size(); i++)
			{
				if (ImGui::Selectable(variables.at(i).name.c_str(), selected == i))
				{
					selected = i;
				}
			}

			ImGui::EndChild();
			ImGui::SameLine();

			// right
			const auto& var = variables.at(selected);
			ImGui::BeginChild("item view"); // Leave room for 1 line below us
			ImGui::Text("Variable ID: %s", std::to_string(selected).c_str());
			ImGui::Text("Variable Name: %s", var.name.c_str());
			ImGui::Text("Variable Value: %s", DataToString(var.value, var.type).c_str());
			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Data"))
		{
			static MemoryEditor lhvmDataEditor;
			auto& data = lhvm.GetData();
			lhvmDataEditor.DrawContents(const_cast<void*>(reinterpret_cast<const void*>(data.data())), data.size(), 0);

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Tasks"))
		{
			DrawTasksTab(lhvm);

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

void LHVMViewer::Update([[maybe_unused]] openblack::Game& game) {}

void LHVMViewer::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) {}

void LHVMViewer::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) {}

void LHVMViewer::DrawScriptsTab(const openblack::lhvm::LHVM& lhvm)
{
	static auto vectorGetter = [](void* vec, int idx, const char** outText) {
		auto& vector = *static_cast<std::vector<std::string>*>(vec);
		if (idx < 0 || idx >= static_cast<int>(vector.size()))
		{
			return false;
		}
		*outText = vector.at(idx).c_str();
		return true;
	};

	auto scripts = lhvm.GetScripts();

	ImGui::PushItemWidth(200);

	if (ImGui::BeginListBox("##scripts", ImVec2(240, ImGui::GetContentRegionAvail().y)))
	{
		for (auto const& script : scripts)
		{
			if (ImGui::Selectable(script.name.c_str(), script.scriptId == _selectedScriptID))
			{
				SelectScript(script.scriptId);
			}

			if (_scrollToSelected && _selectedScriptID == script.scriptId)
			{
				ImGui::SetScrollHereY(0.25f);
				_scrollToSelected = false;
			}
		}
		ImGui::EndListBox();
	}

	ImGui::SameLine();

	auto script = scripts[_selectedScriptID - 1];

	ImGui::BeginChild("##script");
	ImGui::Text("ID: %d", script.scriptId);
	ImGui::SameLine();
	ImGui::Text("Name: %s", script.name.c_str());
	ImGui::Text("Type: %s", lhvm::k_ScriptTypeNames.at(script.type).c_str());
	ImGui::Text("File: %s", script.filename.c_str());
	ImGui::Text("Instruction address: 0x%04x", script.instructionAddress);
	ImGui::Text("Variables offset: 0x%04x", script.variablesOffset);
	ImGui::SameLine();
	ImGui::Text("Parameter count: %d", script.parameterCount);
	if (ImGui::BeginTabBar("##ScriptTabs", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("Code"))
		{
			DrawScriptDisassembly(lhvm, script);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Local Variables"))
		{
			auto scriptVars = script.variables;

			static int selectedVar = 0;
			ImGui::PushItemWidth(-1);
			ImGui::ListBox("", &selectedVar, vectorGetter, static_cast<void*>(&scriptVars), static_cast<int>(scriptVars.size()),
			               21);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
	ImGui::EndChild();
}

void LHVMViewer::DrawScriptDisassembly(const openblack::lhvm::LHVM& lhvm, openblack::lhvm::VMScript& script)
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Disassembly_ColorBG);
	ImGui::PushStyleColor(ImGuiCol_Text, Disassembly_ColorFG);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, Disassembly_ColorBG);
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, Disassembly_ColorComment);

	ImGui::BeginChild("##code");

	if (_resetScriptDisassemblyScroll)
	{
		ImGui::SetScrollHereY(0.0f);
		_resetScriptDisassemblyScroll = false;
	}

	const auto& code = lhvm.GetInstructions();
	const auto* functions = lhvm.GetFunctions();

	for (unsigned int i = script.instructionAddress; i < code.size(); i++)
	{
		const auto& instruction = code[i];
		const auto* const opcodeName = lhvm::k_OpcodeNames.at(static_cast<int>(instruction.code)).c_str();
		const auto* const typeChar = lhvm::k_DataTypeChars.at(static_cast<uint32_t>(instruction.type)).c_str();

		ImGui::TextColored(Disassembly_ColorComment, "0x%04x:", i);
		ImGui::SameLine();

		switch (instruction.code)
		{
		case lhvm::Opcode::Push:
			ImGui::TextColored(Disassembly_ColorKeyword, "PUSH%s", typeChar);
			ImGui::SameLine();

			if (instruction.mode == lhvm::VMMode::Reference)
			{
				DrawVariable(lhvm, script, instruction.data.intVal);
			}
			else if (instruction.mode == lhvm::VMMode::Immediate)
			{
				ImGui::TextColored(Disassembly_ColorConstant, "%s", DataToString(instruction.data, instruction.type).c_str());
			}

			break;
		case lhvm::Opcode::Pop:
			ImGui::TextColored(Disassembly_ColorKeyword, "POP%s", typeChar);

			if (instruction.mode == lhvm::VMMode::Reference)
			{
				ImGui::SameLine();
				DrawVariable(lhvm, script, instruction.data.intVal);
			}

			break;
		case lhvm::Opcode::Add:
		case lhvm::Opcode::Sub:
		case lhvm::Opcode::Cast:
			ImGui::TextColored(Disassembly_ColorKeyword, "%s%s", opcodeName, typeChar);
			break;
		case lhvm::Opcode::Sys:
			ImGui::TextColored(Disassembly_ColorKeyword, "SYS");
			ImGui::SameLine();
			if (functions != nullptr && instruction.data.uintVal < functions->size())
			{
				ImGui::TextColored(Disassembly_ColorFuncName, "%s",
				                   lhvm.GetFunctions()->at(instruction.data.uintVal).name.c_str());
			}
			else
			{
				ImGui::TextColored(Disassembly_ColorFuncName, "FUNC_%u", instruction.data.uintVal);
			}
			break;
		case lhvm::Opcode::Run:
		{
			auto const& runScript = lhvm.GetScripts().at(instruction.data.uintVal - 1);

			ImGui::TextColored(Disassembly_ColorKeyword, "RUN");
			if (instruction.mode == lhvm::VMMode::Async)
			{
				ImGui::SameLine();
				ImGui::TextColored(Disassembly_ColorKeyword, "async");
			}

			ImGui::SameLine();
			if (ImGui::TextButtonColored(Disassembly_ColorFuncName, runScript.name.c_str()))
			{
				SelectScript(instruction.data.uintVal);
			}

			ImGui::SameLine();
			ImGui::TextColored(Disassembly_ColorComment, "// expecting %d parameters", runScript.parameterCount);

			break;
		}
		case lhvm::Opcode::EndExcept:
			if (instruction.mode == lhvm::VMMode::EndExcept)
			{
				ImGui::TextColored(Disassembly_ColorKeyword, "ENDEXCEPT");
			}
			else // Mode::Yield
			{
				ImGui::TextColored(Disassembly_ColorKeyword, "YIELD");
			}
			break;
		case lhvm::Opcode::Jmp:
		case lhvm::Opcode::Wait:
		case lhvm::Opcode::Except:
			ImGui::TextColored(Disassembly_ColorKeyword, "%s", opcodeName);
			ImGui::SameLine();
			ImGui::TextColored(Disassembly_ColorConstant, "0x%04x", instruction.data.uintVal);
			break;
		case lhvm::Opcode::Swap:
			if (instruction.type == lhvm::DataType::Int)
			{
				ImGui::TextColored(Disassembly_ColorKeyword, "SWAP");
			}
			else
			{
				if (instruction.mode == lhvm::VMMode::CopyFrom)
				{
					ImGui::TextColored(Disassembly_ColorKeyword, "COPY from");
				}
				else // Mode::CopyTo
				{
					ImGui::TextColored(Disassembly_ColorKeyword, "COPY to");
				}
				ImGui::SameLine();
				ImGui::TextColored(Disassembly_ColorConstant, "%d", instruction.data.intVal);
			}
			break;
		default:
			ImGui::TextColored(Disassembly_ColorKeyword, "%s", opcodeName);
			break;
		}

		if (instruction.code == lhvm::Opcode::End)
		{
			break;
		}
	}

	ImGui::EndChild();

	ImGui::PopStyleColor(4);
}

void LHVMViewer::DrawTasksTab(const lhvm::LHVM& lhvm)
{
	const auto& tasks = lhvm.GetTasks();

	const auto selectedTaskID = _selectedTaskID; // the selected task may change while drawing

	ImGui::PushItemWidth(200);

	if (ImGui::BeginListBox("##tasks", ImVec2(240, ImGui::GetContentRegionAvail().y)))
	{
		for (auto const& taskEntry : tasks)
		{
			auto const& task = taskEntry.second;

			if (ImGui::Selectable(task.name.c_str(), task.id == selectedTaskID))
			{
				SelectTask(task.id);
			}

			if (_scrollToSelected && selectedTaskID == task.id)
			{
				ImGui::SetScrollHereY(0.25f);
				_scrollToSelected = false;
			}
		}
		ImGui::EndListBox();
	}

	ImGui::SameLine();

	if (tasks.contains(selectedTaskID))
	{
		auto task = tasks.at(selectedTaskID);

		ImGui::BeginChild("##task");
		ImGui::Text("Task ID: %d", task.id);

		ImGui::Text("Script ID: %d", task.scriptId);
		ImGui::SameLine();
		if (ImGui::TextButtonColored(Disassembly_ColorFuncName, std::to_string(task.scriptId).c_str()))
		{
			SelectScript(task.scriptId);
		}

		ImGui::Text("Type: %s", lhvm::k_ScriptTypeNames.at(task.type).c_str());

		ImGui::Text("Name: ");
		ImGui::SameLine();
		if (ImGui::TextButtonColored(Disassembly_ColorFuncName, task.name.c_str()))
		{
			SelectScript(task.scriptId);
		}

		ImGui::Text("File: %s", task.filename.c_str());
		ImGui::Text("Variables offset: 0x%04x", task.variablesOffset);
		ImGui::Text("Instruction address: 0x%04x", task.instructionAddress);
		ImGui::Text("Prev instruction address: 0x%04x", task.pevInstructionAddress);
		ImGui::Text("Ticks: %d", task.ticks);
		ImGui::Text("Sleeping: %s", task.sleeping ? "true" : "false");

		ImGui::Text("Waiting task number: ");
		if (task.waitingTaskId > 0)
		{
			ImGui::SameLine();
			if (ImGui::TextButtonColored(Disassembly_ColorFuncName, std::to_string(task.waitingTaskId).c_str()))
			{
				SelectTask(task.waitingTaskId);
			}
		}

		ImGui::Text("Stop: %s", task.stop ? "true" : "false");
		ImGui::Text("In exception handler: %s", task.inExceptionHandler ? "true" : "false");
		ImGui::Text("Current exception handler: %d", task.currentExceptionHandlerIndex);
		ImGui::Text("Yield: %s", task.iield ? "true" : "false");
		if (ImGui::BeginTabBar("##TaskTabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Local Variables"))
			{
				for (const auto& var : task.localVars)
				{
					ImGui::Text("%s = %s", var.name.c_str(), DataToString(var.value, var.type).c_str());
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Stack"))
			{
				DrawStack(task.stack);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Exception handlers"))
			{
				DrawExceptionHandlers(task.exceptionHandlerIps);
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
		ImGui::EndChild();
	}
}

void LHVMViewer::DrawStack(const openblack::lhvm::VMStack& stack)
{
	ImGui::BeginChild("##stack");

	if (_resetStackScroll)
	{
		ImGui::SetScrollHereY(0.0f);
		_resetStackScroll = false;
	}

	for (unsigned int i = 0; i < stack.count; i++)
	{
		ImGui::Text("%s", DataToString(stack.values.at(i), stack.types.at(i)).c_str());
	}

	ImGui::EndChild();
}

void LHVMViewer::DrawExceptionHandlers(const std::vector<uint32_t>& exceptionHandlerIps)
{
	ImGui::BeginChild("##exception_handlers");

	if (_resetExceptionHandlersScroll)
	{
		ImGui::SetScrollHereY(0.0f);
		_resetExceptionHandlersScroll = false;
	}

	for (const auto address : exceptionHandlerIps)
	{
		ImGui::Text("0x%04x", address);
	}

	ImGui::EndChild();
}

void LHVMViewer::DrawVariable(const openblack::lhvm::LHVM& lhvm, openblack::lhvm::VMScript& script, uint32_t idx)
{
	// local variable
	if (idx > script.variablesOffset)
	{
		const auto& variable = script.variables.at(idx - script.variablesOffset - 1);
		ImGui::TextColored(Disassembly_ColorVariable, "local %s", variable.c_str());
		return;
	}

	// global variable
	auto variable = lhvm.GetVariables()[idx - 1];
	ImGui::TextColored(Disassembly_ColorVariable, "global %s", variable.name.c_str());
}

std::string LHVMViewer::DataToString(lhvm::VMValue data, openblack::lhvm::DataType type)
{
	switch (type)
	{
	case lhvm::DataType::Int:
		return std::to_string(data.intVal);
	case lhvm::DataType::Float:
	case lhvm::DataType::Vector:
		return std::to_string(data.floatVal);
	case lhvm::DataType::Boolean:
		return data.intVal != 0 ? "true" : "false";
	case lhvm::DataType::Object:
		return std::to_string(data.uintVal) + " (object)";
	default:
		return std::to_string(data.intVal) + " (unk type)";
	}
}

void LHVMViewer::SelectScript(uint32_t idx)
{
	_selectedScriptID = idx;
	_scrollToSelected = true;
	_resetScriptDisassemblyScroll = true;
	_openScriptTab = true;
}

void LHVMViewer::SelectTask(uint32_t idx)
{
	_selectedTaskID = idx;
	_resetStackScroll = true;
	_resetExceptionHandlersScroll = true;
}

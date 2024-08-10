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
				if (ImGui::Selectable(variables.at(i).Name.c_str(), selected == i))
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
			ImGui::Text("Variable Name: %s", var.Name.c_str());
			ImGui::Text("Variable Value: %s", DataToString(var.Value, var.Type).c_str());
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

void LHVMViewer::Update([[maybe_unused]] openblack::Game& game, [[maybe_unused]] const openblack::Renderer& renderer) {}

void LHVMViewer::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) {}

void LHVMViewer::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) {}

void LHVMViewer::DrawScriptsTab(const openblack::LHVM::LHVM& lhvm)
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
			if (ImGui::Selectable(script.GetName().c_str(), script.GetScriptID() == _selectedScriptID))
			{
				SelectScript(script.GetScriptID());
			}

			if (_scrollToSelected && _selectedScriptID == script.GetScriptID())
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
	ImGui::Text("ID: %d", script.GetScriptID());
	ImGui::SameLine();
	ImGui::Text("Name: %s", script.GetName().c_str());
	ImGui::Text("Type: %s", LHVM::k_ScriptTypeNames.at(script.GetType()).c_str());
	ImGui::Text("File: %s", script.GetFileName().c_str());
	ImGui::Text("Instruction address: %d", script.GetInstructionAddress());
	ImGui::Text("Variables offset: %d", script.GetVariablesOffset());
	ImGui::SameLine();
	ImGui::Text("Parameter count: %d", script.GetParameterCount());
	if (ImGui::BeginTabBar("##ScriptTabs", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("Code"))
		{
			DrawScriptDisassembly(lhvm, script);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Local Variables"))
		{
			auto scriptVars = script.GetVariables();

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

void LHVMViewer::DrawScriptDisassembly(const openblack::LHVM::LHVM& lhvm, openblack::LHVM::VMScript& script)
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

	auto const& code = lhvm.GetInstructions();

	for (unsigned int i = script.GetInstructionAddress(); i < code.size(); i++)
	{
		auto const& instruction = code[i];
		auto const *const opcodeName = LHVM::k_OpcodeNames.at(static_cast<int>(instruction.Opcode)).c_str();
		auto const *const typeChar = LHVM::k_DataTypeChars.at(static_cast<uint32_t>(instruction.Type)).c_str();

		ImGui::TextColored(Disassembly_ColorComment, "0x%04x:", i);
		ImGui::SameLine();

		switch (instruction.Opcode)
		{
		case LHVM::Opcode::PUSH:
			ImGui::TextColored(Disassembly_ColorKeyword, "PUSH%s", typeChar);
			ImGui::SameLine();

			if (instruction.Mode == LHVM::Mode::REFERENCE)
			{
				DrawVariable(lhvm, script, instruction.IntVal);
			}
			else if (instruction.Mode == LHVM::Mode::IMMEDIATE)
			{
				ImGui::TextColored(Disassembly_ColorConstant, "%s", DataToString(instruction.Data, instruction.Type).c_str());
			}

			break;

		case LHVM::Opcode::POP:
			ImGui::TextColored(Disassembly_ColorKeyword, "POP%s", typeChar);
			if (instruction.Mode == LHVM::Mode::REFERENCE)
			{
				ImGui::SameLine();
				DrawVariable(lhvm, script, instruction.IntVal);
			}

			break;
		case LHVM::Opcode::ADD:
		case LHVM::Opcode::MINUS:
		case LHVM::Opcode::CAST:
			ImGui::TextColored(Disassembly_ColorKeyword, "%s%s", opcodeName, typeChar);
			break;
		case LHVM::Opcode::CALL:
			ImGui::TextColored(Disassembly_ColorKeyword, "SYS");
			ImGui::SameLine();
			ImGui::TextColored(Disassembly_ColorFuncName, "%s", lhvm.GetFunctions()->at(instruction.UintVal).Name.c_str());
			break;
		case LHVM::Opcode::RUN:
		{
			auto const& runScript = lhvm.GetScripts().at(instruction.UintVal - 1);

			ImGui::TextColored(Disassembly_ColorKeyword, "RUN");
			if (instruction.Mode == LHVM::Mode::ASYNC)
			{
				ImGui::SameLine();
				ImGui::TextColored(Disassembly_ColorKeyword, "async");
			}

			ImGui::SameLine();
			if (ImGui::TextButtonColored(Disassembly_ColorFuncName, runScript.GetName().c_str()))
			{
				SelectScript(instruction.UintVal);
			}

			ImGui::SameLine();
			ImGui::TextColored(Disassembly_ColorComment, "// expecting %d parameters", runScript.GetParameterCount());

			break;
		}
		case LHVM::Opcode::ENDEXCEPT:
			if (instruction.Mode == LHVM::Mode::ENDEXCEPT)
			{
				ImGui::TextColored(Disassembly_ColorKeyword, "ENDEXCEPT");
			}
			else // Mode::YIELD
			{
				ImGui::TextColored(Disassembly_ColorKeyword, "YIELD");
			}
			break;
		case LHVM::Opcode::JUMP:
		case LHVM::Opcode::WAIT:
		case LHVM::Opcode::EXCEPT:
			ImGui::TextColored(Disassembly_ColorKeyword, "%s", opcodeName);
			ImGui::SameLine();
			ImGui::TextColored(Disassembly_ColorConstant, "0x%04x", instruction.UintVal);
			break;
		case LHVM::Opcode::SWAP:
			if (instruction.Type == LHVM::DataType::INT)
			{
				ImGui::TextColored(Disassembly_ColorKeyword, "SWAP");
			}
			else
			{
				if (instruction.Mode == LHVM::Mode::COPYFROM)
				{
					ImGui::TextColored(Disassembly_ColorKeyword, "COPY from");
				}
				else // Mode::COPYTO
				{
					ImGui::TextColored(Disassembly_ColorKeyword, "COPY to");
				}
				ImGui::SameLine();
				ImGui::TextColored(Disassembly_ColorConstant, "%d", instruction.IntVal);
			}
			break;
		default:
			ImGui::TextColored(Disassembly_ColorKeyword, "%s", opcodeName);
			break;
		}

		if (instruction.Opcode == LHVM::Opcode::END)
		{
			break;
		}
	}

	ImGui::EndChild();

	ImGui::PopStyleColor(4);
}

void LHVMViewer::DrawTasksTab(const LHVM::LHVM& lhvm)
{
	const auto& tasks = lhvm.GetTasks();

	const auto selectedTaskID = _selectedTaskID; // the selected task may change while drawing

	ImGui::PushItemWidth(200);

	if (ImGui::BeginListBox("##tasks", ImVec2(240, ImGui::GetContentRegionAvail().y)))
	{
		for (auto const& taskEntry : tasks)
		{
			auto const& task = taskEntry.second;

			if (ImGui::Selectable(task.Name.c_str(), task.Id == selectedTaskID))
			{
				SelectTask(task.Id);
			}

			if (_scrollToSelected && selectedTaskID == task.Id)
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
		ImGui::Text("Task ID: %d", task.Id);

		ImGui::Text("Script ID: %d", task.ScriptId);
		ImGui::SameLine();
		if (ImGui::TextButtonColored(Disassembly_ColorFuncName, std::to_string(task.ScriptId).c_str()))
		{
			SelectScript(task.ScriptId);
		}

		ImGui::Text("Type: %s", LHVM::k_ScriptTypeNames.at(task.Type).c_str());

		ImGui::Text("Name: ");
		ImGui::SameLine();
		if (ImGui::TextButtonColored(Disassembly_ColorFuncName, task.Name.c_str()))
		{
			SelectScript(task.ScriptId);
		}

		ImGui::Text("File: %s", task.Filename.c_str());
		ImGui::Text("Variables offset: %d", task.VariablesOffset);
		ImGui::Text("Instruction address: %d", task.InstructionAddress);
		ImGui::Text("Prev instruction address: %d", task.PrevInstructionAddress);
		ImGui::Text("Ticks: %d", task.Ticks);
		ImGui::Text("Sleeping: %s", task.Sleeping ? "true" : "false");

		ImGui::Text("Waiting task number: ");
		if (task.WaitingTaskId > 0)
		{
			ImGui::SameLine();
			if (ImGui::TextButtonColored(Disassembly_ColorFuncName, std::to_string(task.WaitingTaskId).c_str()))
			{
				SelectTask(task.WaitingTaskId);
			}
		}

		ImGui::Text("Stop: %s", task.Stop ? "true" : "false");
		ImGui::Text("In exception handler: %s", task.InExceptionHandler ? "true" : "false");
		ImGui::Text("Current exception handler: %d", task.CurrentExceptionHandlerIndex);
		ImGui::Text("Yield: %s", task.Yield ? "true" : "false");
		if (ImGui::BeginTabBar("##TaskTabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Local Variables"))
			{
				for (const auto& var : task.LocalVars)
				{
					ImGui::Text("%s = %s", var.Name.c_str(), DataToString(var.Value, var.Type).c_str());
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Stack"))
			{
				DrawStack(task.Stack);
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Exception handlers"))
			{
				DrawExceptionHandlers(task.ExceptionHandlerIps);
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
		ImGui::EndChild();
	}
}

void LHVMViewer::DrawStack(const openblack::LHVM::VMStack& stack)
{
	ImGui::BeginChild("##stack");

	if (_resetStackScroll)
	{
		ImGui::SetScrollHereY(0.0f);
		_resetStackScroll = false;
	}

	for (unsigned int i = 0; i < stack.Count; i++)
	{
		ImGui::Text("%s", DataToString(stack.Values.at(i), stack.Types.at(i)).c_str());
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
		ImGui::Text("%d", address);
	}

	ImGui::EndChild();
}

void LHVMViewer::DrawVariable(const openblack::LHVM::LHVM& lhvm, openblack::LHVM::VMScript& script, uint32_t idx)
{
	// local variable
	if (idx > script.GetVariablesOffset())
	{
		const auto& variable = script.GetVariables().at(idx - script.GetVariablesOffset() - 1);
		ImGui::TextColored(Disassembly_ColorVariable, "local %s", variable.c_str());
		return;
	}

	// global variable
	auto variable = lhvm.GetVariables()[idx - 1];
	ImGui::TextColored(Disassembly_ColorVariable, "global %s", variable.Name.c_str());
}

std::string LHVMViewer::DataToString(LHVM::VMValue data, openblack::LHVM::DataType type)
{
	switch (type)
	{
	case LHVM::DataType::INT:
		return std::to_string(data.IntVal);
	case LHVM::DataType::FLOAT:
	case LHVM::DataType::VECTOR:
		return std::to_string(data.FloatVal) + "f";
	case LHVM::DataType::BOOLEAN:
		return data.IntVal != 0 ? "true" : "false";
	case LHVM::DataType::OBJECT:
		return std::to_string(data.UintVal) + " (object)";
	default:
		return std::to_string(data.IntVal) + " (unk type)";
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

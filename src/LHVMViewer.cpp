/* OpenBlack - A reimplementation of Lionheads's Black & White engine
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <LHVMViewer.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLEW

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_memory_editor.h>

using OpenBlack::LHVMViewer;

int LHVMViewer::SelectedScriptID = 1;
static bool ScrollToSelected = false;
const ImVec4 Disassembly_ColorBG = ImVec4(0.152f, 0.156f, 0.133f, 1.0f);
const ImVec4 Disassembly_ColorFG = ImVec4(0.972f, 0.972f, 0.949f, 1.0f);
const ImVec4 Disassembly_ColorComment = ImVec4(0.458f, 0.443f, 0.368f, 1.0f);
const ImVec4 Disassembly_ColorFuncName = ImVec4(0.650f, 0.886f, 0.180f, 1.0f);
const ImVec4 Disassembly_ColorKeyword = ImVec4(0.976f, 0.149f, 0.447f, 1.0f);
const ImVec4 Disassembly_ColorVariable = ImVec4(0.972f, 0.972f, 0.949f, 1.0f);
const ImVec4 Disassembly_ColorConstant = ImVec4(0.682f, 0.505f, 1.0f, 1.0f);

/* some imgui widgets, todo move to seperate file */

namespace ImGui {

inline void AddUnderLine(ImColor col_)
{
	ImVec2 min = ImGui::GetItemRectMin();
	ImVec2 max = ImGui::GetItemRectMax();
	min.y = max.y;
	ImGui::GetWindowDrawList()->AddLine(min, max, col_, 1.0f);
}

inline bool TextButtonColored(ImVec4 color, const char* name_)
{
	bool pressed = false;
	ImGui::TextColored(color, name_);
	if (ImGui::IsItemHovered())
	{
		if (ImGui::IsMouseClicked(0))
			pressed = true;

		AddUnderLine(color);
	}

	return pressed;
}

}

void LHVMViewer::Draw(OpenBlack::LHVM::LHVM *lhvm)
{
	ImGui::SetNextWindowSize(ImVec2(720.0f, 480.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("LHVM");

	if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("Scripts"))
		{
			DrawScriptsTab(lhvm);

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Variables"))
		{
			// left
			static int selected = 0;
			ImGui::BeginChild("left pane", ImVec2(240, 0), true);

			auto variables = lhvm->GetVariables();
			for (int i = 0; i < variables.size(); i++)
			{
				if (ImGui::Selectable(variables[i].c_str(), selected == i))
					selected = i;
			}

			ImGui::EndChild();
			ImGui::SameLine();

			// right
			ImGui::BeginChild("item view"); // Leave room for 1 line below us
			ImGui::Text("Variable ID: %d", selected);
			ImGui::Text("Variable Name: %s", variables[selected].c_str());
			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Data"))
		{
			static MemoryEditor lhvm_data_editor;
			auto const &data = lhvm->GetData();
			lhvm_data_editor.DrawContents((void*)data.data(), data.size(), 0);

			ImGui::EndTabItem();
		}


		ImGui::EndTabBar();
	}

	ImGui::End();
}

void LHVMViewer::DrawScriptsTab(OpenBlack::LHVM::LHVM *lhvm)
{
	static auto scripts_vector_getter = [](void* vec, int idx, const char** out_text) {
		auto& vector = *static_cast<std::vector<OpenBlack::LHVM::VMScript>*>(vec);
		if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
		*out_text = vector.at(idx).GetName().c_str();
		return true;
	};

	static auto vector_getter = [](void* vec, int idx, const char** out_text)
	{
		auto& vector = *static_cast<std::vector<std::string>*>(vec);
		if (idx < 0 || idx >= static_cast<int>(vector.size())) { return false; }
		*out_text = vector.at(idx).c_str();
		return true;
	};

	auto scripts = lhvm->GetScripts();

	ImGui::PushItemWidth(200);

	ImGui::ListBoxHeader("##scripts", scripts.size(), 24);

	for (auto const &script : scripts)
	{
		if (ImGui::Selectable(script.GetName().c_str(), script.GetScriptID() == SelectedScriptID))
			SelectedScriptID = script.GetScriptID();

		if (ScrollToSelected) {
			ImGui::SetScrollHereY(0.5f);
			ScrollToSelected = false;
		}
	}

	ImGui::ListBoxFooter();


	ImGui::SameLine();

	auto script = scripts[SelectedScriptID - 1];

	ImGui::BeginChild("##script");
	ImGui::Text("ID: %d", script.GetScriptID()); ImGui::SameLine();
	ImGui::Text("Name: %s", script.GetName().c_str());
	ImGui::Text("File: %s", script.GetFileName().c_str());
	ImGui::Text("Variable offset: %d", script.GetVariableOffset()); ImGui::SameLine();
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
			auto script_vars = script.GetVariables();

			static int selected_var = 0;
			ImGui::PushItemWidth(-1);
			ImGui::ListBox("", &selected_var, vector_getter, static_cast<void*>(&script_vars), script_vars.size(), 21);
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
	ImGui::EndChild();

}

void LHVMViewer::DrawScriptDisassembly(OpenBlack::LHVM::LHVM * lhvm, OpenBlack::LHVM::VMScript & script)
{
	ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, Disassembly_ColorBG);
	ImGui::PushStyleColor(ImGuiCol_Text, Disassembly_ColorFG);

	ImGui::BeginChild("##code");

	auto const &code = lhvm->GetInstructions();

	for (int i = script.GetInstructionAddress(); i < code.size(); i++)
	{
		auto const &instruction = code[i];

		ImGui::TextColored(Disassembly_ColorComment, "0x%04x:", i); ImGui::SameLine();

		switch (instruction.GetOpcode())
		{
		case LHVM::VMInstruction::Opcode::PUSH:
			ImGui::TextColored(Disassembly_ColorKeyword, "PUSH"); ImGui::SameLine();

			if (instruction.GetAccess() == LHVM::VMInstruction::Access::VARIABLE)
				DrawVariable(lhvm, script, instruction.GetData());
			else if (instruction.GetAccess() == LHVM::VMInstruction::Access::STACK)
				ImGui::TextColored(Disassembly_ColorConstant, "%s", DataToString(instruction.GetData(), instruction.GetDataType()).c_str());

			break;

		case LHVM::VMInstruction::Opcode::POP:
			ImGui::TextColored(Disassembly_ColorKeyword, "POP");
			if (instruction.GetAccess() == LHVM::VMInstruction::Access::VARIABLE)
			{
				ImGui::SameLine(); DrawVariable(lhvm, script, instruction.GetData());
			}

			break;
		case LHVM::VMInstruction::Opcode::ADD:
			if (instruction.GetDataType() == LHVM::VMInstruction::DataType::INT)
				ImGui::TextColored(Disassembly_ColorKeyword, "ADDI");
			else if (instruction.GetDataType() == LHVM::VMInstruction::DataType::FLOAT)
				ImGui::TextColored(Disassembly_ColorKeyword, "ADDF");
			else if (instruction.GetDataType() == LHVM::VMInstruction::DataType::VECTOR)
				ImGui::TextColored(Disassembly_ColorKeyword, "ADDV");
			else
				ImGui::TextColored(Disassembly_ColorKeyword, "ADD");
			break;
		case LHVM::VMInstruction::Opcode::CALL:
			ImGui::TextColored(Disassembly_ColorKeyword, "CALL"); ImGui::SameLine();
			ImGui::TextColored(Disassembly_ColorFuncName, "[FUNCTION %d]", instruction.GetData());
			break;
		case LHVM::VMInstruction::Opcode::RUN:
		{
			auto const &run_script = lhvm->GetScripts().at(instruction.GetData() - 1);

			ImGui::TextColored(Disassembly_ColorKeyword, "RUN"); ImGui::SameLine();
			if (ImGui::TextButtonColored(Disassembly_ColorFuncName, run_script.GetName().c_str()))
				SelectedScriptID = instruction.GetData() - 1;

			ImGui::SameLine();
			ImGui::TextColored(Disassembly_ColorComment, "// expecting %d parameters", run_script.GetParameterCount());

			break;
		}
		default:
			ImGui::TextColored(Disassembly_ColorKeyword, instruction.Disassemble().c_str());
			break;
		}

		if (instruction.GetOpcode() == LHVM::VMInstruction::Opcode::END)
			break;
	}

	ImGui::EndChild();

	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
}

void OpenBlack::LHVMViewer::DrawVariable(OpenBlack::LHVM::LHVM *lhvm, OpenBlack::LHVM::VMScript &script, uint32_t idx)
{
	// local variable
	if (idx > script.GetVariableOffset())
	{
		std::string variable = script.GetVariables()[idx - script.GetVariableOffset() - 1];
		ImGui::TextColored(Disassembly_ColorVariable, "local %s", variable.c_str());
		return;
	}

	// global variable
	std::string variable = lhvm->GetVariables()[idx - 1];
	ImGui::TextColored(Disassembly_ColorVariable, "global %s", variable.c_str());
}

std::string OpenBlack::LHVMViewer::DataToString(uint32_t data, OpenBlack::LHVM::VMInstruction::DataType type)
{
	switch (type)
	{
	case LHVM::VMInstruction::DataType::INT:
		return std::to_string(data);
	case LHVM::VMInstruction::DataType::FLOAT:
	case LHVM::VMInstruction::DataType::VECTOR:
		return std::to_string(*reinterpret_cast<float *>(&data)) + "f";
	case LHVM::VMInstruction::DataType::BOOLEAN:
		return data ? "true" : "false";
	default:
		return std::to_string(data) + " (unk type)";
	}
}

void OpenBlack::LHVMViewer::SelectScript(int idx)
{
	SelectedScriptID = idx;
	ScrollToSelected = true;
}

/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <LHVM/LHVM.h>
#include <imgui.h>

#include <cstdint>

namespace openblack
{
class LHVMViewer
{
public:
	static void Draw(const openblack::LHVM::LHVM*);
	static void DrawScriptsTab(const openblack::LHVM::LHVM*);
	static void DrawScriptDisassembly(const openblack::LHVM::LHVM*, openblack::LHVM::VMScript&);

	static void DrawVariable(const openblack::LHVM::LHVM*, openblack::LHVM::VMScript&, uint32_t idx);
	static std::string DataToString(uint32_t data, openblack::LHVM::VMInstruction::DataType type);

	static uint32_t SelectedScriptID;
	static void SelectScript(uint32_t idx);
};
} // namespace openblack

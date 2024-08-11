/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <LHVM/LHVM.h>

#include "Window.h"

namespace openblack::debug::gui
{

class LHVMViewer: public Window
{
public:
	LHVMViewer();

protected:
	void Draw(Game& game) override;
	void Update(Game& game) override;
	void ProcessEventOpen(const SDL_Event& event) override;
	void ProcessEventAlways(const SDL_Event& event) override;

private:
	void DrawScriptsTab(const openblack::LHVM::LHVM&);
	void DrawScriptDisassembly(const openblack::LHVM::LHVM&, openblack::LHVM::VMScript&);

	void DrawVariable(const openblack::LHVM::LHVM&, openblack::LHVM::VMScript&, uint32_t idx);
	std::string DataToString(uint32_t data, openblack::LHVM::VMInstruction::DataType type);
	void SelectScript(uint32_t idx);

	uint32_t _selectedScriptID {1};
	bool _scrollToSelected {false};
	bool _resetScriptDisassemblyScroll {false};
};

} // namespace openblack::debug::gui

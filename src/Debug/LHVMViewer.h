/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <LHVM.h>

#include "Window.h"

namespace openblack::debug::gui
{

class LHVMViewer: public Window
{
public:
	LHVMViewer(std::vector<LHVM::NativeFunction>* functions);

protected:
	void Draw(Game& game) override;
	void Update(Game& game, const Renderer& renderer) override;
	void ProcessEventOpen(const SDL_Event& event) override;
	void ProcessEventAlways(const SDL_Event& event) override;

private:
	void DrawScriptsTab(const openblack::LHVM::LHVM&);
	void DrawScriptDisassembly(const openblack::LHVM::LHVM&, openblack::LHVM::VMScript&);

	void DrawVariable(const openblack::LHVM::LHVM&, openblack::LHVM::VMScript&, uint32_t idx);
	void SelectScript(uint32_t idx);

	void DrawTasksTab(const openblack::LHVM::LHVM& lhvm);
	void DrawStack(const openblack::LHVM::VMStack& stack);
	void DrawExceptionHandlers(const openblack::LHVM::VMExceptStruct& exceptStruct);
	void SelectTask(uint32_t idx);

	std::vector<LHVM::NativeFunction>* _functions {NULL};

	uint32_t _selectedScriptID {1};
	bool _openScriptTab {false};
	bool _scrollToSelected {false};
	bool _resetScriptDisassemblyScroll {false};

	uint32_t _selectedTaskID {0};
	bool _resetStackScroll {false};
	bool _resetExceptionHandlersScroll {false};

	static std::string DataToString(LHVM::VMValue data, openblack::LHVM::DataType type);
};

} // namespace openblack::debug::gui

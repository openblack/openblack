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
	LHVMViewer();

protected:
	void Draw(Game& game) override;
	void Update(Game& game, const Renderer& renderer) override;
	void ProcessEventOpen(const SDL_Event& event) override;
	void ProcessEventAlways(const SDL_Event& event) override;

private:
	void DrawScriptsTab(const openblack::lhvm::LHVM&);
	void DrawScriptDisassembly(const openblack::lhvm::LHVM&, openblack::lhvm::VMScript&);

	void DrawVariable(const openblack::lhvm::LHVM&, openblack::lhvm::VMScript&, uint32_t idx);
	void SelectScript(uint32_t idx);

	void DrawTasksTab(const openblack::lhvm::LHVM& lhvm);
	void DrawStack(const openblack::lhvm::VMStack& stack);
	void DrawExceptionHandlers(const std::vector<uint32_t>& exceptionHandlerIps);
	void SelectTask(uint32_t idx);

	uint32_t _selectedScriptID {1};
	bool _openScriptTab {false};
	bool _scrollToSelected {false};
	bool _resetScriptDisassemblyScroll {false};

	uint32_t _selectedTaskID {0};
	bool _resetStackScroll {false};
	bool _resetExceptionHandlersScroll {false};

	static std::string DataToString(lhvm::VMValue data, openblack::lhvm::DataType type);
};

} // namespace openblack::debug::gui

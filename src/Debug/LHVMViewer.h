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

class LHVMViewer final: public Window
{
public:
	LHVMViewer() noexcept;

protected:
	void Draw() noexcept override;
	void Update() noexcept override;
	void ProcessEventOpen(const SDL_Event& event) noexcept override;
	void ProcessEventAlways(const SDL_Event& event) noexcept override;

private:
	void DrawScriptsTab(const lhvm::LHVM&) noexcept;
	void DrawScriptDisassembly(const lhvm::LHVM&, lhvm::VMScript&) noexcept;

	void DrawVariable(const lhvm::LHVM&, lhvm::VMScript&, uint32_t idx) noexcept;
	void SelectScript(uint32_t idx) noexcept;

	void DrawTasksTab(const lhvm::LHVM& lhvm) noexcept;
	void DrawStack(const lhvm::VMStack& stack) noexcept;
	void DrawExceptionHandlers(const std::vector<uint32_t>& exceptionHandlerIps) noexcept;
	void SelectTask(uint32_t idx) noexcept;

	uint32_t _selectedScriptID {1};
	bool _openScriptTab {false};
	bool _scrollToSelected {false};
	bool _resetScriptDisassemblyScroll {false};

	uint32_t _selectedTaskID {0};
	bool _resetStackScroll {false};
	bool _resetExceptionHandlersScroll {false};

	static std::string DataToString(lhvm::VMValue data, lhvm::DataType type) noexcept;
};

} // namespace openblack::debug::gui

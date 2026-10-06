/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Editor/Scripts/ScriptModel.h"

struct MemoryEditor;

namespace openblack::lhvm
{
class LHVM;
}

namespace openblack::editor
{

/// The loaded script program, laid out to read and to debug:
/// - the browser lists the scripts, by kind and searched, with how many tasks run each
/// - the code view shows a script's code, coloured, with every name filled in, jumps to click along and back, a search,
///   breakpoints set in its margin, the picked task's next instruction marked, and the source beside it once a
///   decompiler is linked, to read in the scripts' own language and, with a compiler, to edit
/// - the side tabs show the script's details and starting it, the running tasks with their stacks and variables and
///   holding, stepping and continuing them, the global variables to change in place, the natives with which openblack
///   has written, and the program's data
/// Scripts start through the virtual machine's own way of starting them, as the game starts them.
class ScriptsPanel
{
public:
	ScriptsPanel() noexcept;
	ScriptsPanel(const ScriptsPanel&) = delete;
	ScriptsPanel& operator=(const ScriptsPanel&) = delete;
	~ScriptsPanel() noexcept;

	void Draw() noexcept;

private:
	struct Caches
	{
		/// What the caches were worked out for, so a new program starts them afresh
		const void* code {nullptr};
		size_t size {0};
		std::vector<scripts::NativeUse> natives;
		std::vector<uint32_t> visibleScripts;
		std::string filterKey;
		int sortColumn {0};
		bool sortAscending {true};
		std::map<uint32_t, std::optional<scripts::DecompiledSource>> decompiled;
	};

	/// Loading another program, or the game's own again
	void DrawLoading(lhvm::LHVM& vm) noexcept;
	void Refresh(const lhvm::LHVM& vm, const scripts::Program& program) noexcept;
	void DrawBrowser(const lhvm::LHVM& vm, const scripts::Program& program) noexcept;
	void DrawCode(lhvm::LHVM& vm, const scripts::Program& program) noexcept;
	void DrawDisassembly(lhvm::LHVM& vm, const scripts::Program& program, const lhvm::VMScript& script,
	                     scripts::CodeRange range) noexcept;
	void DrawDecompiled(const scripts::Program& program, const lhvm::VMScript& script) noexcept;
	void DrawSide(lhvm::LHVM& vm, const scripts::Program& program) noexcept;
	void DrawDetails(lhvm::LHVM& vm, const scripts::Program& program) noexcept;
	void DrawTasks(lhvm::LHVM& vm, const scripts::Program& program) noexcept;
	void DrawGlobals(lhvm::LHVM& vm, const scripts::Program& program) noexcept;
	void DrawNatives(const scripts::Program& program) noexcept;
	void DrawData(const scripts::Program& program) noexcept;
	/// Draws a line's tokens, each clickable one leading to what it names
	void DrawTokens(const scripts::Program& program, const std::vector<scripts::Token>& tokens) noexcept;

	void SelectScript(uint32_t scriptId) noexcept;
	/// Shows an address in the code view, remembering where it was to go back to
	void GoTo(const scripts::Program& program, uint32_t address, bool remember = true) noexcept;
	/// The picked task's next instruction, if it is still running
	[[nodiscard]] std::optional<uint32_t> TaskAddress(const lhvm::LHVM& vm) const noexcept;

	Caches _caches;
	scripts::ScriptFilter _filter;
	uint32_t _scriptId {0};
	/// The address the code view is on, and where it has been
	std::optional<uint32_t> _address;
	std::vector<uint32_t> _back;
	bool _scrollToAddress {false};
	std::string _search;
	std::string _goTo;
	uint32_t _taskId {0};
	/// The code view keeps to the picked task's next instruction as it runs
	bool _followTask {true};
	std::vector<float> _parameters;
	std::string _lastStart;
	std::string _loadMessage;
	std::string _globalsFilter;
	std::optional<uint32_t> _editingGlobal;
	std::string _editText;
	std::optional<uint32_t> _native;
	std::string _nativesFilter;
	bool _onlyUsedNatives {true};
	bool _onlyUnwrittenNatives {false};
	/// The side tab to bring to the front, once
	std::optional<int> _showSideTab;
	std::unique_ptr<MemoryEditor> _dataView;
	/// The source being edited, to be compiled, and what the compiler said of it
	bool _editingSource {false};
	std::string _sourceText;
	std::vector<std::string> _compileMessages;
};

} // namespace openblack::editor

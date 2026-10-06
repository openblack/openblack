/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <LHVMFile.h>

#include "ScriptModel.h"

namespace openblack::editor::scripts
{

/// Whether a decompiler is linked in to turn scripts into source
[[nodiscard]] bool HasDecompiler();
/// A script's code as source, with each line's instruction, from the decompiler; none without one
[[nodiscard]] std::optional<DecompiledSource> Decompile(const Program& program, const lhvm::VMScript& script);

/// What became of compiling edited source
struct CompileResult
{
	bool compiled {false};
	std::vector<std::string> diagnostics;
	/// The program with the script replaced: its new code is added after the old, so the other scripts and running
	/// tasks keep their addresses
	std::shared_ptr<const lhvm::LHVMFile> program;
};
/// Whether a compiler is linked in to turn edited source back into a program
[[nodiscard]] bool HasCompiler();
/// Compiles a script's edited source into the program; says why not when it doesn't compile
[[nodiscard]] CompileResult Compile(const Program& program, const lhvm::VMScript& script, std::string_view source);

} // namespace openblack::editor::scripts

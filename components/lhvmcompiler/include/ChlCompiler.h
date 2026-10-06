/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

/// Compiler from Challenge Language source to LHVM bytecode, written to reproduce Black & White's own compiler
/// instruction for instruction:
///
///     std::vector<SourceFile> sources = {{.name = "Land1.txt", .text = text}};
///     CompileOptions options {.constants = &constants};
///     auto result = Compile(sources, options);
///     if (result.program) { result.program->Write("challenge.chl"); }
///     for (const auto& diagnostic : result.diagnostics) { std::puts(diagnostic.ToString().c_str()); }
///
/// Files are compiled in the order given, as one program: globals are numbered in declaration order across all files,
/// scripts in definition order, and every string literal is appended to the data section.
///
/// CompileScript() recompiles one script of an existing program, for editing scripts while the game runs.

#include <cstdint>

#include <optional>
#include <span>
#include <string>
#include <vector>

#include <LHVMFile.h>

#include "ChlConstants.h"
#include "ChlDiagnostics.h"
#include "LHVMNatives.h"

namespace openblack::lhvm::chl
{

struct SourceFile
{
	/// File name stored with each script, as the game's compiler did ("Land1.txt")
	std::string name;
	std::string text;
};

struct CompileOptions
{
	/// Native function table. Empty means DefaultNativeSignatures().
	std::span<const NativeSignature> natives;
	/// Named constants from the script header files. Without them only numbers can be written in constant positions.
	const ConstantTable* constants {nullptr};
	/// Id of the first script; later ones count up from it
	uint32_t firstScriptId {1};
};

struct CompileResult
{
	/// The program, unless there were errors
	std::optional<LHVMFile> program;
	std::vector<CompileDiagnostic> diagnostics;

	[[nodiscard]] bool Succeeded() const { return program.has_value(); }
};

/// Compile source files into a program
[[nodiscard]] CompileResult Compile(std::span<const SourceFile> sources, const CompileOptions& options = {});

/// Compile `source`, which holds one or more scripts, into a copy of `program`. A script with the name of an existing
/// one replaces it: its new code is appended to the instruction stream and its record points there, so the other
/// scripts' addresses stay valid. New scripts are added at the end. The source can use the program's globals and call
/// its scripts; new `global` declarations are appended.
[[nodiscard]] CompileResult CompileScript(const LHVMFile& program, const SourceFile& source,
                                          const CompileOptions& options = {});

} // namespace openblack::lhvm::chl

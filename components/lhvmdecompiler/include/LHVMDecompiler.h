/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

/// Turns LHVM bytecode (compiled Black & White challenge scripts) back into Challenge Language (CHL) source.
///
/// Usage:
///
///     lhvm::LHVMFile file;
///     file.Open("Scripts/Quests/challenge.chl");
///     const auto program = lhvm::ProgramView::From(file);
///     const auto script = lhvm::DecompileScript(program, scriptIndex);
///     // script.text is the source of one script; script.lines[n] lists the instruction addresses that output line n
///     // (0-based) was made from, and script.LineForInstruction(ip) goes the other way.
///
/// A running virtual machine can be decompiled too, by filling a ProgramView from its instruction, script and data
/// tables (see ProgramView). Decompiling never fails: control flow that can't be structured falls back to labels and
/// goto, and anything odd is reported in the diagnostics.
///
/// The output follows the layout described in ChlSyntax.h:
///
///     begin help script StandardReminder(AdvisorSpeak)
///     start
///         begin dialogue
///             if good spirit speaks constant AdvisorSpeak
///                 eject good spirit
///             end if
///             say single line constant AdvisorSpeak
///             wait until read
///         end dialogue
///     end script StandardReminder
///
/// Native calls are written in their statement forms (ChlForms.h); those without one use the `native NAME(args)`
/// extension. The aim is source that compiles back to the same bytecode; each script's diagnostics list the places
/// where that isn't possible. What the bytecode can't tell is written one fixed way: constants by their full header
/// names (or numbers without headers), "wait N seconds" rather than "wait until N seconds", "radius 1" and "loop 1"
/// spelt out, "help"-less script kinds as recorded, and no comments, global initial values, constant aliases or
/// "begin known dialogue" blocks.

#include <cstdint>

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <LHVMTypes.h>

#include "ChlAst.h"
#include "ChlConstants.h"
#include "LHVMNatives.h"

namespace openblack::lhvm
{

class LHVMFile;

/// Read-only view of the tables a program is made of. The viewed storage must outlive the view.
struct ProgramView
{
	std::span<const VMInstruction> instructions;
	std::span<const VMScript> scripts;
	/// Global variable names: the variable numbered N in the bytecode is globalNames[N - 1]
	std::span<const std::string> globalNames;
	/// The string data that string literals point into
	std::span<const char> data;
	/// Ids (1-based script numbers) of the scripts started when the program loads
	std::span<const uint32_t> autostart;

	[[nodiscard]] static ProgramView From(const LHVMFile& file);
};

struct DecompileOptions
{
	/// Native function table used to name calls and split their arguments. Empty means DefaultNativeSignatures().
	std::span<const NativeSignature> natives;
	/// Named constants (from the script headers) to write constant arguments by name; numbers are written otherwise
	const chl::ConstantTable* constants {nullptr};
	/// Indentation for one nesting level
	std::string indent {"\t"};
	/// The challenge an earlier "challenge NAME" line of the same file names, if any. Snapshots and highlights take
	/// their challenge from the last such line.
	std::optional<int32_t> challenge;
	/// Lay files out so each statement sits on the source line the bytecode records for it, padding with blank lines
	/// where the original had comments or space (DecompiledProgram::FileText). Compiling the result then reproduces the
	/// line numbers too, as far as the gaps allow.
	bool sourceLines {false};
	/// The script is a file of its own: the first challenge it needs is named above "begin script", where the grammar
	/// allows it. Otherwise it is left to the caller to name at the top of the file (DecompiledScript::leadingChallenge).
	bool standalone {true};
};

enum class DiagnosticSeverity : uint8_t
{
	Info,
	Warning,
	Error
};

struct Diagnostic
{
	DiagnosticSeverity severity {DiagnosticSeverity::Info};
	/// Instruction address the message is about
	uint32_t ip {0};
	std::string message;
};

struct DecompiledScript
{
	size_t scriptIndex {0};
	std::string name;
	/// The script's instructions are [firstIp, endIp)
	uint32_t firstIp {0};
	uint32_t endIp {0};
	/// The source, lines separated by '\n' and ending with one
	std::string text;
	/// For every output line (0-based), the instruction addresses it was made from; empty for lines like braces that
	/// stand for no code
	std::vector<std::vector<uint32_t>> lines;
	std::vector<Diagnostic> diagnostics;
	/// Number of goto statements needed where the control flow could not be structured
	uint32_t gotoCount {0};
	/// Number of other places written with something the Challenge Language can't express (break, low-level comments)
	uint32_t fallbackCount {0};
	/// Native calls written as `native NAME(args)` for want of a statement form
	uint32_t nativeCallCount {0};
	/// Instructions that no output line accounts for (always 0 unless the decompiler has a bug)
	uint32_t unaccountedCount {0};
	/// Instructions with an opcode the decompiler doesn't know
	uint32_t unsupportedCount {0};

	/// The syntax tree the text was written from
	std::shared_ptr<const chl::Script> ast;

	/// For every output line, the source line the bytecode records for its statement, or 0 when it has none
	std::vector<uint32_t> sourceLines;

	/// The "challenge NAME" line the script needs before any other, when none was in effect: written above "begin
	/// script" when standalone, otherwise for the caller to write at the top of the file
	std::string leadingChallenge;
	/// The challenge in effect after the script, to pass on to the next script of the same file
	std::optional<int32_t> challenge;

	/// True when the script was structured without goto or any low-level fallback
	[[nodiscard]] bool IsClean() const { return gotoCount == 0 && fallbackCount == 0 && unaccountedCount == 0; }

	/// The output line showing the instruction at `ip`, if it belongs to this script
	[[nodiscard]] std::optional<size_t> LineForInstruction(uint32_t ip) const;
};

struct DecompileStats
{
	size_t scripts {0};
	size_t clean {0};
	size_t withGoto {0};
	size_t withFallback {0};
	size_t instructions {0};
	size_t unaccountedInstructions {0};
	size_t unsupportedOpcodes {0};
	size_t nativeCalls {0};
	size_t gotos {0};
};

/// One of the source files the program was compiled from, as the original compiler read them: an optional challenge
/// line, the globals the file declares, the scripts it runs at load, and its scripts
struct DecompiledFile
{
	/// The source file name the scripts record
	std::string name;
	/// Everything before the first script
	std::string header;
	/// Indices into DecompiledProgram::scripts
	std::vector<size_t> scripts;
};

struct DecompiledProgram
{
	std::vector<DecompiledScript> scripts;
	std::vector<DecompiledFile> files;
	DecompileStats stats;
	/// The files are laid out on their recorded source lines (DecompileOptions::sourceLines)
	bool sourceLines {false};

	/// The text of one file: its header and its scripts, separated by blank lines
	[[nodiscard]] std::string FileText(const DecompiledFile& file) const;
	/// Every file's text, one after the other
	[[nodiscard]] std::string Text() const;
};

/// Decompile one script, given by its index in ProgramView::scripts
[[nodiscard]] DecompiledScript DecompileScript(const ProgramView& program, size_t scriptIndex,
                                               const DecompileOptions& options = {});

/// Decompile every script, grouped into the source files they came from
[[nodiscard]] DecompiledProgram DecompileAll(const ProgramView& program, const DecompileOptions& options = {});

/// Basic blocks and control flow edges of one script, for tools that want to draw or inspect the flow
struct ControlFlowGraph
{
	struct Block
	{
		/// The block's instructions are [begin, end)
		uint32_t begin {0};
		uint32_t end {0};
		/// Indices of the blocks control can pass to: fall through first, then the jump target
		std::vector<size_t> successors;
		std::vector<size_t> predecessors;
		/// Index of the block the exception handler installed here starts at, if any
		std::optional<size_t> handler;
		bool reachable {false};
	};

	std::vector<Block> blocks;

	/// Index of the block holding `ip`
	[[nodiscard]] std::optional<size_t> BlockAt(uint32_t ip) const;
};

/// Build the control flow graph of the script's instructions [firstIp, endIp)
[[nodiscard]] ControlFlowGraph BuildControlFlowGraph(std::span<const VMInstruction> instructions, uint32_t firstIp,
                                                     uint32_t endIp);

/// The [first, end) instruction range of each script: from its start address to the next script's
[[nodiscard]] std::vector<std::pair<uint32_t, uint32_t>> ScriptRanges(const ProgramView& program);

} // namespace openblack::lhvm

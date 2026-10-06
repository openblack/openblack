/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <LHVMTypes.h>

/// What the editor's Scripts panel shows of a loaded script program, worked out from plain views of the program so it can
/// be tested on made-up programs: the scripts sorted by kind and searched, their code made readable with every name
/// resolved, the natives they call and whether openblack has written them, values read and typed in, and what each task
/// is doing or waiting for.
namespace openblack::editor::scripts
{

/// A loaded program, as views over the virtual machine's tables
struct Program
{
	std::span<const lhvm::VMInstruction> code;
	std::span<const lhvm::VMScript> scripts;
	std::span<const lhvm::VMVar> globals;
	std::span<const lhvm::NativeFunction> natives;
	std::span<const char> data;
};

/// The kinds of script the browser filters by
enum class Category : uint8_t
{
	Script,
	Help,
	ChallengeHelp,
	TempleHelp,
	TempleSpecial,
	MultiplayerHelp,
	Other,

	_Count
};
constexpr size_t k_CategoryCount = static_cast<size_t>(Category::_Count);
[[nodiscard]] Category CategoryOf(lhvm::ScriptType type);
[[nodiscard]] std::string_view Name(Category category);

struct ScriptFilter
{
	/// Words each to be found in the script's name or file, ignoring case
	std::string text;
	std::array<bool, k_CategoryCount> categories {true, true, true, true, true, true, true};
};
[[nodiscard]] bool Matches(const lhvm::VMScript& script, const ScriptFilter& filter);

/// A run of instructions, from the first to one past the last
struct CodeRange
{
	uint32_t begin {0};
	uint32_t end {0};

	[[nodiscard]] bool Contains(uint32_t address) const { return address >= begin && address < end; }
	[[nodiscard]] uint32_t Size() const { return end - begin; }
};
/// A script's code: from where it starts up to and with its END
[[nodiscard]] CodeRange RangeOf(std::span<const lhvm::VMInstruction> code, const lhvm::VMScript& script);
/// The script whose code holds an address, if any
[[nodiscard]] const lhvm::VMScript* ScriptAt(std::span<const lhvm::VMInstruction> code, std::span<const lhvm::VMScript> scripts,
                                             uint32_t address);
/// Where a jump, a conditional jump or an exception handler's start goes
[[nodiscard]] std::optional<uint32_t> JumpTarget(const lhvm::VMInstruction& instruction);

/// A piece of a line of disassembly, coloured by its kind, and what it leads to when clicked: a jump's address, a
/// script's id or a native's number
struct Token
{
	enum class Kind : uint8_t
	{
		Opcode,
		Number,
		Global,
		Local,
		Native,
		Script,
		Jump,
		String,
		Comment,
	};
	Kind kind {Kind::Opcode};
	std::string text;
	uint32_t target {0};
};
/// The instruction at an address made readable: its opcode and type, then its operand with variables, natives,
/// scripts, jump targets and strings named. The script gives the local variables' names.
[[nodiscard]] std::vector<Token> Disassemble(const Program& program, const lhvm::VMScript* script, uint32_t address);
/// The tokens as one line of plain text
[[nodiscard]] std::string LineText(std::span<const Token> tokens);
/// The next address after another in a range whose line holds the search, going round to the start; none if no line does
[[nodiscard]] std::optional<uint32_t> FindNext(const Program& program, const lhvm::VMScript* script, CodeRange range,
                                               std::string_view search, std::optional<uint32_t> after);
/// The name of a variable an instruction refers to, local to the script or global
[[nodiscard]] std::string VariableName(const Program& program, const lhvm::VMScript* script, uint32_t id, bool& local);
/// The text at an offset of the program's data, if one starts there: printable, at least two characters, ended by a
/// nul, and at the start of the data or just after another nul
[[nodiscard]] std::optional<std::string_view> StringAt(std::span<const char> data, int32_t offset);

[[nodiscard]] std::string_view TypeName(lhvm::DataType type);
/// A value as text, by its type
[[nodiscard]] std::string FormatValue(lhvm::VMValue value, lhvm::DataType type);
/// A value typed in, by the type it is to have: a whole number, a number, true or false, or an object's number
[[nodiscard]] std::optional<lhvm::VMValue> ParseValue(std::string_view text, lhvm::DataType type);

/// A native called by a script, by its number, and how many times its code calls it
struct NativeUse
{
	uint32_t native {0};
	uint32_t calls {0};
};
/// The natives a run of code calls, in order of their numbers
[[nodiscard]] std::vector<NativeUse> NativesCalled(std::span<const lhvm::VMInstruction> code, CodeRange range);
/// Whether openblack has written a native, by the sorted numbers of those it hasn't
[[nodiscard]] bool IsImplemented(uint32_t native, std::span<const uint32_t> unimplemented);
/// How many of the natives used are written
struct Coverage
{
	size_t used {0};
	size_t implemented {0};
};
[[nodiscard]] Coverage CoverageOf(std::span<const NativeUse> uses, std::span<const uint32_t> unimplemented);

/// What a task is doing
enum class TaskState : uint8_t
{
	/// Runs at each of the virtual machine's turns
	Running,
	/// Held by the debugger, at a breakpoint or by hand
	Held,
	/// Waits for a script it started to finish
	Waiting,
	/// Its last SLEEP has yet to pass
	Sleeping,
	/// Runs one of its exception handlers
	InExceptionHandler,
	/// Has ended and goes at the end of the turn
	Stopping,
};
[[nodiscard]] TaskState StateOf(const lhvm::VMTask& task, bool held);
[[nodiscard]] std::string_view Name(TaskState state);
/// What the task is doing, as a sentence, with the name of the task it waits for when it waits for one
[[nodiscard]] std::string Describe(const lhvm::VMTask& task, bool held, std::string_view waitedFor);

/// A script's code as source, from a decompiler, and which instruction each line of it comes from
struct DecompiledSource
{
	std::vector<std::string> lines;
	/// Each line's instruction address, if it has one
	std::vector<std::optional<uint32_t>> lineAddresses;
	std::vector<std::string> diagnostics;
};
/// The line of source that shows an address: the last line at or before it in the script's code
[[nodiscard]] std::optional<size_t> LineOf(const DecompiledSource& source, uint32_t address);

/// A piece of a line of script source, coloured by its kind
struct SourceToken
{
	enum class Kind : uint8_t
	{
		Text,
		Keyword,
		Number,
		String,
		Comment,
	};
	Kind kind {Kind::Text};
	std::string_view text;
};
/// Whether a word is one of the script language's own, such as begin, while, wait or until, ignoring case
[[nodiscard]] bool IsKeyword(std::string_view word);
/// A line of script source cut into its keywords, numbers, quoted strings, comment and the rest, in order, together
/// making the whole line
[[nodiscard]] std::vector<SourceToken> HighlightSource(std::string_view line);

} // namespace openblack::editor::scripts

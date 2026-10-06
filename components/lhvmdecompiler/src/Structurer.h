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
#include <optional>
#include <set>
#include <span>
#include <string>
#include <vector>

#include "ChlAst.h"
#include "LHVMDecompiler.h"

namespace openblack::lhvm::detail
{

using namespace openblack::lhvm::chl;

/// A value on the simulated stack: one expression filling `width` slots (3 for a position, 1 otherwise)
struct StackEntry
{
	ExprPtr expr;
	uint8_t width {1};
};

/// Rebuilds statements and structured control flow from a script's instructions.
///
/// The bytecode is a stack machine, so expressions are recovered by running the instructions over a stack of
/// expression trees. The compiler lays out every construct the same way (conditions are straight-line code ending in a
/// conditional jump, loops end with a backward jump, exception handlers follow the code they guard), so control flow is
/// structured by recognising those layouts over address ranges, with the jump targets known from the control flow
/// graph. Anything that doesn't fit becomes a labelled goto.
class Structurer
{
public:
	Structurer(const ProgramView& program, const VMScript& script, std::span<const NativeSignature> natives,
	           const DecompileOptions& options, uint32_t firstIp, uint32_t endIp, std::set<uint32_t> labels);

	/// Statements for the instructions [lo, hi). Instructions that end up on no statement (jumps to the next
	/// instruction, discarded loads) are appended to `trailing` for the caller to show on a closing line.
	[[nodiscard]] StmtList ParseSequence(uint32_t lo, uint32_t hi, std::vector<uint32_t>& trailing);

	/// when/until handlers in [lo, hi); `after` is where an until handler leaves to
	[[nodiscard]] StmtList ParseHandlers(uint32_t lo, uint32_t hi, uint32_t after, std::vector<uint32_t>& trailing);

	/// The [handler, after) layout of the exception block opened at `ip`, if it follows the compiler's pattern
	struct ExceptionLayout
	{
		uint32_t handler {0};
		uint32_t after {0};
		/// The guarded code ends with an explicit end-of-exceptions instruction and a jump over the handlers
		bool explicitEnd {true};
	};
	[[nodiscard]] std::optional<ExceptionLayout> MatchException(uint32_t ip, uint32_t hi) const;

	[[nodiscard]] std::string VariableName(uint32_t id) const;
	[[nodiscard]] bool IsLocal(uint32_t id) const { return id > _script.variablesOffset; }

	void Diagnose(DiagnosticSeverity severity, uint32_t ip, std::string message);

	[[nodiscard]] std::vector<Diagnostic>& Diagnostics() { return _diagnostics; }
	[[nodiscard]] uint32_t GotoCount() const { return _gotoCount; }
	[[nodiscard]] uint32_t FallbackCount() const { return _fallbackCount; }
	[[nodiscard]] uint32_t UnsupportedCount() const { return _unsupportedCount; }
	void CountFallback() { ++_fallbackCount; }
	[[nodiscard]] const std::set<uint32_t>& WantedLabels() const { return _wantedLabels; }
	[[nodiscard]] const std::set<uint32_t>& EmittedLabels() const { return _emittedLabels; }

	[[nodiscard]] static std::string LabelName(uint32_t ip);

private:
	struct LoopContext
	{
		uint32_t head;
		uint32_t exit;
	};

	struct SequenceState
	{
		std::vector<StackEntry> stack;
		std::vector<uint32_t> pending;
		uint32_t stmtStart {0};
		StmtList out;
		/// Variable loaded and thrown away since the last statement, as the compiler does before an assignment
		std::string droppedLoad;
	};

	[[nodiscard]] const VMInstruction& At(uint32_t ip) const { return _program.instructions[ip]; }
	[[nodiscard]] bool InScript(uint32_t ip) const { return ip >= _firstIp && ip < _endIp; }

	// Expression stack
	[[nodiscard]] ExprPtr PopSlot(std::vector<StackEntry>& stack, uint32_t ip, bool strict, bool& failed);
	[[nodiscard]] ExprPtr PopVector(std::vector<StackEntry>& stack, uint32_t ip, bool strict, bool& failed);
	[[nodiscard]] ExprPtr PopWidth(std::vector<StackEntry>& stack, uint8_t width, uint32_t ip, bool strict, bool& failed);
	[[nodiscard]] bool SplitTopVector(std::vector<StackEntry>& stack, size_t entryIndex);
	[[nodiscard]] bool AlignSlots(std::vector<StackEntry>& stack, size_t slots, size_t& entryIndex);

	/// Run an instruction that only computes values. Returns false for instructions that make statements or change
	/// control flow, and (when strict) for anything that would need a value the stack doesn't have.
	[[nodiscard]] bool ExecPure(uint32_t ip, std::vector<StackEntry>& stack, bool strict);
	[[nodiscard]] bool IsPure(uint32_t ip) const;
	[[nodiscard]] uint32_t FirstControl(uint32_t lo, uint32_t hi) const;
	[[nodiscard]] ExprPtr EvalCondition(uint32_t lo, uint32_t hi);

	[[nodiscard]] ExprPtr MakeNativeCall(uint32_t ip, std::vector<StackEntry>& stack, bool strict, bool& failed,
	                                     uint32_t& stackOut);
	[[nodiscard]] ExprPtr ConvertArgument(const ExprPtr& arg, ArgType type);

	// Statements
	void AddStmt(SequenceState& state, StmtPtr stmt);
	void FlushStack(SequenceState& state);
	[[nodiscard]] uint32_t ParseLoop(SequenceState& state, uint32_t ip, uint32_t backJump);
	[[nodiscard]] uint32_t ParseIf(SequenceState& state, ExprPtr cond, uint32_t jumpIp, uint32_t hi);
	[[nodiscard]] std::optional<uint32_t> ParseException(SequenceState& state, uint32_t ip, uint32_t hi);
	void ParseBackwardConditional(SequenceState& state, ExprPtr cond, uint32_t ip);
	void ParseJump(SequenceState& state, uint32_t ip, uint32_t hi);
	void AddGoto(SequenceState& state, ExprPtr negatedCond, uint32_t ip, uint32_t target);
	[[nodiscard]] std::optional<uint32_t> FindLoopBackJump(uint32_t ip, uint32_t hi) const;
	[[nodiscard]] uint32_t Target(uint32_t ip) const { return At(ip).data.uintVal; }
	[[nodiscard]] bool LeadsTo(uint32_t from, uint32_t target) const;

	const ProgramView& _program;
	const VMScript& _script;
	std::span<const NativeSignature> _natives;
	[[maybe_unused]] const DecompileOptions& _options;
	uint32_t _firstIp;
	uint32_t _endIp;
	/// Backward unconditional jumps: target -> sources
	std::map<uint32_t, std::vector<uint32_t>> _backJumps;
	std::vector<LoopContext> _loops;
	std::set<uint32_t> _labels;
	std::set<uint32_t> _wantedLabels;
	std::set<uint32_t> _emittedLabels;
	std::vector<Diagnostic> _diagnostics;
	/// The last exchange of the two topmost values, to tell a call whose first two arguments were swapped
	uint32_t _swapIp {0xFFFFFFFF};
	const Expr* _swapTop {nullptr};
	const Expr* _swapBelow {nullptr};
	uint32_t _gotoCount {0};
	uint32_t _fallbackCount {0};
	uint32_t _unsupportedCount {0};
};

} // namespace openblack::lhvm::detail

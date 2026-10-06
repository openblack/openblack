/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

/// Syntax tree of the Lionhead Challenge Language (CHL), the source language of LHVM bytecode.
///
/// The tree is shared by the decompiler (bytecode -> tree -> CHL text) and the compiler (CHL text -> tree -> bytecode),
/// so it models the language, not either direction:
///
///  - Expressions are trees of Expr. Native functions appear as NativeCall nodes holding the function's name and its
///    arguments in the order of the function's parameters (see LHVMNatives.h); how a call is spelt in CHL
///    ("create VILLAGER ... at [pos]", "[Boy] near [Girl] radius 5") is decided by the statement forms in ChlForms.h.
///  - Statements are Stmt nodes in StmtList blocks. Exception handlers (when/until) belong to the loop or script whose
///    code they guard, in Stmt::handlers and Script::handlers.
///  - Every node carries `ips`: the instruction addresses it stands for. The decompiler fills them from the bytecode;
///    the compiler may fill them with the addresses it emits, which gives both a source map.
///
/// Values in CHL are untyped numbers at the source level; ValueType records what the bytecode used, which the compiler
/// needs to choose between the integer, float, boolean and object forms of an instruction.

#include <cstdint>

#include <memory>
#include <string>
#include <vector>

namespace openblack::lhvm::chl
{

enum class ValueType : uint8_t
{
	Unknown,
	Int,
	Float,
	Bool,
	Object,
	Vector, ///< A position: three numbers, written [x, y, z]
	String
};

enum class ExprKind : uint8_t
{
	Literal,       ///< A constant. `text` is its spelling, `number` its value, `type` Int/Float/Bool/Object/String.
	Variable,      ///< A global or local variable named `text`
	Constant,      ///< A named game constant `text` (an enum member from the script headers) worth `number`
	Unary,         ///< `op` (Neg or Not) applied to args[0]
	Binary,        ///< args[0] `op` args[1]
	Cast,          ///< args[0] converted to `type`. Implicit in CHL except int to float, written "variable X".
	NativeCall,    ///< Native function `text` (number `number` in the native table), arguments in parameter order
	VectorLiteral, ///< [args[0], args[1], args[2]]
	Component,     ///< One number (`component` 0..2) of the position args[0]; not expressible in CHL
	Elapsed,       ///< "args[0] seconds": true once that many seconds have passed since the condition was first tested
	Duplicate,     ///< The value args[0] used a second time (the bytecode copied it on the stack)
	Placeholder    ///< A value that could not be recovered; `text` describes it
};

/// Operators. Spelling and precedence are in ChlSyntax.h.
enum class Op : uint8_t
{
	None,
	Neg,
	Not,
	Mul,
	Div,
	Mod,
	Add,
	Sub,
	Lt,
	Le,
	Gt,
	Ge,
	Eq,
	Ne,
	And,
	Or
};

struct Expr
{
	ExprKind kind {ExprKind::Placeholder};
	ValueType type {ValueType::Unknown};
	Op op {Op::None};
	std::string text;
	std::vector<std::shared_ptr<const Expr>> args;
	/// Instructions that made this node, not counting its children
	std::vector<uint32_t> ips;
	/// Calls a native function somewhere inside, so it can't be dropped or reordered freely
	bool sideEffects {false};
	/// NativeCall: the bytecode pushed the first two arguments in the opposite order and swapped them, as the original
	/// compiler does for some statement forms
	bool swapped {false};
	double number {0.0};
	uint8_t component {0};
};

using ExprPtr = std::shared_ptr<const Expr>;

enum class StmtKind : uint8_t
{
	Expression,  ///< A native call (or other expression) run for its effect: `expr`
	Assign,      ///< `name op expr` with op "=", "+=", "-=", "*=", "/=", or "++"/"--" without expr
	Declaration, ///< A local variable and its initial value, before "start"
	RunScript,   ///< "run [background] script name(args)"; `async` for background
	WaitUntil,   ///< "wait until expr"; "wait N seconds" when expr is Elapsed
	If,          ///< if / elsif / else / end if: `branches`, the last without a condition being the else
	While,       ///< "while expr ... end while", or "begin loop ... end loop" without expr; `handlers` guard it
	DoWhile,     ///< Body then a backward test; not expressible in CHL
	Block,       ///< "begin <name> ... end <name>": cinema, camera, dialogue, dual camera
	Marker,      ///< A begin or end of a block (`name`, e.g. "begin cinema") whose other half isn't in the same block
	When,        ///< Exception handler: runs `body` whenever `expr` holds
	Until,       ///< Exception handler: when `expr` holds, runs `body` and leaves the guarded loop or script
	Exception,   ///< Guarded `body` with `handlers` that isn't a loop or a script; not expressible in CHL
	Break,       ///< Leave the innermost loop; not expressible in CHL
	Continue,    ///< Next iteration of the innermost loop; not expressible in CHL
	Goto,        ///< "goto name"; openblack extension for control flow that has no structured form
	Label,       ///< "name:"; target of a goto
	Return,      ///< Stop the script early; not expressible in CHL
	Raw          ///< `name` printed as a comment: instructions the decompiler couldn't express
};

struct Stmt;
using StmtPtr = std::unique_ptr<Stmt>;
using StmtList = std::vector<StmtPtr>;

struct Branch
{
	ExprPtr cond; ///< null for the final else
	StmtList body;
	/// Instructions shown on the branch's opening line
	std::vector<uint32_t> ips;
	/// Instructions shown on the line that closes the branch's body
	std::vector<uint32_t> closeIps;
};

struct Stmt
{
	StmtKind kind {StmtKind::Raw};
	ExprPtr expr;
	std::string name;
	std::string op;
	std::vector<ExprPtr> args;
	bool async {false};
	/// Assign "=": the bytecode loaded and discarded the variable first, as the original compiler does for plain
	/// assignments but not for compound ones
	bool discardedLoad {false};
	/// While: the loop is wrapped in an exception scope, which the original compiler does for every loop
	bool guarded {false};
	std::vector<Branch> branches;
	StmtList body;
	StmtList handlers;
	/// Instructions shown on the statement's (opening) line
	std::vector<uint32_t> ips;
	/// Instructions shown on the closing line of a block statement
	std::vector<uint32_t> closeIps;
	/// Instructions of the scaffolding between the guarded code and its handlers
	std::vector<uint32_t> midIps;
	/// First instruction of the statement, including any no-op instructions folded into it
	uint32_t startIp {0};
};

/// The kinds of script, in the order of their bits in the bytecode's script type
enum class ScriptKind : uint8_t
{
	Script,
	HelpScript,
	ChallengeHelpScript,
	TempleHelpScript,
	TempleSpecialScript,
	MultiplayerScript,
	MultiplayerHelpScript
};

struct Script
{
	ScriptKind kind {ScriptKind::Script};
	std::string name;
	/// Source file the script came from, when known
	std::string filename;
	std::vector<std::string> params;
	/// Locals in declaration order, with their initial values
	StmtList locals;
	StmtList body;
	/// Exception handlers guarding the whole body
	StmtList handlers;
	/// Instructions shown on the "begin script" line, the "start" line and the "end script" line
	std::vector<uint32_t> beginIps;
	std::vector<uint32_t> startIps;
	std::vector<uint32_t> endIps;
	/// Instructions of the scaffolding between the body and the script's handlers
	std::vector<uint32_t> midIps;
};

[[nodiscard]] inline StmtPtr MakeStmt(StmtKind kind, uint32_t startIp)
{
	auto stmt = std::make_unique<Stmt>();
	stmt->kind = kind;
	stmt->startIp = startIp;
	return stmt;
}

/// Append every instruction of the expression tree to `out`
void CollectIps(const ExprPtr& expr, std::vector<uint32_t>& out);

[[nodiscard]] ExprPtr MakeLiteral(std::string text, ValueType type, double number, std::vector<uint32_t> ips);
[[nodiscard]] ExprPtr MakeUnary(Op op, ExprPtr operand, std::vector<uint32_t> ips);
[[nodiscard]] ExprPtr MakeBinary(Op op, ExprPtr left, ExprPtr right, ValueType type, std::vector<uint32_t> ips);
/// Logical negation, simplifying not (a == b), not (a != b) and not not a
[[nodiscard]] ExprPtr Negate(const ExprPtr& expr, std::vector<uint32_t> ips);
/// The same expression with more instructions attributed to its top node
[[nodiscard]] ExprPtr WithIps(const ExprPtr& expr, const std::vector<uint32_t>& ips);
[[nodiscard]] bool IsLiteralTrue(const ExprPtr& expr);
/// The expression is a literal or constant equal to `value`
[[nodiscard]] bool IsNumber(const ExprPtr& expr, double value);

} // namespace openblack::lhvm::chl

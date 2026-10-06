/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

/// Spelling of the Challenge Language: keywords, operators and their precedence, script kinds and number literals.
///
/// Program layout, as written by the decompiler and read by the compiler:
///
///     global Name                              one line per global variable
///     run script Name                          scripts started when the program loads
///
///     begin [help|challenge help|...] script Name(Param, Param)
///         Local = expression                   locals and their initial values
///     start
///         statements
///         when condition / until condition     handlers guarding the whole script, each followed by its statements
///     end script Name
///
/// Statements: assignments (`X = e`, `X += e`, `X++`), `if c ... elsif c ... else ... end if`, `while c ... end while`,
/// `begin loop ... end loop`, `wait until c`, `wait N seconds`, `run script S(args)`, `run background script S(args)`,
/// `begin cinema|camera|dialogue ... end cinema|camera|dialogue`, `begin dual camera to A B ... end dual camera`, and
/// native statements written in their natural-language forms (ChlForms.h). A loop's when/until handlers come last in
/// its body. Comments start with //.
///
/// openblack extensions, for code the original language can't express: `native NAME(args)` calls a native function
/// that has no statement form, and `goto label` / `label:` stand for unstructured jumps.

#include <array>
#include <optional>
#include <string>
#include <string_view>

#include "ChlAst.h"

namespace openblack::lhvm::chl
{

struct OperatorInfo
{
	Op op;
	std::string_view spelling;
	/// Higher binds tighter
	uint8_t precedence;
	bool unary;
};

/// Operators from loosest to tightest. `not` binds looser than comparisons: "not A == B" is "not (A == B)".
inline constexpr std::array<OperatorInfo, 15> k_Operators = {{
    {.op = Op::Or, .spelling = "or", .precedence = 1, .unary = false},
    {.op = Op::And, .spelling = "and", .precedence = 2, .unary = false},
    {.op = Op::Not, .spelling = "not", .precedence = 3, .unary = true},
    {.op = Op::Eq, .spelling = "==", .precedence = 4, .unary = false},
    {.op = Op::Ne, .spelling = "!=", .precedence = 4, .unary = false},
    {.op = Op::Lt, .spelling = "<", .precedence = 4, .unary = false},
    {.op = Op::Le, .spelling = "<=", .precedence = 4, .unary = false},
    {.op = Op::Gt, .spelling = ">", .precedence = 4, .unary = false},
    {.op = Op::Ge, .spelling = ">=", .precedence = 4, .unary = false},
    {.op = Op::Add, .spelling = "+", .precedence = 5, .unary = false},
    {.op = Op::Sub, .spelling = "-", .precedence = 5, .unary = false},
    {.op = Op::Mul, .spelling = "*", .precedence = 6, .unary = false},
    {.op = Op::Div, .spelling = "/", .precedence = 6, .unary = false},
    {.op = Op::Mod, .spelling = "%", .precedence = 6, .unary = false},
    {.op = Op::Neg, .spelling = "-", .precedence = 7, .unary = true},
}};

/// Precedence of an operand that is a single token or bracketed: variables, literals, [x, y, z], [Object]
inline constexpr uint8_t k_AtomPrecedence = 8;
/// Precedence given to natural-language forms ("get HOUSE at [Pos]", "Boy played") when used as operands: they bind
/// like comparisons, so they're bracketed inside arithmetic
inline constexpr uint8_t k_FormPrecedence = 4;

[[nodiscard]] const OperatorInfo& GetOperator(Op op);
[[nodiscard]] std::optional<Op> FindOperator(std::string_view spelling, bool unary);

/// Keywords that open and close the language's blocks and statements
inline constexpr std::array<std::string_view, 30> k_Keywords = {
    "begin",    "end",   "script", "start", "global", "local", "run",      "background", "if",     "elsif",
    "else",     "while", "loop",   "wait",  "until",  "when",  "seconds",  "second",     "cinema", "camera",
    "dialogue", "dual",  "to",     "and",   "or",     "not",   "variable", "native",     "goto",   "challenge"};

/// "script", "help script", "challenge help script" (also read as "quest help script"), "multiplayer script"...
[[nodiscard]] std::string_view ScriptKindKeyword(ScriptKind kind);
[[nodiscard]] std::optional<ScriptKind> ParseScriptKind(std::string_view keywords);

/// The whole of the text read as a number ("12", "-0.5", "2e+06"), with nothing before or after it. It reads what
/// std::from_chars does, which Apple's C++ library lacks for floating point before iOS 26 and macOS 26.
[[nodiscard]] std::optional<double> ParseDouble(std::string_view text);
[[nodiscard]] std::optional<float> ParseFloat(std::string_view text);

/// A float as CHL writes it: the shortest text that reads back to the same float, without a trailing ".0" or an
/// exponent
[[nodiscard]] std::string FormatNumber(float value);

} // namespace openblack::lhvm::chl

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

/// Natural-language spellings of native function calls ("statement forms").
///
/// Most CHL statements, conditions and object expressions are a single native call written as English words, like
/// "move Boy position to [Girl] radius 5" for MOVE_GAME_THING(Boy, GET_POSITION(Girl), 5). Each form pairs a native
/// function with a pattern that says how its arguments are written. The decompiler picks the first form whose fixed
/// parts match a call; the compiler parses a statement against the patterns and pushes the arguments in parameter
/// order (swapping the first two afterwards when the form is marked `swapped`, and negating the result when it is
/// marked `negated`).
///
/// Pattern syntax (items separated by spaces):
///
///     word            a literal keyword
///     $N              argument N (0-based parameter index), written as an expression of the parameter's type
///     $N:ENUM         argument N, a constant of the script header enum ENUM (written by name when it is known).
///                     Two names are special: $N:CAMERA is a camera position enum, written with or without its
///                     challenge's prefix, and $N:CURRENT_CHALLENGE isn't written: it is the id of the challenge the
///                     last "challenge NAME" line named.
///     $N=V            argument N always has the value V and isn't written
///     {words}$N       written when argument N is true (non-zero); argument N is false when the words are absent
///     (a=V|b=W)$N     one of the alternatives, giving argument N its value; words of one alternative are joined by +
///     [items]         optional; inside, `$N?V` is the argument whose default V is pushed when the group is absent,
///                     and the group is left out when the argument equals V. `$N?` alone means the default is not
///                     known, so the decompiler always writes the group.
///
/// Values are numbers, `true` or `false`.

#include <cstdint>

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace openblack::lhvm::chl
{

struct StatementForm
{
	std::string_view native;
	std::string_view pattern;
	/// The original compiler pushes arguments 1 and 0 in that order and swaps them
	bool swapped {false};
	/// The form is a negative condition: the call's result followed by a logical not ("Victim not exists")
	bool negated {false};
	/// Which of the natives sharing this name the form calls (0 for the first), for the few names used twice
	uint8_t overload {0};
};

/// Every form, most specific first
[[nodiscard]] std::span<const StatementForm> StatementForms();

/// The forms of one native function, most specific first
[[nodiscard]] std::vector<const StatementForm*> FormsForNative(std::string_view native);

enum class PatternItemKind : uint8_t
{
	Word,
	Argument,
	Fixed,
	Flag,
	Choice,
	Optional
};

struct PatternItem
{
	PatternItemKind kind {PatternItemKind::Word};
	/// Word: the keyword. Flag: the words.
	std::string text;
	/// Argument, Fixed, Flag, Choice: the parameter index
	int argument {-1};
	/// Argument: the enum its constant belongs to, if any
	std::string enumName;
	/// Fixed: the required value. Argument in an Optional group: its default.
	std::optional<double> value;
	/// Argument: it is the default-bearing argument of its Optional group
	bool optionalArgument {false};
	/// Choice: the alternatives (words separated by spaces) and the value each gives
	std::vector<std::pair<std::string, double>> choices;
	/// Optional: the group's items
	std::vector<PatternItem> items;
};

/// Split a pattern into items. Malformed patterns give an empty list.
[[nodiscard]] std::vector<PatternItem> ParsePattern(std::string_view pattern);

} // namespace openblack::lhvm::chl

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
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "ChlForms.h"
#include "LHVMNatives.h"

namespace openblack::lhvm::chl
{

/// A statement form ready for matching: its pattern split into items and its native function resolved
struct CompiledForm
{
	const StatementForm* source {nullptr};
	std::vector<PatternItem> items;
	uint32_t native {0};
	const NativeSignature* signature {nullptr};
	/// The form starts with an argument ("$0 played"), so it is tried after an operand has been read
	bool postfix {false};
	/// The form reads as the negation of another form of the same function ("$0 not clicked"): the call's result is
	/// negated
	bool negated {false};
	/// Number of the function's parameters the form gives a value to
	size_t argumentCount {0};
};

/// The result and parameter types the compiler uses for a native function. The shared signature table is followed,
/// except where the game's bytecode shows the original compiler treating a value differently.
[[nodiscard]] ArgType ParameterType(const NativeSignature& signature, size_t index);
[[nodiscard]] ArgType ResultType(const NativeSignature& signature);

/// The statement forms, indexed for the parser
class FormTable
{
public:
	FormTable(std::span<const NativeSignature> natives, const std::multimap<std::string, uint32_t, std::less<>>& index);

	/// Forms starting with the word `word`, most specific first
	[[nodiscard]] std::span<const CompiledForm* const> Prefix(std::string_view word) const;
	/// Forms starting with an argument
	[[nodiscard]] std::span<const CompiledForm* const> Postfix() const { return _postfix; }
	/// Every word used by a form: such identifiers are keywords, not names
	[[nodiscard]] bool IsKeyword(std::string_view word) const;
	/// Forms of a native function
	[[nodiscard]] std::vector<const CompiledForm*> FormsOf(std::string_view native) const;
	/// Patterns that failed to load, for tests
	[[nodiscard]] const std::vector<std::string>& Rejected() const { return _rejected; }

private:
	std::vector<CompiledForm> _forms;
	std::map<std::string, std::vector<const CompiledForm*>, std::less<>> _prefix;
	std::vector<const CompiledForm*> _postfix;
	std::set<std::string, std::less<>> _keywords;
	std::vector<std::string> _rejected;
};

} // namespace openblack::lhvm::chl

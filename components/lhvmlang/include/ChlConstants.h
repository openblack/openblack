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
#include <string>
#include <string_view>
#include <vector>

namespace openblack::lhvm::chl
{

/// Named game constants, read from the files the script compiler takes them from: C headers (enum declarations and
/// #defines) and the game's info tables (lines of a name and a value, grouped under "ENUM_NAME<tab>Value" lines).
/// Scripts write a constant by its full name; short names such as HOUSE are aliases a script declares itself with
/// "global constant".
class ConstantTable
{
public:
	void Add(std::string_view enumName, std::string_view member, int32_t value);

	/// Read the enum declarations and #define constants of a C header. Returns the number of constants added.
	size_t LoadHeader(std::string_view text);

	/// Read an info table: "#" comment lines, "GROUP<tab>Value" lines that start a group (ENUM_ and DETAIL_ are dropped
	/// from the group's name), and "NAME<tab>value" lines. Returns the number of constants added.
	size_t LoadInfo(std::string_view text);

	/// The name of the first member of `enumName` worth `value`. An enum name ending in '*' searches every group whose
	/// name starts with the rest.
	[[nodiscard]] std::optional<std::string> NameOf(std::string_view enumName, int32_t value) const;

	/// The value of a named constant
	[[nodiscard]] std::optional<int32_t> ValueOf(std::string_view name) const;

	[[nodiscard]] bool Empty() const { return _values.empty(); }

private:
	/// enum name -> value -> member names in declaration order
	std::map<std::string, std::map<int32_t, std::vector<std::string>>, std::less<>> _enums;
	std::map<std::string, int32_t, std::less<>> _values;
};

} // namespace openblack::lhvm::chl

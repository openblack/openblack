/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <algorithm>
#include <string>
#include <vector>

namespace openblack::string_utils
{

[[nodiscard]] bool EndsWith(const std::string& str, const std::string& ending);

[[nodiscard]] bool BeginsWith(const std::string& str, const std::string& beginning);

[[nodiscard]] std::string UpperCase(const std::string& str);

[[nodiscard]] std::string LowerCase(const std::string& str);

[[nodiscard]] std::string Capitalise(const std::string& str);

[[nodiscard]] std::vector<std::string> Split(const std::string& string, const std::string& delimiter);

/// Extract a substring of the characters in between the first two quote of a string
[[nodiscard]] std::string ExtractQuote(std::string& string);

} // namespace openblack::string_utils

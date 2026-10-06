/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ChlSyntax.h"

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>

#include <algorithm>

#include <fmt/format.h>

namespace openblack::lhvm::chl
{

namespace
{

constexpr std::array<std::string_view, 7> k_ScriptKindKeywords = {
    "script",
    "help script",
    "challenge help script",
    "temple help script",
    "temple special script",
    "multiplayer script",
    "multiplayer help script",
};

} // namespace

const OperatorInfo& GetOperator(Op op)
{
	const auto it = std::ranges::find(k_Operators, op, &OperatorInfo::op);
	return it != k_Operators.end() ? *it : k_Operators.front();
}

std::optional<Op> FindOperator(std::string_view spelling, bool unary)
{
	for (const auto& info : k_Operators)
	{
		if (info.spelling == spelling && info.unary == unary)
		{
			return info.op;
		}
	}
	return std::nullopt;
}

std::string_view ScriptKindKeyword(ScriptKind kind)
{
	const auto index = static_cast<size_t>(kind);
	return index < k_ScriptKindKeywords.size() ? k_ScriptKindKeywords[index] : k_ScriptKindKeywords.front();
}

std::optional<ScriptKind> ParseScriptKind(std::string_view keywords)
{
	// "quest help" is another spelling of "challenge help"
	if (keywords == "quest help script")
	{
		return ScriptKind::ChallengeHelpScript;
	}
	for (size_t i = 0; i < k_ScriptKindKeywords.size(); ++i)
	{
		if (k_ScriptKindKeywords[i] == keywords)
		{
			return static_cast<ScriptKind>(i);
		}
	}
	return std::nullopt;
}

namespace
{
/// strtod and strtof also read leading spaces, a plus sign and hexadecimal numbers, which from_chars doesn't
bool IsDecimalNumber(std::string_view text)
{
	if (text.starts_with('-'))
	{
		text.remove_prefix(1);
	}
	if (text.empty())
	{
		return false;
	}
	const auto first = static_cast<unsigned char>(text.front());
	if (std::isdigit(first) == 0 && first != '.' && first != 'i' && first != 'I' && first != 'n' && first != 'N')
	{
		return false;
	}
	return !(text.size() > 1 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'));
}

template <typename T, typename Parse>
std::optional<T> ParseWhole(std::string_view text, Parse parse)
{
	if (!IsDecimalNumber(text))
	{
		return std::nullopt;
	}
	// The C functions read a terminated string
	const std::string terminated(text);
	char* end = nullptr;
	errno = 0;
	const T value = parse(terminated.c_str(), &end);
	if (end != terminated.c_str() + terminated.size() || (errno == ERANGE && std::isinf(value)))
	{
		return std::nullopt;
	}
	return value;
}
} // namespace

std::optional<double> ParseDouble(std::string_view text)
{
	return ParseWhole<double>(text, [](const char* first, char** end) { return std::strtod(first, end); });
}

std::optional<float> ParseFloat(std::string_view text)
{
	return ParseWhole<float>(text, [](const char* first, char** end) { return std::strtof(first, end); });
}

std::string FormatNumber(float value)
{
	auto text = fmt::format("{}", value);
	if (text.find('e') == std::string::npos || !std::isfinite(value))
	{
		return text;
	}
	// The language has no exponents: the fewest decimals that read back as the same number
	for (int decimals = 0; decimals < 64; ++decimals)
	{
		text = fmt::format("{:.{}f}", value, decimals);
		const auto parsed = ParseFloat(text);
		if (parsed.has_value() && *parsed == value)
		{
			break;
		}
	}
	return text;
}

} // namespace openblack::lhvm::chl

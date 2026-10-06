/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "EnumHeader.h"

#include <cctype>
#include <charconv>

using namespace openblack::psys;

namespace
{
std::string_view Trim(std::string_view text)
{
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0)
	{
		text.remove_prefix(1);
	}
	while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0)
	{
		text.remove_suffix(1);
	}
	return text;
}

bool IsName(std::string_view text)
{
	if (text.empty() || std::isdigit(static_cast<unsigned char>(text.front())) != 0)
	{
		return false;
	}
	for (const char c : text)
	{
		if (std::isalnum(static_cast<unsigned char>(c)) == 0 && c != '_')
		{
			return false;
		}
	}
	return true;
}
} // namespace

std::map<std::string, int32_t, std::less<>> openblack::psys::ParseEnumHeader(std::string_view text)
{
	std::map<std::string, int32_t, std::less<>> values;
	while (!text.empty())
	{
		const auto end = text.find('\n');
		auto line = text.substr(0, end);
		text = end == std::string_view::npos ? std::string_view {} : text.substr(end + 1);
		if (const auto comment = line.find("//"); comment != std::string_view::npos)
		{
			line = line.substr(0, comment);
		}
		const auto equals = line.find('=');
		if (equals == std::string_view::npos)
		{
			continue;
		}
		const auto name = Trim(line.substr(0, equals));
		auto value = Trim(line.substr(equals + 1));
		if (!value.empty() && value.back() == ',')
		{
			value = Trim(value.substr(0, value.size() - 1));
		}
		int32_t number = 0;
		const auto* last = value.data() + value.size();
		const auto [ptr, error] = std::from_chars(value.data(), last, number);
		if (IsName(name) && error == std::errc() && ptr == last)
		{
			values.insert_or_assign(std::string(name), number);
		}
	}
	return values;
}

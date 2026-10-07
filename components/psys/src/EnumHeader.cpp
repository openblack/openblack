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

#include <optional>

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
	// The value the next enumerator without an initialiser takes, as a C compiler counts: 0 at the start of each enum,
	// then one more than the enumerator before. Unknown after an initialiser that isn't a whole number.
	std::optional<int32_t> next = 0;
	bool inside = false;
	const auto enumerator = [&](std::string_view item) {
		item = Trim(item);
		if (item.empty())
		{
			return;
		}
		const auto equals = item.find('=');
		const auto name = Trim(item.substr(0, equals));
		if (!IsName(name))
		{
			next.reset();
			return;
		}
		if (equals != std::string_view::npos)
		{
			const auto value = Trim(item.substr(equals + 1));
			int32_t number = 0;
			const auto* last = value.data() + value.size();
			const auto [ptr, error] = std::from_chars(value.data(), last, number);
			if (error != std::errc() || ptr != last)
			{
				next.reset();
				return;
			}
			next = number;
		}
		if (next.has_value())
		{
			values.insert_or_assign(std::string(name), *next);
			next = *next + 1;
		}
	};
	while (!text.empty())
	{
		const auto end = text.find('\n');
		auto line = text.substr(0, end);
		text = end == std::string_view::npos ? std::string_view {} : text.substr(end + 1);
		if (const auto comment = line.find("//"); comment != std::string_view::npos)
		{
			line = line.substr(0, comment);
		}
		if (const auto open = line.find('{'); open != std::string_view::npos)
		{
			inside = true;
			next = 0;
			line = line.substr(open + 1);
		}
		const auto close = line.find('}');
		if (inside)
		{
			// One or more enumerators, each ended by a comma (the last one may have none)
			auto items = line.substr(0, close);
			while (!items.empty())
			{
				const auto comma = items.find(',');
				enumerator(items.substr(0, comma));
				items = comma == std::string_view::npos ? std::string_view {} : items.substr(comma + 1);
			}
		}
		if (close != std::string_view::npos)
		{
			inside = false;
		}
	}
	return values;
}

/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TextDatabase.h"

#include <cctype>

#include <optional>
#include <utility>

using namespace openblack::gui;

namespace
{
/// Reads the arguments of the ADD_TEXT lines: ADD_TEXT(<n>, <narrator>, "<name>", "<text>")
class ScriptReader
{
public:
	explicit ScriptReader(std::u16string_view text)
	    : _text(text)
	{
	}

	/// Moves on to the next ADD_TEXT line, false when there are no more
	bool NextAddText()
	{
		static constexpr std::u16string_view k_Command = u"ADD_TEXT";
		while (true)
		{
			const auto found = _text.find(k_Command, _position);
			if (found == std::u16string_view::npos)
			{
				return false;
			}
			_position = found + k_Command.size();
			// Not part of a longer name
			const bool startsWord = found == 0 || !IsNameChar(_text[found - 1]);
			SkipSpaces();
			if (startsWord && Peek() == u'(')
			{
				++_position;
				return true;
			}
		}
	}

	/// The next argument up to a comma or the closing bracket, as it is written
	std::optional<std::u16string_view> Word()
	{
		SkipSpaces();
		const auto start = _position;
		while (_position < _text.size() && _text[_position] != u',' && _text[_position] != u')' && _text[_position] != u'\n')
		{
			++_position;
		}
		auto word = _text.substr(start, _position - start);
		while (!word.empty() && (word.back() == u' ' || word.back() == u'\t' || word.back() == u'\r'))
		{
			word.remove_suffix(1);
		}
		return Separator() ? std::optional(word) : std::nullopt;
	}

	/// The next argument, a quoted string with its escapes resolved
	std::optional<std::u16string> String()
	{
		SkipSpaces();
		if (Peek() != u'"')
		{
			return std::nullopt;
		}
		++_position;
		std::u16string result;
		while (_position < _text.size() && _text[_position] != u'"' && _text[_position] != u'\n')
		{
			auto c = _text[_position++];
			if (c == u'\\' && _position < _text.size())
			{
				c = _text[_position++];
				if (c == u'n')
				{
					c = u'\n';
				}
			}
			result.push_back(c);
		}
		if (Peek() != u'"')
		{
			return std::nullopt;
		}
		++_position;
		SkipSpaces();
		return Separator() ? std::optional(std::move(result)) : std::nullopt;
	}

private:
	static bool IsNameChar(char16_t c) { return c < 0x80 && (std::isalnum(c) != 0 || c == u'_'); }

	[[nodiscard]] char16_t Peek() const { return _position < _text.size() ? _text[_position] : u'\0'; }

	void SkipSpaces()
	{
		while (_position < _text.size() && (_text[_position] == u' ' || _text[_position] == u'\t'))
		{
			++_position;
		}
	}

	/// Steps over the comma or bracket after an argument
	bool Separator()
	{
		SkipSpaces();
		if (Peek() == u',' || Peek() == u')')
		{
			++_position;
			return true;
		}
		return false;
	}

	std::u16string_view _text;
	size_t _position {0};
};
} // namespace

size_t TextDatabase::AddScript(std::span<const uint8_t> script)
{
	std::u16string text(script.size() / 2, u'\0');
	for (size_t i = 0; i < text.size(); ++i)
	{
		text[i] = static_cast<char16_t>(script[i * 2] | (script[(i * 2) + 1] << 8));
	}
	if (!text.empty() && text.front() == u'\xFEFF')
	{
		text.erase(0, 1);
	}

	size_t count = 0;
	ScriptReader reader(text);
	while (reader.NextAddText())
	{
		const auto number = reader.Word();
		const auto narrator = reader.Word();
		auto name = reader.String();
		auto value = reader.String();
		if (!number || !narrator || !name || !value)
		{
			continue;
		}
		_texts.insert_or_assign(ToUtf8(*name), std::move(*value));
		++count;
	}
	return count;
}

std::u16string_view TextDatabase::Get(std::string_view name) const
{
	const auto found = _texts.find(std::string(name));
	return found != _texts.end() ? std::u16string_view(found->second) : std::u16string_view();
}

std::string openblack::gui::ToUtf8(std::u16string_view text)
{
	std::string result;
	result.reserve(text.size());
	for (size_t i = 0; i < text.size(); ++i)
	{
		uint32_t c = text[i];
		if (c >= 0xD800 && c < 0xDC00 && i + 1 < text.size() && text[i + 1] >= 0xDC00 && text[i + 1] < 0xE000)
		{
			c = 0x10000 + ((c - 0xD800) << 10) + (text[++i] - 0xDC00);
		}
		if (c < 0x80)
		{
			result.push_back(static_cast<char>(c));
		}
		else if (c < 0x800)
		{
			result.push_back(static_cast<char>(0xC0 | (c >> 6)));
			result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
		}
		else if (c < 0x10000)
		{
			result.push_back(static_cast<char>(0xE0 | (c >> 12)));
			result.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
			result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
		}
		else
		{
			result.push_back(static_cast<char>(0xF0 | (c >> 18)));
			result.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
			result.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
			result.push_back(static_cast<char>(0x80 | (c & 0x3F)));
		}
	}
	return result;
}

std::u16string openblack::gui::ToUtf16(std::string_view text)
{
	std::u16string result;
	result.reserve(text.size());
	for (size_t i = 0; i < text.size();)
	{
		const auto lead = static_cast<uint8_t>(text[i]);
		uint32_t c = lead;
		size_t length = 1;
		if (lead >= 0xF0)
		{
			c = lead & 0x07;
			length = 4;
		}
		else if (lead >= 0xE0)
		{
			c = lead & 0x0F;
			length = 3;
		}
		else if (lead >= 0xC0)
		{
			c = lead & 0x1F;
			length = 2;
		}
		for (size_t j = 1; j < length && i + j < text.size(); ++j)
		{
			c = (c << 6) | (static_cast<uint8_t>(text[i + j]) & 0x3F);
		}
		i += length;
		if (c >= 0x10000)
		{
			c -= 0x10000;
			result.push_back(static_cast<char16_t>(0xD800 + (c >> 10)));
			result.push_back(static_cast<char16_t>(0xDC00 + (c & 0x3FF)));
		}
		else
		{
			result.push_back(static_cast<char16_t>(c));
		}
	}
	return result;
}

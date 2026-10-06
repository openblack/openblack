/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ChlForms.h"

#include <cctype>

#include <algorithm>
#include <map>

#include "ChlSyntax.h"

namespace openblack::lhvm::chl
{

namespace
{

class PatternParser
{
public:
	explicit PatternParser(std::string_view text)
	    : _text(text)
	{
	}

	/// Items up to `end` (a closing bracket, or the end of the text). False when malformed.
	bool ParseItems(std::vector<PatternItem>& items, char end)
	{
		while (true)
		{
			SkipSpaces();
			if (_pos >= _text.size())
			{
				return end == '\0';
			}
			const char c = _text[_pos];
			if (c == end)
			{
				++_pos;
				return true;
			}
			PatternItem item;
			if (c == '[')
			{
				++_pos;
				item.kind = PatternItemKind::Optional;
				if (!ParseItems(item.items, ']'))
				{
					return false;
				}
				if (Peek() == '$' && !ParseArgumentIndex(item.argument))
				{
					return false;
				}
			}
			else if (c == '{')
			{
				const auto close = _text.find('}', _pos);
				if (close == std::string_view::npos)
				{
					return false;
				}
				item.kind = PatternItemKind::Flag;
				item.text = std::string(_text.substr(_pos + 1, close - _pos - 1));
				_pos = close + 1;
				if (!ParseArgumentIndex(item.argument))
				{
					return false;
				}
			}
			else if (c == '(')
			{
				const auto close = _text.find(')', _pos);
				if (close == std::string_view::npos)
				{
					return false;
				}
				item.kind = PatternItemKind::Choice;
				auto alternatives = _text.substr(_pos + 1, close - _pos - 1);
				_pos = close + 1;
				size_t index = 0;
				while (true)
				{
					const auto bar = alternatives.find('|');
					auto alternative = alternatives.substr(0, bar);
					double value = index == 0 ? 1.0 : 0.0;
					if (const auto equals = alternative.find('='); equals != std::string_view::npos)
					{
						const auto parsed = ParseValue(alternative.substr(equals + 1));
						if (!parsed)
						{
							return false;
						}
						value = *parsed;
						alternative = alternative.substr(0, equals);
					}
					std::string words(alternative);
					std::ranges::replace(words, '+', ' ');
					item.choices.emplace_back(std::move(words), value);
					++index;
					if (bar == std::string_view::npos)
					{
						break;
					}
					alternatives = alternatives.substr(bar + 1);
				}
				if (!ParseArgumentIndex(item.argument))
				{
					return false;
				}
			}
			else if (c == '$')
			{
				if (!ParseArgumentIndex(item.argument))
				{
					return false;
				}
				item.kind = PatternItemKind::Argument;
				if (Peek() == ':')
				{
					++_pos;
					item.enumName = std::string(ReadToken());
				}
				if (Peek() == '=')
				{
					++_pos;
					item.kind = PatternItemKind::Fixed;
					item.value = ParseValue(ReadToken());
					if (!item.value)
					{
						return false;
					}
				}
				else if (Peek() == '?')
				{
					++_pos;
					item.optionalArgument = true;
					const auto token = ReadToken();
					if (!token.empty())
					{
						item.value = ParseValue(token);
						if (!item.value)
						{
							return false;
						}
					}
				}
			}
			else
			{
				item.kind = PatternItemKind::Word;
				item.text = std::string(ReadWord());
				if (item.text.empty())
				{
					return false;
				}
			}
			items.push_back(std::move(item));
		}
	}

private:
	void SkipSpaces()
	{
		while (_pos < _text.size() && _text[_pos] == ' ')
		{
			++_pos;
		}
	}

	[[nodiscard]] char Peek() const { return _pos < _text.size() ? _text[_pos] : '\0'; }

	bool ParseArgumentIndex(int& argument)
	{
		if (Peek() != '$')
		{
			return false;
		}
		++_pos;
		const auto start = _pos;
		while (_pos < _text.size() && std::isdigit(static_cast<unsigned char>(_text[_pos])) != 0)
		{
			++_pos;
		}
		if (_pos == start)
		{
			return false;
		}
		argument = std::stoi(std::string(_text.substr(start, _pos - start)));
		return true;
	}

	/// An identifier or number (stops at spaces and pattern punctuation)
	std::string_view ReadToken()
	{
		const auto start = _pos;
		while (_pos < _text.size())
		{
			const char c = _text[_pos];
			if (c == ' ' || c == ']' || c == '[' || c == '{' || c == '(' || c == '$' || c == '=' || c == '?')
			{
				break;
			}
			++_pos;
		}
		return _text.substr(start, _pos - start);
	}

	std::string_view ReadWord()
	{
		const auto start = _pos;
		while (_pos < _text.size() && _text[_pos] != ' ' && _text[_pos] != ']' && _text[_pos] != '[')
		{
			++_pos;
		}
		return _text.substr(start, _pos - start);
	}

	static std::optional<double> ParseValue(std::string_view text)
	{
		if (text == "true")
		{
			return 1.0;
		}
		if (text == "false")
		{
			return 0.0;
		}
		return ParseDouble(text);
	}

	std::string_view _text;
	size_t _pos {0};
};

} // namespace

std::vector<PatternItem> ParsePattern(std::string_view pattern)
{
	std::vector<PatternItem> items;
	PatternParser parser(pattern);
	if (!parser.ParseItems(items, '\0'))
	{
		return {};
	}
	return items;
}

std::vector<const StatementForm*> FormsForNative(std::string_view native)
{
	static const auto k_Index = [] {
		std::map<std::string_view, std::vector<const StatementForm*>, std::less<>> index;
		for (const auto& form : StatementForms())
		{
			index[form.native].push_back(&form);
		}
		return index;
	}();
	const auto it = k_Index.find(native);
	return it != k_Index.end() ? it->second : std::vector<const StatementForm*> {};
}

} // namespace openblack::lhvm::chl

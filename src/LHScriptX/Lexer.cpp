/* OpenBlack - A reimplementation of Lionheads's Black & White engine
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <LHScriptX/Lexer.h>

using namespace OpenBlack::LHScriptX;

Lexer::Lexer(const std::string &input) : _source(input), _currentLine(1) {
	_current = _source.begin();
	_end = _source.end();
};

void Lexer::advance()
{
	// don't advance if we're at EOF
	if (_current == _end)
		return;

	// if it's a new line, increment the line conter and reset the current character
	if (*_current == '\n')
		_currentLine++;

	// advance our source iterator
	_current++;
}

inline void Lexer::skip_whitespace()
{
	while (_current != _end && is_whitespace(*_current))
		advance();
}

inline void Lexer::skip_comments()
{
	// rem ... \n
	// **** ... ****

	// check if the end is within the next 3 characters
	if (_current == _end || _current + 1 == _end || _current + 2 == _end)
		return;

	// unless we have rem, don't skip
	if (!(imatch(*_current, 'r') && imatch(*(_current + 1), 'e') && imatch(*(_current + 2), 'm')))
		return;

	// consume all characters until newline or eof
	while (_current != _end && *_current != '\n')
		advance();

	// consume the final newline character
	advance();
	_currentLine++;

	if (_current != _end)
	{
		skip_whitespace();
		skip_comments();
	}
}

Token Lexer::GetToken()
{
	// skip all whitespace
	skip_whitespace();

	// skip all comments
	skip_comments();

	// if we're at the end of the file, return EoF token
	if (_current == _end) return Token(EndOfFile, "", _currentLine);

	if (is_letter(*_current))
		return make_identifier();
	if (is_digit(*_current) || *_current == '-')
		return make_number_literal();
	if (*_current == '"')
		return make_string_literal();

	switch (*_current)
	{
	case ',': return make_symbol(ListSeparator);
	case '(': return make_symbol(OpenParanthesis);
	case ')': return make_symbol(CloseParanethesis);
	case '=': return make_symbol(Assignment);
	}

	Token unknown(Unknown, std::string(1, *_current), _currentLine);
	advance();

	return unknown;
}

Token Lexer::make_identifier()
{
	std::string::iterator identifier_begin = _current;

	while (_current != _end && (is_letter(*_current) || is_digit(*_current) || *_current == '_'))
		advance();

	return Token(Identifier, std::string(identifier_begin, _current), _currentLine);
}

Token Lexer::make_number_literal()
{
	// Match the following formats:
	// 123456
	// 123.456

	std::string::iterator number_begin = _current;
	bool found_dot = false;

	while (_current != _end && (is_digit(*_current) || *_current == '.' || *_current == '-'))
	{
		if (*_current == '.')
			found_dot = true;
		advance();
	}

	return Token(found_dot ? FloatLiteral : NumberLiteral, std::string(number_begin, _current), _currentLine);
}

Token Lexer::make_string_literal()
{
	// consume opening "
	advance();

	auto string_begin = _current;

	// consume until closing "
	while (_current != _end && *_current != '"')
		advance();

	auto string_finish = _current;

	// consume closing "
	advance();

	return Token(StringLiteral, std::string(string_begin, string_finish), _currentLine);
}

Token Lexer::make_symbol(TokenType type)
{
	advance();
	return Token(type, "", _currentLine);
}

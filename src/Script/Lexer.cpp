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

/*#include "Lexer.h"
#include "Token.h"

using namespace OpenBlack::Script;

/*


std::list<Token> Script::tokenize(const std::string &source)
{
	// create a list to contain our tokens
	std::list<Token> tokens;

	auto current = source.begin();
	auto end = source.end();

	while (current != end)
	{
		// consume all white space
		if (*current == '\n' || *current == '\r' || *current == '\t' || *current == ' ')
		{
			current++;
			continue;
		}

		// consume comments (rem)
		// todo

		// consume identifiers
		if ((*current >= 'a' && *current <= 'z') || (*current >= 'A' && *current <= 'Z'))
		{

		}

	}

	Token token(TokenType::Identifier, "lol");
	tokens.emplace_back(token);

	return tokens;
}


*/

/*Lexer::Lexer(std::string &input) : _source(input) {
	_current = _source.begin();
	_end = _source.end();
};

std::list<Token> Lexer::GetTokens()
{
	std::list<Token> tokens;

	while (_current != _end)
	{
		auto token = GetToken();
		tokens.push_back(token);
	}

	return tokens;
}

Token& Lexer::GetToken()
{
	if (_current == _end) {
		_token.Type = TokenType::EndOfFile;
		return _token;
	}

	if (*_current == '\r' || *_current == '\n') {
		return MakeEndOfLine();
	}
	//else if (isWhitespace(*m_current))
	//{
	//	return MakeWhitespace();
	//}
	//else if (m_current == "rem")
	//{
	//	return MakeComment()
	//}
	else if (IsLetter(*_current))
	{
		return MakeIdentifier();
	}
	else if (*_current == '"')
	{
		return MakeStringLiteral();
	}
	else if (IsDigit(*_current))
	{
		return MakeNumberLiteral();
	}

	// consume symbols: , ( )
	switch (*_current)
	{
	case ',': return MakePunctuation(TokenType::ListSeparator);
	case '(': return MakePunctuation(TokenType::OpenParanthesis);
	case ')': return MakePunctuation(TokenType::CloseParanethesis);
	}

	// return unknown on default
	_token.Type = TokenType::Unknown;
	_token.Value = std::string(1, *_current);
	Advance();
	return _token;
}

void Lexer::Advance()
{
	// don't advance if we're at EOF
	if (_current == _end || *_current == '\0')
		return;

	// advance our source iterator
	_current++;
}

Token &Lexer::MakeEndOfLine()
{
	_token.Type = TokenType::EndOfLine;

	if (*_current == '\n') { // consume \n
		Advance();
	} else if (*_current == '\r') { // consume \r\n
		Advance();

		if (*_current == '\n') {
			Advance();
		}
	}

	return _token;
}

Token &Lexer::MakeIdentifier()
{
	auto identifier_begin = _current;

	// A-Z 0-9 _
	while (IsLetter(*_current) || IsDigit(*_current) || *_current == '_')
		_current++;

	_token.Type = TokenType::Identifier;
	_token.Value = std::string(identifier_begin, _current);
	return _token;
}

Token &Lexer::MakeStringLiteral()
{
	// consume opening "
	Advance();

	auto string_begin = _current;

	// consume until "
	while (*_current != '"')
		Advance();

	// consume closing " (?)
	Advance();

	_token.Type = TokenType::StringLiteral;
	_token.Value = std::string(string_begin, _current);

	return _token;
}

Token &Lexer::MakeNumberLiteral()
{
	// todo: change token type for float.
	auto number_begin = _current;

	while (IsDigit(*_current) || *_current == '.')
		_current++;

	_token.Type = TokenType::NumberLiteral;
	_token.Value = std::string(number_begin, _current);
	return _token;
}

Token &Lexer::MakePunctuation(TokenType type)
{
	_token.Type = type;
	_token.Value = *_current;
	Advance();
	return _token;
}
*/

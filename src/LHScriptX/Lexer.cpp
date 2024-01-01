/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Lexer.h"

#include <cctype>

#include <utility>

using namespace openblack::lhscriptx;

Lexer::Lexer(std::string source)
    : _source(std::move(source))
{
	_current = _source.begin();
	_end = _source.end();
}

Token Lexer::GetToken()
{
	while (true)
	{
		if (_current == _end)
		{
			return Token::MakeEOFToken();
		}

		unsigned char cc = *_current;
		switch (cc)
		{
		case '/':
			// Comment syntax. Ignore the rest of the line
			if (*(_current + 1) == '/')
			{
				// Skip line
				while (*_current != '\n' && _current != _end)
				{
					_current++;
				}
			}
			break;
		case ' ':
		case '\t':
		case '\r':
			_current++;

			// skip over whitespace quickly
			while (*_current == ' ' || *_current == '\t' || *_current == '\r')
			{
				_current++;
			}
			break;
		case '\n':
			_current++;
			_currentLine++;
			return Token::MakeEOLToken();

		// not sure if it's **** or just *, this can be drastically improved on
		// though
		case '*':
			while (*_current != '\n')
			{
				_current++;
			}
			break;

		// handle potential rem/REM
		case 'R':
		case 'r':
			// todo: potential out of bounds here:
			if ((_current[1] == 'e' || _current[1] == 'E') && (_current[2] == 'm' || _current[2] == 'M'))
			{
				while (*_current != '\n')
				{
					_current++;
				}
				break;
			}
			return GatherIdentifer();

		/* gather identifiers */
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
		case 'G':
		case 'H':
		case 'I':
		case 'J':
		case 'K':
		case 'L':
		case 'M':
		case 'N':
		case 'O':
		case 'P':
		case 'Q': /* case 'R': handled above for REM */
		case 'S':
		case 'T':
		case 'U':
		case 'V':
		case 'W':
		case 'X':
		case 'Y':
		case 'Z':
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'h':
		case 'i':
		case 'j':
		case 'k':
		case 'l':
		case 'm':
		case 'n':
		case 'o':
		case 'p':
		case 'q': /* case 'r': handled above for rem */
		case 's':
		case 't':
		case 'u':
		case 'v':
		case 'w':
		case 'x':
		case 'y':
		case 'z':
		case '_':
			return GatherIdentifer();

		/* gather numbers */
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '-':
			return GatherNumber();

		/* gather strings */
		case '"':
			return GatherString();

		case '=':
			_current++;
			return Token::MakeOperatorToken(Operator::Equal);
		case ',':
			_current++;
			return Token::MakeOperatorToken(Operator::Comma);
		case '(':
			_current++;
			return Token::MakeOperatorToken(Operator::LeftParentheses);
		case ')':
			_current++;
			return Token::MakeOperatorToken(Operator::RightParentheses);
		default:
			// todo: ignore BOM

			_current++;
			throw LexerException("unexpected character: " + std::string(1, cc));
		}
	}

	__builtin_unreachable(); // unreachable
}

Token Lexer::GatherIdentifer()
{
	auto idStart = _current;
	while (HasMore())
	{
		unsigned char cc = *_current;

		/* check for invalid characters */
		if ((cc < 'A' || cc > 'Z') && (cc < 'a' || cc > 'z') && cc != '_' && (cc < '0' || cc > '9'))
		{
			if ((cc >= ' ' && cc < 0x7f) || cc == '\t' || cc == '\r' || cc == '\n')
			{
				break;
			}

			throw LexerException("invalid character " + std::string(1, cc) + "in identifer");
		}

		_current++;
	}

	return Token::MakeIdentifierToken(std::string(idStart, _current));
}

Token Lexer::GatherNumber()
{
	bool isFloat = false;
	bool isNeg = false;

	if (*_current == '-')
	{
		_current++;
		isNeg = true;
	}

	auto numberStart = _current;

	// consume all digits and .
	while (HasMore())
	{
		if (std::isdigit(*_current) != 0)
		{
			_current++;
		}
		else if (*_current == '.')
		{
			_current++;
			isFloat = true;
		}
		else
		{
			break;
		}
	}

	if (isFloat)
	{
		float value = std::stof(std::string(numberStart, _current));
		if (isNeg)
		{
			value = -value;
		}
		return Token::MakeFloatToken(value);
	}

	int value = std::stoi(std::string(numberStart, _current));
	if (isNeg)
	{
		value = -value;
	}
	return Token::MakeIntegerToken(value);
}

Token Lexer::GatherString()
{
	auto stringStart = ++_current;

	// todo: we should check for unterminated strings
	while (HasMore() && *_current != '"')
	{
		_current++;
	}

	return Token::MakeStringToken(std::string(stringStart, _current++));
}

void Token::Print(FILE* file) const
{
	switch (this->_type)
	{
	case Type::Invalid:
		fprintf(file, "invalid");
		break;
	case Type::EndOfFile:
		fprintf(file, "EOF");
		break;
	case Type::EndOfLine:
		fprintf(file, "\n");
		break;
	case Type::Identifier:
		fprintf(file, "identifier \"%s\"", this->_s.c_str());
		break;
	case Type::String:
		fprintf(file, "quoted string \"%s\"", this->_s.c_str());
		break;
	case Type::Integer:
		fprintf(file, "integer %d", this->_u.integerValue);
		break;
	case Type::Float:
		fprintf(file, "float %f", this->_u.floatValue);
		break;
	case Type::Operator:
		fprintf(file, "operator ");
		switch (this->_u.op)
		{
		case Operator::Invalid:
			fprintf(file, "invalid");
			break;
		case Operator::Equal:
			fprintf(file, "=");
			break;
		case Operator::Comma:
			fprintf(file, ",");
			break;
		case Operator::LeftParentheses:
			fprintf(file, "(");
			break;
		case Operator::RightParentheses:
			fprintf(file, ")");
			break;
		default:
			__builtin_unreachable();
		}
		break;
	default:
		__builtin_unreachable();
	}
}

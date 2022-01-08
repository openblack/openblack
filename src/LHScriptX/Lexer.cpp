/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Lexer.h"

#include <cctype>
#include <utility>

using namespace openblack::lhscriptx;

Lexer::Lexer(std::string source)
    : source_(std::move(source))
    , currentLine_(1)
{
	current_ = source_.begin();
	end_ = source_.end();
}

Token Lexer::GetToken()
{
	while (true)
	{
		if (current_ == end_)
			return Token::MakeEOFToken();

		unsigned char cc = *current_;
		switch (cc)
		{
		case '/':
			// Comment syntax. Ignore the rest of the line
			if (*(current_ + 1) == '/')
			{
				// Skip line
				while (*current_ != '\n' && current_ != end_)
					current_++;
			}
			break;
		case ' ':
		case '\t':
		case '\r':
			current_++;

			// skip over whitespace quickly
			while (*current_ == ' ' || *current_ == '\t' || *current_ == '\r') current_++;
			break;
		case '\n':
			current_++;
			currentLine_++;
			return Token::MakeEOLToken();

		// not sure if it's **** or just *, this can be drastically improved on
		// though
		case '*':
			while (*current_ != '\n') current_++;
			break;

		// handle potential rem/REM
		case 'R':
		case 'r':
			// todo: potential out of bounds here:
			if ((current_[1] == 'e' || current_[1] == 'E') && (current_[2] == 'm' || current_[2] == 'M'))
			{
				while (*current_ != '\n') current_++;
				break;
			}
			return gatherIdentifer();

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
			return gatherIdentifer();

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
			return gatherNumber();

		/* gather strings */
		case '"':
			return gatherString();

		case '=':
			current_++;
			return Token::MakeOperatorToken(Operator::Equal);
		case ',':
			current_++;
			return Token::MakeOperatorToken(Operator::Comma);
		case '(':
			current_++;
			return Token::MakeOperatorToken(Operator::LeftParentheses);
		case ')':
			current_++;
			return Token::MakeOperatorToken(Operator::RightParentheses);
		default:
			// todo: ignore BOM

			current_++;
			throw LexerException("unexpected character: " + std::string(1, cc));
		}
	}

	__builtin_unreachable(); // unreachable
}

Token Lexer::gatherIdentifer()
{
	auto id_start = current_;
	while (hasMore())
	{
		unsigned char cc = *current_;

		/* check for invalid characters */
		if ((cc < 'A' || cc > 'Z') && (cc < 'a' || cc > 'z') && cc != '_' && (cc < '0' || cc > '9'))
		{
			if ((cc >= ' ' && cc < 0x7f) || cc == '\t' || cc == '\r' || cc == '\n')
				break;

			throw LexerException("invalid character " + std::string(1, cc) + "in identifer");
		}

		current_++;
	}

	return Token::MakeIdentifierToken(std::string(id_start, current_));
}

Token Lexer::gatherNumber()
{
	bool is_float = false;
	bool is_neg = false;

	if (*current_ == '-')
	{
		current_++;
		is_neg = true;
	}

	auto number_start = current_;

	// consume all digits and .
	while (hasMore())
	{
		if (std::isdigit(*current_))
		{
			current_++;
		}
		else if (*current_ == '.')
		{
			current_++;
			is_float = true;
		}
		else
		{
			break;
		}
	}

	if (is_float)
	{
		float value = std::stof(std::string(number_start, current_));
		if (is_neg)
			value = -value;
		return Token::MakeFloatToken(value);
	}

	int value = std::stoi(std::string(number_start, current_));
	if (is_neg)
		value = -value;
	return Token::MakeIntegerToken(value);
}

Token Lexer::gatherString()
{
	auto string_start = ++current_;

	// todo: we should check for unterminated strings
	while (hasMore() && *current_ != '"') current_++;

	return Token::MakeStringToken(std::string(string_start, current_++));
}

void Token::Print(FILE* file) const
{
	switch (this->type_)
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
		fprintf(file, "identifier \"%s\"", this->u_.identifierValue->c_str());
		break;
	case Type::String:
		fprintf(file, "quoted string \"%s\"", this->u_.stringValue->c_str());
		break;
	case Type::Integer:
		fprintf(file, "integer %d", this->u_.integerValue);
		break;
	case Type::Float:
		fprintf(file, "float %f", this->u_.floatValue);
		break;
	case Type::Operator:
		fprintf(file, "operator ");
		switch (this->u_.op)
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

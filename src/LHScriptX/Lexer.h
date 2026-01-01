/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <stdexcept>
#include <string>

#ifdef _MSC_VER
#define __builtin_unreachable() __assume(0)
#endif

namespace openblack::lhscriptx
{

class LexerException: public std::runtime_error
{
public:
	explicit LexerException(const std::string& msg)
	    : std::runtime_error(msg.c_str())
	{
	}
};

enum class Operator
{
	Invalid,
	Equal,            // =
	Comma,            // ,
	LeftParentheses,  // (
	RightParentheses, // )
};

class Token
{
public:
	enum class Type
	{
		// Token is invalid.
		Invalid,
		// Token indicates end of input.
		EndOfFile,
		// Token indicates end of line.
		EndOfLine,
		// Token is an identifer.
		Identifier,
		// Token is a string of characters.
		String,
		// Token is an integer.
		Integer,
		// Token is a floating point number.
		Float,
		// Token is an operator.
		Operator,
	};

	[[nodiscard]] Type GetType() const { return this->_type; }

	static Token MakeInvalidToken() { return Token(Type::Invalid); }
	static Token MakeEOFToken() { return Token(Type::EndOfFile); }
	static Token MakeEOLToken() { return Token(Type::EndOfLine); }
	static Token MakeIdentifierToken(const std::string& value)
	{
		Token tok(Type::Identifier);
		tok._s = value;
		return tok;
	}
	static Token MakeStringToken(const std::string& value)
	{
		Token tok(Type::String);
		tok._s = value;
		return tok;
	}
	static Token MakeIntegerToken(int value)
	{
		Token tok(Type::Integer);
		tok._u.integerValue = value;
		return tok;
	}
	static Token MakeFloatToken(float value)
	{
		Token tok(Type::Float);
		tok._u.floatValue = value;
		return tok;
	}
	static Token MakeOperatorToken(Operator op)
	{
		Token tok(Type::Operator);
		tok._u.op = op;
		return tok;
	}

	[[nodiscard]] bool IsInvalid() const { return this->_type == Type::Invalid; }
	[[nodiscard]] bool IsEOF() const { return this->_type == Type::EndOfFile; }
	[[nodiscard]] bool IsIdentifier() const { return this->_type == Type::Identifier; }
	[[nodiscard]] bool IsString() const { return this->_type == Type::String; }
	[[nodiscard]] bool IsOP(Operator op) const { return this->_type == Type::Operator && this->_u.op == op; }

	// todo: assert check the type for each of these?
	[[nodiscard]] const std::string& StringValue() const { return this->_s; }
	[[nodiscard]] const std::string& Identifier() const { return this->_s; }
	[[nodiscard]] const int* IntegerValue() const { return &this->_u.integerValue; }
	[[nodiscard]] const float* FloatValue() const { return &this->_u.floatValue; }
	[[nodiscard]] Operator Op() const { return this->_u.op; }

	// print the token for debugging
	void Print(FILE* file) const;

private:
	explicit Token(Type type)
	    : _type(type)
	{
	}

	Type _type;
	union
	{
		int integerValue;
		float floatValue;
		Operator op;
	} _u;
	std::string _s;
};

class Lexer
{
public:
	explicit Lexer(std::string source);

	Token GetToken();

private:
	[[nodiscard]] size_t Remaining() const noexcept { return static_cast<size_t>(_end - _current); }

	[[nodiscard]] bool HasMore() const noexcept { return _current != _end; }

	Token GatherIdentifer();
	Token GatherNumber();
	Token GatherString();

	std::string _source;
	std::string::iterator _current;
	std::string::iterator _end;

	int _currentLine {1};
};

} // namespace openblack::lhscriptx

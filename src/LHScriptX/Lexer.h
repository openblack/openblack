/* OpenBlack - A reimplementation of Lionhead's Black & White.
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

#pragma once

#include <stdexcept>
#include <string>

#ifdef _MSC_VER
#define __builtin_unreachable() __assume(0)
#endif

namespace OpenBlack::LHScriptX
{

class LexerException: public std::runtime_error
{
  public:
	LexerException(const std::string& msg):
	    std::runtime_error(msg.c_str()) {}
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

	Type GetType() const { return this->type_; }

	static Token MakeInvalidToken() { return Token(Type::Invalid); }
	static Token MakeEOFToken() { return Token(Type::EndOfFile); }
	static Token MakeEOLToken() { return Token(Type::EndOfLine); }
	static Token MakeIdentifierToken(const std::string& value)
	{
		Token tok(Type::Identifier);
		tok.u_.identifierValue = new std::string(value);
		return tok;
	}
	static Token MakeStringToken(const std::string& value)
	{
		Token tok(Type::String);
		tok.u_.stringValue = new std::string(value);
		return tok;
	}
	static Token MakeIntegerToken(int value)
	{
		Token tok(Type::Integer);
		tok.u_.integerValue = value;
		return tok;
	}
	static Token MakeFloatToken(float value)
	{
		Token tok(Type::Float);
		tok.u_.floatValue = value;
		return tok;
	}
	static Token MakeOperatorToken(Operator op)
	{
		Token tok(Type::Operator);
		tok.u_.op = op;
		return tok;
	}

	bool IsInvalid() const { return this->type_ == Type::Invalid; }
	bool IsEOF() const { return this->type_ == Type::EndOfFile; }
	bool IsIdentifier() const { return this->type_ == Type::Identifier; }
	bool IsString() const { return this->type_ == Type::String; }
	bool IsOP(Operator op) const { return this->type_ == Type::Operator && this->u_.op == op; }

	// todo: assert check the type for each of these?
	const std::string& Identifier() const { return *this->u_.identifierValue; }
	const std::string& StringValue() const { return *this->u_.stringValue; }
	const int* IntegerValue() const { return &this->u_.integerValue; }
	const float* FloatValue() const { return &this->u_.floatValue; }
	Operator Op() const { return this->u_.op; }

	// print the token for debugging
	void Print(FILE* file) const;

  private:
	Token(Type type):
	    type_(type) {}

	Type type_;
	union {
		std::string* identifierValue;
		std::string* stringValue;
		int integerValue;
		float floatValue;
		Operator op;
	} u_;
};

class Lexer
{
  public:
	Lexer(const std::string& source);

	Token GetToken();

  private:
	const size_t remaining() const noexcept
	{
		return static_cast<size_t>(end_ - current_);
	}

	const bool hasMore() const noexcept
	{
		return current_ != end_;
	}

	Token gatherIdentifer();
	Token gatherNumber();
	Token gatherString();

	std::string source_;
	std::string::iterator current_;
	std::string::iterator end_;

	int currentLine_;
};

} // namespace OpenBlack::LHScriptX

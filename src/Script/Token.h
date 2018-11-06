#pragma once
#ifndef OPENBLACK_SCRIPT_TOKEN_H
#define OPENBLACK_SCRIPT_TOKEN_H

#include <list>
#include <string>

namespace OpenBlack
{
	namespace Script
	{
		enum TokenType
		{
			Unknown,
			EndOfFile,
			EndOfLine,
			WhiteSpace,
			Comment,

			Identifier,
			StringLiteral,
			NumberLiteral,

			Comma,
			LeftParen,
			RightParen,

			Assign
		};

		struct Token {
			Token(TokenType _type) : Type(_type), Value("") {};
			Token(TokenType _type, std::string _value) : Type(_type), Value(_value) {};

			TokenType Type;
			std::string Value;
		};

		std::string GetTokenTypeName(TokenType type);
	}
}

#endif

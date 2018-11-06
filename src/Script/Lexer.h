#pragma once
#ifndef OPENBLACK_SCRIPT_LEXER_H
#define OPENBLACK_SCRIPT_LEXER_H

#include "Token.h"

namespace OpenBlack
{
	namespace Script
	{
		class Lexer {
		public:
			Lexer(std::string &input);

			std::list<Token> GetTokens();
		private:
			Token GetToken();

			Token MakeEndOfLine();
			Token MakeWhitespace();
			Token MakeComment();

			Token MakeIdentifier();
			Token MakeStringLiteral();
			Token MakeNumberLiteral();

			Token MakePunctuation(TokenType type);

			std::string &m_source;
			std::string::iterator m_current;
			std::string::iterator m_end;

			inline bool IsLetter(char c)
			{
				return isalpha(c);
			}

			inline bool IsDigit(char c)
			{
				return isdigit(c);
			}

			inline bool IsWhitespace(char c)
			{
				return (c == ' ' || c == '\t');
			}
		};
	}
}

#endif

/*#pragma once
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
			Token &GetToken();
		private:
			void Advance();

			Token &MakeEndOfLine();
			Token &MakeWhitespace();
			Token &MakeComment();

			Token &MakeIdentifier();
			Token &MakeStringLiteral();
			Token &MakeNumberLiteral();

			Token &MakePunctuation(TokenType type);

			Token _token;
			std::string &_source;
			std::string::iterator _current;
			std::string::iterator _end;


			inline bool IsLetter(char c)
			{
				return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
			}

			inline bool IsDigit(char c)
			{
				return (c >= '0' && c <= '9');
			}

			inline bool isWhitespace(char c)
			{
				return (c == ' ' || c == '\t');
			}
		};
	}
}

#endif
*/

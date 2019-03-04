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

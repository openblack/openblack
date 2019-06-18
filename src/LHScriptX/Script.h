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

#include <LHScriptX/Lexer.h>
#include <vector>

namespace OpenBlack::LHScriptX {

class Script {
public:
	Script() : token_(Token::MakeInvalidToken()) {}

	void LoadFile(const std::string& file);
	//void SetCommands(std::unique_ptr<ScriptCommands> &commands) { _commands = std::move(commands); }
	//ScriptCommands &GetCommands() const { return *_commands; }
private:
	//std::unique_ptr<ScriptCommands> _commands;
	//void processCommand(const std::string& command, std::vector<std::string> parameters);

	const bool isCommand(const std::string& identifier) const;
	void runCommand(const std::string& identifier);

	const Token* peekToken();
	const Token* advanceToken();

	// The lexer output we are parsing
	Lexer* lexer_;
	// The current token.
	Token token_;
};

}

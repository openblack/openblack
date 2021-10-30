/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Lexer.h"

#include <vector>

namespace openblack
{
class Game;
}

namespace openblack::lhscriptx
{

class NotImplemented: public std::logic_error
{
public:
	NotImplemented()
	    : std::logic_error("Function not yet implemented") {};
};

class Script
{
public:
	Script(Game* game)
	    : _game(game)
	    , token_(Token::MakeInvalidToken())
	{
	}

	void Load(const std::string&);

	// void SetCommands(std::unique_ptr<ScriptCommands> &commands) { _commands =
	// std::move(commands); } ScriptCommands &GetCommands() const { return
	// *_commands; }
private:
	// std::unique_ptr<ScriptCommands> _commands;
	// void processCommand(const std::string& command, std::vector<std::string>
	// parameters);

	[[nodiscard]] bool isCommand(const std::string& identifier) const;
	void runCommand(const std::string& identifier, const std::vector<Token>& args);

	const Token* peekToken(Lexer&);
	const Token* advanceToken(Lexer&);

	// Game instance
	Game* _game;
	// The current token.
	Token token_;
};

} // namespace openblack::lhscriptx

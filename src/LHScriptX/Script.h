/*****************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <vector>

#include "Lexer.h"

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
	explicit Script(Game* game)
	    : _game(game)
	    , _token(Token::MakeInvalidToken())
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

	[[nodiscard]] bool IsCommand(const std::string& identifier) const;
	void RunCommand(const std::string& identifier, const std::vector<Token>& args);

	const Token* PeekToken(Lexer&);
	const Token* AdvanceToken(Lexer&);

	// Game instance
	Game* _game;
	// The current token.
	Token _token;
};

} // namespace openblack::lhscriptx

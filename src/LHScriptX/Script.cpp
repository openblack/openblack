/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Script.h"

#include "FeatureScriptCommands.h"
#include "Lexer.h"

#include <spdlog/spdlog.h>

#include <iostream>

using namespace openblack;
using namespace openblack::lhscriptx;

void Script::Load(const std::string& source)
{
	Lexer lexer(source);

	const Token* token = this->peekToken(lexer);
	while (!token->IsEOF())
	{
		token = this->peekToken(lexer);

		if (token->IsIdentifier())
		{
			const std::string& identifier = token->Identifier();

			if (!isCommand(identifier))
				throw std::runtime_error("unknown command: " + identifier);

			token = this->advanceToken(lexer);
			if (!token->IsOP(Operator::LeftParentheses))
				throw std::runtime_error("expected ( after identifier " + identifier);

			std::vector<Token> args;

			// if it's an immediate right parentheses there are no args
			token = this->advanceToken(lexer);
			if (!token->IsOP(Operator::RightParentheses))
			{
				while (true)
				{
					const Token* token = this->peekToken(lexer);
					args.push_back(*token);

					// consume the ,
					token = this->advanceToken(lexer);
					if (!token->IsOP(Operator::Comma))
						break;

					this->advanceToken(lexer);
				}
			}

			if (!token->IsOP(Operator::RightParentheses))
				throw std::runtime_error("missing )");

			// move token to whatever is after ')'
			this->advanceToken(lexer);

			runCommand(identifier, args);
		}

		this->advanceToken(lexer);
	}
}

bool Script::isCommand(const std::string& identifier) const
{
	// this could be done a lot better
	for (const auto& signature : FeatureScriptCommands::Signatures)
		if (signature.name == identifier)
			return true;

	return false;
}

ScriptCommandParameter GetParameter(Token& argument)
{
	const auto type = argument.GetType();

	switch (type)
	{
	case Token::Type::Invalid:
		throw std::runtime_error("Invalid token. Unable to proceed");
	case Token::Type::EndOfFile:
		throw std::runtime_error("Unexpected EOF in script");
	case Token::Type::EndOfLine:
		throw std::runtime_error("Unexpected EOL in script");
	case Token::Type::Identifier:
		return ScriptCommandParameter(argument.Identifier());
	case Token::Type::String:
		return ScriptCommandParameter(argument.StringValue());
	case Token::Type::Integer:
		return ScriptCommandParameter(*argument.IntegerValue());
	case Token::Type::Float:
		return ScriptCommandParameter(*argument.FloatValue());
	case Token::Type::Operator:
		throw std::runtime_error("Operator token as an argument is currently not supported");
	default:
		throw std::runtime_error("Missing switch case for script token argument");
	}
}

void Script::runCommand(const std::string& identifier, const std::vector<Token>& args)
{
	const ScriptCommandSignature* command_signature = nullptr;

	for (const auto& signature : FeatureScriptCommands::Signatures)
	{
		if (signature.name != identifier)
			continue;

		command_signature = &signature;
		break;
	}

	if (command_signature == nullptr)
		throw std::runtime_error("Missing script command signature");

	// Turn tokens into parameters
	auto parameters = ScriptCommandParameters();

	for (auto arg : args)
	{
		ScriptCommandParameter param = GetParameter(arg);
		parameters.push_back(param);
	}

	const auto expected_parameters = command_signature->parameters;
	uint32_t expected_size;
	for (expected_size = 0; expected_size < expected_parameters.size(); ++expected_size)
	{
		// Looping until None because parameters is a fixed sized array.
		// Last Argument is the one before the first None or the 9th
		if (command_signature->parameters[expected_size] == ParameterType::None)
		{
			break;
		}
	}

	// Validate the number of given arguments against what is expected
	if (parameters.size() != expected_size)
	{
		throw std::runtime_error("Invalid number of script arguments");
	}

	// Validate the typing of the given arguments against what is expected
	for (auto i = 0u; i < parameters.size(); i++)
	{
		if (parameters[i].GetType() != expected_parameters[i])
		{
			std::runtime_error("Invalid script argument type");
		}
	}

	ScriptCommandContext ctx(_game, &parameters);
	command_signature->command(ctx);
}

const Token* Script::peekToken(Lexer& lexer)
{
	if (token_.IsInvalid())
		token_ = lexer.GetToken();
	return &token_;
}

const Token* Script::advanceToken(Lexer& lexer)
{
	token_ = lexer.GetToken();
	return &token_;
}

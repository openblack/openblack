/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Script.h"

#include <algorithm>
#include <iostream>
#include <ranges>

#include <glm/vec2.hpp>
#include <spdlog/spdlog.h>

#include "3D/LandIslandInterface.h"
#include "FeatureScriptCommands.h"
#include "Lexer.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::lhscriptx;

Script::Script() = default;

void Script::Load(const std::string& source)
{
	Lexer lexer(source);

	const Token* token = this->PeekToken(lexer);
	while (!token->IsEOF())
	{
		token = this->PeekToken(lexer);

		if (token->IsIdentifier())
		{
			const std::string identifier = token->Identifier();

			if (!IsCommand(identifier))
			{
				throw std::runtime_error("unknown command: " + identifier);
			}

			token = this->AdvanceToken(lexer);
			if (!token->IsOP(Operator::LeftParentheses))
			{
				throw std::runtime_error("expected ( after identifier " + identifier);
			}

			std::vector<Token> args;

			// if it's an immediate right parentheses there are no args
			token = this->AdvanceToken(lexer);
			if (!token->IsOP(Operator::RightParentheses))
			{
				while (true)
				{
					const Token* peekToken = this->PeekToken(lexer);
					args.push_back(*peekToken);

					// consume the ,
					token = this->AdvanceToken(lexer);
					if (!token->IsOP(Operator::Comma))
					{
						break;
					}

					this->AdvanceToken(lexer);
				}
			}

			if (!token->IsOP(Operator::RightParentheses))
			{
				throw std::runtime_error("missing )");
			}

			// move token to whatever is after ')'
			this->AdvanceToken(lexer);

			RunCommand(identifier, args);
		}

		this->AdvanceToken(lexer);
	}
}

bool Script::IsCommand(const std::string& identifier) const
{
	// TODO(handsomematt): this could be done a lot better
	return std::any_of(FeatureScriptCommands::k_Signatures.cbegin(), FeatureScriptCommands::k_Signatures.cend(),
	                   [&identifier](const auto& s) { return s.name.data() == identifier; });
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
	{
		const auto& str = argument.StringValue();
		// Check if it's a vector
		if (std::count_if(str.cbegin(), str.cend(), [](char c) { return c == ','; }) == 1)
		{
			const auto delim = str.find(',');
			char* floatEnd;
			const auto x = std::strtof(str.c_str(), &floatEnd);
			if (str.c_str() + delim == floatEnd)
			{
				const auto z = std::strtof(floatEnd + 1, &floatEnd);
				if (static_cast<size_t>(floatEnd - str.c_str()) == static_cast<size_t>(str.length()))
				{
					const auto& island = Locator::terrainSystem::value();

					return {x, island.GetHeightAt(glm::vec2(x, z)), z};
				}
			}
		}
		return ScriptCommandParameter(str);
	}
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

void Script::RunCommand(const std::string& identifier, const std::vector<Token>& args)
{
	const ScriptCommandSignature* commandSignature = nullptr;

	for (const auto& signature : FeatureScriptCommands::k_Signatures)
	{
		if (signature.name.data() != identifier)
		{
			continue;
		}

		commandSignature = &signature;
		break;
	}

	if (commandSignature == nullptr)
	{
		throw std::runtime_error("Missing script command signature");
	}

	// Turn tokens into parameters
	auto parameters = ScriptCommandParameters();

	for (auto arg : args)
	{
		ScriptCommandParameter param = GetParameter(arg);
		parameters.push_back(param);
	}

	const auto expectedParameters = commandSignature->parameters;
	uint32_t expectedSize;
	for (expectedSize = 0; const auto& p : commandSignature->parameters)
	{
		// Looping until None because parameters is a fixed sized array.
		// Last Argument is the one before the first None or the 9th
		if (p == ParameterType::None)
		{
			break;
		}
		++expectedSize;
	}

	// Validate the number of given arguments against what is expected
	if (parameters.size() != expectedSize)
	{
		throw std::runtime_error("Invalid number of script arguments");
	}

	// Validate the typing of the given arguments against what is expected
	for (const auto& [param, expected] : std::views::zip(parameters, expectedParameters))
	{
		if (param.GetType() != expected)
		{
			throw std::runtime_error("Invalid script argument type");
		}
	}

	commandSignature->command(parameters);
}

const Token* Script::PeekToken(Lexer& lexer)
{
	if (_token.IsInvalid())
	{
		_token = lexer.GetToken();
	}
	return &_token;
}

const Token* Script::AdvanceToken(Lexer& lexer)
{
	_token = lexer.GetToken();
	return &_token;
}

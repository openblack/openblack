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

#include <Common/OSFile.h>
#include <LHScriptX/FeatureScriptCommands.h>
#include <LHScriptX/Lexer.h>
#include <LHScriptX/Script.h>
#include <iostream>

using namespace OpenBlack;
using namespace OpenBlack::LHScriptX;

void Script::LoadFromFile(File& file)
{
	size_t totalFileSize = file.Size();

	char* contents = new char[totalFileSize];
	file.Read(contents, totalFileSize);

	lexer_ = new Lexer(std::string(contents, totalFileSize));

	const Token* token = this->peekToken();

	while (!token->IsEOF())
	{
		token = this->peekToken();

		if (token->IsIdentifier())
		{
			const std::string& identifier = token->Identifier();

			if (!isCommand(identifier))
				throw std::runtime_error("unknown command: " + identifier);

			token = this->advanceToken();
			if (!token->IsOP(Operator::LeftParentheses))
				throw std::runtime_error("expected ( after identifier " + identifier);

			std::vector<Token> args;

			// if it's an immediate right parentheses there are no args
			token = this->advanceToken();
			if (!token->IsOP(Operator::RightParentheses))
			{
				while (true)
				{
					const Token* token = this->peekToken();
					args.push_back(*token);

					// consume the ,
					token = this->advanceToken();
					if (!token->IsOP(Operator::Comma))
						break;

					this->advanceToken();
				}
			}

			if (!token->IsOP(Operator::RightParentheses))
				throw std::runtime_error("missing )");

			// move token to whatever is after ')'
			this->advanceToken();

			runCommand(identifier, args);
		}

		this->advanceToken();
	}

	delete lexer_;
	delete[] contents;
}

const bool Script::isCommand(const std::string& identifier) const
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
	const ScriptCommandSignature* command_signature;

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
	const auto expected_size       = expected_parameters.size();

	// Validate the number of given arguments against what is expected
	if (parameters.size() != expected_size)
	{
		throw std::runtime_error("Invalid number of script arguments");
	}

	// Validate the typing of the given arguments against what is expected
	for (auto i = 0; i < parameters.size(); i++)
	{
		if (parameters[i].GetType() != expected_parameters[i])
		{
			std::runtime_error("Invalid script argument type");
		}
	}

	ScriptCommandContext ctx(_game, &parameters);
	command_signature->command(ctx);
}

const Token* Script::peekToken()
{
	if (token_.IsInvalid())
		token_ = lexer_->GetToken();
	return &token_;
}

const Token* Script::advanceToken()
{
	token_ = lexer_->GetToken();
	return &token_;
}

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

using namespace OpenBlack::LHScriptX;

void Script::LoadFile(const std::string& file)
{
	OSFile osfile;
	osfile.Open(file.c_str(), LH_FILE_MODE::Read);

	size_t totalFileSize = osfile.Size();

	char* contents = new char[totalFileSize];
	osfile.Read(contents, totalFileSize);

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

			printf("run command %s with %d args\n", identifier.c_str(), args.size());

			// expect (

			// check for (, then collect all Tokens seperated by , until )
			// that is our paremter list
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

void Script::runCommand(const std::string& identifier)
{
	const ScriptCommandSignature* command_signature;

	for (const auto& signature : FeatureScriptCommands::Signatures)
	{
		if (signature.name != identifier)
			continue;

		command_signature = &signature;
		break;
	}

	// 1. check the script parameters
	// 2. create the ScriptCommandContext
	// ScriptParameters parameters{ ScriptCommandParameter(2.3000f) };
	// ScriptCommandContext ctx(nullptr, &parameters);

	// 3. run it
	// signature.command(ctx);
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

// Call           = "(" [ ArgumentList [ "," ] ] ")" .
// ArgumentList   = Identifer Literals

/*void Parser::call()
{
	assert(this->peekToken()->IsIdentifier());
	std::string identifier = this->peekToken()->Identifier();

	const Token* token = this->advanceToken();
	if (!token->IsOP(Operator::LeftParentheses))
		throw ParserException("expected left parentheses after identifier " + identifier, 0, 0);

	token = this->advanceToken();

	std::vector<Token> args;

	// if it's an immediate right parentheses there are no args
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
	{
		throw ParserException("missing )", 0, 0);
	}

	// move token to whatever is after ')'
	this->advanceToken();

	printf("call: %s with args: (", identifier.c_str());
	for (auto const& tok : args) {
		tok.Print(stdout);
	}
	printf(")\n");
}*/

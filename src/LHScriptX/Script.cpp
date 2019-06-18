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

#include <LHScriptX/Script.h>
#include <LHScriptX/Lexer.h>

#include <Common/OSFile.h>

#include <iostream>

#include <LHScriptX/FeatureScriptCommands.h>

using namespace OpenBlack::LHScriptX;

void Script::LoadFile(const std::string &file)
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

			for (const auto &signature : FeatureScriptCommands::Signatures)
			{
				if (identifier == signature.name) {
					ScriptCommandContext ctx(nullptr, nullptr);

					try {
						signature.command(ctx);
					}
					catch (std::runtime_error &e) {
						printf("%s\n", e.what());
					}
				}
			}
			
		}

		this->advanceToken();
	}

	delete lexer_;
	delete[] contents;
}

/*void Script::processCommand(const std::string& command, std::vector<std::string> params)
{
	ScriptParameters parameters{ ScriptCommandParameter(2.3000f) };
	ScriptCommandContext ctx(nullptr, &parameters);
	_commands->Call(command, ctx);
}*/

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

/*void Parser::Parse()
{
	const Token* token = this->peekToken();

	while (!token->IsEOF())
	{
		token = this->peekToken();

		if (token->IsIdentifier())
			call();
		else
			throw ParserException("expected identifier", 0, 0);
	}
}*/

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

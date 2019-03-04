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

#include <LHScriptX/Script.h>
#include <LHScriptX/Lexer.h>

#include <Common/OSFile.h>

#include <iostream>

using namespace OpenBlack::LHScriptX;

void Script::LoadFile(const std::string &file)
{
	OSFile osfile;
	osfile.Open(file.c_str(), LH_FILE_MODE::Read);

	size_t totalFileSize = osfile.Size();

	char* contents = new char[totalFileSize];
	osfile.Read(contents, totalFileSize);

	ScanLine(contents);

	delete[] contents;
}

void Script::ScanLine(const std::string &line)
{
	//std::cout << "Scanning line: " << line << std::endl;

	Lexer lexer(line);

	Token token = lexer.GetToken();

	while (token.type != EndOfFile)
	{
		if (token.GetType() == Identifier && _commands->Has(token.GetValue()))
		{

			std::cout << "identifer call: " << token.GetValue() << std::endl;
		}
		else if (token.GetType() == Identifier)
		{
			std::cout << "UNKNOWN IDENTIFIER call: " << token.GetValue() << std::endl;
		}


		if (token.GetType() == Unknown)
			std::cout << token.GetType() << " \"" << token.GetValue() << "\" @ line " << token.GetLine() << std::endl;

		// get next token
		token = lexer.GetToken();
	};

	/*auto tokens = tokenize(line);
	std::cout << "Got " << tokens.size() << " tokens" << std::endl;

	for (auto const& i : tokens) {
		std::cout << i.Type << ": \"" << i.Value << "\"" << std::endl;
	}*/
}

void Script::processCommand(const std::string& command, std::vector<std::string> params)
{
	ScriptParameters parameters{ ScriptParameter(2.3000f) };
	ScriptCommandContext ctx(nullptr, &parameters);
	_commands->Call(command, ctx);
}

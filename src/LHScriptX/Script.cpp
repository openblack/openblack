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

	Lexer lexer(contents);

	Token token = lexer.GetToken();

	while (!token.IsEOF())
	{
		token.Print(stdout);
		printf("\n");

		// get next token
		token = lexer.GetToken();
	};

	delete[] contents;
}

void Script::processCommand(const std::string& command, std::vector<std::string> params)
{
	ScriptParameters parameters{ ScriptParameter(2.3000f) };
	ScriptCommandContext ctx(nullptr, &parameters);
	_commands->Call(command, ctx);
}

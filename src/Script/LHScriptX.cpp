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

#include "LHScriptX.h"

#include <iostream>
#include <vector>

#include <Script/Lexer.h>

/*using namespace OpenBlack;
//using namespace OpenBlack::Script;

void LHScriptX::LoadFile(std::string fileName)
{
	printf("Loading script %s\n", fileName.c_str());

	OSFile* file = new OSFile();
	file->Open(fileName.c_str(), LH_FILE_MODE::Read);

	LoadOneLine(file);

	file->Close();
}

bool LHScriptX::LoadOneLine(OSFile* file)
{
	std::string line;

	char cur;
	while (file->Read(&cur, 1) != 0)
	{
		if (cur == '\n') break;
		if (cur == '\r') {
			if (file->Read(&cur, 1) == 0) {
				break;
			}

			if (cur == '\n') {
				break;
			}
		}

		line.push_back(cur);
	}

	if (line.length() == 0) {
		return false;
	}

	ScanLine(line);

	return true;
}

void LHScriptX::ScanLine(std::string line)
{
	auto lexer = new Lexer(line);
	auto tokens = lexer->GetTokens();	

	// todo: this should be turned into some bytecode rather then executing directly from tokens?
	if (tokens.front().Type == TokenType::Identifier)
	{
		// assume it's a simple function call: Identifer, LeftPareth, Vars, RightPareth
		auto identifier = tokens.front();
		/*Functions::FunctionPointer func = Functions::GetFunctionFromName(identifier.Value);

		std::cout << identifier.Value << ": ID=" << func.id << "(";

		for (int i = 0; i < 12; i++) {
			Type param = func.parameters[i];
			if (param == Type::kTypeNone)
				break;

			std::cout << param;
			//std::cout << "\tP" << i << " Type = " << param << std::endl;
		}

		std::cout << ")" << std::endl;*/

		// next token should be: LeftParen
/*	}
	else {
		throw std::logic_error(std::string{} +"Line " + line + " does not start with an identifier.");
	}

	/*for (auto const& i : tokens) {
		std::cout << GetTokenTypeName(i.Type) << ": '" << i.Value << "'\n";
	}

	std::cout << std::endl;*/
//}

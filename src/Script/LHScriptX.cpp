#include "LHScriptX.h"

#include <iostream>
#include <vector>

#include <Script/Lexer.h>

using namespace OpenBlack;
using namespace OpenBlack::Script;

void LHScriptX::LoadFile(std::string fileName)
{
	printf("Loading script %s\n", fileName.c_str());

	OSFile* file = new OSFile();
	file->Open(fileName.c_str(), LH_FILE_MODE::Read);

	LoadOneLine(file);
	LoadOneLine(file);
	LoadOneLine(file);
	LoadOneLine(file);
	LoadOneLine(file);
	LoadOneLine(file);
	LoadOneLine(file);
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
	printf("scanning: %s\n", line.c_str());

	auto lexer = new Lexer(line);
	auto tokens = lexer->GetTokens();

	for (auto const& i : tokens) {
		std::cout << GetTokenTypeName(i.Type) << ": '" << i.Value << "'\n";
	}

	std::cout << std::endl;
}

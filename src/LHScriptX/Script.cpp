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

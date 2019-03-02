#pragma once
#ifndef OPENBLACK_LHSCRIPTX_SCRIPT_H
#define OPENBLACK_LHSCRIPTX_SCRIPT_H

#include <LHScriptX/Command.h>

#include <memory>
#include <list>

namespace OpenBlack {
namespace LHScriptX {

class Script {
public:
	void LoadFile(const std::string& file);
	void SetCommands(std::unique_ptr<ScriptCommands> &commands) { _commands = std::move(commands); }
	ScriptCommands &GetCommands() const { return *_commands; }

	void ScanLine(const std::string& line);
private:
	std::unique_ptr<ScriptCommands> _commands;
	void processCommand(const std::string& command, std::vector<std::string> parameters);
};

}
}

#endif

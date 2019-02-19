#pragma once
#ifndef OPENBLACK_LHSCRIPTX_COMMANDS_H
#define OPENBLACK_LHSCRIPTX_COMMANDS_H

#include <functional>
#include <Common/Types.h>

#include "Parameter.h"

#include <map>
#include <vector>

namespace OpenBlack {

class Game;

namespace LHScriptX {

class ScriptCommandNotFoundException : public std::exception {
private:
	std::string _command;
public:
	ScriptCommandNotFoundException(const std::string &command) : _command(command) {}
};
class ScriptCommandNotImplemented : public std::exception {};



class ScriptCommandContext {
public:
	ScriptCommandContext(Game* game, const ScriptParameters* parameters) : _game(game), _parameters(parameters) {}

	Game &GetGame() const { return *_game; }
	const ScriptParameters &GetParameters() const { return *_parameters; }

	const ScriptParameter& operator[](unsigned int arg) const {
		return _parameters->at(arg);
	}

	template <class T>
	T GetParameter(unsigned int arg) const;
private:
	Game* _game;
	const ScriptParameters* _parameters;
};

template <>
std::string& ScriptCommandContext::GetParameter<std::string&>(unsigned int arg) const;
template <>
float ScriptCommandContext::GetParameter<float>(unsigned int arg) const;
template <>
int ScriptCommandContext::GetParameter<int>(unsigned int arg) const;
// todo: vector

typedef void(*commandPtr)(const ScriptCommandContext&);

struct ScriptCommandDef {
	uint32_t id;
	const char *name;
	Type parameters[12];
	commandPtr command;
};

/** Function type refers to the actual callable binding of a function */
typedef std::function<void(const ScriptCommandContext&)> ScriptCommand;

/**
	ScriptCommands maps command names to functions and parameters and provides
	a contextual way to call the commands from the interpreter.
*/
class ScriptCommands {
private:
	struct CommandEntry {
		std::string name; // Command name
		ScriptCommand command; // Callable command using std::bind
		Type parameters[12];

		CommandEntry(const std::string _name, const ScriptCommand &_command) : name(_name), command(_command) {}
	};

	void registerCommand(const std::string &name, const ScriptCommand &func);

	typedef std::map<std::string, CommandEntry> ScriptCommandMap;
	ScriptCommandMap _commandMap;
	const CommandEntry &get(const std::string &name) const;
public:
	void RegisterCommands(const ScriptCommandDef *definitions, size_t count);
	void Call(const std::string &name, ScriptCommandContext &ctx);
	bool Has(const std::string &name);
	ScriptCommandMap GetCommands() { return _commandMap; }
};

}
}

#endif

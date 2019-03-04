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

#include "Command.h"

#include <iostream>

using namespace OpenBlack::LHScriptX;

void ScriptCommands::RegisterCommands(const ScriptCommandDef *definitions, size_t count)
{
	for (size_t i = 0; i < count; i++)
	{
		ScriptCommandDef def = definitions[i];
		registerCommand(std::string(def.name), def.command);
	}
}

void ScriptCommands::Call(const std::string &name, ScriptCommandContext &ctx)
{
	std::cout << "[LHSCRIPTX] Calling " + name << std::endl;
	get(name).command(ctx);
}

bool ScriptCommands::Has(const std::string &name)
{
	auto entry = _commandMap.find(name);
	return (entry != _commandMap.end());
}

void ScriptCommands::registerCommand(const std::string &name, const ScriptCommand &command)
{
	std::pair<ScriptCommandMap::iterator, bool> result = _commandMap.insert(ScriptCommandMap::value_type(name, CommandEntry(name, command)));
	if (!result.second)
		throw std::runtime_error("Failed to register LHScriptX command: " + name);
}

const ScriptCommands::CommandEntry &ScriptCommands::get(const std::string &name) const {
	auto entry = _commandMap.find(name);
	if (entry == _commandMap.end())
		throw ScriptCommandNotFoundException(name);

	return entry->second;
}

template <>
std::string & ScriptCommandContext::GetParameter<std::string &>(unsigned int arg) const {
	return GetParameters().at(arg).GetString();
}

template <>
float ScriptCommandContext::GetParameter<float>(unsigned int arg) const {
	return GetParameters().at(arg).GetFloat();
}

template <>
int ScriptCommandContext::GetParameter<int>(unsigned int arg) const {
	return GetParameters().at(arg).GetNumber();
}

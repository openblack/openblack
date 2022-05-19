/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <string>
#include <vector>

namespace openblack::LHVM
{

class VMScript
{
public:
	VMScript() = delete;
	VMScript(std::string name, std::string filename, uint32_t type, uint32_t varOffset, std::vector<std::string> variables,
	         uint32_t instructionAddress, uint32_t parameterCount, uint32_t scriptId)
	    : _name(std::move(name))
	    , _filename(std::move(filename))
	    , _type(type)
	    , _variables(std::move(variables))
	    , _variablesOffset(varOffset)
	    , _instructionAddress(instructionAddress)
	    , _parameterCount(parameterCount)
	    , _scriptId(scriptId)
	{
	}
	~VMScript() = default;

	[[nodiscard]] const std::string& GetName() const { return _name; }
	[[nodiscard]] const std::string& GetFileName() const { return _filename; }
	[[nodiscard]] const std::vector<std::string>& GetVariables() const { return _variables; }
	[[nodiscard]] uint32_t GetVariableOffset() const { return _variablesOffset; }
	[[nodiscard]] uint32_t GetInstructionAddress() const { return _instructionAddress; }
	[[nodiscard]] uint32_t GetParameterCount() const { return _parameterCount; }
	[[nodiscard]] uint32_t GetScriptID() const { return _scriptId; }

private:
	std::string _name;
	std::string _filename;
	uint32_t _type;
	std::vector<std::string> _variables;

	uint32_t _variablesOffset;
	uint32_t _instructionAddress;
	uint32_t _parameterCount;
	uint32_t _scriptId;
};

} // namespace openblack::LHVM

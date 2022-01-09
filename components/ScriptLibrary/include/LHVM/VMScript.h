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

namespace openblack
{
namespace LHVM
{

class VMScript
{
public:
	VMScript() = delete;
	VMScript(const std::string& name, const std::string& filename, uint32_t type, uint32_t var_offset,
	         std::vector<std::string> variables, uint32_t instruction_address, uint32_t parameter_count, uint32_t script_id)
	    : _name(name)
	    , _filename(filename)
	    , _type(type)
	    , _variables(std::move(variables))
	    , _variables_offset(var_offset)
	    , _instruction_address(instruction_address)
	    , _parameter_count(parameter_count)
	    , _script_id(script_id)
	{
	}
	~VMScript() = default;

	const std::string& GetName() const { return _name; }
	const std::string& GetFileName() const { return _filename; }
	const std::vector<std::string>& GetVariables() const { return _variables; }
	uint32_t GetVariableOffset() const { return _variables_offset; }
	uint32_t GetInstructionAddress() const { return _instruction_address; }
	uint32_t GetParameterCount() const { return _parameter_count; }
	uint32_t GetScriptID() const { return _script_id; }

private:
	std::string _name;
	std::string _filename;
	uint32_t _type;
	std::vector<std::string> _variables;

	uint32_t _variables_offset;
	uint32_t _instruction_address;
	uint32_t _parameter_count;
	uint32_t _script_id;
};

} // namespace LHVM
} // namespace openblack

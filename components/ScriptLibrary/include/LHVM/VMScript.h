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

#pragma once

#include <string>
#include <vector>

namespace OpenBlack {
namespace LHVM {

class VMScript {
public:
	VMScript() = delete;
	VMScript(const std::string &name, const std::string &filename, uint32_t type, uint32_t var_offset, std::vector<std::string> variables,
		uint32_t instruction_address, uint32_t parameter_count, uint32_t script_id) :
		_name(name), _filename(filename), _type(type), _variables_offset(var_offset), _variables(std::move(variables)),
		_instruction_address(instruction_address), _parameter_count(parameter_count), _script_id(script_id) {}
	~VMScript() = default;

	const std::string &GetName() const { return _name; }
	const std::string &GetFileName() const { return _filename; }
	const std::vector<std::string> &GetVariables() const { return _variables; }
	uint32_t GetVariableOffset() const { return _variables_offset; }
	uint32_t GetInstructionAddress() const { return _instruction_address; }
	uint32_t GetParameterCount() const { return _parameter_count; }
	uint32_t GetScriptID() const { return _script_id;  }
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

}
}

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

#include <LHVM/LHVM.hpp>
#include <LHVM/VMInstruction.hpp>
#include <LHVM/OpcodeNames.hpp>

#include <cstdio>

namespace OpenBlack {
namespace LHVM {

void LHVM::LoadBinary(const std::string &filename)
{
	std::FILE* file = std::fopen(filename.c_str(), "rb");
	if (file == nullptr)
		throw std::exception("no such file");

	char lhvm[4];
	std::fread(&lhvm, 1, 4, file);

	if (std::strncmp(lhvm, "LHVM", 4) != 0)
		throw std::exception("invalid LHVM header");

	std::fread(&_version, 1, 4, file);

	/* only support bw1 at the moment */
	if (_version != Version::BlackAndWhite)
		throw std::exception("unsupported LHVM version");

	loadVariables(file, _variables);
	loadCode(file);
	loadAuto(file);
	loadScripts(file);
	loadData(file);

	// creature isle
	if (_version == Version::CreatureIsle)
	{
		// VMTypedVariableInfo::Load((std::_FILE *))
		// there is extra data: 512 loop { 2 int32 } final int32
	}

	std::fclose(file);
}

void LHVM::loadVariables(std::FILE* file, std::vector<std::string> &variables)
{
	int32_t count;
	std::fread(&count, 4, 1, file);

	// if there are no variables, return
	if (count <= 0)
		return;

	// preallocate a buffer to reuse for each variable
	char buffer[255];

	for (int32_t i = 0; i < count; i++)
	{
		// reset cur pointer to 0
		char* cur = &buffer[0];

		while (*(cur-1) != '\0') {
			std::fread(cur++, 1, 1, file);
		}

		// throw it into the std::string vector
		variables.emplace_back(buffer);
	}
}

void LHVM::loadCode(std::FILE* file)
{
	int32_t count;
	std::fread(&count, 4, 1, file);

	// if there are no variables, return
	if (count <= 0)
		return;

	for (int32_t i = 0; i < count; i++)
	{
		uint32_t instruction[5]; // quick way to minimize code
		std::fread(&instruction, 4, 5, file);

		_instructions.emplace_back(
			static_cast<VMInstruction::Opcode>(instruction[0]),
			static_cast<VMInstruction::Access>(instruction[1]),
			static_cast<VMInstruction::DataType>(instruction[2]),
			instruction[3],
			instruction[4]
		);
	}
}

void LHVM::loadAuto(std::FILE* file)
{
	int32_t count;
	std::fread(&count, 4, 1, file);

	// if there are no variables, return
	if (count <= 0)
		return;

	for (int32_t i = 0; i < count; i++)
	{
		uint32_t id;
		std::fread(&id, 4, 1, file);
	}
}

void LHVM::loadScripts(std::FILE* file)
{
	int32_t count;
	std::fread(&count, 4, 1, file);

	// if there are no variables, return
	if (count <= 0)
		return;

	char script_name[255];
	char file_name[255];

	for (int32_t i = 0; i < count; i++)
	{
		char* cur = &script_name[0];
		while (*(cur - 1) != '\0') { std::fread(cur++, 1, 1, file); }

		cur = &file_name[0];
		while (*(cur - 1) != '\0') { std::fread(cur++, 1, 1, file); }

		uint32_t script_type, shit;
		std::fread(&script_type, 4, 1, file);
		std::fread(&shit, 4, 1, file);

		std::vector<std::string> variables;
		loadVariables(file, variables);

		uint32_t instruction_address, parameter_count, script_id;
		std::fread(&instruction_address, 4, 1, file);
		std::fread(&parameter_count, 4, 1, file);
		std::fread(&script_id, 4, 1, file);

		_scripts.emplace_back(
			std::string(script_name),
			std::string(file_name),
			script_type,
			shit,
			variables,
			instruction_address,
			parameter_count,
			script_id
		);
	}
}

void LHVM::loadData(std::FILE* file)
{
	uint32_t size;
	std::fread(&size, 4, 1, file);

	_data.resize(size);
	std::fread(_data.data(), 1, size, file);
}

/*
	a pretty long method to turn each different opcode into a somewhat
	human readable string
*/
std::string VMInstruction::Disassemble() const
{
	std::string opcode_name = Opcode_Names[(int)_code];





	return opcode_name;
}

}
}

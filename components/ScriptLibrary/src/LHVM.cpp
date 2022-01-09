/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>

#include <LHVM/LHVM.h>
#include <LHVM/OpcodeNames.h>
#include <LHVM/VMInstruction.h>

namespace openblack::LHVM
{

void LHVM::LoadBinary(const std::string& filename)
{
	std::FILE* file = std::fopen(filename.c_str(), "rb");
	if (file == nullptr)
		throw std::runtime_error("no such file");

	size_t count;
	std::array<char, 4> lhvm;
	count = std::fread(&lhvm, sizeof(lhvm[0]), lhvm.size(), file);

	if (count != lhvm.size() || std::strncmp(lhvm.data(), "LHVM", lhvm.size()) != 0)
		throw std::runtime_error("invalid LHVM header");

	count = std::fread(&_version, sizeof(_version), 1, file);

	/* only support bw1 at the moment */
	if (count != 1 || _version != Version::BlackAndWhite)
		throw std::runtime_error("unsupported LHVM version");

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

void LHVM::loadVariables(std::FILE* file, std::vector<std::string>& variables)
{
	int32_t count;

	if (std::fread(&count, sizeof(count), 1, file) != 1)
		throw std::runtime_error("error reading variable count");

	// if there are no variables, return
	if (count <= 0)
		return;

	// preallocate a buffer to reuse for each variable
	char buffer[255];

	for (int32_t i = 0; i < count; i++)
	{
		// reset cur pointer to 0
		char* cur = &buffer[0];
		do
		{
			if (std::fread(cur++, sizeof(*cur), 1, file) != 1)
				throw std::runtime_error("error reading variable");
		} while (*(cur - 1) != '\0');

		// throw it into the std::string vector
		variables.emplace_back(buffer);
	}
}

void LHVM::loadCode(std::FILE* file)
{
	int32_t count;
	if (std::fread(&count, sizeof(count), 1, file) != 1)
		throw std::runtime_error("error reading code count");

	// if there are no variables, return
	if (count <= 0)
		return;

	for (int32_t i = 0; i < count; i++)
	{
		std::array<uint32_t, 5> instruction; // quick way to minimize code
		if (std::fread(&instruction, sizeof(instruction[0]), instruction.size(), file) != instruction.size())
			throw std::runtime_error("error reading instructions");

		_instructions.emplace_back(static_cast<VMInstruction::Opcode>(instruction[0]),
		                           static_cast<VMInstruction::Access>(instruction[1]),
		                           static_cast<VMInstruction::DataType>(instruction[2]), instruction[3], instruction[4]);
	}
}

void LHVM::loadAuto(std::FILE* file)
{
	int32_t count;
	if (std::fread(&count, sizeof(count), 1, file) != 1)
		throw std::runtime_error("error reading id count");

	// if there are no variables, return
	if (count <= 0)
		return;

	std::vector<uint32_t> ids(count);

	if (std::fread(&count, sizeof(ids[0]), ids.size(), file) != ids.size())
		throw std::runtime_error("error reading ids");
}

void LHVM::loadScripts(std::FILE* file)
{
	int32_t count;
	if (std::fread(&count, sizeof(count), 1, file) != 1)
		throw std::runtime_error("error reading script count");

	// if there are no variables, return
	if (count <= 0)
		return;

	char script_name[255];
	char file_name[255];

	for (int32_t i = 0; i < count; i++)
	{
		char* cur = &script_name[0];
		do
		{
			if (std::fread(cur++, sizeof(*cur), 1, file) != 1)
				throw std::runtime_error("error reading script name");
		} while (*(cur - 1) != '\0');

		cur = &file_name[0];
		do
		{
			if (std::fread(cur++, sizeof(*cur), 1, file) != 1)
				throw std::runtime_error("error reading script filename");
		} while (*(cur - 1) != '\0');

		uint32_t script_type, var_offset;
		if (std::fread(&script_type, sizeof(script_type), 1, file) != 1)
			throw std::runtime_error("error reading script type");
		if (std::fread(&var_offset, sizeof(var_offset), 1, file) != 1)
			throw std::runtime_error("error reading script variables offset");

		std::vector<std::string> variables;
		loadVariables(file, variables);

		uint32_t instruction_address, parameter_count, script_id;
		if (std::fread(&instruction_address, sizeof(instruction_address), 1, file) != 1)
			throw std::runtime_error("error reading instruction_address");
		if (std::fread(&parameter_count, sizeof(parameter_count), 1, file) != 1)
			throw std::runtime_error("error reading parameter_count");
		if (std::fread(&script_id, sizeof(script_id), 1, file) != 1)
			throw std::runtime_error("error reading script_id");

		_scripts.emplace_back(std::string(script_name), std::string(file_name), script_type, var_offset, variables,
		                      instruction_address, parameter_count, script_id);
	}
}

void LHVM::loadData(std::FILE* file)
{
	int32_t size;
	if (std::fread(&size, sizeof(size), 1, file) != 1)
		throw std::runtime_error("error reading data size");

	_data.resize(size);
	if (std::fread(_data.data(), sizeof(_data[0]), _data.size(), file) != _data.size())
		throw std::runtime_error("error reading data");
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

} // namespace openblack::LHVM

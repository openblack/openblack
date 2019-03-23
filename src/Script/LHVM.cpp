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

#include <Script/LHVM.h>

using namespace OpenBlack;

std::array<const char*, 31> OpenBlack::VMOPCode_Names = {
	"END", "WAIT", "PUSH", "POP", "ADD", "CALL", "MINUS", "UMINUS", "TIMES", "DIVIDE", "MODULUS", "NOT", "AND",
	"OR", "EQ", "NEQ", "GEQ", "LEQ", "GT", "LT", "JUMP", "SLEEP", "EXCEPT", "INTCAST", "RUN", "ENDEXCEPT", "RETEXCEPT",
	"FAILEXCEPT", "BRKEXCEPT", "SWAP", "LINE"
};

std::array<const char*, 8> OpenBlack::VMDataType_Names = {
	"NONE", "INT", "FLOAT", "VECTOR", "UNK", "UNK2", "BOOLEAN", "UNK4"
};

std::array<const char*, 2> OpenBlack::VMAccess_Name = { "STACK", "VARIABLE" };

void LHVM::LoadBinary(std::string& filename)
{
	OSFile* file = new OSFile();
	file->Open(filename.c_str(), LH_FILE_MODE::Read);

	loadByteCode(file);

	file->Close();
	delete file;
}

void LHVM::loadByteCode(OSFile* file)
{
	loadHeader(file);
	loadVariable(file);
	loadCode(file);
	loadAuto(file);
	loadScript(file);
	loadData(file);
}

void LHVM::loadHeader(OSFile* file)
{
	char lhvm[4];
	file->Read(&lhvm, 4);

	if (std::strncmp(lhvm, "LHVM", 4) != 0)
		throw std::runtime_error("Invalid LHVM header");

	file->Read(&_version, 4);
	printf("LHVM loaded version %d\n", _version);
}

void LHVM::loadVariable(OSFile* file)
{
	uint32_t count;
	file->Read(&count, 4);

	for (int i = 0; i < count; i++)
	{
		std::string name = file->ReadString();
		_variables.push_back(name);
	}
}

void LHVM::loadCode(OSFile* file)
{
	uint32_t count;
	file->Read(&count, 4);

	for (int i = 0; i < count; i++)
	{
		VMInstruction instruction;
		file->Read(&instruction, 20);

		_instructions.push_back(instruction);
	}
}

void LHVM::loadAuto(OSFile* file)
{
	uint32_t count;
	file->Read(&count, 4);

	for (int i = 0; i < count; i++)
	{
		uint32_t shit;
		file->Read(&shit, 4);
	}
}

void LHVM::loadScript(OSFile* file)
{
	uint32_t count;
	file->Read(&count, 4);

	for (int i = 0; i < count; i++)
	{
		std::string name = file->ReadString();
		std::string file_name = file->ReadString();

		//printf("script %s from %s\n", name.c_str(), file_name.c_str());

		uint32_t shit;
		file->Read(&shit, 4);
		file->Read(&shit, 4);
		file->Read(&shit, 4);

		for (int j = 0; j < shit; j++)
		{
			std::string var_name = file->ReadString();
			//printf("\t[%d] = %s\n", j, var_name.c_str());
		}

		file->Read(&shit, 4);
		file->Read(&shit, 4);
		file->Read(&shit, 4);

	}
}

void LHVM::loadData(OSFile* file)
{
	uint32_t size;
	file->Read(&size, 4);

	_data = std::vector<uint8_t>(size);
	file->Read(_data.data(), size);
}

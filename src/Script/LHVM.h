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
#ifndef OPENBLACK_SCRIPT_LHVM_H
#define OPENBLACK_SCRIPT_LHVM_H

#include <array>
#include <string>
#include <vector>

#include <Common/OSFile.h>

namespace OpenBlack {
	// should just be a uint8_t, but we read directly into VMInstruction struct as of now
	enum class VMOPCode : uint32_t {
		END = 0, WAIT, PUSH, POP, ADD, CALL, MINUS, UMINUS, TIMES, DIVIDE, MODULUS, NOT, AND, OR, EQ, NEQ, GEQ,
		LEQ, GT, LT, JUMP, SLEEP, EXCEPT, INTCAST, RUN, ENDEXCEPT, RETEXCEPT, FAILEXCEPT, BRKEXCEPT, SWAP, LINE
	};

	enum class VMAccess : uint32_t {
		STACK = 0,
		VARIABLE = 1
	};

	enum class VMDataType : uint32_t {
		NONE = 0,
		INT = 1,
		FLOAT = 2,
		VECTOR = 3,
		UNK = 4,
		UNK2 = 5,
		BOOLEAN = 6,
		UNK4 = 7,
	};

	extern std::array<const char*, 31> VMOPCode_Names;
	extern std::array<const char*, 2> VMAccess_Name;
	extern std::array<const char*, 8> VMDataType_Names;

	struct VMInstruction
	{
		VMOPCode code;
		VMAccess access;
		VMDataType type;

		union
		{
			int32_t intValue;
			float floatValue;
		} data;

		uint32_t line;

		const char* getOPCodeName() { return VMOPCode_Names[(int)code]; }
		const char* getAccess() { return VMAccess_Name[(int)access]; }
		const char* getDataTypeName() { return VMDataType_Names[(int)type]; }
		std::string getDataToString() {
			if (access == VMAccess::VARIABLE)
				return "[" + std::to_string(data.intValue) + "]";
			if (type == VMDataType::FLOAT)
				return std::to_string(data.floatValue);
			return std::to_string(data.intValue);
		}
	};

	class LHVM
	{
	public:
		LHVM() = default;

		void LoadBinary(std::string& filename);
	private:
		void loadByteCode(OSFile* file);
		void loadHeader(OSFile* file);
		void loadVariable(OSFile* file);
		void loadCode(OSFile* file);
		void loadAuto(OSFile* file);
		void loadScript(OSFile* file);
		void loadData(OSFile* file);

		std::vector<std::string> _variables;
		std::vector<VMInstruction> _instructions;
		std::vector<uint8_t> _data;
	private:
		uint32_t _version;

	public:
		const std::vector<VMInstruction> &GetInstructions() const { return _instructions; }
		const std::vector<std::string> &GetVariables() const { return _variables; }
		const std::vector<uint8_t> &GetData() const { return _data; }
	};
}

#endif

/* openblack - A reimplementation of Lionheads's Black & White engine
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdint.h>
#include <string>

namespace openblack
{
namespace LHVM
{

class VMInstruction
{
  public:
	enum class Opcode : uint_fast8_t
	{
		END = 0,
		WAIT,
		PUSH,
		POP,
		ADD,
		CALL,
		MINUS,
		UMINUS,
		TIMES,
		DIVIDE,
		MODULUS,
		NOT,
		AND,
		OR,
		EQ,
		NEQ,
		GEQ,
		LEQ,
		GT,
		LT,
		JUMP,
		SLEEP,
		EXCEPT,
		INTCAST,
		RUN,
		ENDEXCEPT,
		RETEXCEPT,
		FAILEXCEPT,
		BRKEXCEPT,
		SWAP,
		LINE
	};
	enum class Access : uint_fast8_t
	{
		STACK = 0,
		VARIABLE
	};
	enum class DataType : uint_fast8_t
	{
		NONE = 0,
		INT,
		FLOAT,
		VECTOR,
		UNK,
		UNK2,
		BOOLEAN,
		UNK4
	};

	VMInstruction() = delete;
	VMInstruction(Opcode code, Access access, DataType type, uint32_t data, uint32_t line):
	    _code(code), _access(access), _type(type), _data(data), _line(line) {}
	~VMInstruction() = default;

	Opcode GetOpcode() const { return _code; }
	Access GetAccess() const { return _access; }
	DataType GetDataType() const { return _type; }
	uint32_t GetData() const { return _data; }
	uint32_t GetLineNumber() const { return _line; }

	std::string Disassemble() const;

  private:
	Opcode _code;
	Access _access;
	DataType _type;
	uint32_t _data;
	uint32_t _line;
};

} // namespace LHVM
} // namespace openblack

/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

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
	VMInstruction(Opcode code, Access access, DataType type, uint32_t data, uint32_t line)
	    : _code(code)
	    , _access(access)
	    , _type(type)
	    , _data(data)
	    , _line(line)
	{
	}
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

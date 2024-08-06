/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cassert>
#include <cstdint>

#include <array>
#include <functional>
#include <map>
#include <string>

namespace openblack::LHVM
{

static const std::array<std::string, 31> Opcode_Names = {
    "END",    "JZ",   "PUSH", "POP",       "ADD",       "SYS",        "SUB",       "NEG",  "MUL", "DIV", "MOD",
    "NOT",    "AND",  "OR",   "EQ",        "NE",        "GE",         "LE",        "GT",   "LT",  "JMP", "SLEEP",
    "EXCEPT", "CAST", "RUN",  "ENDEXCEPT", "RETEXCEPT", "FAILEXCEPT", "BRKEXCEPT", "SWAP", "LINE"};

enum class LHVMVersion : uint32_t
{
	BlackAndWhite = 7,
	CreatureIsle = 8,
	BlackAndWhite2 = 12
};

enum class DataType : uint32_t
{
	NONE = 0,
	INT,
	FLOAT,
	VECTOR,
	OBJECT,
	UNK2,
	BOOLEAN,
	UNK4
};
static_assert(sizeof(DataType) == 4);

static const std::array<std::string, 8> DataType_Names = {"NONE", "INT", "FLOAT", "VECTOR",
                                                          "OBJECT", "UNK2", "BOOLEAN", "UNK4"};

static const std::array<std::string, 8> DataType_Chars = {"", "I", "F", "V", "O", "", "B", ""};

struct VMValue
{
	union
	{
		float_t floatVal;
		int32_t intVal;
		uint32_t uintVal;
	};
};
static_assert(sizeof(VMValue) == 4);

struct VMVar
{
	DataType type;
	union
	{
		VMValue value;
		float_t floatVal;
		int32_t intVal;
		uint32_t uintVal;
	};
	std::string name;
};

struct VMStack
{
	uint32_t count;
	std::array<VMValue, 32> values;
	std::array<DataType, 32> types;
	uint32_t pushCount;
	uint32_t popCount;
};
static_assert(sizeof(VMStack) == 268);

enum class ScriptType : uint32_t
{
	SCRIPT = 1,
	HELP = 2,
	CHALLENGE_HELP = 4,
	TEMPLE_HELP = 8,
	TEMPLE_SPECIAL = 16,
	MULTIPLAYER_HELP = 64,

	ALL = 0xFFFFFFFF
};
static_assert(sizeof(ScriptType) == 4);

inline constexpr ScriptType operator|(ScriptType Lhs, ScriptType Rhs)
{
	return static_cast<ScriptType>(static_cast<uint32_t>(Lhs) | static_cast<uint32_t>(Rhs));
}

inline constexpr bool operator&(ScriptType Lhs, ScriptType Rhs)
{
	return (static_cast<uint32_t>(Lhs) & static_cast<uint32_t>(Rhs)) != 0;
}

inline constexpr bool operator&(ScriptType Lhs, uint32_t Rhs)
{
	return (static_cast<uint32_t>(Lhs) & Rhs) != 0;
}

static const std::map<ScriptType, std::string> ScriptType_Names = {{ScriptType::SCRIPT, "script"},
                                                                   {ScriptType::HELP, "help script"},
                                                                   {ScriptType::CHALLENGE_HELP, "challenge help script"},
                                                                   {ScriptType::TEMPLE_HELP, "temple help script"},
                                                                   {ScriptType::TEMPLE_SPECIAL, "temple special script"},
                                                                   {ScriptType::MULTIPLAYER_HELP, "multiplayer help script"}};

enum class Opcode : uint32_t
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
	CAST, // or ZERO
	RUN,
	ENDEXCEPT, // or FREE
	RETEXCEPT,
	FAILEXCEPT,
	BRKEXCEPT,
	SWAP,
	LINE
};
static_assert(sizeof(Opcode) == 4);

enum class Mode : uint32_t
{
	// Data access modes
	IMMEDIATE = 0,
	REFERENCE = 1,
	// Jump directions
	BACKWARD = 0,
	FORWARD = 1,
	// Call modes
	SYNC = 0,
	ASYNC = 1,
	// CAST or ZERO
	CAST = 0,
	ZERO = 1,
	// ENDEXCEPT or YIELD
	ENDEXCEPT = 0,
	YIELD = 1,
	// SWAP alternatives
	COPYTO = 0,
	COPYFROM = 1,
};
static_assert(sizeof(Mode) == 4);

struct VMInstruction
{
	Opcode opcode;
	Mode mode;
	DataType type;
	union
	{
		VMValue data;
		float_t floatVal;
		int32_t intVal;
		uint32_t uintVal;
	};
	uint32_t line;
};
static_assert(sizeof(VMInstruction) == 20);

struct VMExceptStruct
{
	uint32_t instructionAddress;
	std::vector<uint32_t> exceptionHandlerIps;
};

struct VMTask
{
	std::vector<VMVar> localVars;
	uint32_t scriptId;
	uint32_t id;
	uint32_t instructionAddress;
	uint32_t prevInstructionAddress;
	uint32_t waitingTaskId;
	uint32_t variablesOffset;
	VMStack stack;
	uint32_t currentExceptionHandlerIndex;
	VMExceptStruct exceptStruct;
	uint32_t ticks;
	bool inExceptionHandler;
	bool stop;
	bool yield;
	bool sleeping;
	std::string name;
	std::string filename;
	ScriptType type;
};

struct NativeFunction
{
	std::function<void()> impl;
	int32_t stackIn;
	uint32_t stackOut;
	uint32_t filler;
	char name[128];
};

enum class ErrorCode : uint32_t
{
	NO_ERR = 0,
	STACK_EMPTY,
	STACK_FULL,
	SCRIPT_ID_NOT_FOUND,
	SCRIPT_NAME_NOT_FOUND,
	NO_SCRIPT_OF_TYPE,
	TASK_ID_NOT_FOUND,
	NATIVE_FUNC_NOT_FOUND,
	DIV_BY_ZERO,
	INVALID_TYPE,
};

static const std::array<std::string, 10> Error_Msg = {"no error",
                                                      "stack is empty",
                                                      "stack is full",
                                                      "script id not found",
                                                      "script name not found",
                                                      "no script of type",
                                                      "task id not found",
                                                      "native function not found",
                                                      "division by zero",
                                                      "invalid data type"};

} // namespace openblack::LHVM

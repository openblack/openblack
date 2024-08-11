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

static const std::array<std::string, 31> k_OpcodeNames = {
    "END",    "JZ",   "PUSH", "POP",       "ADD",       "SYS",        "SUB",       "NEG",  "MUL", "DIV", "MOD",
    "NOT",    "AND",  "OR",   "EQ",        "NE",        "GE",         "LE",        "GT",   "LT",  "JMP", "SLEEP",
    "EXCEPT", "CAST", "RUN",  "ENDEXCEPT", "RETEXCEPT", "FAILEXCEPT", "BRKEXCEPT", "SWAP", "LINE"};

enum class LHVMVersion : uint32_t
{
	BlackAndWhite = 7,
	CreatureIsle = 8,
	BlackAndWhite2 = 12,

	Last = 0xFFFFFFFF
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
	UNK4,

	Last = 0xFFFFFFFF
};
static_assert(sizeof(DataType) == 4);

static const std::array<std::string, 8> k_DataTypeNames = {"NONE",   "INT",  "FLOAT",   "VECTOR",
                                                           "OBJECT", "UNK2", "BOOLEAN", "UNK4"};

static const std::array<std::string, 8> k_DataTypeChars = {"", "I", "F", "V", "O", "", "B", ""};

class VMValue
{
public:
	VMValue()
	    : floatVal(0.0f)
	{
	}

	explicit VMValue(float value)
	    : floatVal(value)
	{
	}

	explicit VMValue(int32_t value)
	    : intVal(value)
	{
	}

	explicit VMValue(uint32_t value)
	    : uintVal(value)
	{
	}

	union
	{
		float floatVal;
		int32_t intVal;
		uint32_t uintVal;
	};
};
static_assert(sizeof(VMValue) == 4);

class VMVar
{
public:
	VMVar(DataType type, VMValue value, std::string name)
	    : type(type)
	    , value(value)
	    , name(std::move(name))
	{
	}

	DataType type;
	union
	{
		VMValue value;
		float floatVal;
		int32_t intVal;
		uint32_t uintVal;
	};
	std::string name;
};

class VMStack
{
public:
	VMStack() = default;

	uint32_t count {0};
	std::array<VMValue, 32> values {};
	std::array<DataType, 32> types {};
	uint32_t pushCount {0};
	uint32_t popCount {0};
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

constexpr ScriptType operator|(ScriptType lhs, ScriptType rhs)
{
	return static_cast<ScriptType>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

constexpr bool operator&(ScriptType lhs, ScriptType rhs)
{
	return (static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)) != 0;
}

constexpr bool operator&(ScriptType lhs, uint32_t rhs)
{
	return (static_cast<uint32_t>(lhs) & rhs) != 0;
}

static const std::map<ScriptType, std::string> k_ScriptTypeNames = {{ScriptType::SCRIPT, "script"},
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
	LINE,

	Last = 0xFFFFFFFF
};
static_assert(sizeof(Opcode) == 4);

enum class VMMode : uint32_t
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

	Last = 0xFFFFFFFF
};
static_assert(sizeof(VMMode) == 4);

class VMInstruction
{
public:
	VMInstruction() {}

	VMInstruction(Opcode code, VMMode mode, DataType type, VMValue data, uint32_t line)
	    : code(code)
	    , mode(mode)
	    , type(type)
	    , data(data)
	    , line(line)
	{
	}

	Opcode code {Opcode::LINE};
	VMMode mode {VMMode::IMMEDIATE};
	DataType type {DataType::FLOAT};
	union
	{
		VMValue data;
		float floatVal {0.0f};
		int32_t intVal;
		uint32_t uintVal;
	};
	uint32_t line {0};
};
static_assert(sizeof(VMInstruction) == 20);

class VMScript
{
public:
	VMScript() {};

	VMScript(std::string name, std::string filename, ScriptType type, uint32_t varOffset, std::vector<std::string> variables,
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
	[[nodiscard]] ScriptType GetType() const { return _type; }
	[[nodiscard]] const std::vector<std::string>& GetVariables() const { return _variables; }
	[[nodiscard]] uint32_t GetVariablesOffset() const { return _variablesOffset; }
	[[nodiscard]] uint32_t GetInstructionAddress() const { return _instructionAddress; }
	[[nodiscard]] uint32_t GetParameterCount() const { return _parameterCount; }
	[[nodiscard]] uint32_t GetScriptID() const { return _scriptId; }

private:
	std::string _name;
	std::string _filename;
	ScriptType _type = ScriptType::SCRIPT;
	std::vector<std::string> _variables;

	uint32_t _variablesOffset = 0;
	uint32_t _instructionAddress = 0;
	uint32_t _parameterCount = 0;
	uint32_t _scriptId = 0;
};

class VMTask
{
public:
	VMTask() {}

	VMTask(std::vector<VMVar> localVars, uint32_t scriptId, uint32_t id, uint32_t instructionAddress, uint32_t variablesOffset,
	       VMStack stack, std::string name, std::string filename, ScriptType type)
	    : localVars(localVars)
	    , scriptId(scriptId)
	    , id(id)
	    , instructionAddress(instructionAddress)
	    , variablesOffset(variablesOffset)
	    , stack(stack)
	    , name(name)
	    , filename(filename)
	    , type(type)
	{
	}

	std::vector<VMVar> localVars {};
	uint32_t scriptId {0};
	uint32_t id {0};
	uint32_t instructionAddress {0};
	uint32_t pevInstructionAddress {0};
	uint32_t waitingTaskId {0};
	uint32_t variablesOffset {0};
	VMStack stack;
	uint32_t currentExceptionHandlerIndex {0};
	std::vector<uint32_t> exceptionHandlerIps;
	uint32_t ticks {1};
	bool inExceptionHandler {false};
	bool stop {false};
	bool iield {false};
	bool sleeping {false};
	std::string name;
	std::string filename;
	ScriptType type {ScriptType::SCRIPT};
};

class NativeFunction
{
public:
	NativeFunction(std::function<void()> impl, int32_t stackIn, int32_t stackOut, std::string name)
	    : impl(impl)
	    , stackIn(stackIn)
	    , stackOut(stackOut)
	    , name(name)
	{
	}

	std::function<void()> impl;
	int32_t stackIn;
	uint32_t stackOut;
	std::string name;
};

enum class ErrorCode : uint8_t
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

static const std::array<std::string, 10> k_ErrorMsg = {"no error",
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

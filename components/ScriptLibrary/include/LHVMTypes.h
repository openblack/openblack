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
		: FloatVal(0.0f) {}

	VMValue(float value)
	    : FloatVal(value) {}

	VMValue(int32_t value)
	    : IntVal(value) {}

	VMValue(uint32_t value)
	    : UintVal(value) {}

	union
	{
		float FloatVal;
		int32_t IntVal;
		uint32_t UintVal;
	};
};
static_assert(sizeof(VMValue) == 4);

class VMVar
{
public:
	VMVar(DataType type, VMValue value, std::string name)
	    : Type(type)
	    , Value(value)
	    , Name(name) {}

	DataType Type;
	union
	{
		VMValue Value;
		float FloatVal;
		int32_t IntVal;
		uint32_t UintVal;
	};
	std::string Name;
};

class VMStack
{
public:
	VMStack() {}

	uint32_t Count {0};
	std::array<VMValue, 32> Values {};
	std::array<DataType, 32> Types {};
	uint32_t PushCount {0};
	uint32_t PopCount {0};
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

	Last = 0xFFFFFFFF
};
static_assert(sizeof(Mode) == 4);

class VMInstruction
{
public:
	VMInstruction() {}

	VMInstruction(Opcode opcode, Mode mode, DataType type, VMValue data, uint32_t line)
		: Opcode(opcode)
		, Mode(mode)
		, Type(type)
		, Data(data)
		, Line(line) {}

	Opcode Opcode {Opcode::LINE};
	Mode Mode {Mode::IMMEDIATE};
	DataType Type {DataType::FLOAT};
	union
	{
		VMValue Data;
		float FloatVal {0.0f};
		int32_t IntVal;
		uint32_t UintVal;
	};
	uint32_t Line {0};
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
		: LocalVars(localVars)
		, ScriptId(scriptId)
		, Id(id)
		, InstructionAddress(instructionAddress)
		, VariablesOffset(variablesOffset)
		, Stack(stack)
		, Name(name)
		, Filename(filename)
		, Type(type) {}

	std::vector<VMVar> LocalVars {};
	uint32_t ScriptId {0};
	uint32_t Id {0};
	uint32_t InstructionAddress {0};
	uint32_t PrevInstructionAddress {0};
	uint32_t WaitingTaskId {0};
	uint32_t VariablesOffset {0};
	VMStack Stack;
	uint32_t CurrentExceptionHandlerIndex {0};
	std::vector<uint32_t> ExceptionHandlerIps;
	uint32_t Ticks {1};
	bool InExceptionHandler {false};
	bool Stop {false};
	bool Yield {false};
	bool Sleeping {false};
	std::string Name;
	std::string Filename;
	ScriptType Type {ScriptType::SCRIPT};
};

class NativeFunction
{
public:
	NativeFunction(std::function<void()> impl, int32_t stackIn, int32_t stackOut, std::string name)
	    : Impl(impl)
	    , StackIn(stackIn)
	    , StackOut(stackOut)
	    , Name(name) {}

	std::function<void()> Impl;
	int32_t StackIn;
	uint32_t StackOut;
	std::string Name;
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

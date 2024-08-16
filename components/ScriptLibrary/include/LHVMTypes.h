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

namespace openblack::lhvm
{

enum class LHVMVersion : uint32_t
{
	BlackAndWhite = 7,
	CreatureIsle = 8,
	BlackAndWhite2 = 12
};

enum class DataType : uint32_t
{
	None = 0,
	Int,
	Float,
	Vector,
	Object,
	Unk5,
	Boolean,
	Unk7,
	_Count
};

static const std::array<std::string, static_cast<size_t>(DataType::_Count)> k_DataTypeNames = {
    "NONE", "INT", "FLOAT", "VECTOR", "OBJECT", "UNK2", "BOOLEAN", "UNK4"};

static const std::array<std::string, static_cast<size_t>(DataType::_Count)> k_DataTypeChars = {"",  "I", "F", "V",
                                                                                               "O", "",  "B", ""};

struct VMValue
{
	union
	{
		float floatVal;
		int32_t intVal;
		uint32_t uintVal;
	};

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
	static constexpr const size_t k_Size = 32;

	VMStack() = default;

	uint32_t count {0};
	std::array<VMValue, k_Size> values {};
	std::array<DataType, k_Size> types {};
	uint32_t pushCount {0};
	uint32_t popCount {0};
};
static_assert(sizeof(VMStack) == 268);

enum class ScriptType : uint32_t
{
	None = 0,
	Script = 1 << 0,
	Help = 1 << 1,
	ChallengeHelp = 1 << 2,
	TempleHelp = 1 << 3,
	TempleSpecial = 1 << 4,
	Unknown5 = 1 << 5,
	MultiplayerHelp = 1 << 6,

	All = 0xFFFFFFFF
};

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

static const std::map<ScriptType, std::string> k_ScriptTypeNames = {
	{ScriptType::Script, "script"},
	{ScriptType::Help, "help script"},
	{ScriptType::ChallengeHelp, "challenge help script"},
	{ScriptType::TempleHelp, "temple help script"},
	{ScriptType::TempleSpecial, "temple special script"},
	{ScriptType::MultiplayerHelp, "multiplayer help script"},
};

enum class Opcode : uint32_t
{
	End = 0,
	Wait,
	Push,
	Pop,
	Add,
	Sys,
	Sub,
	Neg,
	Mul,
	Div,
	Mod,
	Not,
	And,
	Or,
	Eq,
	Ne,
	Ge,
	Le,
	Gt,
	Lt,
	Jmp,
	Sleep,
	Except,
	Cast, // or ZERO
	Run,
	EndExcept, // or FREE
	RetExcept,
	FailExcept,
	BrkExcept,
	Swap,
	Line,
	_Count
};

static const std::array<std::string, static_cast<size_t>(Opcode::_Count)> k_OpcodeNames = {
    "END",    "JZ",   "PUSH", "POP",       "ADD",       "SYS",        "SUB",       "NEG",  "MUL", "DIV", "MOD",
    "NOT",    "AND",  "OR",   "EQ",        "NE",        "GE",         "LE",        "GT",   "LT",  "JMP", "SLEEP",
    "EXCEPT", "CAST", "RUN",  "ENDEXCEPT", "RETEXCEPT", "FAILEXCEPT", "BRKEXCEPT", "SWAP", "LINE"};

enum class VMMode : uint32_t
{
	// Data access modes
	Immediate = 0,
	Reference = 1,
	// Jump directions
	Backward = 0,
	Forward = 1,
	// Call modes
	Sync = 0,
	Async = 1,
	// CAST or ZERO
	Cast = 0,
	Zero = 1,
	// ENDEXCEPT or Yield
	EndExcept = 0,
	Yield = 1,
	// SWAP alternatives
	CopyTo = 0,
	CopyFrom = 1,
};

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

	Opcode code {Opcode::Line};
	VMMode mode {VMMode::Immediate};
	DataType type {DataType::Float};
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
	VMScript() = default;

	VMScript(std::string name, std::string filename, ScriptType type, uint32_t varOffset, std::vector<std::string> variables,
	         uint32_t instructionAddress, uint32_t parameterCount, uint32_t scriptId)
	    : name(std::move(name))
	    , filename(std::move(filename))
	    , type(type)
	    , variables(std::move(variables))
	    , variablesOffset(varOffset)
	    , instructionAddress(instructionAddress)
	    , parameterCount(parameterCount)
	    , scriptId(scriptId)
	{
	}

	~VMScript() = default;

	std::string name;
	std::string filename;
	ScriptType type = ScriptType::Script;
	std::vector<std::string> variables;

	uint32_t variablesOffset = 0;
	uint32_t instructionAddress = 0;
	uint32_t parameterCount = 0;
	uint32_t scriptId = 0;
};

class VMTask
{
public:
	VMTask() = default;

	VMTask(std::vector<VMVar> localVars, uint32_t scriptId, uint32_t id, uint32_t instructionAddress, uint32_t variablesOffset,
	       VMStack stack, std::string name, std::string filename, ScriptType type)
	    : localVars(std::move(localVars))
	    , scriptId(scriptId)
	    , id(id)
	    , instructionAddress(instructionAddress)
	    , variablesOffset(variablesOffset)
	    , stack(stack)
	    , name(std::move(name))
	    , filename(std::move(filename))
	    , type(type)
	{
	}

	std::vector<VMVar> localVars;
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
	ScriptType type {ScriptType::Script};
};

class NativeFunction
{
public:
	NativeFunction(std::function<void()> impl, int32_t stackIn, int32_t stackOut, std::string name)
	    : impl(std::move(impl))
	    , stackIn(stackIn)
	    , stackOut(stackOut)
	    , name(std::move(name))
	{
	}

	std::function<void()> impl;
	int32_t stackIn;
	uint32_t stackOut;
	std::string name;
};

enum class ErrorCode : uint8_t
{
	NoErr = 0,
	StackEmpty,
	StackFull,
	ScriptIdNotFound,
	ScriptNameNotFound,
	NoScriptOfType,
	TaskIdNotFound,
	NativeFuncNotFound,
	DivByZero,
	InvalidType,
	InvalidOperand,
	_Count
};

static const std::array<std::string, static_cast<size_t>(ErrorCode::_Count)> k_ErrorMsg = {
	"no error",
	"stack is empty",
	"stack is full",
	"script id not found",
	"script name not found",
	"no script of type",
	"task id not found",
	"native function not found",
	"division by zero",
	"invalid data type",
};

} // namespace openblack::lhvm

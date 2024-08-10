/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LHVM.h"

#include <cassert>
#include <cmath>
#include <cstring>

#include <fstream>
#include <stdexcept>

#include "LHVMFile.h"

namespace openblack::LHVM
{
// Adapted from https://stackoverflow.com/a/13059195/10604387
//          and https://stackoverflow.com/a/46069245/10604387
struct membuf: std::streambuf
{
	membuf(char const* base, size_t size)
	{
		char* p(const_cast<char*>(base));
		this->setg(p, p, p + size);
	}
	std::streampos seekoff(off_type off, std::ios_base::seekdir way, [[maybe_unused]] std::ios_base::openmode which) override
	{
		if (way == std::ios_base::cur)
		{
			gbump(static_cast<int>(off));
		}
		else if (way == std::ios_base::end)
		{
			setg(eback(), egptr() + off, egptr());
		}
		else if (way == std::ios_base::beg)
		{
			setg(eback(), eback() + off, egptr());
		}
		return gptr() - eback();
	}

	std::streampos seekpos([[maybe_unused]] pos_type pos, [[maybe_unused]] std::ios_base::openmode which) override
	{
		return seekoff(pos - static_cast<off_type>(0), std::ios_base::beg, which);
	}
};
struct imemstream: virtual membuf, std::istream
{
	imemstream(char const* base, size_t size)
	    : membuf(base, size)
	    , std::istream(dynamic_cast<std::streambuf*>(this))
	{
	}
};

LHVM::LHVM()
{
	_currentStack = &_mainStack;

	_opcodesImpl[0] = &LHVM::_opcode00End;
	_opcodesImpl[1] = &LHVM::_opcode01Jz;
	_opcodesImpl[2] = &LHVM::_opcode02Push;
	_opcodesImpl[3] = &LHVM::_opcode03Pop;
	_opcodesImpl[4] = &LHVM::_opcode04Add;
	_opcodesImpl[5] = &LHVM::_opcode05Sys;
	_opcodesImpl[6] = &LHVM::_opcode06Sub;
	_opcodesImpl[7] = &LHVM::_opcode07Neg;
	_opcodesImpl[8] = &LHVM::_opcode08Mul;
	_opcodesImpl[9] = &LHVM::_opcode09Div;

	_opcodesImpl[10] = &LHVM::_opcode10Mod;
	_opcodesImpl[11] = &LHVM::_opcode11Not;
	_opcodesImpl[12] = &LHVM::_opcode12And;
	_opcodesImpl[13] = &LHVM::_opcode13Or;
	_opcodesImpl[14] = &LHVM::_opcode14Eq;
	_opcodesImpl[15] = &LHVM::_opcode15Neq;
	_opcodesImpl[16] = &LHVM::_opcode16Geq;
	_opcodesImpl[17] = &LHVM::_opcode17Leq;
	_opcodesImpl[18] = &LHVM::_opcode18Gt;
	_opcodesImpl[19] = &LHVM::_opcode19Lt;
	_opcodesImpl[20] = &LHVM::_opcode20Jmp;
	_opcodesImpl[21] = &LHVM::_opcode21Sleep;
	_opcodesImpl[22] = &LHVM::_opcode22Except;
	_opcodesImpl[23] = &LHVM::_opcode23Cast;
	_opcodesImpl[24] = &LHVM::_opcode24Call;
	_opcodesImpl[25] = &LHVM::_opcode25EndExcept;
	_opcodesImpl[26] = &LHVM::_opcode26RetExcept;
	_opcodesImpl[27] = &LHVM::_opcode27IterExcept;
	_opcodesImpl[28] = &LHVM::_opcode28BrkExcept;
	_opcodesImpl[29] = &LHVM::_opcode29Swap;
	_opcodesImpl[30] = &LHVM::_opcode30Line;
}

/// Error handling
void LHVM::Fail(const std::string& msg)
{
	throw std::runtime_error("LHVM Error: " + msg);
}

void LHVM::Initialise(std::vector<NativeFunction>* functions, std::function<void(const uint32_t func)> nativeCallEnterCallback,
                      std::function<void(const uint32_t func)> nativeCallExitCallback,
                      std::function<void(const uint32_t taskNumber)> stopTaskCallback,
                      std::function<void(const ErrorCode code, const std::string v0, const uint32_t v1)> errorCallback,
                      std::function<void(const uint32_t objId)> addReference,
                      std::function<void(const uint32_t objId)> removeReference)
{
	_functions = functions;
	_nativeCallEnterCallback = nativeCallEnterCallback;
	_nativeCallExitCallback = nativeCallExitCallback;
	_stopTaskCallback = stopTaskCallback;
	_errorCallback = errorCallback;
	_addReference = addReference;
	_removeReference = removeReference;
	Reboot();
}

void LHVM::LoadBinary(const std::filesystem::path& filepath)
{
	LHVMFile file;
	file.Open(filepath);
	LoadBinary(file);
}

void LHVM::LoadBinary(const std::vector<uint8_t>& buffer)
{
	LHVMFile file;
	file.Open(buffer);
	LoadBinary(file);
}

void LHVM::LoadBinary(const LHVMFile& file)
{
	if (file.HasStatus())
	{
		Fail("File contains status data");
	}

	StopAllTasks();

	_instructions = file.GetInstructions();
	_scripts = file.GetScripts();
	_data = file.GetData();
	_mainStack.Count = 0;
	_mainStack.PushCount = 0;
	_mainStack.PopCount = 0;
	_currentStack = &_mainStack;

	_variablesNames = file.GetVariablesNames();
	_variables.clear();
	_variables.reserve(_variablesNames.size() + 1);
	_variables.emplace_back(DataType::FLOAT, VMValue(0.0f), "Null variable");
	for (auto& name : _variablesNames)
	{
		_variables.emplace_back(DataType::FLOAT, VMValue(0.0f), name);
	}

	_tasks.clear();
	_ticks = 0;
	_currentLineNumber = 0;
	_highestTaskId = 0;
	_highestScriptId = _scripts.size();
	_executedInstructions = 0;

	_auto = file.GetAutostart();
	for (const auto scriptId : _auto)
	{
		if (scriptId > 0 && scriptId <= _scripts.size())
		{
			auto& script = _scripts[scriptId - 1];
			StartScript(script.GetName(), ScriptType::ALL);
		}
		else
		{
			SignalError(ErrorCode::SCRIPT_ID_NOT_FOUND, scriptId);
		}
	}
}

void LHVM::RestoreState(const std::filesystem::path& filepath)
{
	auto file = LHVMFile();
	file.Open(filepath);
	if (!file.HasStatus())
	{
		Fail("File doesn't contain status data");
	}

	StopAllTasks();

	_instructions = file.GetInstructions();
	_scripts = file.GetScripts();
	_data = file.GetData();
	_mainStack = file.GetStack();
	_currentStack = &_mainStack;
	_variablesNames = file.GetVariablesNames();
	_variables = file.GetVariablesValues();

	_auto = file.GetAutostart();

	_tasks.clear();
	for (const auto& task : file.GetTasks())
	{
		_tasks.emplace(task.Id, task);
	}

	_ticks = file.GetTicks();
	_currentLineNumber = file.GetCurrentLineNumber();
	_highestTaskId = file.GetHighestTaskId();
	_highestScriptId = file.GetHighestScriptId();
	_executedInstructions = file.GetExecutedInstructions();
}

VMValue LHVM::Pop(DataType& type)
{
	_currentStack->PopCount++;
	if (_currentStack->Count > 0)
	{
		_currentStack->Count--;
		type = _currentStack->Types.at(_currentStack->Count);
		return _currentStack->Values.at(_currentStack->Count);
	}
	else
	{
		SignalError(ErrorCode::STACK_EMPTY);
	}
	return VMValue(0u);
}

VMValue LHVM::Pop()
{
	DataType tmp;
	return Pop(tmp);
}

float LHVM::Popf()
{
	DataType tmp;
	return Pop(tmp).FloatVal;
}

void LHVM::Push(VMValue value, DataType type)
{
	_currentStack->PushCount++;
	if (_currentStack->Count < 32)
	{
		_currentStack->Values.at(_currentStack->Count) = value;
		_currentStack->Types.at(_currentStack->Count) = type;
		_currentStack->Count++;
	}
	else
	{
		SignalError(ErrorCode::STACK_FULL);
	}
}

void LHVM::Pushf(float value)
{
	Push(VMValue(value), DataType::FLOAT);
}

void LHVM::Pushv(float value)
{
	Push(VMValue(value), DataType::VECTOR);
}

void LHVM::Pushi(int32_t value)
{
	Push(VMValue(value), DataType::INT);
}

void LHVM::Pusho(uint32_t value)
{
	Push(VMValue(value), DataType::OBJECT);
}

void LHVM::Pushb(bool value)
{
	Push(VMValue(value ? 1 : 0), DataType::BOOLEAN);
}

void LHVM::Reboot()
{
	StopAllTasks();
	_variables.clear();
	_variablesNames.clear();
	_scripts.clear();
	_auto.clear();
	_instructions.clear();
	_data.clear();

	_ticks = 0;
	_highestTaskId = 0;
	_highestScriptId = 0;
	_currentLineNumber = 0;
	_executedInstructions = 0;

	_mainStack.PopCount += _mainStack.Count;
	_mainStack.Count = 0;

	_currentTask = NULL;
	_currentStack = &_mainStack;
}

void LHVM::SaveBinary(const std::filesystem::path& filepath)
{
	LHVMFile file(LHVMHeader {.magic = k_Magic, .version = LHVMVersion::BlackAndWhite}, _variablesNames, _instructions, _auto,
	              _scripts, _data);
	file.Write(filepath);
}

void LHVM::SaveState(const std::filesystem::path& filepath)
{
	std::vector<VMTask> tasks;
	tasks.reserve(_tasks.size());
	for (const auto& [id, task] : _tasks)
	{
		tasks.emplace_back(task);
	}

	LHVMFile file(LHVMHeader {.magic = k_Magic, .version = LHVMVersion::BlackAndWhite}, _variablesNames, _instructions, _auto,
	              _scripts, _data, _mainStack, _variables, tasks, _ticks, _currentLineNumber, _highestTaskId, _highestScriptId,
	              _executedInstructions);
	file.Write(filepath);
}

void LHVM::LookIn(const ScriptType allowedScriptTypesMask)
{
	// execute exception handlers first
	for (auto& [id, task] : _tasks)
	{
		if (task.Type & allowedScriptTypesMask)
		{
			_currentStack = &task.Stack;
			if (task.InExceptionHandler)
			{
				CpuLoop(task);
			}
			else if (task.WaitingTaskId == 0)
			{
				task.CurrentExceptionHandlerIndex = 0;
				if (GetExceptionHandlersCount() > 0)
				{
					task.PrevInstructionAddress = task.InstructionAddress;
					task.InstructionAddress = GetCurrentExceptionHandlerIp(task.CurrentExceptionHandlerIndex);
					task.InExceptionHandler = true;
					CpuLoop(task);
				}
			}
		}
	}

	// execute normal code
	for (auto& [id, task] : _tasks)
	{
		if (task.Type & allowedScriptTypesMask)
		{
			_currentStack = &task.Stack;
			if (!task.InExceptionHandler)
			{
				CpuLoop(task);
			}
		}
	}

	// handle tasks termination
	for (auto iter = _tasks.begin(); iter != _tasks.end();)
	{
		auto& task = (*iter++).second;
		if (task.Stop)
		{
			StopTask(task.Id);
		}
	}

	// unlock waiting tasks
	for (auto& [id, task] : _tasks)
	{
		if (task.Type & allowedScriptTypesMask)
		{
			task.Ticks++;
			if (task.WaitingTaskId != 0 && !TaskExists(task.WaitingTaskId))
			{
				task.WaitingTaskId = 0;
				task.InstructionAddress++;
			}
		}
	}

	_ticks++;
	_currentStack = &_mainStack;
}

uint32_t LHVM::StartScript(const std::string& name, const ScriptType allowedScriptTypesMask)
{
	const auto script = GetScript(name);
	if (script != NULL)
	{
		if (script->GetType() & allowedScriptTypesMask)
		{
			return StartScript(*script);
		}
		else
		{
			SignalError(ErrorCode::NO_SCRIPT_OF_TYPE, name);
		}
	}
	else
	{
		SignalError(ErrorCode::SCRIPT_NAME_NOT_FOUND, name);
	}
	return 0;
}

uint32_t LHVM::StartScript(const uint32_t id)
{
	if (id > 0 && id <= _scripts.size())
	{
		return StartScript(_scripts.at(id - 1));
	}
	else
	{
		SignalError(ErrorCode::SCRIPT_ID_NOT_FOUND, id);
	}
	return 0;
}

uint32_t LHVM::StartScript(const VMScript& script)
{
	const auto taskNumber = ++_highestTaskId;

	// copy values from current stack to new stack
	VMStack stack {};
	for (unsigned int i = 0; i < script.GetParameterCount(); i++)
	{
		DataType type;
		const auto& value = Pop(type);

		stack.PushCount++;
		if (stack.Count < 31)
		{
			stack.Values.at(stack.Count) = value;
			stack.Types.at(stack.Count) = type;
			stack.Count++;
		}
	}

	// allocate local variables with default values
	const auto& scriptVariables = script.GetVariables();
	std::vector<VMVar> taskVariables;
	taskVariables.reserve(scriptVariables.size());
	for (unsigned int i = 0; i < scriptVariables.size(); i++)
	{
		taskVariables.emplace_back(DataType::FLOAT, VMValue(0.0f), scriptVariables.at(i));
	}

	const auto& task = VMTask(taskVariables, script.GetScriptID(), taskNumber, script.GetInstructionAddress(),
	                          script.GetVariablesOffset(), stack, script.GetName(), script.GetFileName(), script.GetType());

	_tasks.emplace(taskNumber, task);

	return taskNumber;
}

void LHVM::StopAllTasks()
{
	while (_tasks.size() > 0)
	{
		auto iter = _tasks.begin();
		StopTask((*iter).first);
	}
}

void LHVM::StopScripts(std::function<bool(const std::string& name, const std::string& filename)> filter)
{
	std::vector<uint32_t> ids;
	for (const auto& [id, task] : _tasks)
	{
		if (filter(task.Name, task.Filename))
		{
			ids.emplace_back(id);
		}
	}

	for (const auto id : ids)
	{
		StopTask(id);
	}
}

void LHVM::StopTask(uint32_t taskNumber)
{
	if (TaskExists(taskNumber))
	{
		InvokeStopTaskCallback(taskNumber);
		auto& task = _tasks.at(taskNumber);
		for (auto& var : task.LocalVars)
		{
			if (var.Type == DataType::OBJECT)
			{
				RemoveReference(var.UintVal);
			}
		}

		if (_currentTask == &task)
		{
			_currentStack = &_mainStack;
		}

		_tasks.erase(taskNumber);
	}
	else
	{
		SignalError(ErrorCode::TASK_ID_NOT_FOUND, taskNumber);
	}
}

void LHVM::StopTasksOfType(const ScriptType typesMask)
{
	std::vector<uint32_t> ids;
	for (const auto& [id, task] : _tasks)
	{
		if (task.Type & typesMask)
		{
			ids.emplace_back(task.Id);
		}
	}

	for (const auto id : ids)
	{
		StopTask(id);
	}
}

const std::string LHVM::GetString(uint32_t offset)
{
	if (offset < _data.size())
	{
		return std::string(_data.data() + offset);
	}
	else
	{
		return "";
	}
}

void LHVM::InvokeStopTaskCallback(const uint32_t taskNumber)
{
	if (_stopTaskCallback != nullptr)
	{
		_stopTaskCallback(taskNumber);
	}
}

void LHVM::AddReference(const uint32_t objectId)
{
	if (_addReference != nullptr)
	{
		_addReference(objectId);
	}
}

void LHVM::RemoveReference(const uint32_t objectId)
{
	if (_removeReference != nullptr)
	{
		_removeReference(objectId);
	}
}

void LHVM::InvokeNativeCallEnterCallback(const uint32_t funcId)
{
	if (_nativeCallEnterCallback != nullptr)
	{
		_nativeCallEnterCallback(funcId);
	}
}

void LHVM::InvokeNativeCallExitCallback(const uint32_t funcId)
{
	if (_nativeCallExitCallback != nullptr)
	{
		_nativeCallExitCallback(funcId);
	}
}

void LHVM::SignalError(const ErrorCode code)
{
	if (_errorCallback != nullptr)
	{
		_errorCallback(code, "", 0);
	}
}

void LHVM::SignalError(const ErrorCode code, const uint32_t data)
{
	if (_errorCallback != nullptr)
	{
		_errorCallback(code, "", data);
	}
}

void LHVM::SignalError(const ErrorCode code, const std::string& data)
{
	if (_errorCallback != nullptr)
	{
		_errorCallback(code, data, 0);
	}
}

const VMScript* LHVM::GetScript(const std::string& name)
{
	for (const auto& script : _scripts)
	{
		if (script.GetName() == name)
		{
			return &script;
		}
	}
	return NULL;
}

bool LHVM::TaskExists(const uint32_t taskId)
{
	return _tasks.contains(taskId);
}

uint32_t LHVM::GetTicksCount()
{
	return _ticks;
}

void LHVM::PushElaspedTime()
{
	float time = GetTicksCount() * 10.0f;
	Pushf(time);
}

VMVar& LHVM::GetVar(VMTask& task, const uint32_t id)
{
	const auto offset = task.VariablesOffset;
	return (id > offset) ? task.LocalVars.at(id - offset - 1) : _variables.at(id);
}

uint32_t LHVM::GetExceptionHandlersCount()
{
	if (_currentTask != NULL)
	{
		return _currentTask->ExceptionHandlerIps.size();
	}
	else
	{
		return 0;
	}
}

uint32_t LHVM::GetCurrentExceptionHandlerIp(const uint32_t index)
{
	if (_currentTask != NULL)
	{
		return _currentTask->ExceptionHandlerIps.at(_currentTask->ExceptionHandlerIps.size() - index - 1);
	}
	else
	{
		return 0;
	}
}

void LHVM::PrintInstruction(const VMTask& task, const VMInstruction& instruction)
{
	// TODO: improve this
	std::string opcode = k_OpcodeNames[static_cast<int>(instruction.Code)];
	std::string arg = "";
	if (instruction.Code == Opcode::RUN)
	{
		if (instruction.Mode == VMMode::ASYNC)
		{
			arg = "async ";
		}
		arg += _scripts.at(instruction.IntVal - 1).GetName();
	}
	else if (instruction.Code == Opcode::CALL)
	{
		arg += _functions->at(instruction.IntVal).Name;
	}
	else
	{
		if (instruction.Code == Opcode::PUSH || instruction.Code == Opcode::POP || instruction.Code == Opcode::CAST ||
		    instruction.Code == Opcode::ADD || instruction.Code == Opcode::MINUS || instruction.Code == Opcode::TIMES ||
		    instruction.Code == Opcode::DIVIDE || instruction.Code == Opcode::MODULUS)
		{
			opcode += k_DataTypeChars[static_cast<int>(instruction.Type)];
		}
		if ((instruction.Code == Opcode::PUSH || instruction.Code == Opcode::POP) && instruction.Mode == VMMode::REFERENCE)
		{
			if (instruction.IntVal > task.VariablesOffset)
			{
				arg = task.LocalVars[instruction.IntVal - task.VariablesOffset - 1].Name;
			}
			else
			{
				arg = _variables.at(instruction.IntVal).Name;
			}
		}
		else if (instruction.Code == Opcode::PUSH && instruction.Mode == VMMode::IMMEDIATE)
		{
			if (instruction.Type == DataType::FLOAT || instruction.Type == DataType::VECTOR)
			{
				arg = std::to_string(instruction.FloatVal);
			}
			else
			{
				arg = std::to_string(instruction.IntVal);
			}
		}
		else if (instruction.Code == Opcode::JUMP || instruction.Code == Opcode::WAIT || instruction.Code == Opcode::SWAP)
		{
			arg = std::to_string(instruction.IntVal);
		}
		if (instruction.Code == Opcode::WAIT)
		{
			bool val = task.Stack.Values.at(task.Stack.Count - 1).IntVal;
			arg += val ? " [true] -> continue" : " [false] -> JUMP";
		}
	}
	printf("%s:%d %s[%d] %s %s\n", task.Filename.c_str(), instruction.Line, task.Name.c_str(), task.Id, opcode.c_str(),
	       arg.c_str());
}

void LHVM::CpuLoop(VMTask& task)
{
	const auto wasExceptionHandler = task.InExceptionHandler;
	task.Yield = false;
	while (task.WaitingTaskId == 0)
	{
		_currentTask = &task;
		_executedInstructions++;
		const auto& instruction = _instructions.at(task.InstructionAddress);

		// PrintInstruction(task, instruction); // just for debug purposes

		(this->*_opcodesImpl.at(static_cast<int>(instruction.Code)))(task, instruction);

		if (task.Stop || task.Yield || task.WaitingTaskId != 0 || task.InExceptionHandler != wasExceptionHandler)
		{
			break;
		}
		task.InstructionAddress++;
	}
	_currentTask = NULL;
}

float LHVM::Fmod(float a, float b)
{
	return a - b * static_cast<int64_t>(a / b);
}

void LHVM::_opcode00End(VMTask& task, const VMInstruction& instruction)
{
	task.Stop = true;
}

void LHVM::_opcode01Jz(VMTask& task, const VMInstruction& instruction)
{
	const auto val = Pop().IntVal;
	if (val)
	{
		task.Ticks = 1;
	}
	else // jump if popped value isn't zero
	{
		if (instruction.Mode == VMMode::FORWARD)
		{
			task.InstructionAddress = instruction.IntVal - 1;
		}
		else // Mode::BACKWARD
		{
			task.InstructionAddress = instruction.IntVal;
			task.Yield = true;
		}
	}
}

void LHVM::_opcode02Push(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.Mode == VMMode::IMMEDIATE)
	{
		Push(instruction.Data, instruction.Type);
	}
	else // Mode::REFERENCE
	{
		const auto& var = GetVar(task, instruction.UintVal);
		Push(var.Value, var.Type);
	}
}

void LHVM::_opcode03Pop(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.Mode == VMMode::REFERENCE)
	{
		auto& var = GetVar(task, instruction.UintVal);
		DataType type;
		const auto newVal = Pop(type);
		if (type == DataType::OBJECT)
		{
			AddReference(newVal.UintVal);
		}
		if (var.Type == DataType::OBJECT)
		{
			RemoveReference(newVal.UintVal);
		}
		var.Value = newVal;
		var.Type = type;
	}
	else // Mode::IMMEDIATE
	{
		Pop(); // cannot POP to immediate value, just discard the value
	}
}

void LHVM::_opcode04Add(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.Type)
	{
	case DataType::INT:
		a0 = Pop();
		b0 = Pop();
		Pushi(a0.IntVal + b0.IntVal);
		break;
	case DataType::FLOAT:
		a0 = Pop();
		b0 = Pop();
		Pushf(a0.FloatVal + b0.FloatVal);
		break;
	case DataType::VECTOR:
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		b0 = Pop();
		b1 = Pop();
		b2 = Pop();
		Pushv(a2.FloatVal + b2.FloatVal);
		Pushv(a1.FloatVal + b1.FloatVal);
		Pushv(a0.FloatVal + b0.FloatVal);
		break;
	default:
		Pop();
		Pop();
		Pushf(0.0f);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::_opcode05Sys(VMTask& task, const VMInstruction& instruction)
{
	const auto id = instruction.IntVal;
	if (id > 0 && id < _functions->size())
	{
		const auto& func = _functions->at(id);
		if (func.Impl != nullptr)
		{
			_currentStack->PushCount = 0;
			_currentStack->PopCount = 0;
			InvokeNativeCallEnterCallback(id);
			func.Impl();
			InvokeNativeCallExitCallback(id);
		}
		else // if impl not provided, then just adjust the stack
		{
			for (int i = 0; i < func.StackIn; i++)
			{
				Pop();
			}
			for (unsigned int i = 0; i < func.StackOut; i++)
			{
				Pushf(0.0f);
			}
		}
	}
	else
	{
		SignalError(ErrorCode::NATIVE_FUNC_NOT_FOUND, id);
	}
}

void LHVM::_opcode06Sub(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.Type)
	{
	case DataType::INT:
		a0 = Pop();
		b0 = Pop();
		Pushi(b0.IntVal - a0.IntVal);
		break;
	case DataType::FLOAT:
		a0 = Pop();
		b0 = Pop();
		Pushf(b0.FloatVal - a0.FloatVal);
		break;
	case DataType::VECTOR:
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		b0 = Pop();
		b1 = Pop();
		b2 = Pop();
		Pushv(b2.FloatVal - a2.FloatVal);
		Pushv(b1.FloatVal - a1.FloatVal);
		Pushv(b0.FloatVal - a0.FloatVal);
		break;
	default:
		Pop();
		Pop();
		Pushf(0.0f);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::_opcode07Neg(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2;
	switch (instruction.Type)
	{
	case DataType::INT:
		a0 = Pop();
		Pushi(-a0.IntVal);
		break;
	case DataType::FLOAT:
		a0 = Pop();
		Pushf(-a0.FloatVal);
		break;
	case DataType::VECTOR:
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		Pushv(-a2.FloatVal);
		Pushv(-a1.FloatVal);
		Pushv(-a0.FloatVal);
		break;
	default:
		Pop();
		Pushf(0.0f);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::_opcode08Mul(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0;
	switch (instruction.Type)
	{
	case DataType::INT:
		a0 = Pop();
		b0 = Pop();
		Pushi(a0.IntVal * b0.IntVal);
		break;
	case DataType::FLOAT:
		a0 = Pop();
		b0 = Pop();
		Pushf(a0.FloatVal * b0.FloatVal);
		break;
	case DataType::VECTOR:
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		b0 = Pop();
		Pushv(a2.FloatVal * b0.FloatVal);
		Pushv(a1.FloatVal * b0.FloatVal);
		Pushv(a0.FloatVal * b0.FloatVal);
		break;
	default:
		Pop();
		Pop();
		Pushf(0.0f);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::_opcode09Div(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, b0, b1, b2;
	switch (instruction.Type)
	{
	case DataType::INT:
		a0 = Pop();
		b0 = Pop();
		if (a0.IntVal != 0)
		{
			Pushi(b0.IntVal / a0.IntVal);
		}
		else
		{
			Pushi(0);
			SignalError(ErrorCode::DIV_BY_ZERO);
		}
		break;
	case DataType::FLOAT:
		a0 = Pop();
		b0 = Pop();
		if (a0.FloatVal != 0.0f)
		{
			Pushf(b0.FloatVal / a0.FloatVal);
		}
		else
		{
			Pushf(0.0f);
			SignalError(ErrorCode::DIV_BY_ZERO);
		}
		break;
	case DataType::VECTOR:
		a0 = Pop();
		b0 = Pop();
		b1 = Pop();
		b2 = Pop();
		if (a0.FloatVal != 0.0f)
		{
			Pushv(b2.FloatVal / a0.FloatVal);
			Pushv(b1.FloatVal / a0.FloatVal);
			Pushv(b0.FloatVal / a0.FloatVal);
		}
		else
		{
			Pushv(0.0f);
			Pushv(0.0f);
			Pushv(0.0f);
			SignalError(ErrorCode::DIV_BY_ZERO);
		}
		break;
	default:
		Pop();
		Pop();
		Pushf(0.0f);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::_opcode10Mod(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, b0, b1, b2;
	switch (instruction.Type)
	{
	case DataType::INT:
		a0 = Pop();
		b0 = Pop();
		if (a0.IntVal != 0)
		{
			Pushi(b0.IntVal % a0.IntVal);
		}
		else
		{
			Pushi(0);
			SignalError(ErrorCode::DIV_BY_ZERO);
		}
		break;
	case DataType::FLOAT:
		a0 = Pop();
		b0 = Pop();
		if (a0.FloatVal != 0.0f)
		{
			Pushf(Fmod(b0.FloatVal, a0.FloatVal));
		}
		else
		{
			Pushf(0.0f);
			SignalError(ErrorCode::DIV_BY_ZERO);
		}
		break;
	case DataType::VECTOR:
		a0 = Pop();
		b0 = Pop();
		b1 = Pop();
		b2 = Pop();
		if (a0.FloatVal != 0.0f)
		{
			Pushv(Fmod(b2.FloatVal, a0.FloatVal));
			Pushv(Fmod(b1.FloatVal, a0.FloatVal));
			Pushv(Fmod(b0.FloatVal, a0.FloatVal));
		}
		else
		{
			Pushv(0.0f);
			Pushv(0.0f);
			Pushv(0.0f);
			SignalError(ErrorCode::DIV_BY_ZERO);
		}
		break;
	default:
		Pop();
		Pop();
		Pushf(0.0f);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::_opcode11Not(VMTask& task, const VMInstruction& instruction)
{
	bool a = Pop().IntVal;
	Pushb(!a);
}

void LHVM::_opcode12And(VMTask& task, const VMInstruction& instruction)
{
	bool b = Pop().IntVal;
	bool a = Pop().IntVal;
	Pushb(a && b);
}

void LHVM::_opcode13Or(VMTask& task, const VMInstruction& instruction)
{
	bool b = Pop().IntVal;
	bool a = Pop().IntVal;
	Pushb(a || b);
}

void LHVM::_opcode14Eq(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.Type)
	{
	case DataType::INT:
	case DataType::BOOLEAN:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.IntVal == b0.IntVal);
		break;
	case DataType::FLOAT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.FloatVal == b0.FloatVal);
		break;
	case DataType::VECTOR:
		b0 = Pop();
		b1 = Pop();
		b2 = Pop();
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		Pushb(a0.FloatVal == b0.FloatVal && a1.FloatVal == b1.FloatVal && a2.FloatVal == b2.FloatVal);
		break;
	case DataType::OBJECT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.UintVal == b0.UintVal);
		break;
	default:
		Pop();
		Pop();
		Pushb(false);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::_opcode15Neq(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.Type)
	{
	case DataType::INT:
	case DataType::BOOLEAN:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.IntVal != b0.IntVal);
		break;
	case DataType::FLOAT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.FloatVal != b0.FloatVal);
		break;
	case DataType::VECTOR:
		b0 = Pop();
		b1 = Pop();
		b2 = Pop();
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		Pushb(a0.FloatVal != b0.FloatVal || a1.FloatVal != b1.FloatVal || a2.FloatVal != b2.FloatVal);
		break;
	case DataType::OBJECT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.UintVal != b0.UintVal);
		break;
	default:
		Pop();
		Pop();
		Pushb(false);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::_opcode16Geq(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.Type)
	{
	case DataType::INT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.IntVal >= b0.IntVal);
		break;
	case DataType::FLOAT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.FloatVal >= b0.FloatVal);
		break;
	default:
		Pushb(false);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::_opcode17Leq(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.Type)
	{
	case DataType::INT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.IntVal <= b0.IntVal);
		break;
	case DataType::FLOAT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.FloatVal <= b0.FloatVal);
		break;
	default:
		Pushb(false);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::_opcode18Gt(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.Type)
	{
	case DataType::INT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.IntVal > b0.IntVal);
		break;
	case DataType::FLOAT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.FloatVal > b0.FloatVal);
		break;
	default:
		Pushb(false);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::_opcode19Lt(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.Type)
	{
	case DataType::INT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.IntVal < b0.IntVal);
		break;
	case DataType::FLOAT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.FloatVal < b0.FloatVal);
		break;
	default:
		Pushb(false);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::_opcode20Jmp(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.Mode == VMMode::FORWARD)
	{
		task.InstructionAddress = instruction.IntVal - 1;
	}
	else // Mode::BACKWARD
	{
		task.InstructionAddress = instruction.IntVal;
		task.Yield = true;
	}
}

void LHVM::_opcode21Sleep(VMTask& task, const VMInstruction& instruction)
{
	const auto seconds = Pop().FloatVal;
	if (static_cast<uint32_t>(seconds * 10.0f) < task.Ticks)
	{
		task.Sleeping = false;
		Pushb(true);
	}
	else
	{
		task.Sleeping = true;
		Pushb(false);
	}
}

void LHVM::_opcode22Except(VMTask& task, const VMInstruction& instruction)
{
	if (_currentTask != NULL)
	{
		_currentTask->ExceptionHandlerIps.emplace_back(instruction.UintVal);
	}
}

void LHVM::_opcode23Cast(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.Mode == VMMode::ZERO)
	{
		auto& var = GetVar(task, instruction.UintVal);
		if (var.Type == DataType::OBJECT)
		{
			RemoveReference(var.UintVal);
		}
		var.FloatVal = 0.0f;
		var.Type = DataType::FLOAT;
	}
	else // Mode::CAST
	{
		Push(Pop(), instruction.Type);
	}
}

void LHVM::_opcode24Call(VMTask& task, const VMInstruction& instruction)
{
	const auto newTaskId = StartScript(instruction.UintVal);
	if (instruction.Mode == VMMode::SYNC)
	{
		task.WaitingTaskId = newTaskId;
	}
}

void LHVM::_opcode25EndExcept(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.Mode == VMMode::ENDEXCEPT)
	{
		if (_currentTask != NULL && _currentTask->ExceptionHandlerIps.size() > 0)
		{
			_currentTask->ExceptionHandlerIps.pop_back();
		}
	}
	else // Mode::YIELD
	{
		task.Yield = true;
		task.InstructionAddress++;
	}
}

void LHVM::_opcode26RetExcept(VMTask& task, const VMInstruction& instruction)
{
	task.InstructionAddress = task.PrevInstructionAddress;
	task.PrevInstructionAddress = 0;
	task.InExceptionHandler = false;
}

void LHVM::_opcode27IterExcept(VMTask& task, const VMInstruction& instruction)
{
	task.CurrentExceptionHandlerIndex++;
	if (task.CurrentExceptionHandlerIndex < task.ExceptionHandlerIps.size())
	{
		task.InstructionAddress = GetCurrentExceptionHandlerIp(task.CurrentExceptionHandlerIndex) - 1;
	}
	else
	{
		task.InstructionAddress = task.PrevInstructionAddress;
		task.PrevInstructionAddress = 0;
		task.InExceptionHandler = false;
	}
}

void LHVM::_opcode28BrkExcept(VMTask& task, const VMInstruction& instruction)
{
	if (_currentTask != NULL)
	{
		_currentTask->ExceptionHandlerIps.clear();
	}

	task.PrevInstructionAddress = 0;
	task.InstructionAddress++;
	task.InExceptionHandler = false;
}

void LHVM::_opcode29Swap(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.Type == DataType::INT) // swap the 2 topmost values on the stack
	{
		DataType t0, t1;
		VMValue v0 = Pop(t0);
		VMValue v1 = Pop(t1);
		Push(v0, t0);
		Push(v1, t1);
	}
	else
	{
		const auto offset = instruction.IntVal;
		DataType copyType = DataType::FLOAT;
		VMValue copyVal(0.0f);
		std::array<DataType, 32> tmpTypes;
		std::array<VMValue, 32> tmpVals;
		if (instruction.Mode == VMMode::COPYFROM) // push a copy of the Nth value from top of the stack
		{
			for (int i = 0; i < offset; i++)
			{
				DataType ti;
				VMValue vi = Pop(ti);
				if (i == offset - 1)
				{
					copyType = ti;
					copyVal = vi;
				}
				if (i < tmpTypes.size())
				{
					tmpTypes[i] = ti;
					tmpVals[i] = vi;
				}
			}
			for (int i = std::min(offset, 32) - 1; i >= 0; i--)
			{
				Push(tmpVals[i], tmpTypes[i]);
			}
			Push(copyVal, copyType);
		}
		else // Mode::COPYTO insert a copy of the topmost value on the stack N places below
		{
			for (int i = 0; i < offset; i++)
			{
				DataType ti;
				VMValue vi = Pop(ti);
				if (i == 0)
				{
					copyType = ti;
					copyVal = vi;
				}
				if (i < tmpTypes.size())
				{
					tmpTypes[i] = ti;
					tmpVals[i] = vi;
				}
			}
			Push(copyVal, copyType);
			for (int i = std::min(offset, 32) - 1; i >= 0; i--)
			{
				Push(tmpVals[i], tmpTypes[i]);
			}
		}
	}
}

void LHVM::_opcode30Line(VMTask& task, const VMInstruction& instruction) {}

} // namespace openblack::LHVM

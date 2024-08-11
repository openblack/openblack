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

	_opcodesImpl[0] = &LHVM::Opcode00End;
	_opcodesImpl[1] = &LHVM::Opcode01Jz;
	_opcodesImpl[2] = &LHVM::Opcode02Push;
	_opcodesImpl[3] = &LHVM::Opcode03Pop;
	_opcodesImpl[4] = &LHVM::Opcode04Add;
	_opcodesImpl[5] = &LHVM::Opcode05Sys;
	_opcodesImpl[6] = &LHVM::Opcode06Sub;
	_opcodesImpl[7] = &LHVM::Opcode07Neg;
	_opcodesImpl[8] = &LHVM::Opcode08Mul;
	_opcodesImpl[9] = &LHVM::Opcode09Div;

	_opcodesImpl[10] = &LHVM::Opcode10Mod;
	_opcodesImpl[11] = &LHVM::Opcode11Not;
	_opcodesImpl[12] = &LHVM::Opcode12And;
	_opcodesImpl[13] = &LHVM::Opcode13Or;
	_opcodesImpl[14] = &LHVM::Opcode14Eq;
	_opcodesImpl[15] = &LHVM::Opcode15Neq;
	_opcodesImpl[16] = &LHVM::Opcode16Geq;
	_opcodesImpl[17] = &LHVM::Opcode17Leq;
	_opcodesImpl[18] = &LHVM::Opcode18Gt;
	_opcodesImpl[19] = &LHVM::Opcode19Lt;
	_opcodesImpl[20] = &LHVM::Opcode20Jmp;
	_opcodesImpl[21] = &LHVM::Opcode21Sleep;
	_opcodesImpl[22] = &LHVM::Opcode22Except;
	_opcodesImpl[23] = &LHVM::Opcode23Cast;
	_opcodesImpl[24] = &LHVM::Opcode24Call;
	_opcodesImpl[25] = &LHVM::Opcode25EndExcept;
	_opcodesImpl[26] = &LHVM::Opcode26RetExcept;
	_opcodesImpl[27] = &LHVM::Opcode27IterExcept;
	_opcodesImpl[28] = &LHVM::Opcode28BrkExcept;
	_opcodesImpl[29] = &LHVM::Opcode29Swap;
	_opcodesImpl[30] = &LHVM::Opcode30Line;
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
	return Pop(tmp).floatVal;
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
		if (task.type & allowedScriptTypesMask)
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
		if (task.type & allowedScriptTypesMask)
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
		if (task.type & allowedScriptTypesMask)
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
			if (var.type == DataType::OBJECT)
			{
				RemoveReference(var.uintVal);
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
		if (task.type & typesMask)
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
		arg += _scripts.at(instruction.intVal - 1).GetName();
	}
	else if (instruction.Code == Opcode::CALL)
	{
		arg += _functions->at(instruction.intVal).Name;
	}
	else
	{
		if (instruction.Code == Opcode::PUSH || instruction.Code == Opcode::POP || instruction.Code == Opcode::CAST ||
		    instruction.Code == Opcode::ADD || instruction.Code == Opcode::MINUS || instruction.Code == Opcode::TIMES ||
		    instruction.Code == Opcode::DIVIDE || instruction.Code == Opcode::MODULUS)
		{
			opcode += k_DataTypeChars[static_cast<int>(instruction.type)];
		}
		if ((instruction.Code == Opcode::PUSH || instruction.Code == Opcode::POP) && instruction.Mode == VMMode::REFERENCE)
		{
			if (instruction.intVal > task.VariablesOffset)
			{
				arg = task.LocalVars[instruction.intVal - task.VariablesOffset - 1].Name;
			}
			else
			{
				arg = _variables.at(instruction.intVal).Name;
			}
		}
		else if (instruction.Code == Opcode::PUSH && instruction.Mode == VMMode::IMMEDIATE)
		{
			if (instruction.type == DataType::FLOAT || instruction.type == DataType::VECTOR)
			{
				arg = std::to_string(instruction.floatVal);
			}
			else
			{
				arg = std::to_string(instruction.intVal);
			}
		}
		else if (instruction.Code == Opcode::JUMP || instruction.Code == Opcode::WAIT || instruction.Code == Opcode::SWAP)
		{
			arg = std::to_string(instruction.intVal);
		}
		if (instruction.Code == Opcode::WAIT)
		{
			bool val = task.Stack.Values.at(task.Stack.Count - 1).intVal;
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

void LHVM::Opcode00End(VMTask& task, const VMInstruction& instruction)
{
	task.Stop = true;
}

void LHVM::Opcode01Jz(VMTask& task, const VMInstruction& instruction)
{
	const auto val = Pop().intVal;
	if (val)
	{
		task.Ticks = 1;
	}
	else // jump if popped value isn't zero
	{
		if (instruction.Mode == VMMode::FORWARD)
		{
			task.InstructionAddress = instruction.intVal - 1;
		}
		else // Mode::BACKWARD
		{
			task.InstructionAddress = instruction.intVal;
			task.Yield = true;
		}
	}
}

void LHVM::Opcode02Push(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.Mode == VMMode::IMMEDIATE)
	{
		Push(instruction.Data, instruction.type);
	}
	else // Mode::REFERENCE
	{
		const auto& var = GetVar(task, instruction.uintVal);
		Push(var.value, var.type);
	}
}

void LHVM::Opcode03Pop(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.Mode == VMMode::REFERENCE)
	{
		auto& var = GetVar(task, instruction.uintVal);
		DataType type;
		const auto newVal = Pop(type);
		if (type == DataType::OBJECT)
		{
			AddReference(newVal.uintVal);
		}
		if (var.type == DataType::OBJECT)
		{
			RemoveReference(newVal.uintVal);
		}
		var.value = newVal;
		var.type = type;
	}
	else // Mode::IMMEDIATE
	{
		Pop(); // cannot POP to immediate value, just discard the value
	}
}

void LHVM::Opcode04Add(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.type)
	{
	case DataType::INT:
		a0 = Pop();
		b0 = Pop();
		Pushi(a0.intVal + b0.intVal);
		break;
	case DataType::FLOAT:
		a0 = Pop();
		b0 = Pop();
		Pushf(a0.floatVal + b0.floatVal);
		break;
	case DataType::VECTOR:
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		b0 = Pop();
		b1 = Pop();
		b2 = Pop();
		Pushv(a2.floatVal + b2.floatVal);
		Pushv(a1.floatVal + b1.floatVal);
		Pushv(a0.floatVal + b0.floatVal);
		break;
	default:
		Pop();
		Pop();
		Pushf(0.0f);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::Opcode05Sys(VMTask& task, const VMInstruction& instruction)
{
	const auto id = instruction.intVal;
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

void LHVM::Opcode06Sub(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.type)
	{
	case DataType::INT:
		a0 = Pop();
		b0 = Pop();
		Pushi(b0.intVal - a0.intVal);
		break;
	case DataType::FLOAT:
		a0 = Pop();
		b0 = Pop();
		Pushf(b0.floatVal - a0.floatVal);
		break;
	case DataType::VECTOR:
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		b0 = Pop();
		b1 = Pop();
		b2 = Pop();
		Pushv(b2.floatVal - a2.floatVal);
		Pushv(b1.floatVal - a1.floatVal);
		Pushv(b0.floatVal - a0.floatVal);
		break;
	default:
		Pop();
		Pop();
		Pushf(0.0f);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::Opcode07Neg(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2;
	switch (instruction.type)
	{
	case DataType::INT:
		a0 = Pop();
		Pushi(-a0.intVal);
		break;
	case DataType::FLOAT:
		a0 = Pop();
		Pushf(-a0.floatVal);
		break;
	case DataType::VECTOR:
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		Pushv(-a2.floatVal);
		Pushv(-a1.floatVal);
		Pushv(-a0.floatVal);
		break;
	default:
		Pop();
		Pushf(0.0f);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::Opcode08Mul(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0;
	switch (instruction.type)
	{
	case DataType::INT:
		a0 = Pop();
		b0 = Pop();
		Pushi(a0.intVal * b0.intVal);
		break;
	case DataType::FLOAT:
		a0 = Pop();
		b0 = Pop();
		Pushf(a0.floatVal * b0.floatVal);
		break;
	case DataType::VECTOR:
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		b0 = Pop();
		Pushv(a2.floatVal * b0.floatVal);
		Pushv(a1.floatVal * b0.floatVal);
		Pushv(a0.floatVal * b0.floatVal);
		break;
	default:
		Pop();
		Pop();
		Pushf(0.0f);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::Opcode09Div(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, b0, b1, b2;
	switch (instruction.type)
	{
	case DataType::INT:
		a0 = Pop();
		b0 = Pop();
		if (a0.intVal != 0)
		{
			Pushi(b0.intVal / a0.intVal);
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
		if (a0.floatVal != 0.0f)
		{
			Pushf(b0.floatVal / a0.floatVal);
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
		if (a0.floatVal != 0.0f)
		{
			Pushv(b2.floatVal / a0.floatVal);
			Pushv(b1.floatVal / a0.floatVal);
			Pushv(b0.floatVal / a0.floatVal);
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

void LHVM::Opcode10Mod(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, b0, b1, b2;
	switch (instruction.type)
	{
	case DataType::INT:
		a0 = Pop();
		b0 = Pop();
		if (a0.intVal != 0)
		{
			Pushi(b0.intVal % a0.intVal);
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
		if (a0.floatVal != 0.0f)
		{
			Pushf(Fmod(b0.floatVal, a0.floatVal));
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
		if (a0.floatVal != 0.0f)
		{
			Pushv(Fmod(b2.floatVal, a0.floatVal));
			Pushv(Fmod(b1.floatVal, a0.floatVal));
			Pushv(Fmod(b0.floatVal, a0.floatVal));
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

void LHVM::Opcode11Not(VMTask& task, const VMInstruction& instruction)
{
	bool a = Pop().intVal;
	Pushb(!a);
}

void LHVM::Opcode12And(VMTask& task, const VMInstruction& instruction)
{
	bool b = Pop().intVal;
	bool a = Pop().intVal;
	Pushb(a && b);
}

void LHVM::Opcode13Or(VMTask& task, const VMInstruction& instruction)
{
	bool b = Pop().intVal;
	bool a = Pop().intVal;
	Pushb(a || b);
}

void LHVM::Opcode14Eq(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.type)
	{
	case DataType::INT:
	case DataType::BOOLEAN:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.intVal == b0.intVal);
		break;
	case DataType::FLOAT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.floatVal == b0.floatVal);
		break;
	case DataType::VECTOR:
		b0 = Pop();
		b1 = Pop();
		b2 = Pop();
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		Pushb(a0.floatVal == b0.floatVal && a1.floatVal == b1.floatVal && a2.floatVal == b2.floatVal);
		break;
	case DataType::OBJECT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.uintVal == b0.uintVal);
		break;
	default:
		Pop();
		Pop();
		Pushb(false);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::Opcode15Neq(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.type)
	{
	case DataType::INT:
	case DataType::BOOLEAN:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.intVal != b0.intVal);
		break;
	case DataType::FLOAT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.floatVal != b0.floatVal);
		break;
	case DataType::VECTOR:
		b0 = Pop();
		b1 = Pop();
		b2 = Pop();
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		Pushb(a0.floatVal != b0.floatVal || a1.floatVal != b1.floatVal || a2.floatVal != b2.floatVal);
		break;
	case DataType::OBJECT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.uintVal != b0.uintVal);
		break;
	default:
		Pop();
		Pop();
		Pushb(false);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::Opcode16Geq(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.type)
	{
	case DataType::INT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.intVal >= b0.intVal);
		break;
	case DataType::FLOAT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.floatVal >= b0.floatVal);
		break;
	default:
		Pushb(false);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::Opcode17Leq(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.type)
	{
	case DataType::INT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.intVal <= b0.intVal);
		break;
	case DataType::FLOAT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.floatVal <= b0.floatVal);
		break;
	default:
		Pushb(false);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::Opcode18Gt(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.type)
	{
	case DataType::INT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.intVal > b0.intVal);
		break;
	case DataType::FLOAT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.floatVal > b0.floatVal);
		break;
	default:
		Pushb(false);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::Opcode19Lt(VMTask& task, const VMInstruction& instruction)
{
	VMValue a0, a1, a2, b0, b1, b2;
	switch (instruction.type)
	{
	case DataType::INT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.intVal < b0.intVal);
		break;
	case DataType::FLOAT:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.floatVal < b0.floatVal);
		break;
	default:
		Pushb(false);
		SignalError(ErrorCode::INVALID_TYPE);
	}
}

void LHVM::Opcode20Jmp(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.Mode == VMMode::FORWARD)
	{
		task.InstructionAddress = instruction.intVal - 1;
	}
	else // Mode::BACKWARD
	{
		task.InstructionAddress = instruction.intVal;
		task.Yield = true;
	}
}

void LHVM::Opcode21Sleep(VMTask& task, const VMInstruction& instruction)
{
	const auto seconds = Pop().floatVal;
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

void LHVM::Opcode22Except(VMTask& task, const VMInstruction& instruction)
{
	if (_currentTask != NULL)
	{
		_currentTask->ExceptionHandlerIps.emplace_back(instruction.uintVal);
	}
}

void LHVM::Opcode23Cast(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.Mode == VMMode::ZERO)
	{
		auto& var = GetVar(task, instruction.uintVal);
		if (var.type == DataType::OBJECT)
		{
			RemoveReference(var.uintVal);
		}
		var.floatVal = 0.0f;
		var.type = DataType::FLOAT;
	}
	else // Mode::CAST
	{
		Push(Pop(), instruction.type);
	}
}

void LHVM::Opcode24Call(VMTask& task, const VMInstruction& instruction)
{
	const auto newTaskId = StartScript(instruction.uintVal);
	if (instruction.Mode == VMMode::SYNC)
	{
		task.WaitingTaskId = newTaskId;
	}
}

void LHVM::Opcode25EndExcept(VMTask& task, const VMInstruction& instruction)
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

void LHVM::Opcode26RetExcept(VMTask& task, const VMInstruction& instruction)
{
	task.InstructionAddress = task.PrevInstructionAddress;
	task.PrevInstructionAddress = 0;
	task.InExceptionHandler = false;
}

void LHVM::Opcode27IterExcept(VMTask& task, const VMInstruction& instruction)
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

void LHVM::Opcode28BrkExcept(VMTask& task, const VMInstruction& instruction)
{
	if (_currentTask != NULL)
	{
		_currentTask->ExceptionHandlerIps.clear();
	}

	task.PrevInstructionAddress = 0;
	task.InstructionAddress++;
	task.InExceptionHandler = false;
}

void LHVM::Opcode29Swap(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.type == DataType::INT) // swap the 2 topmost values on the stack
	{
		DataType t0, t1;
		VMValue v0 = Pop(t0);
		VMValue v1 = Pop(t1);
		Push(v0, t0);
		Push(v1, t1);
	}
	else
	{
		const auto offset = instruction.intVal;
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

void LHVM::Opcode30Line(VMTask& task, const VMInstruction& instruction) {}

} // namespace openblack::LHVM

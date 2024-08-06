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

	_opcodesImpl[0] = &LHVM::_opcode00_END;
	_opcodesImpl[1] = &LHVM::_opcode01_JZ;
	_opcodesImpl[2] = &LHVM::_opcode02_PUSH;
	_opcodesImpl[3] = &LHVM::_opcode03_POP;
	_opcodesImpl[4] = &LHVM::_opcode04_ADD;
	_opcodesImpl[5] = &LHVM::_opcode05_SYS;
	_opcodesImpl[6] = &LHVM::_opcode06_SUB;
	_opcodesImpl[7] = &LHVM::_opcode07_NEG;
	_opcodesImpl[8] = &LHVM::_opcode08_MUL;
	_opcodesImpl[9] = &LHVM::_opcode09_DIV;

	_opcodesImpl[10] = &LHVM::_opcode10_MOD;
	_opcodesImpl[11] = &LHVM::_opcode11_NOT;
	_opcodesImpl[12] = &LHVM::_opcode12_AND;
	_opcodesImpl[13] = &LHVM::_opcode13_OR;
	_opcodesImpl[14] = &LHVM::_opcode14_EQ;
	_opcodesImpl[15] = &LHVM::_opcode15_NEQ;
	_opcodesImpl[16] = &LHVM::_opcode16_GEQ;
	_opcodesImpl[17] = &LHVM::_opcode17_LEQ;
	_opcodesImpl[18] = &LHVM::_opcode18_GT;
	_opcodesImpl[19] = &LHVM::_opcode19_LT;
	_opcodesImpl[20] = &LHVM::_opcode20_JMP;
	_opcodesImpl[21] = &LHVM::_opcode21_SLEEP;
	_opcodesImpl[22] = &LHVM::_opcode22_EXCEPT;
	_opcodesImpl[23] = &LHVM::_opcode23_CAST;
	_opcodesImpl[24] = &LHVM::_opcode24_CALL;
	_opcodesImpl[25] = &LHVM::_opcode25_ENDEXCEPT;
	_opcodesImpl[26] = &LHVM::_opcode26_RETEXCEPT;
	_opcodesImpl[27] = &LHVM::_opcode27_ITEREXCEPT;
	_opcodesImpl[28] = &LHVM::_opcode28_BRKEXCEPT;
	_opcodesImpl[29] = &LHVM::_opcode29_SWAP;
	_opcodesImpl[30] = &LHVM::_opcode30_LINE;
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
	if (file.HasStatus())
	{
		Fail("File contains status data");
	}

	StopAllTasks();

	_instructions = file.GetInstructions();
	_scripts = file.GetScripts();
	_data = file.GetData();
	_mainStack.count = 0;
	_mainStack.pushCount = 0;
	_mainStack.popCount = 0;
	_currentStack = &_mainStack;

	_variablesNames = file.GetVariablesNames();
	_variables.clear();
	_variables.reserve(_variablesNames.size() + 1);
	_variables.emplace_back(VMVar {.type = DataType::FLOAT, .floatVal = 0.0, .name = "Null variable"});
	for (auto& name : _variablesNames)
	{
		_variables.emplace_back(VMVar {.type = DataType::FLOAT, .floatVal = 0.0, .name = name});
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
		_tasks.emplace(task.id, task);
	}

	_ticks = file.GetTicks();
	_currentLineNumber = file.GetCurrentLineNumber();
	_highestTaskId = file.GetHighestTaskId();
	_highestScriptId = file.GetHighestScriptId();
	_executedInstructions = file.GetScriptInstructionCount();
}

VMValue LHVM::Pop(DataType& type)
{
	_currentStack->popCount++;
	if (_currentStack->count > 0)
	{
		_currentStack->count--;
		type = _currentStack->types[_currentStack->count];
		return _currentStack->values[_currentStack->count];
	}
	else
	{
		SignalError(ErrorCode::STACK_EMPTY);
	}
	return VMValue {.uintVal = 0};
}

VMValue LHVM::Pop()
{
	DataType tmp;
	return Pop(tmp);
}

float_t LHVM::Popf()
{
	DataType tmp;
	return Pop(tmp).floatVal;
}

void LHVM::Push(VMValue value, DataType type)
{
	_currentStack->pushCount++;
	if (_currentStack->count < 32)
	{
		_currentStack->values[_currentStack->count] = value;
		_currentStack->types[_currentStack->count] = type;
		_currentStack->count++;
	}
	else
	{
		SignalError(ErrorCode::STACK_FULL);
	}
}

void LHVM::Pushf(float_t value)
{
	Push(VMValue {.floatVal = value}, DataType::FLOAT);
}

void LHVM::Pushv(float_t value)
{
	Push(VMValue {.floatVal = value}, DataType::VECTOR);
}

void LHVM::Pushi(int32_t value)
{
	Push(VMValue {.intVal = value}, DataType::INT);
}

void LHVM::Pusho(uint32_t value)
{
	Push(VMValue {.uintVal = value}, DataType::OBJECT);
}

void LHVM::Pushb(bool value)
{
	Push(VMValue {.intVal = value ? 1 : 0}, DataType::BOOLEAN);
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

	_mainStack.popCount += _mainStack.count;
	_mainStack.count = 0;

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
			_currentStack = &task.stack;
			_currentExceptStruct = &task.exceptStruct;
			if (task.inExceptionHandler)
			{
				CpuLoop(task);
			}
			else if (task.waitingTaskId == 0)
			{
				task.currentExceptionHandlerIndex = 0;
				if (GetExceptionHandlersCount() > 0)
				{
					task.prevInstructionAddress = task.instructionAddress;
					task.instructionAddress = GetCurrentExceptionHandlerIp(task.currentExceptionHandlerIndex);
					task.inExceptionHandler = true;
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
			_currentStack = &task.stack;
			_currentExceptStruct = &task.exceptStruct;
			if (!task.inExceptionHandler)
			{
				CpuLoop(task);
			}
		}
	}

	// handle tasks termination
	for (auto iter = _tasks.begin(); iter != _tasks.end();)
	{
		auto& task = (*iter++).second;
		if (task.stop)
		{
			StopTask(task.id);
		}
	}

	// unlock waiting tasks
	for (auto& [id, task] : _tasks)
	{
		if (task.type & allowedScriptTypesMask)
		{
			task.ticks++;
			if (task.waitingTaskId != 0 && !TaskExists(task.waitingTaskId))
			{
				task.waitingTaskId = 0;
				task.instructionAddress++;
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

		stack.pushCount++;
		if (stack.count < 31)
		{
			stack.values[stack.count] = value;
			stack.types[stack.count] = type;
			stack.count++;
		}
	}

	// allocate local variables with default values
	const auto& scriptVariables = script.GetVariables();
	std::vector<VMVar> taskVariables;
	taskVariables.reserve(scriptVariables.size());
	for (unsigned int i = 0; i < scriptVariables.size(); i++)
	{
		taskVariables.push_back(VMVar {.type = DataType::FLOAT, .floatVal = 0.0f, .name = scriptVariables.at(i)});
	}

	const auto& task =
	    VMTask(taskVariables, script.GetScriptID(), taskNumber, script.GetInstructionAddress(), 0, 0,
	           script.GetVariablesOffset(), stack, 0, VMExceptStruct {.instructionAddress = script.GetInstructionAddress()}, 1,
	           0, 0, 0, 0, script.GetName(), script.GetFileName(), script.GetType());

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
		if (filter(task.name, task.filename))
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
		for (auto& var : task.localVars)
		{
			if (var.type == DataType::OBJECT)
			{
				RemoveReference(var.uintVal);
			}
		}

		if (_currentTask == &task)
		{
			_currentStack = &_mainStack;
			_currentExceptStruct = NULL;
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
			ids.emplace_back(task.id);
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
	float_t time = GetTicksCount() * 10.0f;
	Pushf(time);
}

VMVar& LHVM::GetVar(VMTask& task, const uint32_t id)
{
	const auto offset = task.variablesOffset;
	return (id > offset) ? task.localVars.at(id - offset - 1) : _variables.at(id);
}

void LHVM::PushExceptionHandler(const uint32_t address)
{
	if (_currentExceptStruct != NULL)
	{
		_currentExceptStruct->exceptionHandlerIps.emplace_back(address);
	}
}

void LHVM::PopExceptionHandler()
{
	if (_currentExceptStruct != NULL && _currentExceptStruct->exceptionHandlerIps.size() > 0)
	{
		_currentExceptStruct->exceptionHandlerIps.pop_back();
	}
}

uint32_t LHVM::GetExceptionHandlersCount()
{
	if (_currentExceptStruct != NULL)
	{
		return _currentExceptStruct->exceptionHandlerIps.size();
	}
	else
	{
		return 0;
	}
}

uint32_t LHVM::GetCurrentExceptionHandlerIp(const uint32_t index)
{
	if (_currentExceptStruct != NULL)
	{
		return _currentExceptStruct->exceptionHandlerIps.at(_currentExceptStruct->exceptionHandlerIps.size() - index - 1);
	}
	else
	{
		return 0;
	}
}

void LHVM::PrintInstruction(const VMTask& task, const VMInstruction& instruction)
{
	// TODO: improve this
	std::string opcode = Opcode_Names[(int)instruction.opcode];
	std::string arg = "";
	if (instruction.opcode == Opcode::RUN)
	{
		if (instruction.mode == Mode::ASYNC)
		{
			arg = "async ";
		}
		arg += _scripts[instruction.intVal - 1].GetName();
	}
	else if (instruction.opcode == Opcode::CALL)
	{
		arg += _functions->at(instruction.intVal).name;
	}
	else
	{
		if (instruction.opcode == Opcode::PUSH || instruction.opcode == Opcode::POP || instruction.opcode == Opcode::CAST ||
		    instruction.opcode == Opcode::ADD || instruction.opcode == Opcode::MINUS || instruction.opcode == Opcode::TIMES ||
		    instruction.opcode == Opcode::DIVIDE || instruction.opcode == Opcode::MODULUS)
		{
			opcode += DataType_Chars[(int)instruction.type];
		}
		if ((instruction.opcode == Opcode::PUSH || instruction.opcode == Opcode::POP) && instruction.mode == Mode::REFERENCE)
		{
			if (instruction.intVal > task.variablesOffset)
			{
				arg = std::format("{}", task.localVars[instruction.intVal - task.variablesOffset - 1].name);
			}
			else
			{
				arg = std::format("{}", _variables[instruction.intVal].name);
			}
		}
		else if (instruction.opcode == Opcode::PUSH && instruction.mode == Mode::IMMEDIATE)
		{
			if (instruction.type == DataType::FLOAT || instruction.type == DataType::VECTOR)
			{
				arg = std::format("{}", instruction.floatVal);
			}
			else
			{
				arg = std::format("{}", instruction.intVal);
			}
		}
		else if (instruction.opcode == Opcode::JUMP || instruction.opcode == Opcode::WAIT || instruction.opcode == Opcode::SWAP)
		{
			arg = std::format("{}", instruction.intVal);
		}
		if (instruction.opcode == Opcode::WAIT)
		{
			bool val = task.stack.values[task.stack.count - 1].intVal;
			arg += val ? " [true] -> continue" : " [false] -> JUMP";
		}
	}
	printf("%s:%d %s[%d] %s %s\n", task.filename.c_str(), instruction.line, task.name.c_str(), task.id, opcode.c_str(),
	       arg.c_str());
}

void LHVM::CpuLoop(VMTask& task)
{
	const auto wasExceptionHandler = task.inExceptionHandler;
	task.yield = false;
	while (task.waitingTaskId == 0)
	{
		_currentTask = &task;
		_executedInstructions++;
		const auto& instruction = _instructions[task.instructionAddress];

		// PrintInstruction(task, instruction); // just for debug purposes

		(this->*_opcodesImpl[static_cast<int>(instruction.opcode)])(task, instruction);

		if (task.stop || task.yield || task.waitingTaskId != 0 || task.inExceptionHandler != wasExceptionHandler)
		{
			break;
		}
		task.instructionAddress++;
	}
	_currentTask = NULL;
}

void LHVM::_opcode00_END(VMTask& task, const VMInstruction& instruction)
{
	task.stop = true;
}

void LHVM::_opcode01_JZ(VMTask& task, const VMInstruction& instruction)
{
	const auto val = Pop().intVal;
	if (val)
	{
		task.ticks = 1;
	}
	else // jump if popped value isn't zero
	{
		if (instruction.mode == Mode::FORWARD)
		{
			task.instructionAddress = instruction.intVal - 1;
		}
		else // Mode::BACKWARD
		{
			task.instructionAddress = instruction.intVal;
			task.yield = true;
		}
	}
}

void LHVM::_opcode02_PUSH(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.mode == Mode::IMMEDIATE)
	{
		Push(instruction.data, instruction.type);
	}
	else // Mode::REFERENCE
	{
		const auto& var = GetVar(task, instruction.uintVal);
		Push(var.value, var.type);
	}
}

void LHVM::_opcode03_POP(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.mode == Mode::REFERENCE)
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

void LHVM::_opcode04_ADD(VMTask& task, const VMInstruction& instruction)
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

void LHVM::_opcode05_SYS(VMTask& task, const VMInstruction& instruction)
{
	const auto id = instruction.intVal;
	if (id > 0 && id < _functions->size())
	{
		const auto& func = _functions->at(id);
		if (func.impl != nullptr)
		{
			_currentStack->pushCount = 0;
			_currentStack->popCount = 0;
			InvokeNativeCallEnterCallback(id);
			func.impl();
			InvokeNativeCallExitCallback(id);
		}
		else // if impl not provided, then just adjust the stack
		{
			for (int i = 0; i < func.stackIn; i++)
			{
				Pop();
			}
			for (unsigned int i = 0; i < func.stackOut; i++)
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

void LHVM::_opcode06_SUB(VMTask& task, const VMInstruction& instruction)
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

void LHVM::_opcode07_NEG(VMTask& task, const VMInstruction& instruction)
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

void LHVM::_opcode08_MUL(VMTask& task, const VMInstruction& instruction)
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

void LHVM::_opcode09_DIV(VMTask& task, const VMInstruction& instruction)
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

void LHVM::_opcode10_MOD(VMTask& task, const VMInstruction& instruction)
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
			Pushf(std::fmodf(b0.floatVal, a0.floatVal));
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
			Pushv(std::fmodf(b2.floatVal, a0.floatVal));
			Pushv(std::fmodf(b1.floatVal, a0.floatVal));
			Pushv(std::fmodf(b0.floatVal, a0.floatVal));
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

void LHVM::_opcode11_NOT(VMTask& task, const VMInstruction& instruction)
{
	bool a = Pop().intVal;
	Pushb(!a);
}

void LHVM::_opcode12_AND(VMTask& task, const VMInstruction& instruction)
{
	bool b = Pop().intVal;
	bool a = Pop().intVal;
	Pushb(a && b);
}

void LHVM::_opcode13_OR(VMTask& task, const VMInstruction& instruction)
{
	bool b = Pop().intVal;
	bool a = Pop().intVal;
	Pushb(a || b);
}

void LHVM::_opcode14_EQ(VMTask& task, const VMInstruction& instruction)
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

void LHVM::_opcode15_NEQ(VMTask& task, const VMInstruction& instruction)
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

void LHVM::_opcode16_GEQ(VMTask& task, const VMInstruction& instruction)
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

void LHVM::_opcode17_LEQ(VMTask& task, const VMInstruction& instruction)
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

void LHVM::_opcode18_GT(VMTask& task, const VMInstruction& instruction)
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

void LHVM::_opcode19_LT(VMTask& task, const VMInstruction& instruction)
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

void LHVM::_opcode20_JMP(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.mode == Mode::FORWARD)
	{
		task.instructionAddress = instruction.intVal - 1;
	}
	else // Mode::BACKWARD
	{
		task.instructionAddress = instruction.intVal;
		task.yield = true;
	}
}

void LHVM::_opcode21_SLEEP(VMTask& task, const VMInstruction& instruction)
{
	const auto seconds = Pop().floatVal;
	if (static_cast<uint32_t>(seconds * 10.0f) < task.ticks)
	{
		task.sleeping = false;
		Pushb(true);
	}
	else
	{
		task.sleeping = true;
		Pushb(false);
	}
}

void LHVM::_opcode22_EXCEPT(VMTask& task, const VMInstruction& instruction)
{
	PushExceptionHandler(instruction.uintVal);
}

void LHVM::_opcode23_CAST(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.mode == Mode::ZERO)
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

void LHVM::_opcode24_CALL(VMTask& task, const VMInstruction& instruction)
{
	const auto newTaskId = StartScript(instruction.uintVal);
	if (instruction.mode == Mode::SYNC)
	{
		task.waitingTaskId = newTaskId;
	}
}

void LHVM::_opcode25_ENDEXCEPT(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.mode == Mode::ENDEXCEPT)
	{
		PopExceptionHandler();
	}
	else // Mode::YIELD
	{
		task.yield = true;
		task.instructionAddress++;
	}
}

void LHVM::_opcode26_RETEXCEPT(VMTask& task, const VMInstruction& instruction)
{
	task.instructionAddress = task.prevInstructionAddress;
	task.prevInstructionAddress = 0;
	task.inExceptionHandler = false;
}

void LHVM::_opcode27_ITEREXCEPT(VMTask& task, const VMInstruction& instruction)
{
	task.currentExceptionHandlerIndex++;
	if (task.currentExceptionHandlerIndex < task.exceptStruct.exceptionHandlerIps.size())
	{
		task.instructionAddress = GetCurrentExceptionHandlerIp(task.currentExceptionHandlerIndex) - 1;
	}
	else
	{
		task.instructionAddress = task.prevInstructionAddress;
		task.prevInstructionAddress = 0;
		task.inExceptionHandler = false;
	}
}

void LHVM::_opcode28_BRKEXCEPT(VMTask& task, const VMInstruction& instruction)
{
	if (_currentExceptStruct != NULL)
	{
		_currentExceptStruct->exceptionHandlerIps.clear();
	}

	task.prevInstructionAddress = 0;
	task.instructionAddress++;
	task.inExceptionHandler = false;
}

void LHVM::_opcode29_SWAP(VMTask& task, const VMInstruction& instruction)
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
		VMValue copyVal {.floatVal = 0.0f};
		std::array<DataType, 32> tmpTypes;
		std::array<VMValue, 32> tmpVals;
		if (instruction.mode == Mode::COPYFROM) // push a copy of the Nth value from top of the stack
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

void LHVM::_opcode30_LINE(VMTask& task, const VMInstruction& instruction) {}

} // namespace openblack::LHVM

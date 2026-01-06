/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LHVM.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

#include <spanstream>

#include "LHVMFile.h"

namespace openblack::lhvm
{

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

LHVM::~LHVM() = default;

void LHVM::Initialise(const std::vector<NativeFunction>* functions, std::function<void(uint32_t func)> nativeCallEnterCallback,
                      std::function<void(uint32_t func)> nativeCallExitCallback,
                      std::function<void(uint32_t taskNumber)> stopTaskCallback,
                      std::function<void(ErrorCode code, const std::string v0, uint32_t v1)> errorCallback,
                      std::function<void(uint32_t objId)> addReference, std::function<void(uint32_t objId)> removeReference)
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

int LHVM::LoadBinary(const std::filesystem::path& filepath)
{
	LHVMFile file;
	file.Open(filepath);
	return LoadBinary(file);
}

int LHVM::LoadBinary(const std::span<const char>& span)
{
	LHVMFile file;
	file.Open(span);
	return LoadBinary(file);
}

int LHVM::LoadBinary(const LHVMFile& file)
{
	if (!file.IsLoaded() || file.HasStatus())
	{
		return EXIT_FAILURE;
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
	_variables.emplace_back(DataType::Float, VMValue(0.0f), "Null variable");
	for (auto& name : _variablesNames)
	{
		_variables.emplace_back(DataType::Float, VMValue(0.0f), name);
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
			StartScript(script.name, ScriptType::All);
		}
		else
		{
			SignalError(ErrorCode::ErrScriptIdNotFound, scriptId);
		}
	}

	return EXIT_SUCCESS;
}

int LHVM::RestoreState(const std::filesystem::path& filepath)
{
	auto file = LHVMFile();
	file.Open(filepath);
	if (!file.IsLoaded() || !file.HasStatus())
	{
		return EXIT_FAILURE;
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
	_executedInstructions = file.GetExecutedInstructions();

	return EXIT_SUCCESS;
}

VMValue LHVM::Pop(DataType& type)
{
	_currentStack->popCount++;
	if (_currentStack->count > 0)
	{
		_currentStack->count--;
		type = _currentStack->types.at(_currentStack->count);
		return _currentStack->values.at(_currentStack->count);
	}
	type = DataType::None;
	SignalError(ErrorCode::ErrStackEmpty);
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
	_currentStack->pushCount++;
	if (_currentStack->count < VMStack::k_Size)
	{
		_currentStack->values.at(_currentStack->count) = value;
		_currentStack->types.at(_currentStack->count) = type;
		_currentStack->count++;
	}
	else
	{
		SignalError(ErrorCode::ErrStackFull);
	}
}

void LHVM::Pushf(float value)
{
	Push(VMValue(value), DataType::Float);
}

void LHVM::Pushv(float value)
{
	Push(VMValue(value), DataType::Vector);
}

void LHVM::Pushi(int32_t value)
{
	Push(VMValue(value), DataType::Int);
}

void LHVM::Pusho(uint32_t value)
{
	Push(VMValue(value), DataType::Object);
}

void LHVM::Pushb(bool value)
{
	Push(VMValue(value ? 1 : 0), DataType::Boolean);
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

	_currentTask = nullptr;
	_currentStack = &_mainStack;
}

int LHVM::SaveBinary(const std::filesystem::path& filepath)
{
	LHVMFile file(LHVMVersion::BlackAndWhite, _variablesNames, _instructions, _auto, _scripts, _data);
	file.Write(filepath);
	return EXIT_SUCCESS;
}

int LHVM::SaveState(const std::filesystem::path& filepath)
{
	std::vector<VMTask> tasks;
	tasks.reserve(_tasks.size());
	for (const auto& [id, task] : _tasks)
	{
		tasks.emplace_back(task);
	}

	LHVMFile file(LHVMVersion::BlackAndWhite, _variablesNames, _instructions, _auto, _scripts, _data, _mainStack, _variables,
	              tasks, _ticks, _currentLineNumber, _highestTaskId, _highestScriptId, _executedInstructions);
	file.Write(filepath);
	return EXIT_SUCCESS;
}

void LHVM::LookIn(const ScriptType allowedScriptTypesMask)
{
	// execute exception handlers first
	for (auto& [id, task] : _tasks)
	{
		if (task.type & allowedScriptTypesMask)
		{
			_currentStack = &task.stack;
			if (task.inExceptionHandler)
			{
				CpuLoop(task);
			}
			else if (task.waitingTaskId == 0)
			{
				task.currentExceptionHandlerIndex = 0;
				if (GetExceptionHandlersCount() > 0)
				{
					task.pevInstructionAddress = task.instructionAddress;
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
	const auto* const script = GetScript(name);
	if (script != nullptr)
	{
		if (script->type & allowedScriptTypesMask)
		{
			return StartScript(*script);
		}
		SignalError(ErrorCode::ErrNoScriptOfType, name);
	}
	else
	{
		SignalError(ErrorCode::ErrScriptNameNotFound, name);
	}
	return 0;
}

uint32_t LHVM::StartScript(const uint32_t id)
{
	if (id > 0 && id <= _scripts.size())
	{
		return StartScript(_scripts.at(id - 1));
	}
	SignalError(ErrorCode::ErrScriptIdNotFound, id);
	return 0;
}

uint32_t LHVM::StartScript(const VMScript& script)
{
	const auto taskNumber = ++_highestTaskId;

	// copy values from current stack to new stack
	VMStack stack {};
	for (unsigned int i = 0; i < script.parameterCount; i++)
	{
		DataType type;
		const auto& value = Pop(type);

		stack.pushCount++;
		if (stack.count < 31)
		{
			stack.values.at(stack.count) = value;
			stack.types.at(stack.count) = type;
			stack.count++;
		}
	}

	// allocate local variables with default values
	const auto& scriptVariables = script.variables;
	std::vector<VMVar> taskVariables;
	taskVariables.reserve(scriptVariables.size());
	for (const auto& name : scriptVariables)
	{
		taskVariables.emplace_back(DataType::Float, VMValue(0.0f), name);
	}

	const auto& task = VMTask(taskVariables, script.scriptId, taskNumber, script.instructionAddress, script.variablesOffset,
	                          stack, script.name, script.filename, script.type);

	_tasks.emplace(taskNumber, task);

	return taskNumber;
}

void LHVM::StopAllTasks()
{
	while (!_tasks.empty())
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
			if (var.type == DataType::Object)
			{
				RemoveReference(var.value.uintVal);
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
		SignalError(ErrorCode::ErrTaskIdNotFound, taskNumber);
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

std::string LHVM::GetString(uint32_t offset)
{
	if (offset < _data.size())
	{
		return {_data.data() + offset};
	}
	return "";
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
		if (script.name == name)
		{
			return &script;
		}
	}
	return nullptr;
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
	const float time = GetTicksCount() * 10.0f;
	Pushf(time);
}

VMVar& LHVM::GetVar(VMTask& task, const uint32_t id)
{
	const auto offset = task.variablesOffset;
	return (id > offset) ? task.localVars.at(id - offset - 1) : _variables.at(id);
}

uint32_t LHVM::GetExceptionHandlersCount()
{
	if (_currentTask != nullptr)
	{
		return _currentTask->exceptionHandlerIps.size();
	}
	return 0;
}

uint32_t LHVM::GetCurrentExceptionHandlerIp(const uint32_t index)
{
	if (_currentTask != nullptr)
	{
		return _currentTask->exceptionHandlerIps.at(_currentTask->exceptionHandlerIps.size() - index - 1);
	}
	return 0;
}

void LHVM::PrintInstruction(const VMTask& task, const VMInstruction& instruction)
{
	// TODO(Daniels118): improve this
	std::string opcode = k_OpcodeNames.at(static_cast<int>(instruction.code));
	std::string arg;
	if (instruction.code == Opcode::Run)
	{
		if (instruction.mode == VMMode::Async)
		{
			arg = "async ";
		}
		arg += _scripts.at(instruction.data.intVal - 1).name;
	}
	else if (instruction.code == Opcode::Sys)
	{
		arg += _functions->at(instruction.data.intVal).name;
	}
	else
	{
		if (instruction.code == Opcode::Push || instruction.code == Opcode::Pop || instruction.code == Opcode::Cast ||
		    instruction.code == Opcode::Add || instruction.code == Opcode::Sub || instruction.code == Opcode::Mul ||
		    instruction.code == Opcode::Div || instruction.code == Opcode::Mod)
		{
			opcode += k_DataTypeChars.at(static_cast<int>(instruction.type));
		}
		if ((instruction.code == Opcode::Push || instruction.code == Opcode::Pop) && instruction.mode == VMMode::Reference)
		{
			if (instruction.data.intVal > task.variablesOffset)
			{
				arg = task.localVars[instruction.data.intVal - task.variablesOffset - 1].name;
			}
			else
			{
				arg = _variables.at(instruction.data.intVal).name;
			}
		}
		else if (instruction.code == Opcode::Push && instruction.mode == VMMode::Immediate)
		{
			if (instruction.type == DataType::Float || instruction.type == DataType::Vector)
			{
				arg = std::to_string(instruction.data.floatVal);
			}
			else
			{
				arg = std::to_string(instruction.data.intVal);
			}
		}
		else if (instruction.code == Opcode::Jmp || instruction.code == Opcode::Wait || instruction.code == Opcode::Swap)
		{
			arg = std::to_string(instruction.data.intVal);
		}
		if (instruction.code == Opcode::Wait)
		{
			const bool val = task.stack.values.at(task.stack.count - 1).intVal != 0;
			arg += val ? " [true] -> continue" : " [false] -> JUMP";
		}
	}
	printf("%s:%d %s[%d] %s %s\n", task.filename.c_str(), instruction.line, task.name.c_str(), task.id, opcode.c_str(),
	       arg.c_str());
}

void LHVM::CpuLoop(VMTask& task)
{
	const auto wasExceptionHandler = task.inExceptionHandler;
	task.iield = false;
	while (task.waitingTaskId == 0)
	{
		_currentTask = &task;
		_executedInstructions++;
		const auto& instruction = _instructions.at(task.instructionAddress);

		// PrintInstruction(task, instruction); // just for debug purposes

		(this->*_opcodesImpl.at(static_cast<int>(instruction.code)))(task, instruction);

		if (task.stop || task.iield || task.waitingTaskId != 0 || task.inExceptionHandler != wasExceptionHandler)
		{
			break;
		}
		task.instructionAddress++;
	}
	_currentTask = nullptr;
}

float LHVM::Fmod(float a, float b)
{
	return a - b * static_cast<int64_t>(a / b);
}

void LHVM::Opcode00End(VMTask& task, const VMInstruction& /*instruction*/)
{
	task.stop = true;
}

void LHVM::Opcode01Jz(VMTask& task, const VMInstruction& instruction)
{
	const bool val = Pop().intVal != 0;
	if (val)
	{
		task.ticks = 1;
	}
	else // jump if popped value isn't zero
	{
		if (instruction.mode == VMMode::Forward)
		{
			task.instructionAddress = instruction.data.intVal - 1;
		}
		else // Mode::BACKWARD
		{
			task.instructionAddress = instruction.data.intVal;
			task.iield = true;
		}
	}
}

void LHVM::Opcode02Push(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.mode == VMMode::Immediate)
	{
		Push(instruction.data, instruction.type);
	}
	else // Mode::REFERENCE
	{
		const auto& var = GetVar(task, instruction.data.uintVal);
		Push(var.value, var.type);
	}
}

void LHVM::Opcode03Pop(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.mode == VMMode::Reference)
	{
		auto& var = GetVar(task, instruction.data.uintVal);
		DataType type;
		const auto newVal = Pop(type);
		if (type == DataType::Object)
		{
			AddReference(newVal.uintVal);
		}
		if (var.type == DataType::Object)
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

void LHVM::Opcode04Add(VMTask& /*task*/, const VMInstruction& instruction)
{
	VMValue a0;
	VMValue a1;
	VMValue a2;
	VMValue b0;
	VMValue b1;
	VMValue b2;
	switch (instruction.type)
	{
	case DataType::Int:
		a0 = Pop();
		b0 = Pop();
		Pushi(a0.intVal + b0.intVal);
		break;
	case DataType::Float:
		a0 = Pop();
		b0 = Pop();
		Pushf(a0.floatVal + b0.floatVal);
		break;
	case DataType::Vector:
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
		SignalError(ErrorCode::ErrInvalidType);
	}
}

void LHVM::Opcode05Sys(VMTask& /*task*/, const VMInstruction& instruction)
{
	const auto id = instruction.data.intVal;
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
		SignalError(ErrorCode::ErrNativeFuncNotFound, id);
	}
}

void LHVM::Opcode06Sub(VMTask& /*task*/, const VMInstruction& instruction)
{
	VMValue a0;
	VMValue a1;
	VMValue a2;
	VMValue b0;
	VMValue b1;
	VMValue b2;
	switch (instruction.type)
	{
	case DataType::Int:
		a0 = Pop();
		b0 = Pop();
		Pushi(b0.intVal - a0.intVal);
		break;
	case DataType::Float:
		a0 = Pop();
		b0 = Pop();
		Pushf(b0.floatVal - a0.floatVal);
		break;
	case DataType::Vector:
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
		SignalError(ErrorCode::ErrInvalidType);
	}
}

void LHVM::Opcode07Neg(VMTask& /*task*/, const VMInstruction& instruction)
{
	VMValue a0;
	VMValue a1;
	VMValue a2;
	switch (instruction.type)
	{
	case DataType::Int:
		a0 = Pop();
		Pushi(-a0.intVal);
		break;
	case DataType::Float:
		a0 = Pop();
		Pushf(-a0.floatVal);
		break;
	case DataType::Vector:
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
		SignalError(ErrorCode::ErrInvalidType);
	}
}

void LHVM::Opcode08Mul(VMTask& /*task*/, const VMInstruction& instruction)
{
	VMValue a0;
	VMValue a1;
	VMValue a2;
	VMValue b0;
	switch (instruction.type)
	{
	case DataType::Int:
		a0 = Pop();
		b0 = Pop();
		Pushi(a0.intVal * b0.intVal);
		break;
	case DataType::Float:
		a0 = Pop();
		b0 = Pop();
		Pushf(a0.floatVal * b0.floatVal);
		break;
	case DataType::Vector:
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
		SignalError(ErrorCode::ErrInvalidType);
	}
}

void LHVM::Opcode09Div(VMTask& /*task*/, const VMInstruction& instruction)
{
	VMValue a0;
	VMValue b0;
	VMValue b1;
	VMValue b2;
	switch (instruction.type)
	{
	case DataType::Int:
		a0 = Pop();
		b0 = Pop();
		if (a0.intVal != 0)
		{
			Pushi(b0.intVal / a0.intVal);
		}
		else
		{
			Pushi(0);
			SignalError(ErrorCode::ErrDivByZero);
		}
		break;
	case DataType::Float:
		a0 = Pop();
		b0 = Pop();
		if (a0.floatVal != 0.0f)
		{
			Pushf(b0.floatVal / a0.floatVal);
		}
		else
		{
			Pushf(0.0f);
			SignalError(ErrorCode::ErrDivByZero);
		}
		break;
	case DataType::Vector:
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
			SignalError(ErrorCode::ErrDivByZero);
		}
		break;
	default:
		Pop();
		Pop();
		Pushf(0.0f);
		SignalError(ErrorCode::ErrInvalidType);
	}
}

void LHVM::Opcode10Mod(VMTask& /*task*/, const VMInstruction& instruction)
{
	VMValue a0;
	VMValue b0;
	VMValue b1;
	VMValue b2;
	switch (instruction.type)
	{
	case DataType::Int:
		a0 = Pop();
		b0 = Pop();
		if (a0.intVal != 0)
		{
			Pushi(b0.intVal % a0.intVal);
		}
		else
		{
			Pushi(0);
			SignalError(ErrorCode::ErrDivByZero);
		}
		break;
	case DataType::Float:
		a0 = Pop();
		b0 = Pop();
		if (a0.floatVal != 0.0f)
		{
			Pushf(Fmod(b0.floatVal, a0.floatVal));
		}
		else
		{
			Pushf(0.0f);
			SignalError(ErrorCode::ErrDivByZero);
		}
		break;
	case DataType::Vector:
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
			SignalError(ErrorCode::ErrDivByZero);
		}
		break;
	default:
		Pop();
		Pop();
		Pushf(0.0f);
		SignalError(ErrorCode::ErrInvalidType);
	}
}

void LHVM::Opcode11Not(VMTask& /*task*/, const VMInstruction& /*instruction*/)
{
	const bool a = Pop().intVal != 0;
	Pushb(!a);
}

void LHVM::Opcode12And(VMTask& /*task*/, const VMInstruction& /*instruction*/)
{
	const bool b = Pop().intVal != 0;
	const bool a = Pop().intVal != 0;
	Pushb(a && b);
}

void LHVM::Opcode13Or(VMTask& /*task*/, const VMInstruction& /*instruction*/)
{
	const bool b = Pop().intVal != 0;
	const bool a = Pop().intVal != 0;
	Pushb(a || b);
}

void LHVM::Opcode14Eq(VMTask& /*task*/, const VMInstruction& instruction)
{
	VMValue a0;
	VMValue a1;
	VMValue a2;
	VMValue b0;
	VMValue b1;
	VMValue b2;
	switch (instruction.type)
	{
	case DataType::Int:
	case DataType::Boolean:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.intVal == b0.intVal);
		break;
	case DataType::Float:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.floatVal == b0.floatVal);
		break;
	case DataType::Vector:
		b0 = Pop();
		b1 = Pop();
		b2 = Pop();
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		Pushb(a0.floatVal == b0.floatVal && a1.floatVal == b1.floatVal && a2.floatVal == b2.floatVal);
		break;
	case DataType::Object:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.uintVal == b0.uintVal);
		break;
	default:
		Pop();
		Pop();
		Pushb(false);
		SignalError(ErrorCode::ErrInvalidType);
	}
}

void LHVM::Opcode15Neq(VMTask& /*task*/, const VMInstruction& instruction)
{
	VMValue a0;
	VMValue a1;
	VMValue a2;
	VMValue b0;
	VMValue b1;
	VMValue b2;
	switch (instruction.type)
	{
	case DataType::Int:
	case DataType::Boolean:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.intVal != b0.intVal);
		break;
	case DataType::Float:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.floatVal != b0.floatVal);
		break;
	case DataType::Vector:
		b0 = Pop();
		b1 = Pop();
		b2 = Pop();
		a0 = Pop();
		a1 = Pop();
		a2 = Pop();
		Pushb(a0.floatVal != b0.floatVal || a1.floatVal != b1.floatVal || a2.floatVal != b2.floatVal);
		break;
	case DataType::Object:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.uintVal != b0.uintVal);
		break;
	default:
		Pop();
		Pop();
		Pushb(false);
		SignalError(ErrorCode::ErrInvalidType);
	}
}

void LHVM::Opcode16Geq(VMTask& /*task*/, const VMInstruction& instruction)
{
	VMValue a0;
	VMValue b0;
	switch (instruction.type)
	{
	case DataType::Int:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.intVal >= b0.intVal);
		break;
	case DataType::Float:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.floatVal >= b0.floatVal);
		break;
	default:
		Pushb(false);
		SignalError(ErrorCode::ErrInvalidType);
	}
}

void LHVM::Opcode17Leq(VMTask& /*task*/, const VMInstruction& instruction)
{
	VMValue a0;
	VMValue b0;
	switch (instruction.type)
	{
	case DataType::Int:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.intVal <= b0.intVal);
		break;
	case DataType::Float:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.floatVal <= b0.floatVal);
		break;
	default:
		Pushb(false);
		SignalError(ErrorCode::ErrInvalidType);
	}
}

void LHVM::Opcode18Gt(VMTask& /*task*/, const VMInstruction& instruction)
{
	VMValue a0;
	VMValue b0;
	switch (instruction.type)
	{
	case DataType::Int:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.intVal > b0.intVal);
		break;
	case DataType::Float:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.floatVal > b0.floatVal);
		break;
	default:
		Pushb(false);
		SignalError(ErrorCode::ErrInvalidType);
	}
}

void LHVM::Opcode19Lt(VMTask& /*task*/, const VMInstruction& instruction)
{
	VMValue a0;
	VMValue b0;
	switch (instruction.type)
	{
	case DataType::Int:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.intVal < b0.intVal);
		break;
	case DataType::Float:
		b0 = Pop();
		a0 = Pop();
		Pushb(a0.floatVal < b0.floatVal);
		break;
	default:
		Pushb(false);
		SignalError(ErrorCode::ErrInvalidType);
	}
}

void LHVM::Opcode20Jmp(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.mode == VMMode::Forward)
	{
		task.instructionAddress = instruction.data.intVal - 1;
	}
	else // Mode::BACKWARD
	{
		task.instructionAddress = instruction.data.intVal;
		task.iield = true;
	}
}

void LHVM::Opcode21Sleep(VMTask& task, const VMInstruction& /*instruction*/)
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

void LHVM::Opcode22Except(VMTask& /*task*/, const VMInstruction& instruction)
{
	if (_currentTask != nullptr)
	{
		_currentTask->exceptionHandlerIps.emplace_back(instruction.data.uintVal);
	}
}

void LHVM::Opcode23Cast(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.mode == VMMode::Zero)
	{
		auto& var = GetVar(task, instruction.data.uintVal);
		if (var.type == DataType::Object)
		{
			RemoveReference(var.value.uintVal);
		}
		var.value.floatVal = 0.0f;
		var.type = DataType::Float;
	}
	else // Mode::CAST
	{
		Push(Pop(), instruction.type);
	}
}

void LHVM::Opcode24Call(VMTask& task, const VMInstruction& instruction)
{
	const auto newTaskId = StartScript(instruction.data.uintVal);
	if (instruction.mode == VMMode::Sync)
	{
		task.waitingTaskId = newTaskId;
	}
}

void LHVM::Opcode25EndExcept(VMTask& task, const VMInstruction& instruction)
{
	if (instruction.mode == VMMode::EndExcept)
	{
		if (_currentTask != nullptr && !_currentTask->exceptionHandlerIps.empty())
		{
			_currentTask->exceptionHandlerIps.pop_back();
		}
	}
	else // Mode::Yield
	{
		task.iield = true;
		task.instructionAddress++;
	}
}

void LHVM::Opcode26RetExcept(VMTask& task, const VMInstruction& /*instruction*/)
{
	task.instructionAddress = task.pevInstructionAddress;
	task.pevInstructionAddress = 0;
	task.inExceptionHandler = false;
}

void LHVM::Opcode27IterExcept(VMTask& task, const VMInstruction& /*instruction*/)
{
	task.currentExceptionHandlerIndex++;
	if (task.currentExceptionHandlerIndex < task.exceptionHandlerIps.size())
	{
		task.instructionAddress = GetCurrentExceptionHandlerIp(task.currentExceptionHandlerIndex) - 1;
	}
	else
	{
		task.instructionAddress = task.pevInstructionAddress;
		task.pevInstructionAddress = 0;
		task.inExceptionHandler = false;
	}
}

void LHVM::Opcode28BrkExcept(VMTask& task, const VMInstruction& /*instruction*/)
{
	if (_currentTask != nullptr)
	{
		_currentTask->exceptionHandlerIps.clear();
	}

	task.pevInstructionAddress = 0;
	task.instructionAddress++;
	task.inExceptionHandler = false;
}

void LHVM::Opcode29Swap(VMTask& /*task*/, const VMInstruction& instruction)
{
	if (instruction.type == DataType::Int) // swap the 2 topmost values on the stack
	{
		DataType t0;
		DataType t1;
		const VMValue v0 = Pop(t0);
		const VMValue v1 = Pop(t1);
		Push(v0, t0);
		Push(v1, t1);
		return;
	}
	const auto offset = static_cast<size_t>(instruction.data.intVal);
	if (offset == 0 || offset >= VMStack::k_Size - 1)
	{
		SignalError(ErrorCode::ErrInvalidOperand);
		return;
	}
	DataType copyType = DataType::Float;
	VMValue copyVal(0.0f);
	std::array<DataType, VMStack::k_Size> tmpTypes;
	std::array<VMValue, VMStack::k_Size> tmpVals;
	if (instruction.mode == VMMode::CopyFrom) // push a copy of the Nth value from top of the stack
	{
		for (size_t i = 0; i < offset; i++)
		{
			DataType ti;
			const VMValue vi = Pop(ti);
			if (i == offset - 1)
			{
				copyType = ti;
				copyVal = vi;
			}
			if (i < tmpTypes.size())
			{
				tmpTypes.at(i) = ti;
				tmpVals.at(i) = vi;
			}
		}
		for (int i = static_cast<int>(std::min(offset, VMStack::k_Size)) - 1; i >= 0; i--)
		{
			Push(tmpVals.at(i), tmpTypes.at(i));
		}
		Push(copyVal, copyType);
	}
	else // Mode::CopyTo insert a copy of the topmost value on the stack N places below
	{
		for (int i = 0; i < offset; i++)
		{
			DataType ti;
			const VMValue vi = Pop(ti);
			if (i == 0)
			{
				copyType = ti;
				copyVal = vi;
			}
			if (i < tmpTypes.size())
			{
				tmpTypes.at(i) = ti;
				tmpVals.at(i) = vi;
			}
		}
		Push(copyVal, copyType);
		for (int i = static_cast<int>(std::min(offset, VMStack::k_Size)) - 1; i >= 0; i--)
		{
			Push(tmpVals.at(i), tmpTypes.at(i));
		}
	}
}

void LHVM::Opcode30Line(VMTask& /*task*/, const VMInstruction& /*instruction*/) {}

} // namespace openblack::lhvm

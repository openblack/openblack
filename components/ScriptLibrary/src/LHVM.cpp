/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

/*
 * The layout of a LHVM File is as follows:
 *
 * - 8 byte header, containing
 *         magic number, containing 4 chars "LHVM"
 *         version number - 4 bytes
 *
 * ------------------------ start of variable block ----------------------------
 * - 4 byte header containing:
 *         number of entries - 4 bytes
 * - number of entries * null terminated strings
 *
 * ------------------------ start of code block --------------------------------
 * - 4 byte header containing:
 *         number of entries - 4 bytes
 * - number of entries, each containing:
 *         opcode - 4 bytes
 *         access code - 4 bytes
 *         three data types - 4 bytes each
 *
 * ------------------------ start of auto block --------------------------------
 * - 4 byte header containing:
 *         number of entries - 4 bytes
 * - number of entries, each containing:
 *         id - 4 bytes
 *
 * ------------------------ start of script block ------------------------------
 * - 4 byte header containing:
 *         number of entries - 4 bytes
 * - number of entries, each containing:
 *         script name - null terminated string
 *         file name - null terminated string
 *         script type - 4 bytes
 *         variable offset - 4 bytes
 *         variable block - see above
 *         instruction address - 4 bytes
 *         parameter count - 4 bytes
 *         script id - 4 bytes
 *
 * ------------------------ start of data block --------------------------------
 * - 4 byte header containing:
 *         size of data - 4 bytes
 *         data - size of data
 */

#include <cassert>
#include <cstring>
#include <fstream>
#include <stdexcept>

#include "LHVM.h"
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

	_opcodesImpl[0] = _opcode00_END;
	_opcodesImpl[1] = _opcode01_JZ;
	_opcodesImpl[2] = _opcode02_PUSH;
	_opcodesImpl[3] = _opcode03_POP;
	_opcodesImpl[4] = _opcode04_ADD;
	_opcodesImpl[5] = _opcode05_SYS;
	_opcodesImpl[6] = _opcode06_SUB;
	_opcodesImpl[7] = _opcode07_NEG;
	_opcodesImpl[8] = _opcode08_MUL;
	_opcodesImpl[9] = _opcode09_DIV;

	_opcodesImpl[10] = _opcode10_MOD;
	_opcodesImpl[11] = _opcode11_NOT;
	_opcodesImpl[12] = _opcode12_AND;
	_opcodesImpl[13] = _opcode13_OR;
	_opcodesImpl[14] = _opcode14_EQ;
	_opcodesImpl[15] = _opcode15_NEQ;
	_opcodesImpl[16] = _opcode16_GEQ;
	_opcodesImpl[17] = _opcode17_LEQ;
	_opcodesImpl[18] = _opcode18_GT;
	_opcodesImpl[19] = _opcode19_LT;

	_opcodesImpl[20] = _opcode20_JMP;
	_opcodesImpl[21] = _opcode21_SLEEP;
	_opcodesImpl[22] = _opcode22_EXCEPT;
	_opcodesImpl[23] = _opcode23_CAST;
	_opcodesImpl[24] = _opcode24_CALL;
	_opcodesImpl[25] = _opcode25_ENDEXCEPT;
	_opcodesImpl[26] = _opcode26_RETEXCEPT;
	_opcodesImpl[27] = _opcode27_ITEREXCEPT;
	_opcodesImpl[28] = _opcode28_BRKEXCEPT;
	_opcodesImpl[29] = _opcode29_SWAP;

	_opcodesImpl[30] = _opcode30_LINE;
}

/// Error handling
void LHVM::Fail(const std::string& msg)
{
	throw std::runtime_error("LHVM Error: " + msg);
}

void LHVM::Initialise(NativeFunction* functions, void (*nativeCallEnterCallback)(uint32_t func),
                      void (*nativeCallExitCallback)(uint32_t func), void (*stopTaskCallback)(uint32_t taskNumber),
                      void (*addReference)(uint32_t objid), void (*removeReference)(uint32_t objid))
{
	_functions = functions;
	_nativeCallEnterCallback = nativeCallEnterCallback;
	_nativeCallExitCallback = nativeCallExitCallback;
	_stopTaskCallback = stopTaskCallback;
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
	for (auto name : _variablesNames)
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
	for (auto &scriptId : _auto)
	{
		if (scriptId > 0 && scriptId <= _scripts.size())
		{
			auto& script = _scripts[scriptId + 1];
			StartScript(script.GetName(), 0xFFFFFFFF);
		}
		else
		{
			SignalError(ErrorCode::SCRIPT_ID_NOT_FOUND, scriptId);
		}
	}
}

void LHVM::RestoreState(const std::filesystem::path& filepath) {
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
	for (auto task : file.GetTasks())
	{
		_tasks.emplace(task.id, task);
	}

	_ticks = file.GetTicks();
	_currentLineNumber = file.GetCurrentLineNumber();
	_highestTaskId = file.GetHighestTaskId();
	_highestScriptId = file.GetHighestScriptId();
	_executedInstructions = file.GetScriptInstructionCount();
}

VMValue LHVM::Pop(DataType& type) {
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

void LHVM::Push(float_t value, DataType type)
{
	Push(VMValue {.floatVal = value}, type);
}

void LHVM::Push(int32_t value, DataType type)
{
	Push(VMValue {.intVal = value}, type);
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
	LHVMFile file(LHVMHeader {.magic = k_Magic, .version = LHVMVersion::BlackAndWhite}, _variablesNames, _instructions,
		_auto, _scripts, _data);
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

	LHVMFile file(LHVMHeader {.magic = k_Magic, .version = LHVMVersion::BlackAndWhite}, _variablesNames, _instructions,
				  _auto, _scripts, _data, _mainStack, _variables, tasks, _ticks, _currentLineNumber, _highestTaskId, _highestScriptId,
	              _executedInstructions);
	file.Write(filepath);
}

void LookIn(uint32_t allowedScriptTypesMask)
{
	// TODO

}

uint32_t LHVM::StartScript(std::string name, uint32_t allowedScriptTypesMask)
{
	const auto script = GetScript(name);
	if (script != NULL)
	{
		if (script->GetType() & allowedScriptTypesMask)
		{
			const auto taskNumber = ++_highestTaskId;

			// copy values from current stack to new stack
			VMStack stack {};
			for (unsigned int i = 0; i < script->GetParameterCount(); i++)
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
			const auto& scriptVariables = script->GetVariables();
			std::vector<VMVar> taskVariables;
			taskVariables.reserve(scriptVariables.size());
			for (unsigned int i = 0; i < scriptVariables.size(); i++)
			{
				taskVariables.push_back(VMVar {.type = DataType::FLOAT, .floatVal = 0.0f, .name = scriptVariables.at(i)});
			}

			const auto& task = VMTask(taskVariables, script->GetScriptID(), taskNumber,
			                          script->GetInstructionAddress(), 0, 0, script->GetVariablesOffset(), stack, 0,
			                          VMExceptStruct {.instructionAddress = script->GetInstructionAddress()}, 1, 0, 0, 0, 0,
			                          script->GetName(), script->GetFileName(), script->GetType());

			_tasks.emplace(taskNumber, task);

			return taskNumber;
		}
		else
		{
			SignalError(ErrorCode::NO_SCRIPT_OF_TYPE, name);
		}
		return 0;
	}
	else
	{
		SignalError(ErrorCode::SCRIPT_NAME_NOT_FOUND, name);
		return 0;
	}
}

void LHVM::StopAllTasks()
{
	while (_tasks.size() > 0)
	{
		auto iter = _tasks.begin();
		StopTask((*iter).first);
	}
}

void LHVM::StopScripts(bool filter(std::string name, std::string filename))
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
	if (_tasks.contains(taskNumber))
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
		_tasks.erase(taskNumber);
	}
	else
	{
		SignalError(ErrorCode::TASK_ID_NOT_FOUND, taskNumber);
	}
}

void LHVM::StopTasksOfType(uint32_t typesMask)
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

void LHVM::InvokeStopTaskCallback(const uint32_t taskNumber)
{
	if (_stopTaskCallback != NULL)
	{
		_stopTaskCallback(taskNumber);
	}
}

void LHVM::AddReference(const uint32_t objectId)
{
	if (_addReference != NULL)
	{
		_addReference(objectId);
	}
}

void LHVM::RemoveReference(const uint32_t objectId)
{
	if (_removeReference != NULL)
	{
		_removeReference(objectId);
	}
}

void LHVM::InvokeNativeCallEnterCallback(const uint32_t taskNumber)
{
	if (_nativeCallEnterCallback != NULL)
	{
		_nativeCallEnterCallback(taskNumber);
	}
}

void LHVM::InvokeNativeCallExitCallback(const uint32_t taskNumber)
{
	if (_nativeCallExitCallback != NULL)
	{
		_nativeCallExitCallback(taskNumber);
	}
}

void LHVM::SignalError(const ErrorCode code)
{
	if (_errorCallback != NULL)
	{
		_errorCallback(code, "", 0);
	}
}

void LHVM::SignalError(const ErrorCode code, const uint32_t data)
{
	if (_errorCallback != NULL)
	{
		_errorCallback(code, "", data);
	}
}

void LHVM::SignalError(const ErrorCode code, const std::string& data)
{
	if (_errorCallback != NULL)
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

uint32_t LHVM::GetTicksCount()
{
	return _ticks;
}

void LHVM::PushElaspedTime()
{
	float_t time = GetTicksCount() * 10.0f;
	Push(time, DataType::FLOAT);
}

void LHVM::CpuLoop(VMTask& task)
{
	bool wasInExceptionHandler = task.inExceptionHandler;

	// TODO

}

void LHVM::_opcode00_END(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode01_JZ(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode02_PUSH(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode03_POP(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode04_ADD(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode05_SYS(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode06_SUB(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode07_NEG(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode08_MUL(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode09_DIV(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode10_MOD(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode11_NOT(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode12_AND(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode13_OR(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode14_EQ(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode15_NEQ(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode16_GEQ(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode17_LEQ(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode18_GT(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode19_LT(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode20_JMP(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode21_SLEEP(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode22_EXCEPT(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode23_CAST(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode24_CALL(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode25_ENDEXCEPT(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode26_RETEXCEPT(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode27_ITEREXCEPT(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode28_BRKEXCEPT(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode29_SWAP(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

void LHVM::_opcode30_LINE(const VMTask& task, const VMInstruction& instruction)
{
	// TODO
}

} // namespace openblack::LHVM

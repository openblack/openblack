/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "LHVMFile.h"

namespace openblack::LHVM
{

class LHVM
{
protected:
	static constexpr const std::array<char, 4> k_Magic = {'L', 'H', 'V', 'M'};

	std::vector<std::string> _variablesNames;
	std::vector<VMInstruction> _instructions;
	std::vector<VMScript> _scripts;
	std::vector<uint32_t> _auto;
	std::vector<char> _data;
	VMStack _mainStack {};
	VMTask* _currentTask {NULL};
	VMExceptStruct* _currentExceptStruct {NULL};
	VMStack* _currentStack {NULL};
	std::vector<VMVar> _variables;
	std::map<uint32_t, VMTask> _tasks;
	uint32_t _ticks {0};
	uint32_t _currentLineNumber {0};
	uint32_t _highestTaskId {0};
	uint32_t _highestScriptId {0};
	uint32_t _executedInstructions {0};

	std::vector<NativeFunction>* _functions {NULL};
	std::function<void(const uint32_t func)> _nativeCallEnterCallback;
	std::function<void(const uint32_t func)> _nativeCallExitCallback;
	std::function<void(const uint32_t taskNumber)> _stopTaskCallback;
	std::function<void(const ErrorCode code, const std::string v0, const uint32_t v1)> _errorCallback;
	std::function<void(const uint32_t objId)> _addReference;
	std::function<void(const uint32_t objId)> _removeReference;

	void (LHVM::*_opcodesImpl[31])(VMTask& task, const VMInstruction& instruction) {};
	void _opcode00_END(VMTask& task, const VMInstruction& instruction);
	void _opcode01_JZ(VMTask& task, const VMInstruction& instruction);
	void _opcode02_PUSH(VMTask& task, const VMInstruction& instruction);
	void _opcode03_POP(VMTask& task, const VMInstruction& instruction);
	void _opcode04_ADD(VMTask& task, const VMInstruction& instruction);
	void _opcode05_SYS(VMTask& task, const VMInstruction& instruction);
	void _opcode06_SUB(VMTask& task, const VMInstruction& instruction);
	void _opcode07_NEG(VMTask& task, const VMInstruction& instruction);
	void _opcode08_MUL(VMTask& task, const VMInstruction& instruction);
	void _opcode09_DIV(VMTask& task, const VMInstruction& instruction);
	void _opcode10_MOD(VMTask& task, const VMInstruction& instruction);
	void _opcode11_NOT(VMTask& task, const VMInstruction& instruction);
	void _opcode12_AND(VMTask& task, const VMInstruction& instruction);
	void _opcode13_OR(VMTask& task, const VMInstruction& instruction);
	void _opcode14_EQ(VMTask& task, const VMInstruction& instruction);
	void _opcode15_NEQ(VMTask& task, const VMInstruction& instruction);
	void _opcode16_GEQ(VMTask& task, const VMInstruction& instruction);
	void _opcode17_LEQ(VMTask& task, const VMInstruction& instruction);
	void _opcode18_GT(VMTask& task, const VMInstruction& instruction);
	void _opcode19_LT(VMTask& task, const VMInstruction& instruction);
	void _opcode20_JMP(VMTask& task, const VMInstruction& instruction);
	void _opcode21_SLEEP(VMTask& task, const VMInstruction& instruction);
	void _opcode22_EXCEPT(VMTask& task, const VMInstruction& instruction);
	void _opcode23_CAST(VMTask& task, const VMInstruction& instruction);
	void _opcode24_CALL(VMTask& task, const VMInstruction& instruction);
	void _opcode25_ENDEXCEPT(VMTask& task, const VMInstruction& instruction);
	void _opcode26_RETEXCEPT(VMTask& task, const VMInstruction& instruction);
	void _opcode27_ITEREXCEPT(VMTask& task, const VMInstruction& instruction);
	void _opcode28_BRKEXCEPT(VMTask& task, const VMInstruction& instruction);
	void _opcode29_SWAP(VMTask& task, const VMInstruction& instruction);
	void _opcode30_LINE(VMTask& task, const VMInstruction& instruction);

	/// Error handling
	void Fail(const std::string& msg);

	void InvokeNativeCallEnterCallback(const uint32_t taskNumber);
	void InvokeNativeCallExitCallback(const uint32_t taskNumber);
	void InvokeStopTaskCallback(const uint32_t taskNumber);
	void SignalError(const ErrorCode code);
	void SignalError(const ErrorCode code, const uint32_t data);
	void SignalError(const ErrorCode code, const std::string& data);

	void AddReference(const uint32_t objectId);
	void RemoveReference(const uint32_t objectId);

	uint32_t StartScript(const uint32_t id);
	uint32_t StartScript(const VMScript& script);
	const VMScript* GetScript(const std::string& name);
	bool TaskExists(const uint32_t taskId);
	uint32_t GetTicksCount();
	void PushElaspedTime();
	VMVar& GetVar(VMTask& task, const uint32_t id);
	void PushExceptionHandler(const uint32_t address);
	void PopExceptionHandler();
	uint32_t GetExceptionHandlersCount();
	uint32_t GetCurrentExceptionHandlerIp(const uint32_t index);

	void PrintInstruction(const VMTask& task, const VMInstruction& instruction);
	void CpuLoop(VMTask& task);

public:
	LHVM();

	~LHVM() = default;

	/// Set environment
	void Initialise(
	    std::vector<NativeFunction>* functions, std::function<void(const uint32_t func)> nativeCallEnterCallback,
	    std::function<void(const uint32_t func)> nativeCallExitCallback,
	    std::function<void(const uint32_t taskNumber)> stopTaskCallback,
	    std::function<void(const ErrorCode code, [[maybe_unused]] const std::string v0, [[maybe_unused]] const uint32_t v1)>
	        errorCallback,
	    std::function<void(const uint32_t objId)> addReference, std::function<void(const uint32_t objId)> removeReference);

	/// Read CHL file from the filesystem
	void LoadBinary(const std::filesystem::path& filepath);

	/// Read SAV file from the filesystem
	void RestoreState(const std::filesystem::path& filepath);

	VMValue Pop(DataType& type);
	VMValue Pop();
	float Popf();
	void Push(VMValue value, DataType type);
	void Pushf(float value);
	void Pushv(float value);
	void Pushi(int32_t value);
	void Pusho(uint32_t value);
	void Pushb(bool value);

	void Reboot();

	/// Write CHL file to filesystem
	void SaveBinary(const std::filesystem::path& filepath);

	/// Write SAV file to filesystem
	void SaveState(const std::filesystem::path& filepath);

	void LookIn(const ScriptType allowedScriptTypesMask);

	uint32_t StartScript(const std::string& name, const ScriptType allowedScriptTypesMask);

	void StopAllTasks();

	void StopScripts(
	    std::function<bool(const std::string& name, const std::string& filename)> filter);

	void StopTask(const uint32_t taskNumber);

	void StopTasksOfType(const ScriptType typesMask);

	[[nodiscard]] const std::string GetString(uint32_t offset);
	[[nodiscard]] const std::vector<NativeFunction>* GetFunctions() const { return _functions; };

	[[nodiscard]] const std::vector<VMVar>& GetVariables() const { return _variables; }
	[[nodiscard]] const std::vector<VMInstruction>& GetInstructions() const { return _instructions; }
	[[nodiscard]] const std::vector<VMScript>& GetScripts() const { return _scripts; }
	[[nodiscard]] const std::map<uint32_t, VMTask>& GetTasks() const { return _tasks; }
	[[nodiscard]] const std::vector<char>& GetData() const { return _data; }
};

} // namespace openblack::LHVM

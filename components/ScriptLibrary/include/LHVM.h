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
#include <string>
#include <vector>
#include <map>

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
	std::vector<uint8_t> _data;
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

	std::vector<NativeFunction*> _functions {};
	void (*_nativeCallEnterCallback)(const uint32_t func) {NULL};
	void (*_nativeCallExitCallback)(const uint32_t func) {NULL};
	void (*_stopTaskCallback)(const uint32_t taskNumber) {NULL};
	void (*_errorCallback)(const ErrorCode code, const std::string v0, uint32_t v1) {NULL};
	void (*_addReference)(const uint32_t objid) {NULL};
	void (*_removeReference)(const uint32_t objid) {NULL};

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
	uint32_t GetTicksCount();
	void PushElaspedTime();
	VMVar& GetVar(VMTask& task, const uint32_t id);
	void PushExceptionHandler(const uint32_t address);
	void PopExceptionHandler();
	uint32_t GetExceptionHandlersCount();
	uint32_t GetCurrentExceptionHandlerIp(const uint32_t index);

	void CpuLoop(VMTask& task);

public:
	LHVM();

	~LHVM() = default;

	/// Set environment
	void Initialise(const std::vector<NativeFunction*>& functions, void (*nativeCallEnterCallback)(uint32_t func),
	                void (*nativeCallExitCallback)(uint32_t func), void (*stopTaskCallback)(uint32_t taskNumber),
	                void (*errorCallback)(const ErrorCode code, const std::string v0, uint32_t v1),
	                void (*addReference)(uint32_t objid), void (*removeReference)(uint32_t objid));

	/// Read CHL file from the filesystem
	void LoadBinary(const std::filesystem::path& filepath);

	/// Read SAV file from the filesystem
	void RestoreState(const std::filesystem::path& filepath);

	VMValue Pop(DataType &type);
	VMValue Pop();
	void Push(VMValue value, DataType type);
	void Pushf(float_t value);
	void Pushv(float_t value);
	void Pushi(int32_t value);
	void Pushb(bool value);

	void Reboot();

	void SaveBinary(const std::filesystem::path& filepath);

	void SaveState(const std::filesystem::path& filepath);

	void LookIn(uint32_t allowedScriptTypesMask);

	uint32_t StartScript(const std::string& name, const uint32_t allowedScriptTypesMask);

	void StopAllTasks();

	void StopScripts(bool filter(std::string name, std::string filename));

	void StopTask(uint32_t taskNumber);

	void StopTasksOfType(uint32_t typesMask);

	[[nodiscard]] const std::vector<VMVar>& GetVariables() const { return _variables; }
	[[nodiscard]] const std::vector<VMInstruction>& GetInstructions() const { return _instructions; }
	[[nodiscard]] const std::vector<VMScript>& GetScripts() const { return _scripts; }
	[[nodiscard]] const std::map<uint32_t, VMTask>& GetTasks() const { return _tasks; }
	[[nodiscard]] const std::vector<uint8_t>& GetData() const { return _data; }
};

} // namespace openblack::LHVM

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
	VMStack* _currentStack {NULL};
	std::vector<VMVar> _variables;
	std::map<uint32_t, VMTask> _tasks;
	uint32_t _ticks {0};
	uint32_t _currentLineNumber {0};
	uint32_t _highestTaskId {0};
	uint32_t _highestScriptId {0};
	uint32_t _executedInstructions {0};

	NativeFunction* _functions {NULL};
	void (*_nativeCallEnterCallback)(const uint32_t func) {NULL};
	void (*_nativeCallExitCallback)(const uint32_t func) {NULL};
	void (*_stopTaskCallback)(const uint32_t taskNumber) {NULL};
	void (*_errorCallback)(const ErrorCode code, const std::string v0, uint32_t v1) {NULL};
	void (*_addReference)(const uint32_t objid) {NULL};
	void (*_removeReference)(const uint32_t objid) {NULL};

	void (*_opcodesImpl[31])(const VMTask& task, const VMInstruction& instruction) {};
	static void _opcode00_END(const VMTask& task, const VMInstruction& instruction);
	static void _opcode01_JZ(const VMTask& task, const VMInstruction& instruction);
	static void _opcode02_PUSH(const VMTask& task, const VMInstruction& instruction);
	static void _opcode03_POP(const VMTask& task, const VMInstruction& instruction);
	static void _opcode04_ADD(const VMTask& task, const VMInstruction& instruction);
	static void _opcode05_SYS(const VMTask& task, const VMInstruction& instruction);
	static void _opcode06_SUB(const VMTask& task, const VMInstruction& instruction);
	static void _opcode07_NEG(const VMTask& task, const VMInstruction& instruction);
	static void _opcode08_MUL(const VMTask& task, const VMInstruction& instruction);
	static void _opcode09_DIV(const VMTask& task, const VMInstruction& instruction);
	static void _opcode10_MOD(const VMTask& task, const VMInstruction& instruction);
	static void _opcode11_NOT(const VMTask& task, const VMInstruction& instruction);
	static void _opcode12_AND(const VMTask& task, const VMInstruction& instruction);
	static void _opcode13_OR(const VMTask& task, const VMInstruction& instruction);
	static void _opcode14_EQ(const VMTask& task, const VMInstruction& instruction);
	static void _opcode15_NEQ(const VMTask& task, const VMInstruction& instruction);
	static void _opcode16_GEQ(const VMTask& task, const VMInstruction& instruction);
	static void _opcode17_LEQ(const VMTask& task, const VMInstruction& instruction);
	static void _opcode18_GT(const VMTask& task, const VMInstruction& instruction);
	static void _opcode19_LT(const VMTask& task, const VMInstruction& instruction);
	static void _opcode20_JMP(const VMTask& task, const VMInstruction& instruction);
	static void _opcode21_SLEEP(const VMTask& task, const VMInstruction& instruction);
	static void _opcode22_EXCEPT(const VMTask& task, const VMInstruction& instruction);
	static void _opcode23_CAST(const VMTask& task, const VMInstruction& instruction);
	static void _opcode24_CALL(const VMTask& task, const VMInstruction& instruction);
	static void _opcode25_ENDEXCEPT(const VMTask& task, const VMInstruction& instruction);
	static void _opcode26_RETEXCEPT(const VMTask& task, const VMInstruction& instruction);
	static void _opcode27_ITEREXCEPT(const VMTask& task, const VMInstruction& instruction);
	static void _opcode28_BRKEXCEPT(const VMTask& task, const VMInstruction& instruction);
	static void _opcode29_SWAP(const VMTask& task, const VMInstruction& instruction);
	static void _opcode30_LINE(const VMTask& task, const VMInstruction& instruction);

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

	const VMScript* GetScript(const std::string& name);
	uint32_t GetTicksCount();
	void PushElaspedTime();

	void CpuLoop(VMTask& task);

public:
	LHVM();

	~LHVM() = default;

	/// Set environment
	void Initialise(NativeFunction* functions, void (*nativeCallEnterCallback)(uint32_t func),
	                void (*nativeCallExitCallback)(uint32_t func), void (*stopTaskCallback)(uint32_t taskNumber),
	                void (*addReference)(uint32_t objid), void (*removeReference)(uint32_t objid));

	/// Read CHL file from the filesystem
	void LoadBinary(const std::filesystem::path& filepath);

	/// Read SAV file from the filesystem
	void RestoreState(const std::filesystem::path& filepath);

	VMValue Pop(DataType &type);
	void Push(VMValue value, DataType type);
	void Push(float_t value, DataType type);
	void Push(int32_t value, DataType type);

	void Reboot();

	void SaveBinary(const std::filesystem::path& filepath);

	void SaveState(const std::filesystem::path& filepath);

	void LookIn(uint32_t allowedScriptTypesMask);

	uint32_t StartScript(std::string name, uint32_t allowedScriptTypesMask);

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

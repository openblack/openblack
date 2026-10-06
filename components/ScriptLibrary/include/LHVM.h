/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
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
#include <set>
#include <string>
#include <vector>

#include "LHVMFile.h"

namespace openblack::lhvm
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
	VMStack _mainStack;
	VMTask* _currentTask {nullptr};
	VMStack* _currentStack {nullptr};
	std::vector<VMVar> _variables;
	std::map<uint32_t, VMTask> _tasks;
	uint32_t _ticks {0};
	uint32_t _currentLineNumber {0};
	uint32_t _highestTaskId {0};
	uint32_t _highestScriptId {0};
	uint32_t _executedInstructions {0};

	/// The debugger's breakpoints, by instruction address; the held tasks, each with the instructions it may still run
	/// before it is held again; and the address each continued task starts from, which its breakpoint lets it past once
	std::set<uint32_t> _breakpoints;
	std::map<uint32_t, uint32_t> _heldTasks;
	std::map<uint32_t, uint32_t> _resumeFrom;

	const std::vector<NativeFunction>* _functions {nullptr};
	std::function<void(const uint32_t func)> _nativeCallEnterCallback;
	std::function<void(const uint32_t func)> _nativeCallExitCallback;
	std::function<void(const uint32_t taskNumber)> _stopTaskCallback;
	std::function<void(const ErrorCode code, const std::string v0, const uint32_t v1)> _errorCallback;
	std::function<void(const uint32_t objId)> _addReference;
	std::function<void(const uint32_t objId)> _removeReference;

	std::array<void (LHVM::*)(VMTask& task, const VMInstruction& instruction), static_cast<size_t>(Opcode::_Count)>
	    _opcodesImpl {};
	void Opcode00End(VMTask& task, const VMInstruction& instruction);
	void Opcode01Jz(VMTask& task, const VMInstruction& instruction);
	void Opcode02Push(VMTask& task, const VMInstruction& instruction);
	void Opcode03Pop(VMTask& task, const VMInstruction& instruction);
	void Opcode04Add(VMTask& task, const VMInstruction& instruction);
	void Opcode05Sys(VMTask& task, const VMInstruction& instruction);
	void Opcode06Sub(VMTask& task, const VMInstruction& instruction);
	void Opcode07Neg(VMTask& task, const VMInstruction& instruction);
	void Opcode08Mul(VMTask& task, const VMInstruction& instruction);
	void Opcode09Div(VMTask& task, const VMInstruction& instruction);
	void Opcode10Mod(VMTask& task, const VMInstruction& instruction);
	void Opcode11Not(VMTask& task, const VMInstruction& instruction);
	void Opcode12And(VMTask& task, const VMInstruction& instruction);
	void Opcode13Or(VMTask& task, const VMInstruction& instruction);
	void Opcode14Eq(VMTask& task, const VMInstruction& instruction);
	void Opcode15Neq(VMTask& task, const VMInstruction& instruction);
	void Opcode16Geq(VMTask& task, const VMInstruction& instruction);
	void Opcode17Leq(VMTask& task, const VMInstruction& instruction);
	void Opcode18Gt(VMTask& task, const VMInstruction& instruction);
	void Opcode19Lt(VMTask& task, const VMInstruction& instruction);
	void Opcode20Jmp(VMTask& task, const VMInstruction& instruction);
	void Opcode21Sleep(VMTask& task, const VMInstruction& instruction);
	void Opcode22Except(VMTask& task, const VMInstruction& instruction);
	void Opcode23Cast(VMTask& task, const VMInstruction& instruction);
	void Opcode24Call(VMTask& task, const VMInstruction& instruction);
	void Opcode25EndExcept(VMTask& task, const VMInstruction& instruction);
	void Opcode26RetExcept(VMTask& task, const VMInstruction& instruction);
	void Opcode27IterExcept(VMTask& task, const VMInstruction& instruction);
	void Opcode28BrkExcept(VMTask& task, const VMInstruction& instruction);
	void Opcode29Swap(VMTask& task, const VMInstruction& instruction);
	void Opcode30Line(VMTask& task, const VMInstruction& instruction);

	void InvokeNativeCallEnterCallback(uint32_t funcId);
	void InvokeNativeCallExitCallback(uint32_t funcId);
	void InvokeStopTaskCallback(uint32_t taskNumber);
	void SignalError(ErrorCode code);
	void SignalError(ErrorCode code, uint32_t data);
	void SignalError(ErrorCode code, const std::string& data);

	void AddReference(uint32_t objectId);
	void RemoveReference(uint32_t objectId);

	uint32_t StartScript(uint32_t id);
	uint32_t StartScript(const VMScript& script);
	const VMScript* GetScript(const std::string& name);
	bool TaskExists(uint32_t taskId);
	uint32_t GetTicksCount();
	void PushElaspedTime();
	VMVar& GetVar(VMTask& task, uint32_t id);
	uint32_t GetExceptionHandlersCount();
	uint32_t GetCurrentExceptionHandlerIp(uint32_t index);

	void PrintInstruction(const VMTask& task, const VMInstruction& instruction);
	void CpuLoop(VMTask& task);
	/// Whether the debugger stops the task before its next instruction: held, or reaching a breakpoint
	bool DebuggerStops(VMTask& task);
	/// Held with no instruction left to run, so its turns pass it by
	[[nodiscard]] bool IsParked(uint32_t taskNumber) const;

	static float Fmod(float a, float b);

public:
	LHVM();

	~LHVM();

	/// Set environment
	void Initialise(
	    const std::vector<NativeFunction>* functions, std::function<void(uint32_t func)> nativeCallEnterCallback,
	    std::function<void(uint32_t func)> nativeCallExitCallback, std::function<void(uint32_t taskNumber)> stopTaskCallback,
	    std::function<void(ErrorCode code, [[maybe_unused]] const std::string v0, [[maybe_unused]] uint32_t v1)> errorCallback,
	    std::function<void(uint32_t objId)> addReference, std::function<void(uint32_t objId)> removeReference);

	/// Read CHL file from the filesystem
	int LoadBinary(const std::filesystem::path& filepath);

	int LoadBinary(const std::vector<uint8_t>& buffer);

	int LoadBinary(const LHVMFile& file);

	/// Read SAV file from the filesystem
	int RestoreState(const std::filesystem::path& filepath);

	/// Take a newer build of the loaded program that keeps the existing code where it is and adds to it, as recompiling
	/// a script does: the instructions, scripts and data are replaced and new globals added, while running tasks and
	/// variable values carry on. Tasks of a recompiled script finish its old code; new ones run the new code.
	int UpdateProgram(const LHVMFile& file);

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
	int SaveBinary(const std::filesystem::path& filepath);

	/// Write SAV file to filesystem
	int SaveState(const std::filesystem::path& filepath);

	void LookIn(ScriptType allowedScriptTypesMask);

	uint32_t StartScript(const std::string& name, ScriptType allowedScriptTypesMask);

	void StopAllTasks();

	void StopScripts(std::function<bool(const std::string& name, const std::string& filename)> filter);

	void StopTask(uint32_t taskNumber);

	void StopTasksOfType(ScriptType typesMask);

	[[nodiscard]] std::string GetString(uint32_t offset);
	[[nodiscard]] const std::vector<NativeFunction>* GetFunctions() const { return _functions; };

	[[nodiscard]] const std::vector<VMVar>& GetVariables() const { return _variables; }
	[[nodiscard]] const std::vector<VMInstruction>& GetInstructions() const { return _instructions; }
	[[nodiscard]] const std::vector<VMScript>& GetScripts() const { return _scripts; }
	[[nodiscard]] const std::map<uint32_t, VMTask>& GetTasks() const { return _tasks; }
	/// The number of the task running now, 0 between tasks
	[[nodiscard]] uint32_t GetCurrentTaskNumber() const { return _currentTask != nullptr ? _currentTask->id : 0; }
	[[nodiscard]] const std::vector<char>& GetData() const { return _data; }

	/// Changes a global variable's value, keeping its type
	void SetVariable(uint32_t id, VMValue value);
	/// Changes one of a task's local variables, by its place among them, keeping its type
	void SetTaskVariable(uint32_t taskNumber, size_t index, VMValue value);

	/// The debugger. A breakpoint holds a task before the instruction at its address runs. A held task sits out the VM's
	/// turns, as if time stood still for it, until it is stepped one instruction at a time or continued.
	void SetBreakpoint(uint32_t address, bool enabled);
	[[nodiscard]] const std::set<uint32_t>& GetBreakpoints() const { return _breakpoints; }
	void HoldTask(uint32_t taskNumber);
	/// Lets a held task run one more instruction at its next turn
	void StepTask(uint32_t taskNumber);
	/// Lets a held task run on, past the breakpoint it waits at
	void ContinueTask(uint32_t taskNumber);
	[[nodiscard]] bool IsTaskHeld(uint32_t taskNumber) const { return _heldTasks.contains(taskNumber); }
};

} // namespace openblack::lhvm

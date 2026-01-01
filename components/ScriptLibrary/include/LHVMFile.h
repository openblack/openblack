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
#include <string>
#include <vector>

#include "LHVMTypes.h"

namespace openblack::lhvm
{

class LHVMFile
{
protected:
	static constexpr const std::array<char, 4> k_Magic = {'L', 'H', 'V', 'M'};

	/// True when a file has been loaded
	bool _isLoaded {false};

	LHVMVersion _version;
	std::vector<std::string> _variablesNames;
	std::vector<VMInstruction> _instructions;
	std::vector<uint32_t> _autostart;
	std::vector<VMScript> _scripts;
	std::vector<char> _data;
	bool _hasStatus {false};
	VMStack _stack;
	std::vector<VMVar> _variableValues;
	std::vector<VMTask> _tasks;
	uint32_t _ticks {0};
	uint32_t _currentLineNumber {0};
	uint32_t _highestTaskId {0};
	uint32_t _highestScriptId {0};
	uint32_t _executedInstructions {0};

	/// Read file from the input source
	void ReadFile(std::istream& stream);

	int LoadVariablesNames(std::istream& stream, std::vector<std::string>& variables);
	int LoadCode(std::istream& stream);
	int LoadAuto(std::istream& stream);
	int LoadScripts(std::istream& stream);
	int LoadScript(std::istream& stream, VMScript& script);
	int LoadData(std::istream& stream);
	int LoadStatus(std::istream& stream);
	int LoadStack(std::istream& stream, VMStack& stack);
	int LoadVariableValues(std::istream& stream, std::vector<VMVar>& variables);
	int LoadTasks(std::istream& stream);
	int LoadTask(std::istream& stream, VMTask& task);
	int LoadRuntimeInfo(std::istream& stream);

public:
	LHVMFile();

	LHVMFile(LHVMVersion version, const std::vector<std::string>& variablesNames,
	         const std::vector<VMInstruction>& instructions, const std::vector<uint32_t>& autostart,
	         const std::vector<VMScript>& scripts, const std::vector<char>& data);

	LHVMFile(LHVMVersion version, const std::vector<std::string>& variablesNames,
	         const std::vector<VMInstruction>& instructions, const std::vector<uint32_t>& autostart,
	         const std::vector<VMScript>& scripts, const std::vector<char>& data, const VMStack& stack,
	         const std::vector<VMVar>& variableValues, const std::vector<VMTask>& tasks, uint32_t ticks,
	         uint32_t currentLineNumber, uint32_t highestTaskId, uint32_t highestScriptId, uint32_t executedInstructions);

	~LHVMFile();

	/// Read lhvm file from the filesystem
	void Open(const std::filesystem::path& filepath);

	/// Read lhvm file from a buffer
	void Open(const std::vector<uint8_t>& buffer);

	void Write(const std::filesystem::path& filepath);

	[[nodiscard]] bool IsLoaded() const { return _isLoaded; }
	[[nodiscard]] LHVMVersion GetVersion() const { return _version; }
	[[nodiscard]] const std::vector<std::string>& GetVariablesNames() const { return _variablesNames; }
	[[nodiscard]] const std::vector<VMInstruction>& GetInstructions() const { return _instructions; }
	[[nodiscard]] const std::vector<uint32_t>& GetAutostart() const { return _autostart; }
	[[nodiscard]] const std::vector<VMScript>& GetScripts() const { return _scripts; }
	[[nodiscard]] const std::vector<char>& GetData() const { return _data; }
	[[nodiscard]] bool HasStatus() const { return _hasStatus; }
	[[nodiscard]] const VMStack& GetStack() const { return _stack; }
	[[nodiscard]] const std::vector<VMVar>& GetVariablesValues() const { return _variableValues; }
	[[nodiscard]] const std::vector<VMTask>& GetTasks() const { return _tasks; }
	[[nodiscard]] uint32_t GetTicks() const { return _ticks; }
	[[nodiscard]] uint32_t GetCurrentLineNumber() const { return _currentLineNumber; }
	[[nodiscard]] uint32_t GetHighestTaskId() const { return _highestTaskId; }
	[[nodiscard]] uint32_t GetHighestScriptId() const { return _highestScriptId; }
	[[nodiscard]] uint32_t GetExecutedInstructions() const { return _executedInstructions; }
};

} // namespace openblack::lhvm

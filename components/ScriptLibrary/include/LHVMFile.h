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

#include "LHVMTypes.h"
#include "VMScript.h"

namespace openblack::LHVM
{

struct LHVMHeader
{
	std::array<char, 4> magic;
	LHVMVersion version;
};

class LHVMFile
{
protected:
	static constexpr const std::array<char, 4> k_Magic = {'L', 'H', 'V', 'M'};

	/// True when a file has been loaded
	bool _isLoaded {false};

	std::filesystem::path _filename;

	LHVMHeader _header;
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

	/// Error handling
	void Fail(const std::string& msg);

	/// Read file from the input source
	void ReadFile(std::istream& stream);

	void LoadVariablesNames(std::istream& stream, std::vector<std::string>& variables);
	void LoadCode(std::istream& stream);
	void LoadAuto(std::istream& stream);
	void LoadScripts(std::istream& stream);
	const VMScript LoadScript(std::istream& stream);
	void LoadData(std::istream& stream);
	bool LoadStatus(std::istream& stream);
	bool LoadStack(std::istream& stream, VMStack& stack);
	void LoadVariableValues(std::istream& stream, std::vector<VMVar>& variables);
	void LoadTasks(std::istream& stream);
	const VMTask LoadTask(std::istream& stream);
	void LoadRuntimeInfo(std::istream& stream);

public:
	LHVMFile()
	    : _header {.magic = {' ', ' ', ' ', ' '}, .version = LHVMVersion::BlackAndWhite}
	    , _hasStatus(false)
	    , _stack {.count = 0, .values = {}, .types = {}, .pushCount = 0, .popCount = 0}
	    , _ticks(0)
	    , _currentLineNumber(0)
	    , _highestTaskId(0)
	    , _highestScriptId(0)
	    , _executedInstructions(0) {};

	LHVMFile(const LHVMHeader& header, const std::vector<std::string>& variablesNames,
	         const std::vector<VMInstruction>& instructions, const std::vector<uint32_t>& autostart,
	         const std::vector<VMScript>& scripts, const std::vector<char>& data)
	    : _isLoaded(true)
	    , _header(header)
	    , _variablesNames(variablesNames)
	    , _instructions(instructions)
	    , _autostart(autostart)
	    , _scripts(scripts)
	    , _data(data)
	    , _hasStatus(false)
	    , _stack {.count = 0, .values = {}, .types = {}, .pushCount = 0, .popCount = 0}
	    , _ticks(0)
	    , _currentLineNumber(0)
	    , _highestTaskId(0)
	    , _highestScriptId(0)
	    , _executedInstructions(0) {};

	LHVMFile(const LHVMHeader& header, const std::vector<std::string>& variablesNames,
	         const std::vector<VMInstruction>& instructions, const std::vector<uint32_t>& autostart,
	         const std::vector<VMScript>& scripts, const std::vector<char>& data, const VMStack& stack,
	         const std::vector<VMVar>& variableValues, const std::vector<VMTask>& tasks, const uint32_t ticks,
	         const uint32_t currentLineNumber, const uint32_t highestTaskId, const uint32_t highestScriptId,
	         const uint32_t executedInstructions)
	    : _isLoaded(true)
	    , _header(header)
	    , _variablesNames(variablesNames)
	    , _instructions(instructions)
	    , _autostart(autostart)
	    , _scripts(scripts)
	    , _data(data)
	    , _hasStatus(true)
	    , _stack(stack)
	    , _variableValues(variableValues)
	    , _tasks(tasks)
	    , _ticks(ticks)
	    , _currentLineNumber(currentLineNumber)
	    , _highestTaskId(highestTaskId)
	    , _highestScriptId(highestScriptId)
	    , _executedInstructions(executedInstructions) {};

	~LHVMFile() = default;

	/// Read lhvm file from the filesystem
	void Open(const std::filesystem::path& filepath);

	/// Read lhvm file from a buffer
	void Open(const std::vector<uint8_t>& buffer);

	void Write(const std::filesystem::path& filepath);

	[[nodiscard]] const LHVMHeader& GetHeader() const { return _header; }
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

} // namespace openblack::LHVM

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
 * - arbitrary size data based on size in head
 *
 * ------------------ start of optional runtime data (.sav) --------------------
 *
 * ------------------------ start of stack block -------------------------------
 * - 12 byte header containing:
 *         number of values/types - 4 bytes
 *         push count - 4 bytes
 *         pop count - 4 bytes
 * - number of values * 4 bytes (can be either float, int or uint)
 * - number of types * 4 bytes
 *
 * ----------------------- start of vars values block --------------------------
 * - 4 byte header containing:
 *         number of entries - 4 bytes
 * - number of entries, each containing:
 *         type - 4 bytes
 *         values - 4 bytes
 *         name - null terminated string
 *
 * -------------------------- start of tasks block -----------------------------
 * - 4 byte header containing:
 *         number of entries - 4 bytes
 * - number of entries, each containing:
 *         local vars block - see above
 *         task number - 4 bytes
 *         current instruction address - 4 bytes
 *         previous instruction address - 4 bytes
 *         waiting task number - 4 bytes
 *         variable offset - 4 bytes
 *         current exception handler index - 4 bytes
 *         clock ticks - 4 bytes
 *         script id - 4 bytes
 *         script type - 4 bytes
 *         is in exception handler - 1 byte
 *         must stop - 1 byte
 *         must stop exception handler - 1 byte
 *         is sleeping - 1 byte
 *         stack - see above
 *         number of active exception handlers - 4 bytes
 *         number of active exception handlers * 4 bytes
 *
 * -------------------------- start of status block ----------------------------
 * - 20 byte header containing:
 *         clock ticks - 4 bytes
 *         current line number - 4 bytes
 *         highest task number - 4 bytes
 *         highest script id - 4 bytes
 *         script instruction count - 4 bytes
 */

#include "LHVMFile.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <fstream>
#include <spanstream>

#include "LHVMTypes.h"

using namespace openblack::lhvm;

LHVMFile::LHVMFile()
    : _version(LHVMVersion::BlackAndWhite) {};

LHVMFile::LHVMFile(const LHVMVersion version, const std::vector<std::string>& variablesNames,
                   const std::vector<VMInstruction>& instructions, const std::vector<uint32_t>& autostart,
                   const std::vector<VMScript>& scripts, const std::vector<char>& data)
    : _isLoaded(true)
    , _version(version)
    , _variablesNames(variablesNames)
    , _instructions(instructions)
    , _autostart(autostart)
    , _scripts(scripts)
    , _data(data) {};

LHVMFile::LHVMFile(const LHVMVersion version, const std::vector<std::string>& variablesNames,
                   const std::vector<VMInstruction>& instructions, const std::vector<uint32_t>& autostart,
                   const std::vector<VMScript>& scripts, const std::vector<char>& data, const VMStack& stack,
                   const std::vector<VMVar>& variableValues, const std::vector<VMTask>& tasks, uint32_t ticks,
                   uint32_t currentLineNumber, uint32_t highestTaskId, uint32_t highestScriptId, uint32_t executedInstructions)
    : _isLoaded(true)
    , _version(version)
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

LHVMFile::~LHVMFile() = default;

void LHVMFile::ReadFile(std::istream& stream)
{
	assert(!_isLoaded);

	// Total file size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fsize < 8)
	{
		return; // File too small to be a valid LHVM file.
	}

	// First 8 bytes
	std::array<char, 4> magic;
	stream.read(reinterpret_cast<char*>(magic.data()), 4);
	if (magic != k_Magic)
	{
		return; // Unrecognized LHVM header
	}

	stream.read(reinterpret_cast<char*>(&_version), sizeof(_version));
	/* only support bw1 at the moment */
	if (_version != LHVMVersion::BlackAndWhite)
	{
		return; // Unsupported LHVM version
	}

	if (LoadVariablesNames(stream, _variablesNames) != EXIT_SUCCESS)
	{
		return;
	}
	if (LoadCode(stream) != EXIT_SUCCESS)
	{
		return;
	}
	if (LoadAuto(stream) != EXIT_SUCCESS)
	{
		return;
	}
	if (LoadScripts(stream) != EXIT_SUCCESS)
	{
		return;
	}
	if (LoadData(stream) != EXIT_SUCCESS)
	{
		return;
	}

	// VM status data (.sav files only)
	if (LoadStatus(stream) != EXIT_SUCCESS)
	{
		return;
	}

	_isLoaded = true;
}

void LHVMFile::Open(const std::filesystem::path& filepath)
{
	assert(!_isLoaded);

	std::ifstream stream(filepath, std::ios::binary);

	if (!stream.is_open())
	{
		return; // Could not open file.
	}

	ReadFile(stream);
}

void LHVMFile::Open(const std::span<const char>& span)
{
	assert(!_isLoaded);

	auto stream = std::ispanstream(span);

	ReadFile(stream);
}

void LHVMFile::Write([[maybe_unused]] const std::filesystem::path& filepath)
{
	assert(_isLoaded);

	// TODO(Daniels118): write static CHL data

	if (_hasStatus)
	{
		// TODO(Daniels118): write runtime status data
	}
}

int LHVMFile::LoadVariablesNames(std::istream& stream, std::vector<std::string>& variables)
{
	int32_t count;

	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count)))
	{
		return EXIT_FAILURE; // Error reading variable count
	}

	// if there are no variables, return
	if (count <= 0)
	{
		return EXIT_SUCCESS;
	}

	// preallocate a buffer to reuse for each variable
	std::array<char, 255> buffer;

	variables.reserve(count);
	for (int32_t i = 0; i < count; i++)
	{
		// reset cur pointer to 0
		char* cur = buffer.data();
		do
		{
			if (!stream.read(cur++, sizeof(*cur)))
			{
				return EXIT_FAILURE; // Error reading variable
			}
		} while (*(cur - 1) != '\0');

		// throw it into the std::string vector
		variables.emplace_back(buffer.data());
	}

	return EXIT_SUCCESS;
}

int LHVMFile::LoadCode(std::istream& stream)
{
	int32_t count;
	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count)))
	{
		return EXIT_FAILURE; // Error reading code count
	}

	// if there are no variables, return
	if (count <= 0)
	{
		return EXIT_SUCCESS;
	}

	_instructions.resize(count);
	if (!stream.read(reinterpret_cast<char*>(_instructions.data()), sizeof(VMInstruction) * _instructions.size()))
	{
		return EXIT_FAILURE; // Error reading instructions
	}

	return EXIT_SUCCESS;
}

int LHVMFile::LoadAuto(std::istream& stream)
{
	int32_t count;
	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count)))
	{
		return EXIT_FAILURE; // error reading id count
	}

	// if there are no auto scripts, return
	if (count <= 0)
	{
		return EXIT_SUCCESS;
	}

	_autostart.resize(count);
	if (!stream.read(reinterpret_cast<char*>(_autostart.data()), sizeof(_autostart[0]) * _autostart.size()))
	{
		return EXIT_FAILURE; // error reading ids
	}

	return EXIT_SUCCESS;
}

int LHVMFile::LoadScripts(std::istream& stream)
{
	int32_t count;
	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count)))
	{
		return EXIT_FAILURE; // error reading script count
	}

	// if there are no variables, return
	if (count <= 0)
	{
		return EXIT_SUCCESS;
	}

	_scripts.reserve(count);
	for (int32_t i = 0; i < count; i++)
	{
		if (LoadScript(stream, _scripts.emplace_back()) != EXIT_SUCCESS)
		{
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

int LHVMFile::LoadScript(std::istream& stream, VMScript& script)
{
	std::array<char, 256> scriptName;
	std::array<char, 256> filename;

	char* cur = scriptName.data();
	do
	{
		if (!stream.read(cur++, sizeof(*cur)))
		{
			return EXIT_FAILURE; // Error script name
		}
	} while (*(cur - 1) != '\0');
	script.name = std::string(scriptName.data());

	cur = filename.data();
	do
	{
		if (!stream.read(cur++, sizeof(*cur)))
		{
			return EXIT_FAILURE; // Error reading script filename
		}
	} while (*(cur - 1) != '\0');
	script.filename = std::string(filename.data());

	if (!stream.read(reinterpret_cast<char*>(&script.type), sizeof(script.type)))
	{
		return EXIT_FAILURE; // Error reading script type
	}

	if (!stream.read(reinterpret_cast<char*>(&script.variablesOffset), sizeof(script.variablesOffset)))
	{
		return EXIT_FAILURE; // Error reading script variables offset
	}

	if (LoadVariablesNames(stream, script.variables) != EXIT_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	if (!stream.read(reinterpret_cast<char*>(&script.instructionAddress), sizeof(script.instructionAddress)))
	{
		return EXIT_FAILURE; // Error reading instruction address
	}

	if (!stream.read(reinterpret_cast<char*>(&script.parameterCount), sizeof(script.parameterCount)))
	{
		return EXIT_FAILURE; // Error reading parameter count
	}

	if (!stream.read(reinterpret_cast<char*>(&script.scriptId), sizeof(script.scriptId)))
	{
		return EXIT_FAILURE; // Error reading script_id
	}

	return EXIT_SUCCESS;
}

int LHVMFile::LoadData(std::istream& stream)
{
	int32_t size;
	if (!stream.read(reinterpret_cast<char*>(&size), sizeof(size)))
	{
		return EXIT_FAILURE; // Error reading data size
	}

	_data.resize(size);
	if (size > 0)
	{
		if (!stream.read(reinterpret_cast<char*>(_data.data()), sizeof(_data[0]) * _data.size()))
		{
			return EXIT_FAILURE; // Error reading data
		}
	}

	return EXIT_SUCCESS;
}

int LHVMFile::LoadStatus(std::istream& stream)
{
	const int rc = LoadStack(stream, _stack);
	if (rc == EXIT_FAILURE)
	{
		return EXIT_FAILURE;
	}
	if (rc == EOF)
	{
		return EXIT_SUCCESS;
	}

	if (LoadVariableValues(stream, _variableValues) != EXIT_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if (LoadTasks(stream) != EXIT_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	if (LoadRuntimeInfo(stream) != EXIT_SUCCESS)
	{
		return EXIT_FAILURE;
	}
	_hasStatus = true;

	return EXIT_SUCCESS;
}

int LHVMFile::LoadStack(std::istream& stream, VMStack& stack)
{
	if (!stream.read(reinterpret_cast<char*>(&stack.count), sizeof(stack.count)))
	{
		if (stream.eof())
		{
			return EOF;
		}
		return EXIT_FAILURE; // Error reading stack count
	}
	if (stack.count > VMStack::k_Size)
	{
		return EXIT_FAILURE; // Invalid stack count
	}

	if (!stream.read(reinterpret_cast<char*>(&stack.pushCount), sizeof(stack.pushCount)))
	{
		return EXIT_FAILURE; // Error reading stack push count
	}

	if (!stream.read(reinterpret_cast<char*>(&stack.popCount), sizeof(stack.popCount)))
	{
		return EXIT_FAILURE; // Error reading stack pop count
	}

	for (int i = 0; i < stack.count; i++)
	{
		if (!stream.read(reinterpret_cast<char*>(&stack.values.at(i)), sizeof(stack.values[0])))
		{
			return EXIT_FAILURE; // Error reading stack values
		}
	}

	for (int i = 0; i < stack.count; i++)
	{
		if (!stream.read(reinterpret_cast<char*>(&stack.types.at(i)), sizeof(stack.types[0])))
		{
			return EXIT_FAILURE; // Error reading stack types
		}
	}

	return EXIT_SUCCESS;
}

int LHVMFile::LoadVariableValues(std::istream& stream, std::vector<VMVar>& variables)
{
	uint32_t count;
	uint8_t type;
	VMValue value;
	std::array<char, 256> name;

	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count)))
	{
		return EXIT_FAILURE; // Error reading variables count
	}

	variables.reserve(count);
	for (int i = 0; i < count; i++)
	{
		if (!stream.read(reinterpret_cast<char*>(&type), sizeof(type)))
		{
			return EXIT_FAILURE; // Error reading variable type
		}

		if (!stream.read(reinterpret_cast<char*>(&value), sizeof(value)))
		{
			return EXIT_FAILURE; // Error reading variable value
		}

		char* cur = name.data();
		do
		{
			if (!stream.read(cur++, sizeof(*cur)))
			{
				return EXIT_FAILURE; // Error reading variable name
			}
		} while (*(cur - 1) != '\0');

		variables.emplace_back(DataType(type), value, std::string(name.data()));
	}

	return EXIT_SUCCESS;
}

int LHVMFile::LoadTasks(std::istream& stream)
{
	uint32_t count;

	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count)))
	{
		return EXIT_FAILURE; // Error reading tasks count
	}

	_tasks.reserve(count);
	for (int i = 0; i < count; i++)
	{
		if (LoadTask(stream, _tasks.emplace_back()) != EXIT_SUCCESS)
		{
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}

int LHVMFile::LoadTask(std::istream& stream, VMTask& task)
{
	if (LoadVariableValues(stream, task.localVars) != EXIT_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	if (!stream.read(reinterpret_cast<char*>(&task.id), sizeof(task.id)))
	{
		return EXIT_FAILURE; // Error reading task number
	}

	if (!stream.read(reinterpret_cast<char*>(&task.instructionAddress), sizeof(task.instructionAddress)))
	{
		return EXIT_FAILURE; // Error reading instruction address
	}

	if (!stream.read(reinterpret_cast<char*>(&task.pevInstructionAddress), sizeof(task.pevInstructionAddress)))
	{
		return EXIT_FAILURE; // Error reading prev instruction address
	}

	if (!stream.read(reinterpret_cast<char*>(&task.waitingTaskId), sizeof(task.waitingTaskId)))
	{
		return EXIT_FAILURE; // Error reading waiting task
	}

	if (!stream.read(reinterpret_cast<char*>(&task.variablesOffset), sizeof(task.variablesOffset)))
	{
		return EXIT_FAILURE; // Error reading var offset
	}

	if (!stream.read(reinterpret_cast<char*>(&task.currentExceptionHandlerIndex), sizeof(task.currentExceptionHandlerIndex)))
	{
		return EXIT_FAILURE; // Error reading current exception handler index
	}

	if (!stream.read(reinterpret_cast<char*>(&task.ticks), sizeof(task.ticks)))
	{
		return EXIT_FAILURE; // Error reading ticks
	}

	if (!stream.read(reinterpret_cast<char*>(&task.scriptId), sizeof(task.scriptId)))
	{
		return EXIT_FAILURE; // Error reading script id
	}

	if (!stream.read(reinterpret_cast<char*>(&task.type), sizeof(task.type)))
	{
		return EXIT_FAILURE; // Error reading type
	}

	if (!stream.read(reinterpret_cast<char*>(&task.inExceptionHandler), sizeof(task.inExceptionHandler)))
	{
		return EXIT_FAILURE; // Error reading 'in exception handler'
	}

	if (!stream.read(reinterpret_cast<char*>(&task.stop), sizeof(task.stop)))
	{
		return EXIT_FAILURE; // Error reading stop
	}

	if (!stream.read(reinterpret_cast<char*>(&task.iield), sizeof(task.iield)))
	{
		return EXIT_FAILURE; // Error reading yield
	}

	if (!stream.read(reinterpret_cast<char*>(&task.sleeping), sizeof(task.sleeping)))
	{
		return EXIT_FAILURE; // Error reading sleeping
	}

	if (LoadStack(stream, task.stack) != EXIT_SUCCESS)
	{
		return EXIT_FAILURE; // Error reading stack
	}

	uint32_t exceptStructCount;
	if (!stream.read(reinterpret_cast<char*>(&exceptStructCount), sizeof(exceptStructCount)))
	{
		return EXIT_FAILURE; // Error reading except struct count
	}
	task.exceptionHandlerIps.resize(exceptStructCount);
	if (!stream.read(reinterpret_cast<char*>(task.exceptionHandlerIps.data()), sizeof(uint32_t) * exceptStructCount))
	{
		return EXIT_FAILURE; // Error reading except struct
	}

	if (task.scriptId < 1 || task.scriptId >= _scripts.size())
	{
		return EXIT_FAILURE; // Script not found
	}
	auto& script = _scripts[task.scriptId - 1];
	task.name = script.name;
	task.filename = script.filename;

	return EXIT_SUCCESS;
}

int LHVMFile::LoadRuntimeInfo(std::istream& stream)
{
	if (!stream.read(reinterpret_cast<char*>(&_ticks), sizeof(_ticks)))
	{
		return EXIT_FAILURE; // Error reading clock ticks
	}

	if (!stream.read(reinterpret_cast<char*>(&_currentLineNumber), sizeof(_currentLineNumber)))
	{
		return EXIT_FAILURE; // Error reading current line number
	}

	if (!stream.read(reinterpret_cast<char*>(&_highestTaskId), sizeof(_highestTaskId)))
	{
		return EXIT_FAILURE; // Error reading highest task id
	}

	if (!stream.read(reinterpret_cast<char*>(&_highestScriptId), sizeof(_highestScriptId)))
	{
		return EXIT_FAILURE; // Error reading highest script id
	}

	if (!stream.read(reinterpret_cast<char*>(&_executedInstructions), sizeof(_executedInstructions)))
	{
		return EXIT_FAILURE; // Error reading script instruction count
	}

	return EXIT_SUCCESS;
}

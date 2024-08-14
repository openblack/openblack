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
#include <cstring>

#include <fstream>
#include <stdexcept>

#include "LHVMTypes.h"

using namespace openblack::lhvm;

namespace
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
} // namespace

/// Error handling
void LHVMFile::Fail(const std::string& msg)
{
	throw std::runtime_error("LHVM file error: " + msg + "\nFilename: " + _filename.string());
}

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
		Fail("File too small to be a valid LHVM file.");
	}

	// First 8 bytes
	std::array<char, 4> magic;
	stream.read(reinterpret_cast<char*>(magic.data()), 4);
	if (magic != k_Magic)
	{
		Fail("Unrecognized LHVM header");
	}

	stream.read(reinterpret_cast<char*>(&_version), sizeof(_version));
	/* only support bw1 at the moment */
	if (_version != LHVMVersion::BlackAndWhite)
	{
		Fail("Unsupported LHVM version");
	}

	LoadVariablesNames(stream, _variablesNames);
	LoadCode(stream);
	LoadAuto(stream);
	LoadScripts(stream);
	LoadData(stream);

	// creature isle
	if (_header.version == LHVMVersion::CreatureIsle)
	{
		// VMTypedVariableInfo::Load((std::_FILE *))
		// there is extra data: 512 loop { 2 int32 } final int32
	}

	// VM status data (.sav files only)
	LoadStatus(stream);

	_isLoaded = true;
}

void LHVMFile::Open(const std::filesystem::path& filepath)
{
	assert(!_isLoaded);

	_filename = filepath;

	std::ifstream stream(_filename, std::ios::binary);

	if (!stream.is_open())
	{
		Fail("Could not open file.");
	}

	ReadFile(stream);
}

void LHVMFile::Open(const std::vector<uint8_t>& buffer)
{
	assert(!_isLoaded);

	imemstream stream(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(buffer[0]));

	// File name set to "buffer" when file is load from a buffer
	// Impact code using L3DFile::GetFilename method
	_filename = std::filesystem::path("buffer");

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

void LHVMFile::LoadVariablesNames(std::istream& stream, std::vector<std::string>& variables)
{
	int32_t count;

	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count)))
	{
		Fail("Error reading variable count");
	}

	// if there are no variables, return
	if (count <= 0)
	{
		return;
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
				Fail("Error reading variable");
			}
		} while (*(cur - 1) != '\0');

		// throw it into the std::string vector
		variables.emplace_back(buffer.data());
	}
}

void LHVMFile::LoadCode(std::istream& stream)
{
	int32_t count;
	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count)))
	{
		Fail("Error reading code count");
	}

	// if there are no variables, return
	if (count <= 0)
	{
		return;
	}

	_instructions.resize(count);
	if (!stream.read(reinterpret_cast<char*>(_instructions.data()), sizeof(VMInstruction) * _instructions.size()))
	{
		Fail("Error reading instructions");
	}
}

void LHVMFile::LoadAuto(std::istream& stream)
{
	int32_t count;
	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count)))
	{
		Fail("error reading id count");
	}

	// if there are no auto scripts, return
	if (count <= 0)
	{
		return;
	}

	std::vector<uint32_t> ids;
	ids.resize(count);
	if (!stream.read(reinterpret_cast<char*>(ids.data()), sizeof(ids[0]) * ids.size()))
	{
		Fail("error reading ids");
	}

	_autostart = ids;
}

void LHVMFile::LoadScripts(std::istream& stream)
{
	int32_t count;
	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count)))
	{
		Fail("error reading script count");
	}

	// if there are no variables, return
	if (count <= 0)
	{
		return;
	}

	_scripts.reserve(count);
	for (int32_t i = 0; i < count; i++)
	{
		_scripts.emplace_back(LoadScript(stream));
	}
}

VMScript LHVMFile::LoadScript(std::istream& stream)
{
	std::array<char, 256> scriptName;
	std::array<char, 256> filename;

	char* cur = scriptName.data();
	do
	{
		if (!stream.read(cur++, sizeof(*cur)))
		{
			Fail("Error script name");
		}
	} while (*(cur - 1) != '\0');

	cur = filename.data();
	do
	{
		if (!stream.read(cur++, sizeof(*cur)))
		{
			Fail("Error reading script filename");
		}
	} while (*(cur - 1) != '\0');

	ScriptType scriptType;
	if (!stream.read(reinterpret_cast<char*>(&scriptType), sizeof(scriptType)))
	{
		Fail("Error reading script type");
	}

	uint32_t varOffset;
	if (!stream.read(reinterpret_cast<char*>(&varOffset), sizeof(varOffset)))
	{
		Fail("Error reading script variables offset");
	}

	std::vector<std::string> variables;
	LoadVariablesNames(stream, variables);

	uint32_t instructionAddress;
	if (!stream.read(reinterpret_cast<char*>(&instructionAddress), sizeof(instructionAddress)))
	{
		Fail("Error reading instruction address");
	}

	uint32_t parameterCount;
	if (!stream.read(reinterpret_cast<char*>(&parameterCount), sizeof(parameterCount)))
	{
		Fail("Error reading parameter count");
	}

	uint32_t scriptId;
	if (!stream.read(reinterpret_cast<char*>(&scriptId), sizeof(scriptId)))
	{
		Fail("Error reading script_id");
	}

	return {std::string(scriptName.data()),
	        std::string(filename.data()),
	        scriptType,
	        varOffset,
	        variables,
	        instructionAddress,
	        parameterCount,
	        scriptId};
}

void LHVMFile::LoadData(std::istream& stream)
{
	int32_t size;
	if (!stream.read(reinterpret_cast<char*>(&size), sizeof(size)))
	{
		Fail("Error reading data size");
	}

	_data.resize(size);
	if (size > 0)
	{
		if (!stream.read(reinterpret_cast<char*>(_data.data()), sizeof(_data[0]) * _data.size()))
		{
			Fail("Error reading data");
		}
	}
}

bool LHVMFile::LoadStatus(std::istream& stream)
{
	if (!LoadStack(stream, _stack))
	{
		return false;
	}

	_variableValues = LoadVariableValues(stream);
	LoadTasks(stream);
	LoadRuntimeInfo(stream);
	_hasStatus = true;
	return true;
}

bool LHVMFile::LoadStack(std::istream& stream, VMStack& stack)
{
	if (!stream.read(reinterpret_cast<char*>(&stack.count), sizeof(stack.count)))
	{
		if (stream.eof())
		{
			return false;
		}
		Fail("Error reading stack count");
	}
	if (stack.count > 32)
	{
		Fail("Invalid stack count");
	}

	if (!stream.read(reinterpret_cast<char*>(&stack.pushCount), sizeof(stack.pushCount)))
	{
		Fail("Error reading stack push count");
	}

	if (!stream.read(reinterpret_cast<char*>(&stack.popCount), sizeof(stack.popCount)))
	{
		Fail("Error reading stack pop count");
	}

	for (int i = 0; i < stack.count; i++)
	{
		if (!stream.read(reinterpret_cast<char*>(&stack.values.at(i)), sizeof(stack.values[0])))
		{
			Fail("Error reading stack values");
		}
	}

	for (int i = 0; i < stack.count; i++)
	{
		if (!stream.read(reinterpret_cast<char*>(&stack.types.at(i)), sizeof(stack.types[0])))
		{
			Fail("Error reading stack types");
		}
	}

	return true;
}

std::vector<VMVar> LHVMFile::LoadVariableValues(std::istream& stream)
{
	uint32_t count;
	uint8_t type;
	VMValue value;
	std::array<char, 256> name;

	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count)))
	{
		Fail("Error reading variables count");
	}

	std::vector<VMVar> variables;
	variables.reserve(count);
	for (int i = 0; i < count; i++)
	{
		if (!stream.read(reinterpret_cast<char*>(&type), sizeof(type)))
		{
			Fail("Error reading variable type");
		}

		if (!stream.read(reinterpret_cast<char*>(&value), sizeof(value)))
		{
			Fail("Error reading variable value");
		}

		char* cur = name.data();
		do
		{
			if (!stream.read(cur++, sizeof(*cur)))
			{
				Fail("Error reading variable name");
			}
		} while (*(cur - 1) != '\0');

		variables.emplace_back(DataType(type), value, std::string(name.data()));
	}

	return variables;
}

void LHVMFile::LoadTasks(std::istream& stream)
{
	uint32_t count;

	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count)))
	{
		Fail("Error reading tasks count");
	}

	_tasks.reserve(count);
	for (int i = 0; i < count; i++)
	{
		_tasks.emplace_back(LoadTask(stream));
	}
}

VMTask LHVMFile::LoadTask(std::istream& stream)
{
	VMTask task;

	task.localVars = LoadVariableValues(stream);

	if (!stream.read(reinterpret_cast<char*>(&task.id), sizeof(task.id)))
	{
		Fail("Error reading task number");
	}

	if (!stream.read(reinterpret_cast<char*>(&task.instructionAddress), sizeof(task.instructionAddress)))
	{
		Fail("Error reading instruction address");
	}

	if (!stream.read(reinterpret_cast<char*>(&task.pevInstructionAddress), sizeof(task.pevInstructionAddress)))
	{
		Fail("Error reading prev instruction address");
	}

	if (!stream.read(reinterpret_cast<char*>(&task.waitingTaskId), sizeof(task.waitingTaskId)))
	{
		Fail("Error reading waiting task");
	}

	if (!stream.read(reinterpret_cast<char*>(&task.variablesOffset), sizeof(task.variablesOffset)))
	{
		Fail("Error reading var offset");
	}

	if (!stream.read(reinterpret_cast<char*>(&task.currentExceptionHandlerIndex), sizeof(task.currentExceptionHandlerIndex)))
	{
		Fail("Error reading current exception handler index");
	}

	if (!stream.read(reinterpret_cast<char*>(&task.ticks), sizeof(task.ticks)))
	{
		Fail("Error reading ticks");
	}

	if (!stream.read(reinterpret_cast<char*>(&task.scriptId), sizeof(task.scriptId)))
	{
		Fail("Error reading script id");
	}

	if (!stream.read(reinterpret_cast<char*>(&task.type), sizeof(task.type)))
	{
		Fail("Error reading type");
	}

	if (!stream.read(reinterpret_cast<char*>(&task.inExceptionHandler), sizeof(task.inExceptionHandler)))
	{
		Fail("Error reading 'in exception handler'");
	}

	if (!stream.read(reinterpret_cast<char*>(&task.stop), sizeof(task.stop)))
	{
		Fail("Error reading stop");
	}

	if (!stream.read(reinterpret_cast<char*>(&task.iield), sizeof(task.iield)))
	{
		Fail("Error reading yield");
	}

	if (!stream.read(reinterpret_cast<char*>(&task.sleeping), sizeof(task.sleeping)))
	{
		Fail("Error reading sleeping");
	}

	if (!LoadStack(stream, task.stack))
	{
		Fail("Error reading stack");
	}

	uint32_t exceptStructCount;
	if (!stream.read(reinterpret_cast<char*>(&exceptStructCount), sizeof(exceptStructCount)))
	{
		Fail("Error reading except struct count");
	}
	task.exceptionHandlerIps.resize(exceptStructCount);
	if (!stream.read(reinterpret_cast<char*>(task.exceptionHandlerIps.data()), sizeof(uint32_t) * exceptStructCount))
	{
		Fail("Error reading except struct");
	}

	if (task.scriptId < 1 || task.scriptId >= _scripts.size())
	{
		Fail("Script not found");
	}
	auto& script = _scripts[task.scriptId - 1];
	task.name = script.GetName();
	task.filename = script.GetFileName();

	return task;
}

void LHVMFile::LoadRuntimeInfo(std::istream& stream)
{
	if (!stream.read(reinterpret_cast<char*>(&_ticks), sizeof(_ticks)))
	{
		Fail("Error reading clock ticks");
	}

	if (!stream.read(reinterpret_cast<char*>(&_currentLineNumber), sizeof(_currentLineNumber)))
	{
		Fail("Error reading current line number");
	}

	if (!stream.read(reinterpret_cast<char*>(&_highestTaskId), sizeof(_highestTaskId)))
	{
		Fail("Error reading highest task id");
	}

	if (!stream.read(reinterpret_cast<char*>(&_highestScriptId), sizeof(_highestScriptId)))
	{
		Fail("Error reading highest script id");
	}

	if (!stream.read(reinterpret_cast<char*>(&_executedInstructions), sizeof(_executedInstructions)))
	{
		Fail("Error reading script instruction count");
	}
}

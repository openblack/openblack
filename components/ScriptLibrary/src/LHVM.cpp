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

#include "LHVM/LHVM.h"

#include <cassert>
#include <cstring>

#include <fstream>
#include <stdexcept>

#include "LHVM/OpcodeNames.h"
#include "LHVM/VMInstruction.h"

using namespace openblack::LHVM;

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
void LHVM::Fail(const std::string& msg)
{
	throw std::runtime_error("LLVM Error: " + msg + "\nFilename: " + _filename.string());
}

LHVM::LHVM() = default;
LHVM::~LHVM() = default;

void LHVM::ReadFile(std::istream& stream)
{
	assert(!_isLoaded);

	// Total file size
	std::size_t fsize = 0;
	if (stream.seekg(0, std::ios_base::end))
	{
		fsize = static_cast<std::size_t>(stream.tellg());
		stream.seekg(0);
	}

	if (fsize < sizeof(LHVMHeader))
	{
		Fail("File too small to be a valid LLVM file.");
	}

	// First 8 bytes
	stream.read(reinterpret_cast<char*>(&_header), sizeof(LHVMHeader));
	if (_header.magic != k_Magic)
	{
		Fail("Unrecognized LLVM header");
	}

	/* only support bw1 at the moment */
	if (_header.version != LHVMVersion::BlackAndWhite)
	{
		Fail("Unsupported LHVM version");
	}

	LoadVariables(stream, _variables);
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

	_isLoaded = true;
}

void LHVM::Open(const std::filesystem::path& filepath)
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

void LHVM::Open(const std::vector<uint8_t>& buffer)
{
	assert(!_isLoaded);

	imemstream stream(reinterpret_cast<const char*>(buffer.data()), buffer.size() * sizeof(buffer[0]));

	// File name set to "buffer" when file is load from a buffer
	// Impact code using L3DFile::GetFilename method
	_filename = std::filesystem::path("buffer");

	ReadFile(stream);
}

void LHVM::LoadVariables(std::istream& stream, std::vector<std::string>& variables)
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
	char buffer[255];

	for (int32_t i = 0; i < count; i++)
	{
		// reset cur pointer to 0
		char* cur = &buffer[0];
		do
		{
			if (!stream.read(cur++, sizeof(*cur)))
			{
				Fail("Error reading variable");
			}
		} while (*(cur - 1) != '\0');

		// throw it into the std::string vector
		variables.emplace_back(buffer);
	}
}

void LHVM::LoadCode(std::istream& stream)
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

	for (int32_t i = 0; i < count; i++)
	{
		std::array<uint32_t, 5> instruction; // quick way to minimize code
		if (!stream.read(reinterpret_cast<char*>(instruction.data()), sizeof(instruction[0]) * instruction.size()))
		{
			Fail("Error reading instructions");
		}

		_instructions.emplace_back(static_cast<VMInstruction::Opcode>(instruction[0]),
		                           static_cast<VMInstruction::Access>(instruction[1]),
		                           static_cast<VMInstruction::DataType>(instruction[2]), instruction[3], instruction[4]);
	}
}

void LHVM::LoadAuto(std::istream& stream)
{
	int32_t count;
	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(count)))
	{
		Fail("error reading id count");
	}

	// if there are no variables, return
	if (count <= 0)
	{
		return;
	}

	std::vector<uint32_t> ids(count);

	if (!stream.read(reinterpret_cast<char*>(&count), sizeof(ids[0]) * ids.size()))
	{
		Fail("error reading ids");
	}
}

void LHVM::LoadScripts(std::istream& stream)
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

	char script_name[255];
	char file_name[255];

	for (int32_t i = 0; i < count; i++)
	{
		char* cur = &script_name[0];
		do
		{
			if (!stream.read(cur++, sizeof(*cur)))
			{
				Fail("Error script name");
			}
		} while (*(cur - 1) != '\0');

		cur = &file_name[0];
		do
		{
			if (!stream.read(cur++, sizeof(*cur)))
			{
				Fail("Error reading script filename");
			}
		} while (*(cur - 1) != '\0');

		uint32_t scriptType;
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
		LoadVariables(stream, variables);

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

		_scripts.emplace_back(std::string(script_name), std::string(file_name), scriptType, varOffset, variables,
		                      instructionAddress, parameterCount, scriptId);
	}
}

void LHVM::LoadData(std::istream& stream)
{
	int32_t size;
	if (!stream.read(reinterpret_cast<char*>(&size), sizeof(size)))
	{
		Fail("Error reading data size");
	}

	_data.resize(size);
	if (!stream.read(reinterpret_cast<char*>(_data.data()), sizeof(_data[0]) * _data.size()))
	{
		Fail("Error reading data");
	}
}

/*
    a pretty long method to turn each different opcode into a somewhat
    human readable string
*/
std::string VMInstruction::Disassemble() const
{
	std::string opcodeName = Opcode_Names.at(static_cast<int>(_code));

	return opcodeName;
}

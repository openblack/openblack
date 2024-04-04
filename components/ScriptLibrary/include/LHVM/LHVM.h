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

#include <LHVM/VMInstruction.h>
#include <LHVM/VMScript.h>

namespace openblack::LHVM
{

enum class LHVMVersion : uint32_t
{
	BlackAndWhite = 7,
	CreatureIsle = 8,
	BlackAndWhite2 = 12
};

struct LHVMHeader
{
	std::array<char, 4> magic;
	LHVMVersion version;
};

class LHVM
{
protected:
	static constexpr const std::array<char, 4> k_Magic = {'L', 'H', 'V', 'M'};

	/// True when a file has been loaded
	bool _isLoaded {false};

	std::filesystem::path _filename;

	LHVMHeader _header;
	std::vector<std::string> _variables;
	std::vector<VMInstruction> _instructions;
	std::vector<VMScript> _scripts;
	std::vector<uint8_t> _data;

	/// Error handling
	void Fail(const std::string& msg);

	void LoadVariables(std::istream& stream, std::vector<std::string>& variables);
	void LoadCode(std::istream& stream);
	void LoadAuto(std::istream& stream);
	void LoadScripts(std::istream& stream);
	void LoadData(std::istream& stream);

public:
	LHVM();
	virtual ~LHVM();

	/// Read file from the input source
	virtual void ReadFile(std::istream& stream);

	/// Read lhvm file from the filesystem
	void Open(const std::filesystem::path& filepath);

	/// Read lhvm file from a buffer
	void Open(const std::vector<uint8_t>& buffer);

	[[nodiscard]] const LHVMHeader& GetHeader() const { return _header; }
	[[nodiscard]] const std::vector<std::string>& GetVariables() const { return _variables; }
	[[nodiscard]] const std::vector<VMInstruction>& GetInstructions() const { return _instructions; }
	[[nodiscard]] const std::vector<VMScript>& GetScripts() const { return _scripts; }
	[[nodiscard]] const std::vector<uint8_t>& GetData() const { return _data; }
};

} // namespace openblack::LHVM

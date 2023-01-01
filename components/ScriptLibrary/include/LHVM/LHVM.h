/*****************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <string>
#include <vector>

#include <LHVM/VMInstruction.h>
#include <LHVM/VMScript.h>

namespace openblack::LHVM
{

class LHVM
{
public:
	enum class Version : uint32_t
	{
		BlackAndWhite = 7,
		CreatureIsle = 8,
		BlackAndWhite2 = 12
	};

	LHVM() = default;
	~LHVM() = default;

	void LoadBinary(const std::string& filename);

	[[nodiscard]] const std::vector<std::string>& GetVariables() const { return _variables; }
	[[nodiscard]] const std::vector<VMInstruction>& GetInstructions() const { return _instructions; }
	[[nodiscard]] const std::vector<VMScript>& GetScripts() const { return _scripts; }
	std::vector<uint8_t>& GetData() { return _data; }
	[[nodiscard]] const std::vector<uint8_t>& GetData() const { return _data; }

	[[nodiscard]] Version GetVersion() const { return _version; }

private:
	void LoadVariables(std::FILE* file, std::vector<std::string>& variables);
	void LoadCode(std::FILE* file);
	void LoadAuto(std::FILE* file);
	void LoadScripts(std::FILE* file);
	void LoadData(std::FILE* file);

	Version _version;

	std::vector<std::string> _variables;
	std::vector<VMInstruction> _instructions;
	std::vector<VMScript> _scripts;
	std::vector<uint8_t> _data;
};

} // namespace openblack::LHVM

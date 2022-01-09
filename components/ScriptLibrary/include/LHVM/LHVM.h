/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <LHVM/VMInstruction.h>
#include <LHVM/VMScript.h>
#include <string>
#include <vector>

namespace openblack
{
namespace LHVM
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

	const std::vector<std::string>& GetVariables() const { return _variables; }
	const std::vector<VMInstruction>& GetInstructions() const { return _instructions; }
	const std::vector<VMScript>& GetScripts() const { return _scripts; }
	const std::vector<uint8_t>& GetData() const { return _data; }

	Version GetVersion() const { return _version; }

private:
	void loadVariables(std::FILE* file, std::vector<std::string>& variables);
	void loadCode(std::FILE* file);
	void loadAuto(std::FILE* file);
	void loadScripts(std::FILE* file);
	void loadData(std::FILE* file);

	Version _version;

	std::vector<std::string> _variables;
	std::vector<VMInstruction> _instructions;
	std::vector<VMScript> _scripts;
	std::vector<uint8_t> _data;
};

} // namespace LHVM
} // namespace openblack

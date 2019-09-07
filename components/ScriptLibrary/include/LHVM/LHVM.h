/* OpenBlack - A reimplementation of Lionheads's Black & White engine
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>
#include <vector>

#include <LHVM/VMInstruction.h>
#include <LHVM/VMScript.h>

namespace openblack {
namespace LHVM {

class LHVM {
public:
	enum class Version : uint32_t {
		BlackAndWhite = 7,
		CreatureIsle = 8,
		BlackAndWhite2 = 12
	};

	LHVM() = default;
	~LHVM() = default;

	void LoadBinary(const std::string &filename);

	const std::vector<std::string> &GetVariables() const { return _variables; }
	const std::vector<VMInstruction> &GetInstructions() const { return _instructions; }
	const std::vector<VMScript> &GetScripts() const { return _scripts; }
	const std::vector<uint8_t> &GetData() const { return _data; }

	Version GetVersion() const { return _version; }
private:
	void loadVariables(std::FILE* file, std::vector<std::string> &variables);
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

}
}

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
#ifndef OPENBLACK_LHSCRIPTX_H
#define OPENBLACK_LHSCRIPTX_H

#include <Common/OSFile.h>
#include <Common/Types.h>

#include <list>

namespace OpenBlack
{
	class LHScriptX
	{
	public:
		void LoadFile(std::string fileName);
		bool LoadOneLine(OSFile* file);
		void ScanLine(std::string line);
		int GetNextWord();

	private:
		std::list<std::string> tokenize(std::string str);

		// .DontProcessNumLines__12LHScriptX<c>FR16LHReleasedOSFilePcUl
		// .InitIfLevel__12LHScriptX_c_Fv
		// .LoadOneLine__12LHScriptX<c>FR16LHReleasedOSFilePc
		// .Load__12LHScriptX<c>FPcP19LHScriptCommandX<c>PFlP16LHScriptPramX<c>_9LH_RETURNPl
		// .Load__12LHScriptX<w>FPcP19LHScriptCommandX<w>PFlP16LHScriptPramX<w>_9LH_RETURNPl
		// .ScanLine__12LHScriptX<c>FPc
		// .SetVariables__12LHScriptX<c>FP19LHScriptCommandX<c>PFlP16LHScriptPramX<c>_9LH_RETURNPl

		// .LoadFile__12LHScriptX<c>FPcPl
		// .LoadFile__12LHScriptX<w>FPcPl

		// .LoadOneLine__12LHScriptX<c>FR16LHReleasedOSFilePc
		// .LoadOneLine__12LHScriptX<w>FR16LHReleasedOSFilePw

		// .ProcessCommand__12LHScriptX<c>FlP16LHScriptPramX<c>

		// .ScanLine__12LHScriptX<c>FPc
	};
}


#endif

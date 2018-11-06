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

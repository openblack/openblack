#pragma once
#ifndef OPENBLACK_LHSCRIPTX_IMPL_MAPCOMMANDS_H
#define OPENBLACK_LHSCRIPTX_IMPL_MAPCOMMANDS_H

#include "Command.h"

namespace OpenBlack {

namespace LHScriptX {

class Impl_MapCommands {
public:
	static void setNoPlayers(const ScriptCommandContext &ctx);
	static void loadTribeDance(const ScriptCommandContext &ctx);
	static void setDate(const ScriptCommandContext &ctx);
	static void setTime(const ScriptCommandContext &ctx);
	static void setTurnsPerYear(const ScriptCommandContext &ctx);
	static void setGameTickTime(const ScriptCommandContext &ctx);
	static void loadFeatureScript(const ScriptCommandContext &ctx);
	static void pauseGame(const ScriptCommandContext &ctx);
	static void createCreature(const ScriptCommandContext &ctx);
	static void outputVillagers(const ScriptCommandContext &ctx);
	static void outputTown(const ScriptCommandContext &ctx);
	static void outputCreatures(const ScriptCommandContext &ctx);
	static void outputCollide(const ScriptCommandContext &ctx);
	static void outputAlloc(const ScriptCommandContext &ctx);
	static void saveForNetDebug(const ScriptCommandContext &ctx);
	static void loadForNetDebug(const ScriptCommandContext &ctx);
	static void loadLandscape(const ScriptCommandContext &ctx);
	static void loadGameScript(const ScriptCommandContext &ctx);
	static void loadRawGameScript(const ScriptCommandContext &ctx);
	static void loadLanguage(const ScriptCommandContext &ctx);

	static const ScriptCommandDef Definitions[20];
};

}
}
#endif

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

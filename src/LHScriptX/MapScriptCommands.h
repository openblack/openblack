/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <LHScriptX/CommandSignature.h>
#include <array>

namespace openblack::LHScriptX
{

class MapScriptCommands
{
  public:
	static const std::array<const ScriptCommandSignature, 20> Signatures;

	static void SetNoPlayers(const ScriptCommandContext& ctx);
	static void LoadTribeDance(const ScriptCommandContext& ctx);
	static void SetDate(const ScriptCommandContext& ctx);
	static void SetTime(const ScriptCommandContext& ctx);
	static void SetTurnsPerYear(const ScriptCommandContext& ctx);
	static void SetGameTickTime(const ScriptCommandContext& ctx);
	static void LoadFeatureScript(const ScriptCommandContext& ctx);
	static void PauseGame(const ScriptCommandContext& ctx);
	static void CreateCreature(const ScriptCommandContext& ctx);
	static void OutputVillagers(const ScriptCommandContext& ctx);
	static void OutputTown(const ScriptCommandContext& ctx);
	static void OutputCreatures(const ScriptCommandContext& ctx);
	static void OutputCollide(const ScriptCommandContext& ctx);
	static void OutputAlloc(const ScriptCommandContext& ctx);
	static void SaveForNetDebug(const ScriptCommandContext& ctx);
	static void LoadForNetDebug(const ScriptCommandContext& ctx);
	static void LoadLandscape(const ScriptCommandContext& ctx);
	static void LoadGameScript(const ScriptCommandContext& ctx);
	static void LoadRawGameScript(const ScriptCommandContext& ctx);
	static void LoadLanguage(const ScriptCommandContext& ctx);
};

} // namespace openblack::LHScriptX

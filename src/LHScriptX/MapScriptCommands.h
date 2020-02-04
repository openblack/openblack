/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "CommandSignature.h"

#include <array>

namespace openblack::lhscriptx
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

} // namespace openblack::lhscriptx

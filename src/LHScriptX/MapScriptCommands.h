/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "CommandSignature.h"

#include <array>

#include <glm/vec3.hpp>

namespace openblack::lhscriptx
{

class MapScriptCommands
{
public:
	static const std::array<const ScriptCommandSignature, 20> Signatures;

	static void SetNoPlayers(int32_t number);
	static void LoadTribeDance(glm::vec3 position, int32_t);
	static void SetDate(int32_t, int32_t, int32_t);
	static void SetTime(int32_t, int32_t, int32_t);
	static void SetTurnsPerYear(int32_t turns_per_year);
	static void SetGameTickTime(int32_t game_tick_time);
	static void LoadFeatureScript(glm::vec3);
	static void PauseGame();
	static void CreateCreature(int32_t, int32_t, int32_t, int32_t);
	static void OutputVillagers();
	static void OutputTown();
	static void OutputCreatures();
	static void OutputCollide();
	static void OutputAlloc();
	static void SaveForNetDebug(int32_t, int32_t);
	static void LoadForNetDebug(int32_t, int32_t);
	static void LoadLandscape(glm::vec3);
	static void LoadGameScript(glm::vec3);
	static void LoadRawGameScript(glm::vec3);
	static void LoadLanguage(glm::vec3);
};

} // namespace openblack::lhscriptx

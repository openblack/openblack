/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "MapScriptCommands.h"

#include <array>
#include <stdexcept>
#include <string>

#include "ScriptingBindingUtils.h"

using namespace openblack::lhscriptx;

// alias parameter types for signature list readability
const constexpr ParameterType TString = ParameterType::String;
const constexpr ParameterType TNumber = ParameterType::Number;
const constexpr ParameterType TFloat = ParameterType::Float;
const constexpr ParameterType TVector = ParameterType::Vector;

const std::array<const ScriptCommandSignature, 20> MapScriptCommands::Signatures = {{
    CREATE_COMMAND_BINDING("SET_NO_PLAYERS", MapScriptCommands::SetNoPlayers),
    CREATE_COMMAND_BINDING("LOAD_TRIBE_DANCE", MapScriptCommands::LoadTribeDance),
    CREATE_COMMAND_BINDING("SET_DATE", MapScriptCommands::SetDate),
    CREATE_COMMAND_BINDING("SET_TIME", MapScriptCommands::SetTime),
    CREATE_COMMAND_BINDING("SET_TURNS_PER_YEAR", MapScriptCommands::SetTurnsPerYear),
    CREATE_COMMAND_BINDING("SET_GAME_TICK_TIME", MapScriptCommands::SetGameTickTime),
    CREATE_COMMAND_BINDING("LOAD_FEATURE_SCRIPT", MapScriptCommands::LoadFeatureScript),
    CREATE_COMMAND_BINDING("PAUSE_GAME", MapScriptCommands::PauseGame),
    CREATE_COMMAND_BINDING("CREATE_CREATURE", MapScriptCommands::CreateCreature),
    CREATE_COMMAND_BINDING("OUTPUT_VILLAGERS", MapScriptCommands::OutputVillagers),
    CREATE_COMMAND_BINDING("OUTPUT_TOWN", MapScriptCommands::OutputTown),
    CREATE_COMMAND_BINDING("OUTPUT_CREATURES", MapScriptCommands::OutputCreatures),
    CREATE_COMMAND_BINDING("OUTPUT_COLLIDE", MapScriptCommands::OutputCollide),
    CREATE_COMMAND_BINDING("OUTPUT_ALLOC", MapScriptCommands::OutputAlloc),
    CREATE_COMMAND_BINDING("SAVE_FOR_NET_DEBUG", MapScriptCommands::SaveForNetDebug),
    CREATE_COMMAND_BINDING("LOAD_FOR_NET_DEBUG", MapScriptCommands::LoadForNetDebug),
    CREATE_COMMAND_BINDING("LOAD_LANDSCAPE", MapScriptCommands::LoadLandscape),
    CREATE_COMMAND_BINDING("LOAD_GAME_SCRIPT", MapScriptCommands::LoadGameScript),
    CREATE_COMMAND_BINDING("LOAD_RAW_GAME_SCRIPT", MapScriptCommands::LoadRawGameScript),
    CREATE_COMMAND_BINDING("LOAD_LANGUAGE", MapScriptCommands::LoadLanguage),
}};

void MapScriptCommands::SetNoPlayers([[maybe_unused]] int32_t number)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::LoadTribeDance([[maybe_unused]] glm::vec3 position, int32_t)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::SetDate(int32_t, int32_t, int32_t)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::SetTime(int32_t, int32_t, int32_t)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::SetTurnsPerYear([[maybe_unused]] int32_t turns_per_year)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::SetGameTickTime([[maybe_unused]] int32_t game_tick_time)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::LoadFeatureScript(glm::vec3)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::PauseGame()
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::CreateCreature(int32_t, int32_t, int32_t, int32_t)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::OutputVillagers()
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::OutputTown()
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::OutputCreatures()
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::OutputCollide()
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::OutputAlloc()
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::SaveForNetDebug(int32_t, int32_t)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::LoadForNetDebug(int32_t, int32_t)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::LoadLandscape(glm::vec3)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::LoadGameScript(glm::vec3)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::LoadRawGameScript(glm::vec3)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

void MapScriptCommands::LoadLanguage(glm::vec3)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" +
	                       std::to_string(__LINE__));
}

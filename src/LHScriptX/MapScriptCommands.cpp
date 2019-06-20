/* OpenBlack - A reimplementation of Lionhead's Black & White.
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

#include <LHScriptX/MapScriptCommands.h>

using namespace OpenBlack::LHScriptX;

// alias parameter types for signature list readability
const constexpr ParameterType TString = ParameterType::String;
const constexpr ParameterType TNumber = ParameterType::Number;
const constexpr ParameterType TFloat  = ParameterType::Float;
const constexpr ParameterType TVector = ParameterType::Vector;

// clang-format off
const std::array<const ScriptCommandSignature, 20> MapScriptCommands::Signatures = { {
	{ "SET_NO_PLAYERS",       &MapScriptCommands::SetNoPlayers,      { TNumber }                            },
	{ "LOAD_TRIBE_DANCE",     &MapScriptCommands::LoadTribeDance,    { TVector, TNumber }                   },
	{ "SET_DATE",             &MapScriptCommands::SetDate,           { TNumber, TNumber, TNumber }          },
	{ "SET_TIME",             &MapScriptCommands::SetTime,           { TNumber, TNumber, TNumber }          },
	{ "SET_TURNS_PER_YEAR",   &MapScriptCommands::SetTurnsPerYear,   { TNumber }                            },
	{ "SET_GAME_TICK_TIME",   &MapScriptCommands::SetGameTickTime,   { TNumber }                            },
	{ "LOAD_FEATURE_SCRIPT",  &MapScriptCommands::LoadFeatureScript, { TVector }                            },
	{ "PAUSE_GAME",           &MapScriptCommands::PauseGame,         { }                                    },
	{ "CREATE_CREATURE",      &MapScriptCommands::CreateCreature,    { TNumber, TNumber, TNumber, TNumber } },
	{ "OUTPUT_VILLAGERS",     &MapScriptCommands::OutputVillagers,   { }                                    },
	{ "OUTPUT_TOWN",          &MapScriptCommands::OutputTown,        { }                                    },
	{ "OUTPUT_CREATURES",     &MapScriptCommands::OutputCreatures,   { }                                    },
	{ "OUTPUT_COLLIDE",       &MapScriptCommands::OutputCollide,     { }                                    },
	{ "OUTPUT_ALLOC",         &MapScriptCommands::OutputAlloc,       { }                                    },
	{ "SAVE_FOR_NET_DEBUG",   &MapScriptCommands::SaveForNetDebug,   { TNumber, TNumber }                   },
	{ "LOAD_FOR_NET_DEBUG",   &MapScriptCommands::LoadForNetDebug,   { TNumber }                            },
	{ "LOAD_LANDSCAPE",       &MapScriptCommands::LoadLandscape,     { TVector }                            },
	{ "LOAD_GAME_SCRIPT",     &MapScriptCommands::LoadGameScript,    { TVector }                            },
	{ "LOAD_RAW_GAME_SCRIPT", &MapScriptCommands::LoadRawGameScript, { TVector }                            },
	{ "LOAD_LANGUAGE",        &MapScriptCommands::LoadLanguage,      { }                                    },
}};
// clang-format on

void MapScriptCommands::SetNoPlayers(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::LoadTribeDance(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::SetDate(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::SetTime(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::SetTurnsPerYear(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::SetGameTickTime(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::LoadFeatureScript(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::PauseGame(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::CreateCreature(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::OutputVillagers(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::OutputTown(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::OutputCreatures(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::OutputCollide(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::OutputAlloc(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::SaveForNetDebug(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::LoadForNetDebug(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::LoadLandscape(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::LoadGameScript(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::LoadRawGameScript(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void MapScriptCommands::LoadLanguage(const ScriptCommandContext& ctx)
{
	throw std::logic_error(std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

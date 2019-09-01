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

#include <LHScriptX/FeatureScriptCommands.h>
#include <Game.h>
#include <AllMeshes.h>
#include <3D/LandIsland.h>
#include <Entities/Components/Model.h>
#include <Entities/Components/Transform.h>
#include <Entities/Components/Villager.h>
#include <Entities/Components/Abode.h>
#include <Entities/Registry.h>
#include <spdlog/spdlog.h>
#include <cmath>
#include <iostream>
#include <random>
#include <tuple>


using namespace OpenBlack::LHScriptX;

// alias parameter types for signature list readability
const constexpr ParameterType TString = ParameterType::String;
const constexpr ParameterType TNumber = ParameterType::Number;
const constexpr ParameterType TFloat  = ParameterType::Float;
const constexpr ParameterType TVector = ParameterType::Vector;

// clang-format off
const std::array<const ScriptCommandSignature, 105> FeatureScriptCommands::Signatures = { {
	{ "CREATE_MIST",                       &FeatureScriptCommands::CreateMist,                    { TVector, TFloat, TNumber, TFloat, TFloat }                                      },
	{ "CREATE_PATH",                       &FeatureScriptCommands::CreatePath,                    { TNumber, TNumber, TNumber, TNumber }                                            },
	{ "CREATE_TOWN",                       &FeatureScriptCommands::CreateTown,                    { TNumber, TVector, TString, TNumber, TString }                                   },
	{ "SET_TOWN_BELIEF",                   &FeatureScriptCommands::SetTownBelief,                 { TNumber, TString, TFloat }                                                      },
	{ "SET_TOWN_BELIEF_CAP",               &FeatureScriptCommands::SetTownBeliefCap,              { TNumber, TString, TFloat }                                                      },
	{ "SET_TOWN_UNINHABITABLE",            &FeatureScriptCommands::SetTownUninhabitable,          { TNumber }                                                                       },
	{ "SET_TOWN_CONGREGATION_POS",         &FeatureScriptCommands::SetTownCongregationPos,        { TNumber, TVector }                                                              },
	{ "CREATE_ABODE",                      &FeatureScriptCommands::CreateAbode,                   { TNumber, TVector, TString, TNumber, TNumber, TNumber, TNumber }                 },
	{ "CREATE_PLANNED_ABODE",              &FeatureScriptCommands::CreatePlannedAbode,            { TNumber, TVector, TString, TNumber, TNumber, TNumber, TNumber }                 },
	{ "CREATE_TOWN_CENTRE",                &FeatureScriptCommands::CreateTownCentre,              { TNumber, TVector, TString, TNumber, TNumber, TNumber }                          },
	{ "CREATE_TOWN_SPELL",                 &FeatureScriptCommands::CreateTownSpell,               { TNumber, TString }                                                              },
	{ "CREATE_NEW_TOWN_SPELL",             &FeatureScriptCommands::CreateNewTownSpell,            { TNumber, TString }                                                              },
	{ "CREATE_TOWN_CENTRE_SPELL_ICON",     &FeatureScriptCommands::CreateTownCentreSpellIcon,     { TNumber, TString }                                                              },
	{ "CREATE_SPELL_ICON",                 &FeatureScriptCommands::CreateSpellIcon,               { TVector, TString, TNumber, TNumber, TNumber }                                   },
	{ "CREATE_PLANNED_SPELL_ICON",         &FeatureScriptCommands::CreatePlannedSpellIcon,        { TNumber, TVector, TString, TNumber, TNumber, TNumber }                          },
	{ "CREATE_VILLAGER",                   &FeatureScriptCommands::CreateVillager,                { TVector, TVector, TString }                                                     },
	{ "CREATE_TOWN_VILLAGER",              &FeatureScriptCommands::CreateTownVillager,            { TNumber, TVector, TVector, TNumber }                                            },
	{ "CREATE_SPECIAL_TOWN_VILLAGER",      &FeatureScriptCommands::CreateSpecialTownVillager,     { TNumber, TVector, TNumber, TNumber }                                            },
	{ "CREATE_VILLAGER_POS",               &FeatureScriptCommands::CreateVillagerPos,             { TVector, TVector, TString, TNumber }                                            },
	{ "CREATE_CITADEL",                    &FeatureScriptCommands::CreateCitadel,                 { TVector, TNumber, TString, TNumber, TNumber }                                   },
	{ "CREATE_PLANNED_CITADEL",            &FeatureScriptCommands::CreatePlannedCitadel,          { TNumber, TVector, TNumber, TString, TNumber, TNumber }                          },
	{ "CREATE_CREATURE_PEN",               &FeatureScriptCommands::CreateCreaturePen,             { TVector, TNumber, TNumber, TNumber, TNumber, TNumber }                          },
	{ "CREATE_WORSHIP_SITE",               &FeatureScriptCommands::CreateWorshipSite,             { TVector, TNumber, TString, TString, TNumber, TNumber }                          },
	{ "CREATE_PLANNED_WORSHIP_SITE",       &FeatureScriptCommands::CreatePlannedWorshipSite,      { TVector, TNumber, TString, TString, TNumber, TNumber }                          },
	{ "CREATE_ANIMAL",                     &FeatureScriptCommands::CreateAnimal,                  { TVector, TNumber, TNumber, TNumber },                                           },
	{ "CREATE_NEW_ANIMAL",                 &FeatureScriptCommands::CreateNewAnimal,               { TVector, TNumber, TNumber, TNumber, TNumber },                                  },
	{ "CREATE_FOREST",                     &FeatureScriptCommands::CreateForest,                  { TNumber, TVector },                                                             },
	{ "CREATE_TREE",                       &FeatureScriptCommands::CreateTree,                    { TNumber, TVector, TNumber, TNumber, TNumber },                                  },
	{ "CREATE_NEW_TREE",                   &FeatureScriptCommands::CreateNewTree,                 { TNumber, TVector, TNumber, TNumber, TFloat, TFloat, TFloat },                   },
	{ "CREATE_FIELD",                      &FeatureScriptCommands::CreateField,                   { TVector, TNumber },                                                             },
	{ "CREATE_TOWN_FIELD",                 &FeatureScriptCommands::CreateTownField,               { TNumber, TVector, TNumber },                                                    },
	{ "CREATE_FISH_FARM",                  &FeatureScriptCommands::CreateFishFarm,                { TVector, TNumber },                                                             },
	{ "CREATE_TOWN_FISH_FARM",             &FeatureScriptCommands::CreateTownFishFarm,            { TNumber, TVector, TNumber },                                                    },
	{ "CREATE_FEATURE",                    &FeatureScriptCommands::CreateFeature,                 { TVector, TNumber, TNumber, TNumber, TNumber },                                  },
	{ "CREATE_FLOWERS",                    &FeatureScriptCommands::CreateFlowers,                 { TVector, TNumber, TFloat, TFloat },                                             },
	{ "CREATE_WALL_SECTION",               &FeatureScriptCommands::CreateWallSection,             { TVector, TNumber, TNumber, TNumber, TNumber },                                  },
	{ "CREATE_PLANNED_WALL_SECTION",       &FeatureScriptCommands::CreatePlannedWallSection,      { TVector, TNumber, TNumber, TNumber, TNumber },                                  },
	{ "CREATE_PITCH",                      &FeatureScriptCommands::CreatePitch,                   { TVector, TNumber, TNumber, TNumber, TNumber, TNumber },                         },
	{ "CREATE_POT",                        &FeatureScriptCommands::CreatePot,                     { TVector, TNumber, TNumber, TNumber },                                           },
	{ "CREATE_TOWN_TEMPORARY_POTS",        &FeatureScriptCommands::CreateTownTemporaryPots,       { TNumber, TNumber, TNumber },                                                    },
	{ "CREATE_MOBILEOBJECT",               &FeatureScriptCommands::CreateMobileobject,            { TVector, TNumber, TNumber, TNumber },                                           },
	{ "CREATE_MOBILESTATIC",               &FeatureScriptCommands::CreateMobileStatic,            { TVector, TNumber, TFloat, TFloat },                                             },
	{ "CREATE_MOBILE_STATIC",              &FeatureScriptCommands::CreateMobileStatic,            { TVector, TNumber, TFloat, TFloat, TFloat, TFloat, TFloat },                     },
	{ "CREATE_DEAD_TREE",                  &FeatureScriptCommands::CreateDeadTree,                { TVector, TString, TNumber, TFloat, TFloat, TFloat, TFloat },                    },
	{ "CREATE_SCAFFOLD",                   &FeatureScriptCommands::CreateScaffold,                { TNumber, TVector, TNumber, TNumber, TNumber },                                  },
	{ "COUNTRY_CHANGE",                    &FeatureScriptCommands::CountryChange,                 { TVector, TNumber },                                                             },
	{ "HEIGHT_CHANGE",                     &FeatureScriptCommands::HeightChange,                  { TVector, TNumber },                                                             },
	{ "CREATE_CREATURE",                   &FeatureScriptCommands::CreateCreature,                { TVector, TNumber, TNumber },                                                    },
	{ "CREATE_CREATURE_FROM_FILE",         &FeatureScriptCommands::CreateCreatureFromFile,        { TString, TNumber, TVector, TVector },                                           },
	{ "CREATE_FLOCK",                      &FeatureScriptCommands::CreateFlock,                   { TNumber, TVector, TVector, TNumber, TNumber, TNumber },                         },
	{ "LOAD_LANDSCAPE",                    &FeatureScriptCommands::LoadLandscape,                 { TString },                                                                      },
	{ "VERSION",                           &FeatureScriptCommands::Version,                       { TFloat },                                                                       },
	{ "CREATE_AREA",                       &FeatureScriptCommands::CreateArea,                    { TVector, TFloat },                                                              },
	{ "START_CAMERA_POS",                  &FeatureScriptCommands::StartCameraPos,                { TVector },                                                                      },
	{ "FLY_BY_FILE",                       &FeatureScriptCommands::FlyByFile,                     { TString },                                                                      },
	{ "TOWN_NEEDS_POS",                    &FeatureScriptCommands::TownNeedsPos,                  { TNumber, TVector },                                                             },
	{ "CREATE_FURNITURE",                  &FeatureScriptCommands::CreateFurniture,               { TVector, TNumber, TFloat },                                                     },
	{ "CREATE_BIG_FOREST",                 &FeatureScriptCommands::CreateBigForest,               { TVector, TNumber, TFloat, TFloat },                                             },
	{ "CREATE_NEW_BIG_FOREST",             &FeatureScriptCommands::CreateNewBigForest,            { TVector, TNumber, TNumber, TFloat, TFloat },                                    },
	{ "CREATE_INFLUENCE_RING",             &FeatureScriptCommands::CreateInfluenceRing,           { TVector, TNumber, TFloat, TNumber },                                            },
	{ "CREATE_WEATHER_CLIMATE",            &FeatureScriptCommands::CreateWeatherClimate,          { TNumber, TNumber, TVector, TFloat, TFloat },                                    },
	{ "CREATE_WEATHER_CLIMATE_RAIN",       &FeatureScriptCommands::CreateWeatherClimateRain,      { TNumber, TFloat, TNumber, TNumber, TNumber },                                   },
	{ "CREATE_WEATHER_CLIMATE_TEMP",       &FeatureScriptCommands::CreateWeatherClimateTemp,      { TNumber, TFloat, TFloat },                                                      },
	{ "CREATE_WEATHER_CLIMATE_WIND",       &FeatureScriptCommands::CreateWeatherClimateWind,      { TNumber, TFloat, TFloat, TFloat },                                              },
	{ "CREATE_WEATHER_STORM",              &FeatureScriptCommands::CreateWeatherStorm,            { TNumber, TVector, TFloat, TNumber, TVector, TVector, TVector, TFloat, TVector } },
	{ "BRUSH_SIZE",                        &FeatureScriptCommands::BrushSize,                     { TFloat, TFloat },                                                               },
	{ "CREATE_STREAM",                     &FeatureScriptCommands::CreateStream,                  { TNumber },                                                                      },
	{ "CREATE_STREAM_POINT",               &FeatureScriptCommands::CreateStreamPoint,             { TNumber, TVector },                                                             },
	{ "CREATE_WATERFALL",                  &FeatureScriptCommands::CreateWaterfall,               { TVector },                                                                      },
	{ "CREATE_ARENA",                      &FeatureScriptCommands::CreateArena,                   { TVector, TFloat },                                                              },
	{ "CREATE_FOOTPATH",                   &FeatureScriptCommands::CreateFootpath,                { TNumber },                                                                      },
	{ "CREATE_FOOTPATH_NODE",              &FeatureScriptCommands::CreateFootpathNode,            { TNumber, TVector },                                                             },
	{ "LINK_FOOTPATH",                     &FeatureScriptCommands::LinkFootpath,                  { TNumber },                                                                      },
	{ "CREATE_BONFIRE",                    &FeatureScriptCommands::CreateBonfire,                 { TVector, TFloat, TFloat, TFloat },                                              },
	{ "CREATE_BASE",                       &FeatureScriptCommands::CreateBase,                    { TVector, TNumber },                                                             },
	{ "CREATE_NEW_FEATURE",                &FeatureScriptCommands::CreateNewFeature,              { TVector, TString, TNumber, TNumber, TNumber },                                  },
	{ "SET_INTERACT_DESIRE",               &FeatureScriptCommands::SetInteractDesire,             { TFloat },                                                                       },
	{ "TOGGLE_COMPUTER_PLAYER",            &FeatureScriptCommands::ToggleComputerPlayer,          { TString, TNumber },                                                             },
	{ "SET_COMPUTER_PLAYER_CREATURE_LIKE", &FeatureScriptCommands::SetComputerPlayerCreatureLike, { TString, TString },                                                             },
	{ "MULTIPLAYER_DEBUG",                 &FeatureScriptCommands::MultiplayerDebug,              { TNumber, TNumber },                                                             },
	{ "CREATE_STREET_LANTERN",             &FeatureScriptCommands::CreateStreetLantern,           { TVector, TNumber },                                                             },
	{ "CREATE_STREET_LIGHT",               &FeatureScriptCommands::CreateStreetLight,             { TVector },                                                                      },
	{ "SET_LAND_NUMBER",                   &FeatureScriptCommands::SetLandNumber,                 { TNumber },                                                                      },
	{ "CREATE_ONE_SHOT_SPELL",             &FeatureScriptCommands::CreateOneShotSpell,            { TVector, TString },                                                             },
	{ "CREATE_ONE_SHOT_SPELL_PU",          &FeatureScriptCommands::CreateOneShotSpellPu,          { TVector, TString },                                                             },
	{ "CREATE_FIRE_FLY",                   &FeatureScriptCommands::CreateFireFly,                 { TVector },                                                                      },
	{ "TOWN_DESIRE_BOOST",                 &FeatureScriptCommands::TownDesireBoost,               { TNumber, TString, TFloat },                                                     },
	{ "CREATE_ANIMATED_STATIC",            &FeatureScriptCommands::CreateAnimatedStatic,          { TVector, TString, TNumber, TNumber },                                           },
	{ "FIRE_FLY_SPELL_REWARD_PROB",        &FeatureScriptCommands::FireFlySpellRewardProb,        { TVector, TFloat },                                                              },
	{ "CREATE_NEW_TOWN_FIELD",             &FeatureScriptCommands::CreateNewTownField,            { TNumber, TVector, TNumber, TFloat },                                            },
	{ "CREATE_SPELL_DISPENSER",            &FeatureScriptCommands::CreateSpellDispenser,          { TNumber, TVector, TString, TString, TFloat, TFloat, TFloat },                   },
	{ "LOAD_COMPUTER_PLAYER_PERSONALLTY",  &FeatureScriptCommands::LoadComputerPlayerPersonallty, { TNumber, TVector },                                                             },
	{ "SET_COMPUTER_PLAYER_PERSONALLTY",   &FeatureScriptCommands::SetComputerPlayerPersonallty,  { TString, TVector, TFloat },                                                     },
	{ "SET_GLOBAL_LAND_BALANCE",           &FeatureScriptCommands::SetGlobalLandBalance,          { TNumber, TFloat },                                                              },
	{ "SET_LAND_BALANCE",                  &FeatureScriptCommands::SetLandBalance,                { TString, TNumber, TFloat },                                                     },
	{ "CREATE_DRINK_WAYPOINT",             &FeatureScriptCommands::CreateDrinkWaypoint,           { TVector },                                                                      },
	{ "SET_TOWN_INFLUENCE_MULTIPLIER",     &FeatureScriptCommands::SetTownInfluenceMultiplier,    { TFloat },                                                                       },
	{ "SET_PLAYER_INFLUENCE_MULTIPLIER",   &FeatureScriptCommands::SetPlayerInfluenceMultiplier,  { TFloat },                                                                       },
	{ "SET_TOWN_BALANCE_BELIEF_SCALE",     &FeatureScriptCommands::SetTownBalanceBeliefScale,     { TNumber, TFloat },                                                              },
	{ "START_GAME_MESSAGE",                &FeatureScriptCommands::StartGameMessage,              { TVector, TNumber },                                                             },
	{ "ADD_GAME_MESSAGE_LINE",             &FeatureScriptCommands::AddGameMessageLine,            { TVector, TNumber },                                                             },
	{ "EDIT_LEVEL",                        &FeatureScriptCommands::EditLevel,                     { },                                                                              },
	{ "SET_NIGHTTIME",                     &FeatureScriptCommands::SetNighttime,                  { TFloat, TFloat, TFloat },                                                       },
	{ "MAKE_LAST_OBJECT_ARTIFACT",         &FeatureScriptCommands::MakeLastObjectArtifact,        { TNumber, TString, TFloat },                                                     },
	{ "SET_LOST_TOWN_SCALE",               &FeatureScriptCommands::SetLostTownScale,              { TFloat },                                                                       },
} };
// clang-format on

glm::vec2 GetXYPosFromString(const std::string& str)
{
	const auto pos = str.find_first_of(',');
	const auto y   = std::stof(str.substr(pos + 1));
	const auto x   = std::stof(str.substr(0, pos));
	return glm::vec2(x, y);
}

std::tuple<VillageEthnicities, VillagerTypes> GetVillagerEthnicityAndType(const std::string& villagerEthnicityWithType)
{
	const auto pos          = villagerEthnicityWithType.find_first_of('_');
	const auto ethnicityStr = villagerEthnicityWithType.substr(0, pos);
	const auto typeStr      = villagerEthnicityWithType.substr(pos + 1);

	try
	{

		const auto ethnicity = VillageEthnicitiesLookup.at(ethnicityStr);
		const auto type      = VillagerTypesLookup.at(typeStr);
		return std::make_tuple(ethnicity, type);
	}
	catch (std::out_of_range& ex)
	{
		throw std::runtime_error("Could not recognize either villager ethnicity or type");
	}
}

void FeatureScriptCommands::Version(const ScriptCommandContext& ctx)
{
	float version = ctx.GetParameters()[0].GetFloat();
	spdlog::debug("Land version set to: {}", version);
}

void FeatureScriptCommands::CreateMist(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreatePath(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateTown(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetTownBelief(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetTownBeliefCap(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetTownUninhabitable(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetTownCongregationPos(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateAbode(const ScriptCommandContext& ctx)
{
	Game& game                  = ctx.GetGame();
	const auto& params          = ctx.GetParameters();
	auto& island                = game.GetLandIsland();
	auto& registry              = game.GetEntityRegistry();
	const auto entity           = registry.Create();
	const uint32_t townId       = params[0].GetNumber();
	const auto position         = GetXYPosFromString(params[1].GetString());
	const auto& abodeType       = params[2].GetString();
	float rotation              = params[3].GetNumber();
	float size                  = params[4].GetNumber();
	const uint32_t foodAmount   = params[5].GetNumber();
	const uint32_t woodAmount   = params[6].GetNumber();
	uint32_t meshId             = 0;

	if (abodeType == "NORSE_ABODE_STORAGE_PIT")
		meshId = MESH_LIST::MSH_B_NORS_STORAGE_PIT;
	else if (abodeType == "NORSE_ABODE_GRAVEYARD")
		meshId = MESH_LIST::MSH_B_NORS_GRAVEYARD;
	else if (abodeType == "NORSE_ABODE_CRECHE")
		meshId = MESH_LIST::MSH_B_NORS_CRECHE;
	else if (abodeType == "NORSE_ABODE_A")
		meshId = MESH_LIST::MSH_B_NORS_1;
	else if (abodeType == "NORSE_ABODE_B")
		meshId = MESH_LIST::MSH_B_NORS_2;
	else if (abodeType == "NORSE_ABODE_C")
		meshId = MESH_LIST::MSH_B_NORS_3;
	else if (abodeType == "NORSE_ABODE_D")
		meshId = MESH_LIST::MSH_B_NORS_4;
	else if (abodeType == "NORSE_ABODE_E")
		meshId = MESH_LIST::MSH_B_NORS_2_A;
	else if (abodeType == "NORSE_ABODE_F")
		meshId = MESH_LIST::MSH_B_NORS_2;

	float radians = -(rotation * 0.001);
	size          = (size * 0.001);

	registry.Assign<Abode>(entity, townId, foodAmount, woodAmount);
	registry.Assign<Transform>(entity, position.x, island.GetHeightAt(position), position.y, size, 0.0f, radians, 0.0f);
	registry.Assign<Model>(entity, meshId, 0.0f, 0.0f, 0.0f);
}

void FeatureScriptCommands::CreatePlannedAbode(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateTownCentre(const ScriptCommandContext& ctx)
{
	Game& game                  = ctx.GetGame();
	const auto& params          = ctx.GetParameters();
	auto& island                = game.GetLandIsland();
	auto& registry              = game.GetEntityRegistry();
	const auto entity           = registry.Create();
	const auto associatedTownId = params[0].GetNumber();
	const auto position         = GetXYPosFromString(params[1].GetString());
	const auto& centreType      = params[2].GetString();
	float rotation              = params[3].GetNumber();
	float size                  = params[4].GetNumber();
	//const auto unknown          = params[5].GetNumber();
	uint32_t meshId = 0;

	if (centreType == "NORSE_ABODE_TOWN_CENTRE")
		meshId = MESH_LIST::MSH_B_NORS_VILLAGECENTRE;

	float radians = -(rotation * 0.001);
	size          = (size * 0.001);

	registry.Assign<Transform>(entity, position.x, island.GetHeightAt(position), position.y, size, 0.0f, radians, 0.0f);
	registry.Assign<Model>(entity, meshId, 0.0f, 0.0f, 0.0f);
}

void FeatureScriptCommands::CreateTownSpell(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateNewTownSpell(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateTownCentreSpellIcon(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateSpellIcon(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreatePlannedSpellIcon(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateVillager(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateTownVillager(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateSpecialTownVillager(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateVillagerPos(const ScriptCommandContext& ctx)
{
	Game& game          = ctx.GetGame();
	const auto& params  = ctx.GetParameters();
	auto& island        = game.GetLandIsland();
	const auto position = GetXYPosFromString(params[0].GetString());
	auto& registry      = game.GetEntityRegistry();
	const auto entity   = registry.Create();
	registry.Assign<Transform>(entity, position.x, island.GetHeightAt(position), position.y, 1.0f, 3.14159f, 0.0f, 0.0f);
	uint32_t health       = 100;
	uint32_t age          = 18;
	uint32_t hunger       = 100;
	auto ethnicityAndType = GetVillagerEthnicityAndType(params[2].GetString());
	auto ethnicity        = std::get<0>(ethnicityAndType);
	auto type             = std::get<1>(ethnicityAndType);
	auto sex              = (type == VillagerTypes::HOUSEWIFE) ? VillagerSex::FEMALE : VillagerSex::MALE;
	registry.Assign<Villager>(entity, health, age, hunger, sex, ethnicity, type);

	MESH_LIST maleMesh;
	MESH_LIST femaleMesh;

	switch (ethnicity)
	{
	case VillageEthnicities::AZTEC:
	{
		maleMesh   = MESH_LIST::MSH_P_AZTC_M_A_1;
		femaleMesh = MESH_LIST::MSH_P_AZTC_F_A_1;
	}
	break;
	case VillageEthnicities::CELTIC:
	{
		maleMesh   = MESH_LIST::MSH_P_CELT_M_A_1;
		femaleMesh = MESH_LIST::MSH_P_CELT_F_A_1;
	}
	break;
	case VillageEthnicities::EGYPTIAN:
	{
		maleMesh   = MESH_LIST::MSH_P_EGPT_M_A_1;
		femaleMesh = MESH_LIST::MSH_P_EGPT_F_A_1;
	}
	break;
	case VillageEthnicities::GREEK:
	{
		maleMesh   = MESH_LIST::MSH_P_GREK_M_A_1;
		femaleMesh = MESH_LIST::MSH_P_GREK_F_A_1;
	}
	break;
	case VillageEthnicities::INDIAN:
	{
		maleMesh   = MESH_LIST::MSH_P_AZTC_M_A_1;
		femaleMesh = MESH_LIST::MSH_P_AZTC_F_A_1;
	}
	break;
	case VillageEthnicities::JAPANESE:
	{
		maleMesh   = MESH_LIST::MSH_P_JAPN_M_A_1;
		femaleMesh = MESH_LIST::MSH_P_JAPN_F_A_1;
	}
	break;
	case VillageEthnicities::NORSE:
	{
		maleMesh   = MESH_LIST::MSH_P_NORS_M_A_1;
		femaleMesh = MESH_LIST::MSH_P_NORS_F_A_1;
	}
	break;
	case VillageEthnicities::TIBETAN:
	{
		maleMesh   = MESH_LIST::MSH_P_TIBT_M_A_1;
		femaleMesh = MESH_LIST::MSH_P_TIBT_F_A_1;
	}
	break;
	case VillageEthnicities::AFRICAN:
	{
		maleMesh   = MESH_LIST::MSH_P_EGPT_M_A_1;
		femaleMesh = MESH_LIST::MSH_P_EGPT_F_A_1;
	}
	break;
	default:
		throw std::runtime_error("Unknown ethnic type");
	}

	uint32_t meshId = (sex == VillagerSex::MALE) ? maleMesh : femaleMesh;
	registry.Assign<Model>(entity, meshId, 0.0f, 0.0f, 0.0f);
}

void FeatureScriptCommands::CreateCitadel(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreatePlannedCitadel(const ScriptCommandContext& ctx)
{
	Game& game                  = ctx.GetGame();
	const auto& params          = ctx.GetParameters();
	auto& island                = game.GetLandIsland();
	auto& registry              = game.GetEntityRegistry();
	const auto entity           = registry.Create();
	const auto associatedTownId = params[0].GetNumber();
	const auto position         = GetXYPosFromString(params[1].GetString());
	//const auto unknown          = params[2].GetString();
	const auto& affiliation     = params[3].GetString();
	auto rotation               = params[4].GetNumber();
	float size                   = params[5].GetNumber();
	uint32_t meshId = 0;

	meshId = MESH_LIST::MSH_DUMMY;

	float radians = -(rotation * 0.001);
	size          = (size * 0.001) * 10;

	registry.Assign<Transform>(entity, position.x, island.GetHeightAt(position), position.y, size, 0.0f, radians, 0.0f);
	registry.Assign<Model>(entity, meshId, 0.0f, 0.0f, 0.0f);
}

void FeatureScriptCommands::CreateCreaturePen(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWorshipSite(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreatePlannedWorshipSite(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateAnimal(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateNewAnimal(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateForest(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateTree(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateNewTree(const ScriptCommandContext& ctx)
{
	Game& game                  = ctx.GetGame();
	const auto& params          = ctx.GetParameters();
	auto& island                = game.GetLandIsland();
	auto& registry              = game.GetEntityRegistry();
	const auto entity           = registry.Create();
	const auto associatedForest = params[0].GetNumber();
	const auto position         = GetXYPosFromString(params[1].GetString());
	const auto& treeType        = params[2].GetNumber();
	const bool isScenic         = params[3].GetNumber() >= 1;
	float radians               = -(params[4].GetFloat());
	float size                  = params[5].GetFloat();  // Initial size
	float finalSize             = params[6].GetNumber(); // Max growth size of the tree
	uint32_t meshId             = 0;

	switch (treeType)
	{
	case 8:
		meshId = MESH_LIST::MSH_T_PALM;
		break;
	case 9:
		meshId = MESH_LIST::MSH_T_PALM_B;
		break;
	case 22:
		meshId = MESH_LIST::MSH_O_BURNT_TREE;
		break;
	// todo: Figure out the different IDs to their types
	case 3:
		meshId = MESH_LIST::MSH_T_CONIFER;
		break;
	case 4:
		meshId = MESH_LIST::MSH_T_CONIFER_A;
		break;
	case 1:
		meshId = MESH_LIST::MSH_T_OAK;
		break;
	case 2:
		meshId = MESH_LIST::MSH_T_OAK_A;
	}

	registry.Assign<Transform>(entity, position.x, island.GetHeightAt(position), position.y, size, 0.0f, radians, 0.0f);
	registry.Assign<Model>(entity, meshId, 0.0f, 0.0f, 0.0f);
}

void FeatureScriptCommands::CreateField(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateTownField(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFishFarm(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateTownFishFarm(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFeature(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFlowers(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWallSection(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreatePlannedWallSection(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreatePitch(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreatePot(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateTownTemporaryPots(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateMobileobject(const ScriptCommandContext& ctx)
{
	Game& game            = ctx.GetGame();
	const auto& params    = ctx.GetParameters();
	auto& island          = game.GetLandIsland();
	auto& registry        = game.GetEntityRegistry();
	const auto entity     = registry.Create();
	const auto position   = GetXYPosFromString(params[0].GetString());
	const auto objectType = params[1].GetNumber();
	const auto rotation   = params[2].GetNumber();
	float size            = params[3].GetNumber();
	uint32_t meshId       = 0;

	switch (objectType)
	{
	case 0: // Barrel
		meshId = MESH_LIST::MSH_B_EGPT_BARREL;
		break;
	case 1: // Egyptian Cart
		meshId = MESH_LIST::MSH_B_EGPT_CART;
		break;
	case 2: // A pot
		meshId = MESH_LIST::MSH_B_EGPT_POT_A;
		break;
	case 3: // A pot
		meshId = MESH_LIST::MSH_B_EGPT_POT_B;
		break;
	case 4: // Cube w/ baby picture
		meshId = MESH_LIST::MSH_DUMMY;
		break;
	case 5: // A piece of poo
		meshId = MESH_LIST::MSH_O_CREATURE_TURD;
		break;
	case 6: // Cube w/ baby picture
		meshId = MESH_LIST::MSH_DUMMY;
		break;
	case 7: // Tiny triangular prism
		break;
	case 8: // Cube w/ baby picture
		meshId = MESH_LIST::MSH_DUMMY;
		break;
	case 9: // Egyptian cart
		meshId = MESH_LIST::MSH_B_EGPT_CART;
		break;
	case 10: // A pot
		break;
	case 11: // A pot
		break;
	case 12: // A pot
		break;
	case 13: // Another pot!!
		break;
	case 14: // Stack of wood
		break;
	case 15: // Bush that turns into grain
		break;
	case 16: // Cube w/ baby picture
		meshId = MESH_LIST::MSH_DUMMY;
		break;
	case 17: // Grey mushroom
		meshId = MESH_LIST::MSH_O_MAGIC_MUSHROOM;
		break;
	case 18: // Blue mushroom
		meshId = MESH_LIST::MSH_O_MAGIC_MUSHROOM;
		break;
	case 19: // Red mushroom
		meshId = MESH_LIST::MSH_O_MAGIC_MUSHROOM;
		break;
	case 20: // Cube
		break;
	case 21: // Part of the Creed
		meshId = MESH_LIST::MSH_CREED;
		break;
	case 22: // Part of the Creed
		meshId = MESH_LIST::MSH_CREED;
		break;
	case 23: // Ship (Land 1)
		meshId = MESH_LIST::MSH_O_ARK_DRY_DOCK;
		break;
	case 24: // A fish
		break;
	case 25: // Cube
		break;
	case 26: // Pole (Land 2 flooding puzzle)
		break;
	case 27: // Base of temple (Land 2 flooding puzzle)
		break;
	case 28: // Large ring of temple (Land 2 flooding puzzle)
		break;
	case 29: // Middle ring of temple (Land 2 flooding puzzle)
		break;
	case 30: // Small ring of temple (Land 2 flooding puzzle)
		break;
	case 31: // A cauldron
		meshId = MESH_LIST::MSH_O_CAULDRON;
		break;
	}

	float radians = -(rotation * 0.001);
	size          = (size * 0.001);

	registry.Assign<Transform>(entity, position.x, island.GetHeightAt(position), position.y, size, 0.0f, radians, 0.0f);
	registry.Assign<Model>(entity, meshId, 0.0f, 0.0f, 0.0f);
}

void FeatureScriptCommands::CreateMobileStatic(const ScriptCommandContext& ctx)
{
	Game& game                = ctx.GetGame();
	const auto& params        = ctx.GetParameters();
	auto& island              = game.GetLandIsland();
	auto& registry            = game.GetEntityRegistry();
	const auto entity         = registry.Create();
	const auto position       = GetXYPosFromString(params[0].GetString());
	const auto itemType       = params[1].GetNumber();
	const auto verticalOffset = params[2].GetFloat();
	const auto UpOrDownPitch  = params[3].GetFloat();
	float radians             = -(params[4].GetFloat());
	float leftOrRightLean     = params[5].GetFloat();
	float size                = params[6].GetFloat();
	uint32_t meshId           = 0;

	switch (itemType)
	{
	case 0: // Fence - Short
		meshId = MESH_LIST::MSH_B_CELT_FENCE_SHORT;
		break;
	case 1: // Fence - Tall
		meshId = MESH_LIST::MSH_B_CELT_FENCE_TALL;
		break;
	case 2: // Rock - Grey, round
		break;
	case 3: // Cube w/ baby picture
		meshId = MESH_LIST::MSH_DUMMY;
		break;
	case 4:                                     // 'Physical Shield' web
		meshId = MESH_LIST::MSH_S_SOLID_SHIELD; // todo: Check right mesh
		break;
	case 5: // Singing Stone
		meshId = MESH_LIST::MSH_SINGING_STONE;
		break;
	case 6: // Black Circle
		break;
	case 7: // Nothing (Invisible?)
		break;
	case 8: // Campfire
		meshId = MESH_LIST::MSH_B_CAMPFIRE;
		break;
	case 9: // Cube w/ baby picture (throwable)
		meshId = MESH_LIST::MSH_DUMMY;
		break;
	case 10: // Idol (Land 2)
		meshId = MESH_LIST::MSH_F_IDOL;
		break;
	case 11: // Idol (Land 2)
		meshId = MESH_LIST::MSH_F_IDOL;
		break;
	case 12: // Cube w/ baby picture
		meshId = MESH_LIST::MSH_DUMMY;
		break;
	case 13: // Cube w/ baby picture
		meshId = MESH_LIST::MSH_DUMMY;
		break;
	case 14: // Stone w/ face (Land 3)
		break;
	case 15: // Stone w/ face (Land 3)
		break;
	case 16: // Rock - Light grey, small, oval
		break;
	case 17: // Rock - Grey, small, oval
		break;
	case 18: // Rock - Brown, small oval
		break;
	case 19: // Rock - Black, small oval
		break;
	case 20: // Rock - Light grey, flat, oval
		break;
	case 21: // Rock - Grey, flat, oval
		break;
	case 22: // Rock - Brown, flat, oval
		break;
	case 23: // Rock - Black, flat, oval
		break;
	case 24: // Rock - Light grey, oval
		break;
	case 25: // Rock - Grey, oval
		break;
	case 26: // Rock - Brown, oval
		break;
	case 27: // Rock - Black, oval
		break;
	case 28: // Rock - Light grey, flat
		break;
	case 29: // Rock - Grey, flat
		break;
	case 30: // Rock - Brown, flat
		break;
	case 31: // Rock - Black, flat
		break;
	case 32: // Rock - Light grey, long
		break;
	case 33: // Rock - Grey, long
		break;
	case 34: // Rock - Brown, long
		break;
	case 35: // Rock - Black, long
		break;
	case 36: // Rock - Light grey
		break;
	case 37: // Rock - Grey
		break;
	case 38: // Rock - Brown
		break;
	case 39: // Rock - Black
		break;
	case 40: // Rock - Light grey, triangle
		break;
	case 41: // Rock - Grey, triangle
		break;
	case 42: // Rock - Brown, triangle
		break;
	case 43: // Rock - Black, triangle
		break;
	case 44: // Rock - Light grey, square
		break;
	case 45: // Rock - Grey, square
		break;
	case 46: // Rock - Brown, square
		break;
	case 47: // Rock - Black, square
		break;
	case 48: // Rock - Black
		break;
	case 49: // Gate Stone - Ape
		meshId = MESH_LIST::MSH_O_GATE_TOTEM_APE;
		break;
	case 50: // Gate Stone - Blank
		break;
	case 51: // Gate Stone - Cow
		meshId = MESH_LIST::MSH_O_GATE_TOTEM_COW;
		break;
	case 52: // Gate Stone - Tiger
		meshId = MESH_LIST::MSH_O_GATE_TOTEM_TIGER;
		break;
	case 53: // Toy ball
		meshId = MESH_LIST::MSH_O_TOY_BALL;
		break;
	case 54: // Teddy bear
		meshId = MESH_LIST::MSH_O_TOY_CUDDLY;
		break;
	case 55: // Die (as in half a pair of dice)
		meshId = MESH_LIST::MSH_O_TOY_DIE;
		break;
	case 56: // Bowling pins
		break;
	case 57: // Bowling ball
		meshId = MESH_LIST::MSH_O_TOY_BOWLING_BALL;
		break;
	case 58: // Statue
		meshId = MESH_LIST::MSH_B_GREK_STATUE;
		break;
	case 59: // Blackened ground
		break;
	case 60: // Glass prism
		break;
	}

	registry.Assign<Transform>(entity, position.x, island.GetHeightAt(position), position.y, size, 0.0f, radians, 0.0f);
	registry.Assign<Model>(entity, meshId, 0.0f, 0.0f, 0.0f);
}

void FeatureScriptCommands::CreateDeadTree(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateScaffold(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CountryChange(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::HeightChange(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateCreature(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateCreatureFromFile(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFlock(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::LoadLandscape(const ScriptCommandContext& ctx)
{
	//ctx.GetGame().LoadLandscape(ctx.GetParameter<std::string>(0));
}

void FeatureScriptCommands::CreateArea(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::StartCameraPos(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::FlyByFile(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::TownNeedsPos(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFurniture(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateBigForest(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateNewBigForest(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateInfluenceRing(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWeatherClimate(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWeatherClimateRain(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWeatherClimateTemp(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWeatherClimateWind(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWeatherStorm(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::BrushSize(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateStream(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateStreamPoint(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWaterfall(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateArena(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFootpath(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFootpathNode(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::LinkFootpath(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateBonfire(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateBase(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateNewFeature(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetInteractDesire(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::ToggleComputerPlayer(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetComputerPlayerCreatureLike(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::MultiplayerDebug(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateStreetLantern(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateStreetLight(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetLandNumber(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateOneShotSpell(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateOneShotSpellPu(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFireFly(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::TownDesireBoost(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateAnimatedStatic(const ScriptCommandContext& ctx)
{
	Game& game            = ctx.GetGame();
	const auto& params    = ctx.GetParameters();
	auto& island          = game.GetLandIsland();
	auto& registry        = game.GetEntityRegistry();
	const auto entity     = registry.Create();
	const auto position   = GetXYPosFromString(params[0].GetString());
	const auto objectType = params[1].GetString();
	const auto rotation   = params[2].GetNumber();
	float size            = params[3].GetNumber();
	uint32_t meshId       = 0;

	if (objectType == "Norse Gate")
	{
		meshId = MESH_LIST::MSH_B_NORS_GATE;
	}
	else if (objectType == "Gate Stone Plinth")
	{
		meshId = MESH_LIST::MSH_O_GATE_TOTEM_PLINTHE;
	}
	else if (objectType == "Piper Cave Entrance")
	{
		meshId = MESH_LIST::MSH_F_PIPER_CAVE;
	}

	float radians = -(rotation * 0.001) + 3.14159;
	size          = (size * 0.001);

	registry.Assign<Transform>(entity, position.x, island.GetHeightAt(position), position.y, size, 0.0f, radians, 0.0f);
	registry.Assign<Model>(entity, meshId, 0.0f, 0.0f, 0.0f);
}

void FeatureScriptCommands::FireFlySpellRewardProb(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateNewTownField(const ScriptCommandContext& ctx)
{
	Game& game                  = ctx.GetGame();
	const auto& params          = ctx.GetParameters();
	auto& island                = game.GetLandIsland();
	auto& registry              = game.GetEntityRegistry();
	const auto entity           = registry.Create();
	const auto associatedTownId = params[0].GetNumber();
	const auto position         = GetXYPosFromString(params[1].GetString());
	//const auto unknown          = params[2].GetNumber();
	float rotation  = params[3].GetNumber();
	uint32_t meshId = MESH_LIST::MSH_T_WHEAT;
	float radians   = -(rotation * 0.001);

	registry.Assign<Transform>(entity, position.x, island.GetHeightAt(position), position.y, 1.0f, 0.0f, radians, 0.0f);
	registry.Assign<Model>(entity, meshId, 0.0f, 0.0f, 0.0f);
}

void FeatureScriptCommands::CreateSpellDispenser(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::LoadComputerPlayerPersonallty(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetComputerPlayerPersonallty(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetGlobalLandBalance(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetLandBalance(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateDrinkWaypoint(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetTownInfluenceMultiplier(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetPlayerInfluenceMultiplier(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetTownBalanceBeliefScale(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::StartGameMessage(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::AddGameMessageLine(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::EditLevel(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetNighttime(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::MakeLastObjectArtifact(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetLostTownScale(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

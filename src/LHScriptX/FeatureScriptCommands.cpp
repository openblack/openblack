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

using namespace openblack::LHScriptX;

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
	Game& game                = ctx.GetGame();
	const auto& params        = ctx.GetParameters();
	auto& island              = game.GetLandIsland();
	auto& registry            = game.GetEntityRegistry();
	const auto entity         = registry.Create();
	const uint32_t townId     = params[0].GetNumber();
	const auto position       = GetXYPosFromString(params[1].GetString());
	const auto& abodeType     = params[2].GetString();
	float rotation            = params[3].GetNumber();
	float size                = params[4].GetNumber();
	const uint32_t foodAmount = params[5].GetNumber();
	const uint32_t woodAmount = params[6].GetNumber();
	uint32_t meshId           = 0;

	if (abodeType == "NORSE_ABODE_STORAGE_PIT")
		meshId = static_cast<uint32_t>(Mesh::BuildingNorseStoragePit);
	else if (abodeType == "NORSE_ABODE_GRAVEYARD")
		meshId = static_cast<uint32_t>(Mesh::BuildingNorseGraveyard);
	else if (abodeType == "NORSE_ABODE_CRECHE")
        meshId = static_cast<uint32_t>(Mesh::BuildingNorseCreche);
	else if (abodeType == "NORSE_ABODE_A")
	    meshId = static_cast<uint32_t>(Mesh::BuildingNorse1);
	else if (abodeType == "NORSE_ABODE_B")
		meshId = static_cast<uint32_t>(Mesh::BuildingNorse2);
	else if (abodeType == "NORSE_ABODE_C")
		meshId = static_cast<uint32_t>(Mesh::BuildingNorse3);
	else if (abodeType == "NORSE_ABODE_D")
		meshId = static_cast<uint32_t>(Mesh::BuildingNorse4);
	else if (abodeType == "NORSE_ABODE_E")
		meshId = static_cast<uint32_t>(Mesh::BuildingNorse2A);
	else if (abodeType == "NORSE_ABODE_F")
	    meshId = static_cast<uint32_t>(Mesh::BuildingNorse2);

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
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
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
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateCitadel(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreatePlannedCitadel(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
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
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
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
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateMobileStatic(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
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
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::FireFlySpellRewardProb(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateNewTownField(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
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

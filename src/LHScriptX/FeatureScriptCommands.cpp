/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "FeatureScriptCommands.h"

#include "3D/Camera.h"
#include "3D/LandIsland.h"
#include "AllMeshes.h"
#include "Entities/Components/Abode.h"
#include "Entities/Components/AnimatedStatic.h"
#include "Entities/Components/Stream.h"
#include "Entities/Components/Transform.h"
#include "Entities/Components/Tree.h"
#include "Entities/Components/Villager.h"
#include "Entities/Registry.h"
#include "Enums.h"
#include "Game.h"

#include <glm/gtx/euler_angles.hpp>
#include <spdlog/spdlog.h>

#include <iostream>
#include <tuple>

using namespace openblack::lhscriptx;

// alias parameter types for signature list readability
const constexpr ParameterType TString = ParameterType::String;
const constexpr ParameterType TNumber = ParameterType::Number;
const constexpr ParameterType TFloat = ParameterType::Float;
const constexpr ParameterType TVector = ParameterType::Vector;

namespace openblack
{
std::unordered_map<std::string, AbodeInfo> abodeIdLookup {
    // Norse
    {"NORSE_ABODE_TOWN_CENTRE", AbodeInfo::NorseTownCentre},
    {"NORSE_ABODE_STORAGE_PIT", AbodeInfo::NorseStoragePit},
    {"NORSE_ABODE_GRAVEYARD", AbodeInfo::NorseGraveyard},
    {"NORSE_ABODE_WORKSHOP", AbodeInfo::NorseWorkshop},
    {"NORSE_ABODE_CRECHE", AbodeInfo::NorseCreche},
    {"NORSE_ABODE_A", AbodeInfo::NorseHut},
    {"NORSE_ABODE_B", AbodeInfo::NorseShackX},
    {"NORSE_ABODE_C", AbodeInfo::NorseShackY},
    {"NORSE_ABODE_D", AbodeInfo::NorseTent},
    {"NORSE_ABODE_E", AbodeInfo::NorseTempleX},
    {"NORSE_ABODE_F", AbodeInfo::NorseTempleY},
    {"NORSE_WONDER", AbodeInfo::NorseWonder},
    // Celtic
    {"CELTIC_ABODE_TOWN_CENTRE", AbodeInfo::CelticTownCentre},
    {"CELTIC_ABODE_STORAGE_PIT", AbodeInfo::CelticStoragePit},
    {"CELTIC_ABODE_GRAVEYARD", AbodeInfo::CelticGraveyard},
    {"CELTIC_ABODE_WORKSHOP", AbodeInfo::CelticGraveyard},
    {"CELTIC_ABODE_CRECHE", AbodeInfo::CelticCreche},
    {"CELTIC_ABODE_A", AbodeInfo::CelticHut},
    {"CELTIC_ABODE_B", AbodeInfo::CelticShackX},
    {"CELTIC_ABODE_C", AbodeInfo::CelticShackY},
    {"CELTIC_ABODE_D", AbodeInfo::CelticTent},
    {"CELTIC_ABODE_E", AbodeInfo::CelticTempleX},
    {"CELTIC_ABODE_F", AbodeInfo::CelticTempleY},
    {"CELTIC_WONDER", AbodeInfo::CelticWonder},
    // Japanese
    {"JAPANESE_ABODE_TOWN_CENTRE", AbodeInfo::JapaneseTownCentre},
    {"JAPANESE_ABODE_STORAGE_PIT", AbodeInfo::JapaneseStoragePit},
    {"JAPANESE_ABODE_GRAVEYARD", AbodeInfo::JapaneseGraveyard},
    {"JAPANESE_ABODE_WORKSHOP", AbodeInfo::JapaneseWorkshop},
    {"JAPANESE_ABODE_CRECHE", AbodeInfo::JapaneseCreche},
    {"JAPANESE_ABODE_A", AbodeInfo::JapaneseHut},
    {"JAPANESE_ABODE_B", AbodeInfo::JapaneseShackX},
    {"JAPANESE_ABODE_C", AbodeInfo::JapaneseShackY},
    {"JAPANESE_ABODE_D", AbodeInfo::JapaneseTent},
    {"JAPANESE_ABODE_E", AbodeInfo::JapaneseTempleX},
    {"JAPANESE_ABODE_F", AbodeInfo::JapaneseTempleY},
    {"JAPANESE_WONDER", AbodeInfo::JapaneseWonder},
    // Aztec
    {"AZTEC_ABODE_TOWN_CENTRE", AbodeInfo::AztecTownCentre},
    {"AZTEC_ABODE_STORAGE_PIT", AbodeInfo::AztecStoragePit},
    {"AZTEC_ABODE_GRAVEYARD", AbodeInfo::AztecGraveyard},
    {"AZTEC_ABODE_WORKSHOP", AbodeInfo::AztecGraveyard},
    {"AZTEC_ABODE_CRECHE", AbodeInfo::AztecCreche},
    {"AZTEC_ABODE_A", AbodeInfo::AztecHut},
    {"AZTEC_ABODE_B", AbodeInfo::AztecShackX},
    {"AZTEC_ABODE_C", AbodeInfo::AztecShackY},
    {"AZTEC_ABODE_D", AbodeInfo::AztecTent},
    {"AZTEC_ABODE_E", AbodeInfo::AztecTempleX},
    {"AZTEC_ABODE_F", AbodeInfo::AztecTempleY},
    {"AZTEC_WONDER", AbodeInfo::AztecWonder},
    // Tibetan
    {"TIBETAN_ABODE_TOWN_CENTRE", AbodeInfo::TibetanTownCentre},
    {"TIBETAN_ABODE_STORAGE_PIT", AbodeInfo::TibetanStoragePit},
    {"TIBETAN_ABODE_GRAVEYARD", AbodeInfo::TibetanGraveyard},
    {"TIBETAN_ABODE_WORKSHOP", AbodeInfo::TibetanWorkshop},
    {"TIBETAN_ABODE_CRECHE", AbodeInfo::TibetanCreche},
    {"TIBETAN_ABODE_A", AbodeInfo::TibetanHut},
    {"TIBETAN_ABODE_B", AbodeInfo::TibetanShackX},
    {"TIBETAN_ABODE_C", AbodeInfo::TibetanShackY},
    {"TIBETAN_ABODE_D", AbodeInfo::TibetanTent},
    {"TIBETAN_ABODE_E", AbodeInfo::TibetanTempleX},
    {"TIBETAN_ABODE_F", AbodeInfo::TibetanTempleY},
    {"TIBETAN_WONDER", AbodeInfo::TibetanWonder},
    // American Indian
    {"INDIAN_ABODE_TOWN_CENTRE", AbodeInfo::IndianTownCentre},
    {"INDIAN_ABODE_STORAGE_PIT", AbodeInfo::IndianStoragePit},
    {"INDIAN_ABODE_GRAVEYARD", AbodeInfo::IndianGraveyard},
    {"INDIAN_ABODE_WORKSHOP", AbodeInfo::IndianWorkshop},
    {"INDIAN_ABODE_CRECHE", AbodeInfo::IndianCreche},
    {"INDIAN_ABODE_A", AbodeInfo::IndianHut},
    {"INDIAN_ABODE_B", AbodeInfo::IndianShackX},
    {"INDIAN_ABODE_C", AbodeInfo::IndianShackY},
    {"INDIAN_ABODE_D", AbodeInfo::IndianTent},
    {"INDIAN_ABODE_E", AbodeInfo::IndianTempleX},
    {"INDIAN_ABODE_F", AbodeInfo::IndianTempleY},
    {"INDIAN_WONDER", AbodeInfo::IndianWonder},
    // Greek
    {"GREEK_ABODE_TOWN_CENTRE", AbodeInfo::GreekTownCentre},
    {"GREEK_ABODE_STORAGE_PIT", AbodeInfo::GreekStoragePit},
    {"GREEK_ABODE_GRAVEYARD", AbodeInfo::GreekGraveyard},
    {"GREEK_ABODE_WORKSHOP", AbodeInfo::GreekWorkshop},
    {"GREEK_ABODE_CRECHE", AbodeInfo::GreekCreche},
    {"GREEK_ABODE_A", AbodeInfo::GreekHut},
    {"GREEK_ABODE_B", AbodeInfo::GreekShackX},
    {"GREEK_ABODE_C", AbodeInfo::GreekShackY},
    {"GREEK_ABODE_D", AbodeInfo::GreekTent},
    {"GREEK_ABODE_E", AbodeInfo::GreekTempleX},
    {"GREEK_ABODE_F", AbodeInfo::GreekTempleY},
    {"GREEK_WONDER", AbodeInfo::GreekWonder},
};

std::unordered_map<std::string, FeatureInfo> featureInfoLookup {{"Fat Pilar Lime", FeatureInfo::FatPilarLime},
                                                                {"Pilar3 Lime", FeatureInfo::Pilar3Lime},
                                                                {"Aztec Statue Feature", FeatureInfo::AztcStatue},
                                                                {"Spikey Pilar Lime", FeatureInfo::SpikeyPilarLime},
                                                                {"Pilar2 Lime", FeatureInfo::Pilar2Lime},
                                                                {"Crater", FeatureInfo::Crater},
                                                                {"Pier", FeatureInfo::Pier}};
} // namespace openblack

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
	{ "CREATE_MOBILE_STATIC",              &FeatureScriptCommands::CreateMobileUStatic,           { TVector, TNumber, TFloat, TFloat, TFloat, TFloat, TFloat },                     },
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

glm::vec2 GetHorizontalPosition(const std::string& str)
{
	const auto pos = str.find_first_of(',');
	const auto y = std::stof(str.substr(pos + 1));
	const auto x = std::stof(str.substr(0, pos));
	return glm::vec2(x, y);
}

openblack::AbodeInfo GetAbodeInfo(const std::string& abodeType)
{
	auto item = openblack::abodeIdLookup.find(abodeType);

	if (item == openblack::abodeIdLookup.end())
	{
		spdlog::error("Missing abode mesh lookup for \"{}\".", abodeType);
		return openblack::AbodeInfo::TibetanWonder;
	}

	return item->second;
}

openblack::FeatureInfo GetFeatureInfo(const std::string& featureType)
{
	auto item = openblack::featureInfoLookup.find(featureType);

	if (item == openblack::featureInfoLookup.end())
	{
		spdlog::error("Missing abode mesh lookup for \"{}\".", featureType);
		return openblack::FeatureInfo::Ark;
	}

	return item->second;
}

inline float GetRadians(const float rotation)
{
	return -(rotation * 0.001f);
}

inline float GetSize(const float size)
{
	return size * 0.001;
}

void FeatureScriptCommands::Version(const ScriptCommandContext& ctx)
{
	float version = ctx.GetParameters()[0].GetFloat();
	spdlog::debug("Land version set to: {}", version);
}

void FeatureScriptCommands::CreateMist(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::CreatePath(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::CreateTown(const ScriptCommandContext& ctx)
{
	Game& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	int townId = params[0].GetNumber();
	const auto position = GetHorizontalPosition(params[1].GetString());
	const auto& playerOwner = params[2].GetString();
	// float notUsed          = params[3].GetNumber();
	const auto& civilisation = params[4].GetString();

	spdlog::debug(R"(Creating town {} for "{}" with civilisation "{}".)", townId, playerOwner, civilisation);
	registry.Assign<Town>(entity, townId);
	auto& registryContext = registry.Context();
	registryContext.towns.insert({townId, entity});
}

void FeatureScriptCommands::SetTownBelief(const ScriptCommandContext& ctx)
{
	Game& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	auto& registryContext = registry.Context();
	int townId = params[0].GetNumber();
	const auto& playerOwner = params[1].GetString();
	const auto& belief = params[2].GetFloat();

	Town& town = registry.Get<Town>(registryContext.towns.at(townId));
	town.beliefs.insert({playerOwner, belief});
}

void FeatureScriptCommands::SetTownBeliefCap(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::SetTownUninhabitable(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::SetTownCongregationPos(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::CreateAbode(const ScriptCommandContext& ctx)
{
	Game& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	const uint32_t townId = params[0].GetNumber();
	const auto position = GetHorizontalPosition(params[1].GetString());
	const auto& abodeType = params[2].GetString();
	float rotation = GetRadians(params[3].GetNumber());
	float size = params[4].GetNumber();
	const uint32_t foodAmount = params[5].GetNumber();
	const uint32_t woodAmount = params[6].GetNumber();
	auto abodeInfo = GetAbodeInfo(abodeType);
	size = GetSize(size);

	const glm::vec3 pos(position.x, island.GetHeightAt(position), position.y);
	const glm::mat3 rot = glm::eulerAngleY(rotation);

	registry.Assign<Transform>(entity, pos, rot, glm::vec3(size));
	registry.Assign<Abode>(entity, abodeInfo, townId, foodAmount, woodAmount);
}

void FeatureScriptCommands::CreatePlannedAbode(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::CreateTownCentre(const ScriptCommandContext& ctx)
{
	Game& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	uint32_t associatedTownId = params[0].GetNumber();
	const auto position = GetHorizontalPosition(params[1].GetString());
	const auto& centreType = params[2].GetString();
	float rotation = GetRadians(params[3].GetNumber());
	float size = params[4].GetNumber();
	// const auto unknown          = params[5].GetNumber();
	auto abodeId = GetAbodeInfo(centreType);
	size = GetSize(size);
	auto submeshIds = std::vector {3};

	const glm::vec3 pos(position.x, island.GetHeightAt(position), position.y);
	const glm::mat3 rot = glm::eulerAngleY(rotation);

	registry.Assign<Transform>(entity, pos, rot, glm::vec3(size));
	registry.Assign<Abode>(entity, abodeId, associatedTownId, static_cast<uint32_t>(0), static_cast<uint32_t>(0));
}

void FeatureScriptCommands::CreateTownSpell(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::CreateNewTownSpell(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::CreateTownCentreSpellIcon(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::CreateSpellIcon(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::CreatePlannedSpellIcon(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::CreateVillager(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::CreateTownVillager(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::CreateSpecialTownVillager(const ScriptCommandContext& ctx)
{
	spdlog::error("Function {} not implemented. {}: {}", __func__, __FILE__, __LINE__);
}

void FeatureScriptCommands::CreateVillagerPos(const ScriptCommandContext& ctx)
{
	Game& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	const auto position = GetHorizontalPosition(params[0].GetString());
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	const glm::vec3 pos(position.x, island.GetHeightAt(position), position.y);
	const glm::mat3 rot = glm::eulerAngleY(glm::radians(180.0f));
	registry.Assign<Transform>(entity, pos, rot, glm::vec3(1.0));
	uint32_t health = 100;
	uint32_t age = params[3].GetNumber();
	uint32_t hunger = 100;
	auto ethnicityAndRole = Villager::GetVillagerEthnicityAndRole(params[2].GetString());
	auto ethnicity = std::get<0>(ethnicityAndRole);
	auto role = std::get<1>(ethnicityAndRole);
	auto lifeStage = age >= 18 ? VillagerLifeStage::Adult : VillagerLifeStage::Child;
	auto sex = (role == VillagerRoles::HOUSEWIFE) ? VillagerSex::FEMALE : VillagerSex::MALE;
	auto task = VillagerTasks::IDLE;
	registry.Assign<Villager>(entity, health, age, hunger, lifeStage, sex, ethnicity, role, task);
}

void FeatureScriptCommands::CreateCitadel(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreatePlannedCitadel(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateCreaturePen(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWorshipSite(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreatePlannedWorshipSite(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateAnimal(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateNewAnimal(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateForest(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateTree(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateNewTree(const ScriptCommandContext& ctx)
{
	Game& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	// const auto associatedForest = params[0].GetNumber();
	const auto position = GetHorizontalPosition(params[1].GetString());
	const auto& treeType = params[2].GetNumber();
	// const bool isScenic = params[3].GetNumber() >= 1;
	float rotation = -params[4].GetFloat();
	float size = params[5].GetFloat();
	// float finalSize = params[6].GetNumber(); // Max growth size of the tree
	auto treeInfo = TreeInfo(treeType);
	const glm::vec3 pos(position.x, island.GetHeightAt(position), position.y);
	const glm::mat3 rot = glm::eulerAngleY(rotation);

	registry.Assign<Transform>(entity, pos, rot, glm::vec3(size));
	registry.Assign<Tree>(entity, treeInfo);
}

void FeatureScriptCommands::CreateField(const ScriptCommandContext& ctx) {}

void FeatureScriptCommands::CreateTownField(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFishFarm(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateTownFishFarm(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFeature(const ScriptCommandContext& ctx)
{
	std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__)
	          << std::endl;
}

void FeatureScriptCommands::CreateFlowers(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWallSection(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreatePlannedWallSection(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreatePitch(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreatePot(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateTownTemporaryPots(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateMobileobject(const ScriptCommandContext& ctx)
{
	Game& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	const auto position = GetHorizontalPosition(params[0].GetString());
	const auto objectType = params[1].GetNumber();
	const auto rotation = GetRadians(params[2].GetNumber());
	float size = GetSize(params[3].GetNumber());
	auto mobileObjectInfo = static_cast<MobileObjectInfo>(objectType);

	const glm::vec3 pos(position.x, island.GetHeightAt(position), position.y);
	const glm::mat3 rot = glm::eulerAngleY(rotation);
	const glm::vec3 scale(size);

	registry.Assign<MobileObject>(entity, mobileObjectInfo);
	registry.Assign<Transform>(entity, pos, rot, scale);
}

void FeatureScriptCommands::CreateMobileStatic(const ScriptCommandContext& ctx)
{
	std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__)
	          << std::endl;
}

void FeatureScriptCommands::CreateMobileUStatic(const ScriptCommandContext& ctx)
{
	auto& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	const auto position = GetHorizontalPosition(params[0].GetString());
	const auto type = params[1].GetNumber();
	const auto verticalOffset = params[2].GetFloat();
	float pitch = -params[3].GetFloat();
	float rotation = -params[4].GetFloat();
	float lean = -params[5].GetFloat();
	float size = params[6].GetFloat();
	auto mobileStaticInfo = MobileStaticInfo(type);

	const glm::vec3 pos(position.x, island.GetHeightAt(position) + verticalOffset, position.y);
	const glm::mat3 rot = glm::eulerAngleXYZ(pitch, rotation, lean);
	const glm::vec3 scale(size);

	registry.Assign<MobileStatic>(entity, mobileStaticInfo);
	registry.Assign<Transform>(entity, pos, rot, scale);
}

void FeatureScriptCommands::CreateDeadTree(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateScaffold(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CountryChange(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::HeightChange(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateCreature(const ScriptCommandContext& ctx)
{
	std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__)
	          << std::endl;
}

void FeatureScriptCommands::CreateCreatureFromFile(const ScriptCommandContext& ctx)
{
	std::cout << std::string {} + "Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__)
	          << std::endl;
}

void FeatureScriptCommands::CreateFlock(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::LoadLandscape(const ScriptCommandContext& ctx)
{
	auto params = ctx.GetParameters();
	ctx.GetGame().LoadLandscape(params[0].GetString());
}

void FeatureScriptCommands::CreateArea(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::StartCameraPos(const ScriptCommandContext& ctx)
{
	auto params = ctx.GetParameters();
	auto pos = GetHorizontalPosition(params[0].GetString());
	auto& camera = ctx.GetGame().GetCamera();
	auto height = ctx.GetGame().GetLandIsland().GetHeightAt(pos) + 10.0f;
	camera.SetPosition(glm::vec3(pos[0], height, pos[1]));
}

void FeatureScriptCommands::FlyByFile(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::TownNeedsPos(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFurniture(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateBigForest(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateNewBigForest(const ScriptCommandContext& ctx)
{
	auto& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	const auto position = GetHorizontalPosition(params[0].GetString());
	// const auto type = params[1].GetNumber(); // Circular == 0, Oval == 1
	// auto unknown = params[2].GetFloat();
	float rotation = -params[3].GetFloat();
	float size = params[4].GetFloat();

	const glm::vec3 pos(position.x, island.GetHeightAt(position), position.y);
	const glm::mat3 rot = glm::eulerAngleY(rotation);
	const glm::vec3 scale(size);

	registry.Assign<Forest>(entity);
	registry.Assign<Transform>(entity, pos, rot, scale);
}

void FeatureScriptCommands::CreateInfluenceRing(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWeatherClimate(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWeatherClimateRain(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWeatherClimateTemp(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWeatherClimateWind(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateWeatherStorm(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::BrushSize(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateStream(const ScriptCommandContext& ctx)
{
	auto& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	const auto streamId = params[0].GetNumber();
	registry.Assign<Stream>(entity, streamId);
	auto& registryContext = registry.Context();
	registryContext.streams.insert({streamId, entity});
}

void FeatureScriptCommands::CreateStreamPoint(const ScriptCommandContext& ctx)
{
	auto& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	auto& registry = game.GetEntityRegistry();
	const auto streamId = params[0].GetNumber();
	const auto pos = GetHorizontalPosition(params[1].GetString());
	const glm::vec3 position(pos.x, island.GetHeightAt(pos), pos.y);
	auto& registryContext = registry.Context();

	Stream& stream = registry.Get<Stream>(registryContext.streams.at(streamId));
	const StreamNode newNode(position, stream.streamNodes);
	stream.streamNodes.push_back(newNode);
}

void FeatureScriptCommands::CreateWaterfall(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateArena(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFootpath(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFootpathNode(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::LinkFootpath(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateBonfire(const ScriptCommandContext& ctx)
{
	auto& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	const auto position = GetHorizontalPosition(params[0].GetString());
	float rotation = -params[1].GetFloat();
	// auto unknown        = params[1].GetFloat();
	float size = params[3].GetFloat() * 1;

	const glm::vec3 pos(position.x, island.GetHeightAt(position), position.y);
	const glm::mat3 rot = glm::eulerAngleY(rotation);
	const glm::vec3 scale(size);

	registry.Assign<MobileStatic>(entity, MobileStaticInfo::Bonfire);
	registry.Assign<Transform>(entity, pos, rot, scale);
}

void FeatureScriptCommands::CreateBase(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateNewFeature(const ScriptCommandContext& ctx)
{
	auto& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	const auto position = GetHorizontalPosition(params[0].GetString());
	auto type = GetFeatureInfo(params[1].GetString());
	float rotation = GetRadians(params[2].GetNumber());
	float size = GetSize(params[3].GetNumber());

	const glm::vec3 pos(position.x, island.GetHeightAt(position), position.y);
	const glm::mat3 rot = glm::eulerAngleY(rotation);
	const glm::vec3 scale(size);

	registry.Assign<Feature>(entity, type);
	registry.Assign<Transform>(entity, pos, rot, scale);
}

void FeatureScriptCommands::SetInteractDesire(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::ToggleComputerPlayer(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetComputerPlayerCreatureLike(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::MultiplayerDebug(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateStreetLantern(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateStreetLight(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetLandNumber(const ScriptCommandContext& ctx)
{
	int landNumber = ctx.GetParameters()[0].GetNumber();
	spdlog::debug("Land number set to: {}", landNumber);
}

void FeatureScriptCommands::CreateOneShotSpell(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateOneShotSpellPu(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateFireFly(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::TownDesireBoost(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateAnimatedStatic(const ScriptCommandContext& ctx)
{
	auto& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	const auto position = GetHorizontalPosition(params[0].GetString());
	const auto objectType = params[1].GetString();
	const auto rotation = -params[2].GetFloat() + glm::radians(180.0f);
	auto size = GetSize(params[3].GetNumber());

	const glm::vec3 pos(position.x, island.GetHeightAt(position), position.y);
	const glm::mat3 rot = glm::eulerAngleY(rotation);

	registry.Assign<AnimatedStatic>(entity, objectType);
	registry.Assign<Transform>(entity, pos, rot, glm::vec3(size));
}

void FeatureScriptCommands::FireFlySpellRewardProb(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateNewTownField(const ScriptCommandContext& ctx)
{
	auto& game = ctx.GetGame();
	const auto& params = ctx.GetParameters();
	auto& island = game.GetLandIsland();
	auto& registry = game.GetEntityRegistry();
	const auto entity = registry.Create();
	auto townId = params[0].GetNumber();
	const auto position = GetHorizontalPosition(params[1].GetString());
	// auto unknown = params[2].GetNumber();
	const auto rotation = -params[3].GetFloat() + glm::radians(180.0f);

	const glm::vec3 pos(position.x, island.GetHeightAt(position), position.y);
	const glm::mat3 rot = glm::eulerAngleY(rotation);

	registry.Assign<Field>(entity, townId);
	registry.Assign<Transform>(entity, pos, rot, glm::vec3(1.0));
}

void FeatureScriptCommands::CreateSpellDispenser(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::LoadComputerPlayerPersonallty(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetComputerPlayerPersonallty(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetGlobalLandBalance(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetLandBalance(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::CreateDrinkWaypoint(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetTownInfluenceMultiplier(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetPlayerInfluenceMultiplier(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetTownBalanceBeliefScale(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::StartGameMessage(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::AddGameMessageLine(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::EditLevel(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetNighttime(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::MakeLastObjectArtifact(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

void FeatureScriptCommands::SetLostTownScale(const ScriptCommandContext& ctx)
{
	// std::cout << std::string {} + "Function " + __func__ + " not implemented.
	// " + __FILE__ + ":" + std::to_string(__LINE__) << std::endl;
}

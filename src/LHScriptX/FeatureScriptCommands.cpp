/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "FeatureScriptCommands.h"

#include <tuple>

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/string_cast.hpp>
#include <spdlog/spdlog.h>

#include "3D/Camera.h"
#include "3D/LandIsland.h"
#include "AllMeshes.h"
#include "ECS/Archetypes/AbodeArchetype.h"
#include "ECS/Archetypes/TownArchetype.h"
#include "ECS/Archetypes/VillagerArchetype.h"
#include "ECS/Components/AnimatedStatic.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Footpath.h"
#include "ECS/Components/Forest.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Stream.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "Enums.h"
#include "Game.h"
#include "ScriptingBindingUtils.h"

using namespace openblack;
using namespace openblack::lhscriptx;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

// alias parameter types for signature list readability
const constexpr ParameterType TString = ParameterType::String;
const constexpr ParameterType TNumber = ParameterType::Number;
const constexpr ParameterType TFloat = ParameterType::Float;
const constexpr ParameterType TVector = ParameterType::Vector;

namespace
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
    {"NORSE_ABODE_WONDER", AbodeInfo::NorseWonder},
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
    {"CELTIC_ABODE_WONDER", AbodeInfo::CelticWonder},
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
    {"JAPANESE_ABODE_WONDER", AbodeInfo::JapaneseWonder},
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
    {"AZTEC_ABODE_WONDER", AbodeInfo::AztecWonder},
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
    {"TIBETAN_ABODE_WONDER", AbodeInfo::TibetanWonder},
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
    {"INDIAN_ABODE_WONDER", AbodeInfo::IndianWonder},
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
    {"GREEK_ABODE_WONDER", AbodeInfo::GreekWonder},
};

std::unordered_map<std::string, Feature::Info> featureInfoLookup {
    {"Fat Pilar Lime", Feature::Info::FatPilarLime},
    {"Pilar3 Lime", Feature::Info::Pilar3Lime},
    {"Aztec Statue Feature", Feature::Info::AztcStatue},
    {"Spikey Pilar Lime", Feature::Info::SpikeyPilarLime},
    {"Pilar2 Lime", Feature::Info::Pilar2Lime},
    {"Crater", Feature::Info::Crater},
    {"Pier", Feature::Info::Pier},
};

const auto playerLookup = makeLookup<PlayerNames>(PlayerNamesStrs);
const auto tribeLookup = makeLookup<Tribe>(TribeStrs);
const auto villagerNumberLookup = makeLookup<VillagerNumber>(VillagerNumberStrs);

std::tuple<Tribe, VillagerNumber> GetVillagerTribeAndNumber(const std::string& villagerTribeWithType)
{
	const auto pos = villagerTribeWithType.find_first_of('_');
	const auto tribeStr = villagerTribeWithType.substr(0, pos);
	const auto roleStr = villagerTribeWithType.substr(pos + 1);

	try
	{
		const auto tribe = tribeLookup.at(tribeStr);
		const auto role = villagerNumberLookup.at(roleStr);
		return std::make_tuple(tribe, role);
	}
	catch (...)
	{
		std::throw_with_nested(std::runtime_error("Could not recognize either villager tribe or role"));
	}
}

} // namespace

const std::array<const ScriptCommandSignature, 105> FeatureScriptCommands::Signatures = {{
    CREATE_COMMAND_BINDING("CREATE_MIST", CreateMist),
    CREATE_COMMAND_BINDING("CREATE_PATH", CreatePath),
    CREATE_COMMAND_BINDING("CREATE_TOWN", CreateTown),
    CREATE_COMMAND_BINDING("SET_TOWN_BELIEF", SetTownBelief),
    CREATE_COMMAND_BINDING("SET_TOWN_BELIEF_CAP", SetTownBeliefCap),
    CREATE_COMMAND_BINDING("SET_TOWN_UNINHABITABLE", SetTownUninhabitable),
    CREATE_COMMAND_BINDING("SET_TOWN_CONGREGATION_POS", SetTownCongregationPos),
    CREATE_COMMAND_BINDING("CREATE_ABODE", CreateAbode),
    CREATE_COMMAND_BINDING("CREATE_PLANNED_ABODE", CreatePlannedAbode),
    CREATE_COMMAND_BINDING("CREATE_TOWN_CENTRE", CreateTownCentre),
    CREATE_COMMAND_BINDING("CREATE_TOWN_SPELL", CreateTownSpell),
    CREATE_COMMAND_BINDING("CREATE_NEW_TOWN_SPELL", CreateNewTownSpell),
    CREATE_COMMAND_BINDING("CREATE_TOWN_CENTRE_SPELL_ICON", CreateTownCentreSpellIcon),
    CREATE_COMMAND_BINDING("CREATE_SPELL_ICON", CreateSpellIcon),
    CREATE_COMMAND_BINDING("CREATE_PLANNED_SPELL_ICON", CreatePlannedSpellIcon),
    CREATE_COMMAND_BINDING("CREATE_VILLAGER", CreateVillager),
    CREATE_COMMAND_BINDING("CREATE_TOWN_VILLAGER", CreateTownVillager),
    CREATE_COMMAND_BINDING("CREATE_SPECIAL_TOWN_VILLAGER", CreateSpecialTownVillager),
    CREATE_COMMAND_BINDING("CREATE_VILLAGER_POS", CreateVillagerPos),
    CREATE_COMMAND_BINDING("CREATE_CITADEL", CreateCitadel),
    CREATE_COMMAND_BINDING("CREATE_PLANNED_CITADEL", CreatePlannedCitadel),
    CREATE_COMMAND_BINDING("CREATE_CREATURE_PEN", CreateCreaturePen),
    CREATE_COMMAND_BINDING("CREATE_WORSHIP_SITE", CreateWorshipSite),
    CREATE_COMMAND_BINDING("CREATE_PLANNED_WORSHIP_SITE", CreatePlannedWorshipSite),
    CREATE_COMMAND_BINDING("CREATE_ANIMAL", CreateAnimal),
    CREATE_COMMAND_BINDING("CREATE_NEW_ANIMAL", CreateNewAnimal),
    CREATE_COMMAND_BINDING("CREATE_FOREST", CreateForest),
    CREATE_COMMAND_BINDING("CREATE_TREE", CreateTree),
    CREATE_COMMAND_BINDING("CREATE_NEW_TREE", CreateNewTree),
    CREATE_COMMAND_BINDING("CREATE_FIELD", CreateField),
    CREATE_COMMAND_BINDING("CREATE_TOWN_FIELD", CreateTownField),
    CREATE_COMMAND_BINDING("CREATE_FISH_FARM", CreateFishFarm),
    CREATE_COMMAND_BINDING("CREATE_TOWN_FISH_FARM", CreateTownFishFarm),
    CREATE_COMMAND_BINDING("CREATE_FEATURE", CreateFeature),
    CREATE_COMMAND_BINDING("CREATE_FLOWERS", CreateFlowers),
    CREATE_COMMAND_BINDING("CREATE_WALL_SECTION", CreateWallSection),
    CREATE_COMMAND_BINDING("CREATE_PLANNED_WALL_SECTION", CreatePlannedWallSection),
    CREATE_COMMAND_BINDING("CREATE_PITCH", CreatePitch),
    CREATE_COMMAND_BINDING("CREATE_POT", CreatePot),
    CREATE_COMMAND_BINDING("CREATE_TOWN_TEMPORARY_POTS", CreateTownTemporaryPots),
    CREATE_COMMAND_BINDING("CREATE_MOBILEOBJECT", CreateMobileObject),
    CREATE_COMMAND_BINDING("CREATE_MOBILESTATIC", CreateMobileStatic),
    CREATE_COMMAND_BINDING("CREATE_MOBILE_STATIC", CreateMobileUStatic),
    CREATE_COMMAND_BINDING("CREATE_DEAD_TREE", CreateDeadTree),
    CREATE_COMMAND_BINDING("CREATE_SCAFFOLD", CreateScaffold),
    CREATE_COMMAND_BINDING("COUNTRY_CHANGE", CountryChange),
    CREATE_COMMAND_BINDING("HEIGHT_CHANGE", HeightChange),
    CREATE_COMMAND_BINDING("CREATE_CREATURE", CreateCreature),
    CREATE_COMMAND_BINDING("CREATE_CREATURE_FROM_FILE", CreateCreatureFromFile),
    CREATE_COMMAND_BINDING("CREATE_FLOCK", CreateFlock),
    CREATE_COMMAND_BINDING("LOAD_LANDSCAPE", LoadLandscape),
    CREATE_COMMAND_BINDING("VERSION", Version),
    CREATE_COMMAND_BINDING("CREATE_AREA", CreateArea),
    CREATE_COMMAND_BINDING("START_CAMERA_POS", StartCameraPos),
    CREATE_COMMAND_BINDING("FLY_BY_FILE", FlyByFile),
    CREATE_COMMAND_BINDING("TOWN_NEEDS_POS", TownNeedsPos),
    CREATE_COMMAND_BINDING("CREATE_FURNITURE", CreateFurniture),
    CREATE_COMMAND_BINDING("CREATE_BIG_FOREST", CreateBigForest),
    CREATE_COMMAND_BINDING("CREATE_NEW_BIG_FOREST", CreateNewBigForest),
    CREATE_COMMAND_BINDING("CREATE_INFLUENCE_RING", CreateInfluenceRing),
    CREATE_COMMAND_BINDING("CREATE_WEATHER_CLIMATE", CreateWeatherClimate),
    CREATE_COMMAND_BINDING("CREATE_WEATHER_CLIMATE_RAIN", CreateWeatherClimateRain),
    CREATE_COMMAND_BINDING("CREATE_WEATHER_CLIMATE_TEMP", CreateWeatherClimateTemp),
    CREATE_COMMAND_BINDING("CREATE_WEATHER_CLIMATE_WIND", CreateWeatherClimateWind),
    CREATE_COMMAND_BINDING("CREATE_WEATHER_STORM", CreateWeatherStorm),
    CREATE_COMMAND_BINDING("BRUSH_SIZE", BrushSize),
    CREATE_COMMAND_BINDING("CREATE_STREAM", CreateStream),
    CREATE_COMMAND_BINDING("CREATE_STREAM_POINT", CreateStreamPoint),
    CREATE_COMMAND_BINDING("CREATE_WATERFALL", CreateWaterfall),
    CREATE_COMMAND_BINDING("CREATE_ARENA", CreateArena),
    CREATE_COMMAND_BINDING("CREATE_FOOTPATH", CreateFootpath),
    CREATE_COMMAND_BINDING("CREATE_FOOTPATH_NODE", CreateFootpathNode),
    CREATE_COMMAND_BINDING("LINK_FOOTPATH", LinkFootpath),
    CREATE_COMMAND_BINDING("CREATE_BONFIRE", CreateBonfire),
    CREATE_COMMAND_BINDING("CREATE_BASE", CreateBase),
    CREATE_COMMAND_BINDING("CREATE_NEW_FEATURE", CreateNewFeature),
    CREATE_COMMAND_BINDING("SET_INTERACT_DESIRE", SetInteractDesire),
    CREATE_COMMAND_BINDING("TOGGLE_COMPUTER_PLAYER", ToggleComputerPlayer),
    CREATE_COMMAND_BINDING("SET_COMPUTER_PLAYER_CREATURE_LIKE", SetComputerPlayerCreatureLike),
    CREATE_COMMAND_BINDING("MULTIPLAYER_DEBUG", MultiplayerDebug),
    CREATE_COMMAND_BINDING("CREATE_STREET_LANTERN", CreateStreetLantern),
    CREATE_COMMAND_BINDING("CREATE_STREET_LIGHT", CreateStreetLight),
    CREATE_COMMAND_BINDING("SET_LAND_NUMBER", SetLandNumber),
    CREATE_COMMAND_BINDING("CREATE_ONE_SHOT_SPELL", CreateOneShotSpell),
    CREATE_COMMAND_BINDING("CREATE_ONE_SHOT_SPELL_PU", CreateOneShotSpellPu),
    CREATE_COMMAND_BINDING("CREATE_FIRE_FLY", CreateFireFly),
    CREATE_COMMAND_BINDING("TOWN_DESIRE_BOOST", TownDesireBoost),
    CREATE_COMMAND_BINDING("CREATE_ANIMATED_STATIC", CreateAnimatedStatic),
    CREATE_COMMAND_BINDING("FIRE_FLY_SPELL_REWARD_PROB", FireFlySpellRewardProb),
    CREATE_COMMAND_BINDING("CREATE_NEW_TOWN_FIELD", CreateNewTownField),
    CREATE_COMMAND_BINDING("CREATE_SPELL_DISPENSER", CreateSpellDispenser),
    CREATE_COMMAND_BINDING("LOAD_COMPUTER_PLAYER_PERSONALLTY", LoadComputerPlayerPersonality),
    CREATE_COMMAND_BINDING("SET_COMPUTER_PLAYER_PERSONALLTY", SetComputerPlayerPersonality),
    CREATE_COMMAND_BINDING("SET_GLOBAL_LAND_BALANCE", SetGlobalLandBalance),
    CREATE_COMMAND_BINDING("SET_LAND_BALANCE", SetLandBalance),
    CREATE_COMMAND_BINDING("CREATE_DRINK_WAYPOINT", CreateDrinkWaypoint),
    CREATE_COMMAND_BINDING("SET_TOWN_INFLUENCE_MULTIPLIER", SetTownInfluenceMultiplier),
    CREATE_COMMAND_BINDING("SET_PLAYER_INFLUENCE_MULTIPLIER", SetPlayerInfluenceMultiplier),
    CREATE_COMMAND_BINDING("SET_TOWN_BALANCE_BELIEF_SCALE", SetTownBalanceBeliefScale),
    CREATE_COMMAND_BINDING("START_GAME_MESSAGE", StartGameMessage),
    CREATE_COMMAND_BINDING("ADD_GAME_MESSAGE_LINE", AddGameMessageLine),
    CREATE_COMMAND_BINDING("EDIT_LEVEL", EditLevel),
    CREATE_COMMAND_BINDING("SET_NIGHTTIME", SetNighttime),
    CREATE_COMMAND_BINDING("MAKE_LAST_OBJECT_ARTIFACT", MakeLastObjectArtifact),
    CREATE_COMMAND_BINDING("SET_LOST_TOWN_SCALE", SetLostTownScale),
}};

AbodeInfo GetAbodeInfo(const std::string& abodeType)
{
	auto item = abodeIdLookup.find(abodeType);

	if (item == abodeIdLookup.end())
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "Missing abode mesh lookup for \"{}\".", abodeType);
		return AbodeInfo::TibetanWonder;
	}

	return item->second;
}

Feature::Info GetFeatureInfo(const std::string& featureType)
{
	auto item = featureInfoLookup.find(featureType);

	if (item == featureInfoLookup.end())
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "Missing abode mesh lookup for \"{}\".", featureType);
		return Feature::Info::Ark;
	}

	return item->second;
}

inline glm::mat4 GetRotation(int rotation)
{
	return glm::eulerAngleY(static_cast<float>(rotation) * -0.001f);
}

inline glm::vec3 GetSize(int size)
{
	return glm::vec3(size, size, size) * 0.001f;
}

void FeatureScriptCommands::CreateMist(glm::vec3 position, float param_2, int32_t param_3, float param_4, float param_5)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}, {}, {}, {}) not implemented.",
	                    __FILE__, __LINE__, __func__, glm::to_string(position), param_2, param_3, param_4, param_5);
}

void FeatureScriptCommands::CreatePath(int32_t param_1, int32_t param_2, int32_t param_3, int32_t param_4)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}, {}, {}) not implemented.", __FILE__,
	                    __LINE__, __func__, param_1, param_2, param_3, param_4);
}

void FeatureScriptCommands::CreateTown(int32_t townId, glm::vec3 position, const std::string& playerOwner,
                                       [[maybe_unused]] int32_t, const std::string& tribeType)
{
	spdlog::get("scripting")
	    ->debug(R"(LHScriptX: Creating town {} for "{}" with tribe type "{}".)", townId, playerOwner, tribeType);

	PlayerNames player;
	try
	{
		player = playerLookup.at(playerOwner);
	}
	catch (...)
	{
		std::throw_with_nested(std::runtime_error("Could not recognize village owner"));
	}

	Tribe tribe;
	try
	{
		tribe = tribeLookup.at(tribeType);
	}
	catch (...)
	{
		std::throw_with_nested(std::runtime_error("Could not recognize village tribe"));
	}

	TownArchetype::Create(townId, position, player, tribe);
}

void FeatureScriptCommands::SetTownBelief(int32_t townId, const std::string& playerOwner, float belief)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	auto& registryContext = registry.Context();

	Town& town = registry.Get<Town>(registryContext.towns.at(townId));
	town.beliefs.insert({playerOwner, belief});
}

void FeatureScriptCommands::SetTownBeliefCap(int32_t townId, const std::string& playerOwner, float belief)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}, {}) not implemented.", __FILE__,
	                    __LINE__, __func__, townId, playerOwner, belief);
}

void FeatureScriptCommands::SetTownUninhabitable(int32_t townId)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}) not implemented.", __FILE__, __LINE__,
	                    __func__, townId);
}

void FeatureScriptCommands::SetTownCongregationPos(int32_t townId, glm::vec3 position)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}) not implemented.", __FILE__, __LINE__,
	                    __func__, townId, glm::to_string(position));
}

void FeatureScriptCommands::CreateAbode(int32_t townId, glm::vec3 position, const std::string& abodeInfo, int32_t rotation,
                                        int32_t size, int32_t foodAmount, int32_t woodAmount)
{
	// Does not use 3d angle to game angle
	AbodeArchetype::Create(townId, position, GetAbodeInfo(abodeInfo), rotation * 0.001f, size * 0.001f,
	                       static_cast<uint32_t>(foodAmount), static_cast<uint32_t>(woodAmount));
}

void FeatureScriptCommands::CreatePlannedAbode(int32_t townId, glm::vec3 position, const std::string& abodeInfo,
                                               int32_t rotation, int32_t size, int32_t foodAmount, int32_t woodAmount)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}, {}, {}, {}, {}, {}) not implemented.",
	                    __FILE__, __LINE__, __func__, townId, glm::to_string(position), abodeInfo, rotation, size, foodAmount,
	                    woodAmount);
}

void FeatureScriptCommands::CreateTownCentre(int32_t townId, glm::vec3 position, const std::string& abodeInfo, int32_t rotation,
                                             int32_t size, [[maybe_unused]] int32_t worshipPercentage)
{
	AbodeArchetype::Create(townId, position, GetAbodeInfo(abodeInfo), rotation * 0.001f, size * 0.001f,
	                       static_cast<uint32_t>(0), static_cast<uint32_t>(0));
}

void FeatureScriptCommands::CreateTownSpell(int32_t townId, const std::string& spellName)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}) not implemented.", __FILE__, __LINE__,
	                    __func__, townId, spellName);
}

void FeatureScriptCommands::CreateNewTownSpell(int32_t townId, const std::string& spellName)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}) not implemented.", __FILE__, __LINE__,
	                    __func__, townId, spellName);
}

void FeatureScriptCommands::CreateTownCentreSpellIcon(int32_t param_1, const std::string& param_2)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}) not implemented.", __FILE__, __LINE__,
	                    __func__, param_1, param_2);
}

void FeatureScriptCommands::CreateSpellIcon(glm::vec3 position, const std::string& param_2, int32_t param_3, int32_t param_4,
                                            int32_t param_5)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}, {}, {}, {}) not implemented.",
	                    __FILE__, __LINE__, __func__, glm::to_string(position), param_2, param_3, param_4, param_5);
}

void FeatureScriptCommands::CreatePlannedSpellIcon(int32_t param_1, glm::vec3 position, const std::string& param_3,
                                                   int32_t param_4, int32_t param_5, int32_t param_6)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}, {}, {}, {}, {}) not implemented.",
	                    __FILE__, __LINE__, __func__, param_1, glm::to_string(position), param_3, param_4, param_5, param_6);
}

void FeatureScriptCommands::CreateVillager(glm::vec3 param_1, glm::vec3 param_2, const std::string& param_3)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}, {}) not implemented.", __FILE__,
	                    __LINE__, __func__, glm::to_string(param_1), glm::to_string(param_2), param_3);
}

void FeatureScriptCommands::CreateTownVillager(int32_t townId, glm::vec3 position, const std::string& villagerType, int32_t age)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}, {}, {}) not implemented.", __FILE__,
	                    __LINE__, __func__, townId, glm::to_string(position), villagerType, age);
}

void FeatureScriptCommands::CreateSpecialTownVillager(int32_t param_1, glm::vec3 position, int32_t param_3, int32_t param_4)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}, {}, {}) not implemented.", __FILE__,
	                    __LINE__, __func__, param_1, glm::to_string(position), param_3, param_4);
}

void FeatureScriptCommands::CreateVillagerPos(glm::vec3 abodePosition, glm::vec3 position, const std::string& tribeAndNumber,
                                              int32_t age)
{
	auto [tribe, number] = GetVillagerTribeAndNumber(tribeAndNumber);
	VillagerArchetype::Create(abodePosition, position, GVillagerInfo::Find(tribe, number), age);
}

void FeatureScriptCommands::CreateCitadel([[maybe_unused]] glm::vec3 position, int32_t, const std::string&, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreatePlannedCitadel(int32_t, [[maybe_unused]] glm::vec3 position, int32_t, const std::string&,
                                                 int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateCreaturePen([[maybe_unused]] glm::vec3 position, int32_t, int32_t, int32_t, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateWorshipSite([[maybe_unused]] glm::vec3 position, int32_t, const std::string&,
                                              const std::string&, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreatePlannedWorshipSite([[maybe_unused]] glm::vec3 position, int32_t, const std::string&,
                                                     const std::string&, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateAnimal([[maybe_unused]] glm::vec3 position, int32_t, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateNewAnimal([[maybe_unused]] glm::vec3 position, int32_t, int32_t, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateForest([[maybe_unused]] int32_t forestId, [[maybe_unused]] glm::vec3 position)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateTree([[maybe_unused]] int32_t forestId, [[maybe_unused]] glm::vec3 position, int32_t, int32_t,
                                       int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateNewTree([[maybe_unused]] int32_t forestId, glm::vec3 position, int32_t treeType,
                                          [[maybe_unused]] int32_t isNonScenic, float rotation, float currentSize,
                                          [[maybe_unused]] float maxSize)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	registry.Assign<Transform>(entity, position, glm::eulerAngleY(-rotation), glm::vec3(currentSize));
	const auto& tree = registry.Assign<Tree>(entity, Tree::Info(treeType));
	registry.Assign<Mesh>(entity, treeMeshLookup[tree.type], static_cast<int8_t>(0), static_cast<int8_t>(-1));
}

void FeatureScriptCommands::CreateField([[maybe_unused]] glm::vec3 position, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateTownField([[maybe_unused]] int32_t townId, [[maybe_unused]] glm::vec3 position, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateFishFarm([[maybe_unused]] glm::vec3 position, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateTownFishFarm([[maybe_unused]] int32_t townId, [[maybe_unused]] glm::vec3 position, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateFeature(glm::vec3 position, int32_t param_2, int32_t param_3, int32_t param_4,
                                          int32_t param_5)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}, {}, {}, {}) not implemented.",
	                    __FILE__, __LINE__, __func__, glm::to_string(position), param_2, param_3, param_4, param_5);
}

void FeatureScriptCommands::CreateFlowers([[maybe_unused]] glm::vec3 position, int32_t, float, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateWallSection([[maybe_unused]] glm::vec3 position, int32_t, int32_t, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreatePlannedWallSection([[maybe_unused]] glm::vec3 position, int32_t, int32_t, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreatePitch([[maybe_unused]] glm::vec3 position, int32_t, int32_t, int32_t, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreatePot([[maybe_unused]] glm::vec3 position, int32_t, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateTownTemporaryPots(int32_t, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateMobileObject(glm::vec3 position, int32_t type, int32_t rotation, int32_t scale)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	registry.Assign<Transform>(entity, position, GetRotation(rotation), GetSize(scale));
	const auto& object = registry.Assign<MobileObject>(entity, static_cast<MobileObject::Info>(type));
	registry.Assign<Mesh>(entity, mobileObjectMeshLookup[object.type], static_cast<int8_t>(0), static_cast<int8_t>(1));
}

void FeatureScriptCommands::CreateMobileStatic(glm::vec3 position, int32_t param_2, float param_3, float param_4)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}, {}, {}) not implemented.", __FILE__,
	                    __LINE__, __func__, glm::to_string(position), param_2, param_3, param_4);
}

void FeatureScriptCommands::CreateMobileUStatic(glm::vec3 position, int32_t type, float verticalOffset, float pitch,
                                                float rotation, float lean, float scale)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	glm::vec3 offset(0.0f, verticalOffset, 0.0f);

	registry.Assign<Transform>(entity, position + offset, glm::eulerAngleXYZ(-pitch, -rotation, -lean), glm::vec3(scale));
	const auto& mobile = registry.Assign<MobileStatic>(entity, MobileStatic::Info(type));
	registry.Assign<Mesh>(entity, mobileStaticMeshLookup[mobile.type], static_cast<int8_t>(0), static_cast<int8_t>(1));
}

void FeatureScriptCommands::CreateDeadTree([[maybe_unused]] glm::vec3 position, const std::string&, int32_t, float, float,
                                           float, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateScaffold(int32_t, [[maybe_unused]] glm::vec3 position, int32_t, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CountryChange([[maybe_unused]] glm::vec3 position, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::HeightChange([[maybe_unused]] glm::vec3 position, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateCreature(glm::vec3 position, int32_t param_2, int32_t param_3)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}, {}) not implemented.", __FILE__,
	                    __LINE__, __func__, glm::to_string(position), param_2, param_3);
}

void FeatureScriptCommands::CreateCreatureFromFile(const std::string& playerName, int32_t creatureType,
                                                   const std::string& creatureMind, glm::vec3 position)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}, {}, {}, {}) not implemented.", __FILE__,
	                    __LINE__, __func__, playerName, creatureType, creatureMind, glm::to_string(position));
}

void FeatureScriptCommands::CreateFlock(int32_t, glm::vec3, glm::vec3, int32_t, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::LoadLandscape(const std::string& path)
{
	Game::instance()->LoadLandscape(path);
}

void FeatureScriptCommands::Version(float version)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("scripting"), "LHScriptX: Land version set to: {}", version);
}

void FeatureScriptCommands::CreateArea([[maybe_unused]] glm::vec3 position, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::StartCameraPos(glm::vec3 position)
{
	auto& camera = Game::instance()->GetCamera();
	const glm::vec3 offset(0.0f, 10.0f, 0.0f);
	camera.SetPosition(position + offset);
}

void FeatureScriptCommands::FlyByFile([[maybe_unused]] const std::string& path)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::TownNeedsPos([[maybe_unused]] int32_t townId, [[maybe_unused]] glm::vec3 position)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateFurniture([[maybe_unused]] glm::vec3 position, int32_t, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateBigForest([[maybe_unused]] glm::vec3 position, int32_t, float, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateNewBigForest(glm::vec3 position, [[maybe_unused]] int32_t type,
                                               [[maybe_unused]] int32_t param_3, float rotation, float scale)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	registry.Assign<Transform>(entity, position, glm::eulerAngleY(-rotation), glm::vec3(scale));
	registry.Assign<Forest>(entity);
	registry.Assign<Mesh>(entity, MeshPackId::FeatureForest, static_cast<int8_t>(0), static_cast<int8_t>(1));
}

void FeatureScriptCommands::CreateInfluenceRing([[maybe_unused]] glm::vec3 position, int32_t, float, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateWeatherClimate(int32_t, int32_t, glm::vec3, float, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateWeatherClimateRain(int32_t, float, int32_t, int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateWeatherClimateTemp(int32_t, float, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateWeatherClimateWind(int32_t, float, float, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateWeatherStorm(int32_t, glm::vec3, float, int32_t, glm::vec3, glm::vec3, glm::vec3, float,
                                               glm::vec3)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::BrushSize(float, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateStream(int32_t streamId)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	auto& registryContext = registry.Context();
	const auto entity = registry.Create();

	registry.Assign<Stream>(entity, streamId);
	registryContext.streams.insert({streamId, entity});
}

void FeatureScriptCommands::CreateStreamPoint(int32_t streamId, glm::vec3 position)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	auto& registryContext = registry.Context();

	Stream& stream = registry.Get<Stream>(registryContext.streams.at(streamId));
	stream.nodes.emplace_back(position, stream.nodes);
}

void FeatureScriptCommands::CreateWaterfall([[maybe_unused]] glm::vec3 position)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateArena([[maybe_unused]] glm::vec3 position, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateFootpath(int32_t footpathId)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();
	registry.Assign<Footpath>(entity);
	auto& registryContext = registry.Context();
	registryContext.footpaths.insert({footpathId, entity});
}

void FeatureScriptCommands::CreateFootpathNode(int footpathId, glm::vec3 position)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	auto& registryContext = registry.Context();
	auto& footpath = registry.Get<Footpath>(registryContext.footpaths.at(footpathId));
	footpath.nodes.emplace_back(Footpath::Node {position});
}

void FeatureScriptCommands::LinkFootpath(int32_t footpathId)
{
	// TODO: The last MultiMapFixed created in this script is an implicit param
	//       This Command adds the footpath to a list in a FootpathLink on the MultiMapFixed
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}) not implemented.", __FILE__, __LINE__,
	                    __func__, footpathId);
}

void FeatureScriptCommands::CreateBonfire(glm::vec3 position, float rotation, [[maybe_unused]] float param_3, float scale)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	registry.Assign<Transform>(entity, position, glm::eulerAngleY(-rotation), glm::vec3(scale));
	const auto& mobile = registry.Assign<MobileStatic>(entity, MobileStatic::Info::Bonfire);
	registry.Assign<Mesh>(entity, mobileStaticMeshLookup[mobile.type], static_cast<int8_t>(0), static_cast<int8_t>(1));
}

void FeatureScriptCommands::CreateBase([[maybe_unused]] glm::vec3 position, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateNewFeature(glm::vec3 position, const std::string& type, int32_t rotation, int32_t scale,
                                             [[maybe_unused]] int32_t param_5)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	registry.Assign<Transform>(entity, position, GetRotation(rotation), GetSize(scale));
	const auto& feature = registry.Assign<Feature>(entity, GetFeatureInfo(type));
	registry.Assign<Mesh>(entity, featureMeshLookup[feature.type], static_cast<int8_t>(0), static_cast<int8_t>(1));
}

void FeatureScriptCommands::SetInteractDesire(float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::ToggleComputerPlayer(const std::string&, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::SetComputerPlayerCreatureLike(const std::string&, const std::string&)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::MultiplayerDebug(int32_t, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateStreetLantern([[maybe_unused]] glm::vec3 position, int32_t)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateStreetLight([[maybe_unused]] glm::vec3 position)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::SetLandNumber(int32_t number)
{
	SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {}({}) not implemented.", __FILE__, __LINE__,
	                    __func__, number);
}

void FeatureScriptCommands::CreateOneShotSpell([[maybe_unused]] glm::vec3 position, const std::string&)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateOneShotSpellPu([[maybe_unused]] glm::vec3 position, const std::string&)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateFireFly([[maybe_unused]] glm::vec3 position)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::TownDesireBoost([[maybe_unused]] int32_t townId, const std::string&, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateAnimatedStatic(glm::vec3 position, const std::string& type, int32_t rotation, int32_t scale)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	registry.Assign<Transform>(entity, position, GetRotation(rotation), GetSize(scale));
	const auto& animated = registry.Assign<AnimatedStatic>(entity, type);
	MeshPackId meshPackId = MeshPackId::Dummy;
	if (animated.type == "Norse Gate")
	{
		meshPackId = MeshPackId::BuildingNorseGate;
	}
	else if (animated.type == "Gate Stone Plinth")
	{
		meshPackId = MeshPackId::ObjectGateTotemPlinthe;
	}
	else if (animated.type == "Piper Cave Entrance")
	{
		meshPackId = MeshPackId::BuildingMineEntrance;
	}
	registry.Assign<Mesh>(entity, static_cast<MeshId>(meshPackId), static_cast<int8_t>(0), static_cast<int8_t>(0));
}

void FeatureScriptCommands::FireFlySpellRewardProb([[maybe_unused]] const std::string& spell,
                                                   [[maybe_unused]] float probability)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateNewTownField(int32_t townId, glm::vec3 position, [[maybe_unused]] int32_t param_3,
                                               float rotation)
{
	auto& registry = Game::instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	registry.Assign<Transform>(entity, position, GetRotation(rotation), GetSize(1000));
	registry.Assign<Field>(entity, townId);
	registry.Assign<Mesh>(entity, MeshPackId::TreeWheat, static_cast<int8_t>(0), static_cast<int8_t>(0));
}

void FeatureScriptCommands::CreateSpellDispenser(int32_t, [[maybe_unused]] glm::vec3 position, const std::string&,
                                                 const std::string&, float, float, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::LoadComputerPlayerPersonality(int32_t, glm::vec3)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::SetComputerPlayerPersonality(const std::string&, glm::vec3, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::SetGlobalLandBalance(int32_t, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::SetLandBalance(const std::string&, int32_t, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::CreateDrinkWaypoint([[maybe_unused]] glm::vec3 position)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::SetTownInfluenceMultiplier([[maybe_unused]] float multiplier)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::SetPlayerInfluenceMultiplier([[maybe_unused]] float multiplier)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::SetTownBalanceBeliefScale([[maybe_unused]] int32_t townId, [[maybe_unused]] float scale)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::StartGameMessage([[maybe_unused]] const std::string& message, [[maybe_unused]] int32_t landNumber)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::AddGameMessageLine([[maybe_unused]] const std::string& message, [[maybe_unused]] int32_t landNumber)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::EditLevel()
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::SetNighttime(float, float, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::MakeLastObjectArtifact(int32_t, const std::string&, float)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

void FeatureScriptCommands::SetLostTownScale([[maybe_unused]] float scale)
{
	// SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "LHScriptX: {}:{}: Function {} not implemented.", __FILE__, __LINE__,
	// __func__);
}

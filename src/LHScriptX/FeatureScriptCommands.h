/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <array>

#include <glm/vec3.hpp>

#include "CommandSignature.h"
#include "Enums.h"

namespace openblack::lhscriptx
{

class FeatureScriptCommands
{
public:
	static const std::array<const ScriptCommandSignature, 106> k_Signatures;

	static void SetATownInfluenceMultiplier(int32_t townId, float mult);
	static void CreateMist(glm::vec3 position, float param2, int32_t param3, float param4, float param5);
	static void CreatePath(int32_t param1, int32_t param2, int32_t param3, int32_t param4);
	static void CreateTown(int32_t townId, glm::vec3 position, const std::string& playerOwner, int32_t notUsed,
	                       const std::string& tribeType);
	static void SetTownBelief(int32_t townId, const std::string& playerOwner, float belief);
	static void SetTownBeliefCap(int32_t townId, const std::string& playerOwner, float belief);
	static void SetTownUninhabitable(int32_t townId);
	static void SetTownCongregationPos(int32_t townId, glm::vec3 position);
	static void CreateAbode(int32_t townId, glm::vec3 position, const std::string& abodeInfo, int32_t rotation, int32_t size,
	                        int32_t foodAmount, int32_t woodAmount);
	static void CreatePlannedAbode(int32_t townId, glm::vec3 position, const std::string& abodeInfo, int32_t rotation,
	                               int32_t size, int32_t foodAmount, int32_t woodAmount);
	static void CreateTownCentre(int32_t townId, glm::vec3 position, const std::string& abodeInfo, int32_t rotation,
	                             int32_t size, int32_t);
	static void CreateTownSpell(int32_t townId, const std::string& spellName);
	static void CreateNewTownSpell(int32_t townId, const std::string& spellName);
	static void CreateTownCentreSpellIcon(int32_t, const std::string&);
	static void CreateSpellIcon(glm::vec3 position, const std::string&, int32_t, int32_t, int32_t);
	static void CreatePlannedSpellIcon(int32_t, glm::vec3 position, const std::string&, int32_t, int32_t, int32_t);
	static void CreateVillager(glm::vec3, glm::vec3, const std::string&);
	static void CreateTownVillager(int32_t townId, glm::vec3 position, const std::string& villagerType, int32_t age);
	static void CreateSpecialTownVillager(int32_t, glm::vec3, int32_t, int32_t);
	static void CreateVillagerPos(glm::vec3, glm::vec3, const std::string&, int32_t);
	static void CreateCitadel(glm::vec3 position, int32_t, const std::string&, int32_t, int32_t);
	static void CreatePlannedCitadel(int32_t, glm::vec3 position, int32_t, const std::string&, int32_t, int32_t);
	static void CreateCreaturePen(glm::vec3 position, int32_t, int32_t, int32_t, int32_t, int32_t);
	static void CreateWorshipSite(glm::vec3 position, int32_t, const std::string&, const std::string&, int32_t, int32_t);
	static void CreatePlannedWorshipSite(glm::vec3 position, int32_t, const std::string&, const std::string&, int32_t, int32_t);
	static void CreateAnimal(glm::vec3 position, int32_t, int32_t, int32_t);
	static void CreateNewAnimal(glm::vec3 position, int32_t, int32_t, int32_t, int32_t);
	static void CreateForest(int32_t forestId, glm::vec3 position);
	static void CreateTree(int32_t forestId, glm::vec3 position, TreeInfo treeType, int32_t rotation, int32_t scale);
	static void CreateNewTree(int32_t forestId, glm::vec3 position, TreeInfo treeType, int32_t isNonScenic, float rotation,
	                          float currentSize, float maxSize);
	static void CreateField(glm::vec3 position, FieldTypeInfo type);
	static void CreateTownField(int32_t townId, glm::vec3 position, FieldTypeInfo type);
	static void CreateFishFarm(glm::vec3 position, int32_t);
	static void CreateTownFishFarm(int32_t townId, glm::vec3 position, int32_t);
	static void CreateFeature(glm::vec3 position, FeatureInfo type, int32_t rotation, int32_t scale, int32_t);
	static void CreateFlowers(glm::vec3 position, int32_t, float, float);
	static void CreateWallSection(glm::vec3 position, int32_t, int32_t, int32_t, int32_t);
	static void CreatePlannedWallSection(glm::vec3 position, int32_t, int32_t, int32_t, int32_t);
	static void CreatePitch(glm::vec3 position, int32_t, int32_t, int32_t, int32_t, int32_t);
	static void CreatePot(glm::vec3 position, PotInfo type, int32_t, int32_t amount);
	static void CreateTownTemporaryPots(int32_t townId, int32_t, int32_t);
	static void CreateMobileObject(glm::vec3 position, MobileObjectInfo type, int32_t rotation, int32_t);
	static void CreateMobileStatic(glm::vec3 position, MobileStaticInfo type, float, float);
	static void CreateMobileUStatic(glm::vec3 position, MobileStaticInfo type, float verticalOffset, float xRotation,
	                                float yRotation, float zRotation, float scale);
	static void CreateDeadTree(glm::vec3 position, const std::string&, int32_t, float, float, float, float);
	static void CreateScaffold(int32_t, glm::vec3, int32_t, int32_t, int32_t);
	static void CountryChange(glm::vec3 position, int32_t);
	static void HeightChange(glm::vec3 position, int32_t);
	static void CreateCreature(glm::vec3 position, int32_t, int32_t);
	static void CreateCreatureFromFile(const std::string& playerName, CreatureType creatureType,
	                                   const std::string& creatureMind, glm::vec3 position);
	static void CreateFlock(int32_t, glm::vec3, glm::vec3, int32_t, int32_t, int32_t);
	static void LoadLandscape(const std::string& path);
	static void Version(float version);
	static void CreateArea(glm::vec3 position, float);
	static void StartCameraPos(glm::vec3 position);
	static void FlyByFile(const std::string& path);
	static void TownNeedsPos(int32_t townId, glm::vec3 position);
	static void CreateFurniture(glm::vec3 position, int32_t, float);
	static void CreateBigForest(glm::vec3 position, BigForestInfo type, float rotation, float scale);
	static void CreateNewBigForest(glm::vec3 position, BigForestInfo type, int32_t unknown, float rotation, float scale);
	static void CreateInfluenceRing(glm::vec3 position, int32_t, float, int32_t);
	static void CreateWeatherClimate(int32_t, int32_t, glm::vec3, float, float);
	static void CreateWeatherClimateRain(int32_t, float, int32_t, int32_t, int32_t);
	static void CreateWeatherClimateTemp(int32_t, float, float);
	static void CreateWeatherClimateWind(int32_t, float, float, float);
	static void CreateWeatherStorm(int32_t, glm::vec3, float, int32_t, const std::string&, const std::string&,
	                               const std::string&, float, glm::vec3);
	static void BrushSize(float, float);
	static void CreateStream(int32_t streamId);
	static void CreateStreamPoint(int32_t streamId, glm::vec3 position);
	static void CreateWaterfall(glm::vec3 position);
	static void CreateArena(glm::vec3 position, float);
	static void CreateFootpath(int32_t footpathId);
	static void CreateFootpathNode(int footpathId, glm::vec3 position);
	static void LinkFootpath(int32_t footpathId);
	static void CreateBonfire(glm::vec3 position, float rotation, float param3, float scale);
	static void CreateBase(glm::vec3 position, int32_t);
	static void CreateNewFeature(glm::vec3 position, const std::string& type, int32_t rotation, int32_t scale, int32_t param5);
	static void SetInteractDesire(float);
	static void ToggleComputerPlayer(const std::string&, int32_t);
	static void SetComputerPlayerCreatureLike(const std::string&, const std::string&);
	static void MultiplayerDebug(int32_t, int32_t);
	static void CreateStreetLantern(glm::vec3 position, int32_t);
	static void CreateStreetLight(glm::vec3 position);
	static void SetLandNumber(int32_t number);
	static void CreateOneShotSpell(glm::vec3 position, const std::string&);
	static void CreateOneShotSpellPu(glm::vec3 position, const std::string&);
	static void CreateFireFly(glm::vec3 position);
	static void TownDesireBoost(int32_t townId, const std::string&, float);
	static void CreateAnimatedStatic(glm::vec3 position, const std::string& type, int32_t rotation, int32_t scale);
	static void FireFlySpellRewardProb(const std::string& spell, float probability);
	static void CreateNewTownField(int32_t townId, glm::vec3 position, FieldTypeInfo type, float rotation);
	static void CreateSpellDispenser(int32_t, glm::vec3 position, const std::string&, const std::string&, float, float, float);
	static void LoadComputerPlayerPersonality(int32_t, glm::vec3);
	static void SetComputerPlayerPersonality(const std::string&, glm::vec3, float);
	static void SetGlobalLandBalance(int32_t, float);
	static void SetLandBalance(const std::string&, int32_t, float);
	static void CreateDrinkWaypoint(glm::vec3 position);
	static void SetTownInfluenceMultiplier(float multiplier);
	static void SetPlayerInfluenceMultiplier(float multiplier);
	static void SetTownBalanceBeliefScale(int32_t townId, float scale);
	static void StartGameMessage(const std::string& message, int32_t landNumber);
	static void AddGameMessageLine(const std::string& message, int32_t landNumber);
	static void EditLevel();
	static void SetNighttime(float, float, float);
	static void MakeLastObjectArtifact(int32_t, const std::string&, float);
	static void SetLostTownScale(float scale);
};

} // namespace openblack::lhscriptx

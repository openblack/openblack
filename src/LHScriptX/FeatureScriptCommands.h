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

class FeatureScriptCommands
{
public:
	static const std::array<const ScriptCommandSignature, 105> Signatures;

	static void CreateMist(const ScriptCommandContext& ctx);
	static void CreatePath(const ScriptCommandContext& ctx);
	static void CreateTown(const ScriptCommandContext& ctx);
	static void SetTownBelief(const ScriptCommandContext& ctx);
	static void SetTownBeliefCap(const ScriptCommandContext& ctx);
	static void SetTownUninhabitable(const ScriptCommandContext& ctx);
	static void SetTownCongregationPos(const ScriptCommandContext& ctx);
	static void CreateAbode(const ScriptCommandContext& ctx);
	static void CreatePlannedAbode(const ScriptCommandContext& ctx);
	static void CreateTownCentre(const ScriptCommandContext& ctx);
	static void CreateTownSpell(const ScriptCommandContext& ctx);
	static void CreateNewTownSpell(const ScriptCommandContext& ctx);
	static void CreateTownCentreSpellIcon(const ScriptCommandContext& ctx);
	static void CreateSpellIcon(const ScriptCommandContext& ctx);
	static void CreatePlannedSpellIcon(const ScriptCommandContext& ctx);
	static void CreateVillager(const ScriptCommandContext& ctx);
	static void CreateTownVillager(const ScriptCommandContext& ctx);
	static void CreateSpecialTownVillager(const ScriptCommandContext& ctx);
	static void CreateVillagerPos(const ScriptCommandContext& ctx);
	static void CreateCitadel(const ScriptCommandContext& ctx);
	static void CreatePlannedCitadel(const ScriptCommandContext& ctx);
	static void CreateCreaturePen(const ScriptCommandContext& ctx);
	static void CreateWorshipSite(const ScriptCommandContext& ctx);
	static void CreatePlannedWorshipSite(const ScriptCommandContext& ctx);
	static void CreateAnimal(const ScriptCommandContext& ctx);
	static void CreateNewAnimal(const ScriptCommandContext& ctx);
	static void CreateForest(const ScriptCommandContext& ctx);
	static void CreateTree(const ScriptCommandContext& ctx);
	static void CreateNewTree(const ScriptCommandContext& ctx);
	static void CreateField(const ScriptCommandContext& ctx);
	static void CreateTownField(const ScriptCommandContext& ctx);
	static void CreateFishFarm(const ScriptCommandContext& ctx);
	static void CreateTownFishFarm(const ScriptCommandContext& ctx);
	static void CreateFeature(const ScriptCommandContext& ctx);
	static void CreateFlowers(const ScriptCommandContext& ctx);
	static void CreateWallSection(const ScriptCommandContext& ctx);
	static void CreatePlannedWallSection(const ScriptCommandContext& ctx);
	static void CreatePitch(const ScriptCommandContext& ctx);
	static void CreatePot(const ScriptCommandContext& ctx);
	static void CreateTownTemporaryPots(const ScriptCommandContext& ctx);
	static void CreateMobileobject(const ScriptCommandContext& ctx);
	static void CreateMobileStatic(const ScriptCommandContext& ctx);
	static void CreateMobileUStatic(const ScriptCommandContext& ctx);
	static void CreateDeadTree(const ScriptCommandContext& ctx);
	static void CreateScaffold(const ScriptCommandContext& ctx);
	static void CountryChange(const ScriptCommandContext& ctx);
	static void HeightChange(const ScriptCommandContext& ctx);
	static void CreateCreature(const ScriptCommandContext& ctx);
	static void CreateCreatureFromFile(const ScriptCommandContext& ctx);
	static void CreateFlock(const ScriptCommandContext& ctx);
	static void LoadLandscape(const ScriptCommandContext& ctx);
	static void Version(const ScriptCommandContext& ctx);
	static void CreateArea(const ScriptCommandContext& ctx);
	static void StartCameraPos(const ScriptCommandContext& ctx);
	static void FlyByFile(const ScriptCommandContext& ctx);
	static void TownNeedsPos(const ScriptCommandContext& ctx);
	static void CreateFurniture(const ScriptCommandContext& ctx);
	static void CreateBigForest(const ScriptCommandContext& ctx);
	static void CreateNewBigForest(const ScriptCommandContext& ctx);
	static void CreateInfluenceRing(const ScriptCommandContext& ctx);
	static void CreateWeatherClimate(const ScriptCommandContext& ctx);
	static void CreateWeatherClimateRain(const ScriptCommandContext& ctx);
	static void CreateWeatherClimateTemp(const ScriptCommandContext& ctx);
	static void CreateWeatherClimateWind(const ScriptCommandContext& ctx);
	static void CreateWeatherStorm(const ScriptCommandContext& ctx);
	static void BrushSize(const ScriptCommandContext& ctx);
	static void CreateStream(const ScriptCommandContext& ctx);
	static void CreateStreamPoint(const ScriptCommandContext& ctx);
	static void CreateWaterfall(const ScriptCommandContext& ctx);
	static void CreateArena(const ScriptCommandContext& ctx);
	static void CreateFootpath(const ScriptCommandContext& ctx);
	static void CreateFootpathNode(const ScriptCommandContext& ctx);
	static void LinkFootpath(const ScriptCommandContext& ctx);
	static void CreateBonfire(const ScriptCommandContext& ctx);
	static void CreateBase(const ScriptCommandContext& ctx);
	static void CreateNewFeature(const ScriptCommandContext& ctx);
	static void SetInteractDesire(const ScriptCommandContext& ctx);
	static void ToggleComputerPlayer(const ScriptCommandContext& ctx);
	static void SetComputerPlayerCreatureLike(const ScriptCommandContext& ctx);
	static void MultiplayerDebug(const ScriptCommandContext& ctx);
	static void CreateStreetLantern(const ScriptCommandContext& ctx);
	static void CreateStreetLight(const ScriptCommandContext& ctx);
	static void SetLandNumber(const ScriptCommandContext& ctx);
	static void CreateOneShotSpell(const ScriptCommandContext& ctx);
	static void CreateOneShotSpellPu(const ScriptCommandContext& ctx);
	static void CreateFireFly(const ScriptCommandContext& ctx);
	static void TownDesireBoost(const ScriptCommandContext& ctx);
	static void CreateAnimatedStatic(const ScriptCommandContext& ctx);
	static void FireFlySpellRewardProb(const ScriptCommandContext& ctx);
	static void CreateNewTownField(const ScriptCommandContext& ctx);
	static void CreateSpellDispenser(const ScriptCommandContext& ctx);
	static void LoadComputerPlayerPersonallty(const ScriptCommandContext& ctx);
	static void SetComputerPlayerPersonallty(const ScriptCommandContext& ctx);
	static void SetGlobalLandBalance(const ScriptCommandContext& ctx);
	static void SetLandBalance(const ScriptCommandContext& ctx);
	static void CreateDrinkWaypoint(const ScriptCommandContext& ctx);
	static void SetTownInfluenceMultiplier(const ScriptCommandContext& ctx);
	static void SetPlayerInfluenceMultiplier(const ScriptCommandContext& ctx);
	static void SetTownBalanceBeliefScale(const ScriptCommandContext& ctx);
	static void StartGameMessage(const ScriptCommandContext& ctx);
	static void AddGameMessageLine(const ScriptCommandContext& ctx);
	static void EditLevel(const ScriptCommandContext& ctx);
	static void SetNighttime(const ScriptCommandContext& ctx);
	static void MakeLastObjectArtifact(const ScriptCommandContext& ctx);
	static void SetLostTownScale(const ScriptCommandContext& ctx);
};

} // namespace openblack::lhscriptx

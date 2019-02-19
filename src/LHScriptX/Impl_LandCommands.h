#pragma once
#ifndef OPENBLACK_LHSCRIPTX_IMPL_LANDCOMMANDS_H
#define OPENBLACK_LHSCRIPTX_IMPL_LANDCOMMANDS_H

#include "Command.h"

namespace OpenBlack {

namespace LHScriptX {

class Impl_LandCommands {
public:
	// custom openblack
	static void setWindowTitle(const ScriptCommandContext &ctx);

	// game
	static void createMist(const ScriptCommandContext &ctx);
	static void createPath(const ScriptCommandContext &ctx);
	static void createTown(const ScriptCommandContext &ctx);
	static void setTownBelief(const ScriptCommandContext &ctx);
	static void setTownBeliefCap(const ScriptCommandContext &ctx);
	static void setTownUninhabitable(const ScriptCommandContext &ctx);
	static void setTownCongregationPos(const ScriptCommandContext &ctx);
	static void createAbode(const ScriptCommandContext &ctx);
	static void createPlannedAbode(const ScriptCommandContext &ctx);
	static void createTownCentre(const ScriptCommandContext &ctx);
	static void createTownSpell(const ScriptCommandContext &ctx);
	static void createNewTownSpell(const ScriptCommandContext &ctx);
	static void createTownCentreSpellIcon(const ScriptCommandContext &ctx);
	static void createSpellIcon(const ScriptCommandContext &ctx);
	static void createPlannedSpellIcon(const ScriptCommandContext &ctx);
	static void createVillager(const ScriptCommandContext &ctx);
	static void createTownVillager(const ScriptCommandContext &ctx);
	static void createSpecialTownVillager(const ScriptCommandContext &ctx);
	static void createVillagerPos(const ScriptCommandContext &ctx);
	static void createCitadel(const ScriptCommandContext &ctx);
	static void createPlannedCitadel(const ScriptCommandContext &ctx);
	static void createCreaturePen(const ScriptCommandContext &ctx);
	static void createWorshipSite(const ScriptCommandContext &ctx);
	static void createPlannedWorshipSite(const ScriptCommandContext &ctx);
	static void createAnimal(const ScriptCommandContext &ctx);
	static void createNewAnimal(const ScriptCommandContext &ctx);
	static void createForest(const ScriptCommandContext &ctx);
	static void createTree(const ScriptCommandContext &ctx);
	static void createNewTree(const ScriptCommandContext &ctx);
	static void createField(const ScriptCommandContext &ctx);
	static void createTownField(const ScriptCommandContext &ctx);
	static void createFishFarm(const ScriptCommandContext &ctx);
	static void createTownFishFarm(const ScriptCommandContext &ctx);
	static void createFeature(const ScriptCommandContext &ctx);
	static void createFlowers(const ScriptCommandContext &ctx);
	static void createWallSection(const ScriptCommandContext &ctx);
	static void createPlannedWallSection(const ScriptCommandContext &ctx);
	static void createPitch(const ScriptCommandContext &ctx);
	static void createPot(const ScriptCommandContext &ctx);
	static void createTownTemporaryPots(const ScriptCommandContext &ctx);
	static void createMobileobject(const ScriptCommandContext &ctx);
	static void createMobilestatic(const ScriptCommandContext &ctx);
	static void createMobileStatic(const ScriptCommandContext &ctx);
	static void createDeadTree(const ScriptCommandContext &ctx);
	static void createScaffold(const ScriptCommandContext &ctx);
	static void countryChange(const ScriptCommandContext &ctx);
	static void heightChange(const ScriptCommandContext &ctx);
	static void createCreature(const ScriptCommandContext &ctx);
	static void createCreatureFromFile(const ScriptCommandContext &ctx);
	static void createFlock(const ScriptCommandContext &ctx);
	static void loadLandscape(const ScriptCommandContext &ctx);
	static void version(const ScriptCommandContext &ctx);
	static void createArea(const ScriptCommandContext &ctx);
	static void startCameraPos(const ScriptCommandContext &ctx);
	static void flyByFile(const ScriptCommandContext &ctx);
	static void townNeedsPos(const ScriptCommandContext &ctx);
	static void createFurniture(const ScriptCommandContext &ctx);
	static void createBigForest(const ScriptCommandContext &ctx);
	static void createNewBigForest(const ScriptCommandContext &ctx);
	static void createInfluenceRing(const ScriptCommandContext &ctx);
	static void createWeatherClimate(const ScriptCommandContext &ctx);
	static void createWeatherClimateRain(const ScriptCommandContext &ctx);
	static void createWeatherClimateTemp(const ScriptCommandContext &ctx);
	static void createWeatherClimateWind(const ScriptCommandContext &ctx);
	static void createWeatherStorm(const ScriptCommandContext &ctx);
	static void brushSize(const ScriptCommandContext &ctx);
	static void createStream(const ScriptCommandContext &ctx);
	static void createStreamPoint(const ScriptCommandContext &ctx);
	static void createWaterfall(const ScriptCommandContext &ctx);
	static void createArena(const ScriptCommandContext &ctx);
	static void createFootpath(const ScriptCommandContext &ctx);
	static void createFootpathNode(const ScriptCommandContext &ctx);
	static void linkFootpath(const ScriptCommandContext &ctx);
	static void createBonfire(const ScriptCommandContext &ctx);
	static void createBase(const ScriptCommandContext &ctx);
	static void createNewFeature(const ScriptCommandContext &ctx);
	static void setInteractDesire(const ScriptCommandContext &ctx);
	static void toggleComputerPlayer(const ScriptCommandContext &ctx);
	static void setComputerPlayerCreatureLike(const ScriptCommandContext &ctx);
	static void multiplayerDebug(const ScriptCommandContext &ctx);
	static void createStreetLantern(const ScriptCommandContext &ctx);
	static void createStreetLight(const ScriptCommandContext &ctx);
	static void setLandNumber(const ScriptCommandContext &ctx);
	static void createOneShotSpell(const ScriptCommandContext &ctx);
	static void createOneShotSpellPu(const ScriptCommandContext &ctx);
	static void createFireFly(const ScriptCommandContext &ctx);
	static void townDesireBoost(const ScriptCommandContext &ctx);
	static void createAnimatedStatic(const ScriptCommandContext &ctx);
	static void fireFlySpellRewardProb(const ScriptCommandContext &ctx);
	static void createNewTownField(const ScriptCommandContext &ctx);
	static void createSpellDispenser(const ScriptCommandContext &ctx);
	static void loadComputerPlayerPersonallty(const ScriptCommandContext &ctx);
	static void setComputerPlayerPersonallty(const ScriptCommandContext &ctx);
	static void setGlobalLandBalance(const ScriptCommandContext &ctx);
	static void setLandBalance(const ScriptCommandContext &ctx);
	static void createDrinkWaypoint(const ScriptCommandContext &ctx);
	static void setTownInfluenceMultiplier(const ScriptCommandContext &ctx);
	static void setPlayerInfluenceMultiplier(const ScriptCommandContext &ctx);
	static void setTownBalanceBeliefScale(const ScriptCommandContext &ctx);
	static void startGameMessage(const ScriptCommandContext &ctx);
	static void addGameMessageLine(const ScriptCommandContext &ctx);
	static void editLevel(const ScriptCommandContext &ctx);
	static void setNighttime(const ScriptCommandContext &ctx);
	static void makeLastObjectArtifact(const ScriptCommandContext &ctx);
	static void setLostTownScale(const ScriptCommandContext &ctx);

	static const ScriptCommandDef Definitions[105];
};

}
}
#endif

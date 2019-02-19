#include "Impl_LandCommands.h"

#include <iostream>

#include <Game.h>

using namespace OpenBlack::LHScriptX;

void Impl_LandCommands::setWindowTitle(const ScriptCommandContext &ctx)
{
	std::string title = ctx.GetParameter<std::string&>(0);
	ctx.GetGame().GetWindow().SetTitle(title);
}

void Impl_LandCommands::version(const ScriptCommandContext &ctx)
{
	float version = ctx.GetParameter<float>(0);
	std::cout << "[LHSCRIPTX] VERSION called: " << version << std::endl;
}

void Impl_LandCommands::createMist(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createPath(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createTown(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setTownBelief(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setTownBeliefCap(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setTownUninhabitable(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setTownCongregationPos(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createAbode(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createPlannedAbode(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createTownCentre(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createTownSpell(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createNewTownSpell(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createTownCentreSpellIcon(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createSpellIcon(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createPlannedSpellIcon(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createVillager(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createTownVillager(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createSpecialTownVillager(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createVillagerPos(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createCitadel(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createPlannedCitadel(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createCreaturePen(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createWorshipSite(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createPlannedWorshipSite(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createAnimal(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createNewAnimal(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createForest(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createTree(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createNewTree(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createField(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createTownField(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createFishFarm(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createTownFishFarm(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createFeature(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createFlowers(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createWallSection(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createPlannedWallSection(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createPitch(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createPot(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createTownTemporaryPots(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createMobileobject(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createMobilestatic(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createMobileStatic(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createDeadTree(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createScaffold(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::countryChange(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::heightChange(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createCreature(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createCreatureFromFile(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createFlock(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::loadLandscape(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createArea(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::startCameraPos(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::flyByFile(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::townNeedsPos(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createFurniture(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createBigForest(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createNewBigForest(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createInfluenceRing(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createWeatherClimate(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createWeatherClimateRain(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createWeatherClimateTemp(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createWeatherClimateWind(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createWeatherStorm(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::brushSize(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createStream(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createStreamPoint(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createWaterfall(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createArena(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createFootpath(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createFootpathNode(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::linkFootpath(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createBonfire(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createBase(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createNewFeature(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setInteractDesire(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::toggleComputerPlayer(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setComputerPlayerCreatureLike(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::multiplayerDebug(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createStreetLantern(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createStreetLight(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setLandNumber(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createOneShotSpell(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createOneShotSpellPu(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createFireFly(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::townDesireBoost(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createAnimatedStatic(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::fireFlySpellRewardProb(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createNewTownField(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createSpellDispenser(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::loadComputerPlayerPersonallty(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setComputerPlayerPersonallty(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setGlobalLandBalance(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setLandBalance(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::createDrinkWaypoint(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setTownInfluenceMultiplier(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setPlayerInfluenceMultiplier(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setTownBalanceBeliefScale(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::startGameMessage(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::addGameMessageLine(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::editLevel(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setNighttime(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::makeLastObjectArtifact(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

void Impl_LandCommands::setLostTownScale(const ScriptCommandContext &ctx)
{
	throw std::runtime_error(std::string{} +"Function " + __func__ + " not implemented. " + __FILE__ + ":" + std::to_string(__LINE__));
}

#pragma once
#ifndef OPENBLACK_SCRIPT_FUNCTIONS_H
#define OPENBLACK_SCRIPT_FUNCTIONS_H

#include <Common/Types.h>

#include <vector>

namespace OpenBlack
{
	namespace Script
	{
		enum Type {
			kTypeNone = 0,
			kTypeString,     // L
			kTypeFloat,		 // F
			kTypeNumber,	 // N
			kTypeVector		 // A
		};

		class FunctionContext;

		class Variable {
		private:
			Type _type;
			union {
				int32_t _int;
				float _float;
				std::string *_string;
			} _value;
		};

		typedef std::vector<class Variable> Parameters;

		class Functions {
		private:
			typedef void (*funcPtr)(FunctionContext &ctx);

			struct FunctionPointer {
				uint32_t id;
				const char *name;
				Type parameters[12];
				funcPtr func;
			};
			
			static void createMist(FunctionContext &ctx);
			static void createPath(FunctionContext &ctx);
			static void createTown(FunctionContext &ctx);
			static void setTownBelief(FunctionContext &ctx);
			static void setTownBeliefCap(FunctionContext &ctx);
			static void setTownUninhabitable(FunctionContext &ctx);
			static void setTownCongregationPos(FunctionContext &ctx);
			static void createAbode(FunctionContext &ctx);
			static void createPlannedAbode(FunctionContext &ctx);
			static void createTownCentre(FunctionContext &ctx);
			static void createTownSpell(FunctionContext &ctx);
			static void createNewTownSpell(FunctionContext &ctx);
			static void createTownCentreSpellIcon(FunctionContext &ctx);
			static void createSpellIcon(FunctionContext &ctx);
			static void createPlannedSpellIcon(FunctionContext &ctx);
			static void createVillager(FunctionContext &ctx);
			static void createTownVillager(FunctionContext &ctx);
			static void createSpecialTownVillager(FunctionContext &ctx);
			static void createVillagerPos(FunctionContext &ctx);
			static void createCitadel(FunctionContext &ctx);
			static void createPlannedCitadel(FunctionContext &ctx);
			static void createCreaturePen(FunctionContext &ctx);
			static void createWorshipSite(FunctionContext &ctx);
			static void createPlannedWorshipSite(FunctionContext &ctx);
			static void createAnimal(FunctionContext &ctx);
			static void createNewAnimal(FunctionContext &ctx);
			static void createForest(FunctionContext &ctx);
			static void createTree(FunctionContext &ctx);
			static void createNewTree(FunctionContext &ctx);
			static void createField(FunctionContext &ctx);
			static void createTownField(FunctionContext &ctx);
			static void createFishFarm(FunctionContext &ctx);
			static void createTownFishFarm(FunctionContext &ctx);
			static void createFeature(FunctionContext &ctx);
			static void createFlowers(FunctionContext &ctx);
			static void createWallSection(FunctionContext &ctx);
			static void createPlannedWallSection(FunctionContext &ctx);
			static void createPitch(FunctionContext &ctx);
			static void createPot(FunctionContext &ctx);
			static void createTownTemporaryPots(FunctionContext &ctx);
			static void createMobileobject(FunctionContext &ctx);
			static void createMobilestatic(FunctionContext &ctx);
			static void createMobileStatic(FunctionContext &ctx);
			static void createDeadTree(FunctionContext &ctx);
			static void createScaffold(FunctionContext &ctx);
			static void countryChange(FunctionContext &ctx);
			static void heightChange(FunctionContext &ctx);
			static void createCreature(FunctionContext &ctx);
			static void createCreatureFromFile(FunctionContext &ctx);
			static void createFlock(FunctionContext &ctx);
			static void loadLandscape(FunctionContext &ctx);
			static void version(FunctionContext &ctx);
			static void createArea(FunctionContext &ctx);
			static void startCameraPos(FunctionContext &ctx);
			static void flyByFile(FunctionContext &ctx);
			static void townNeedsPos(FunctionContext &ctx);
			static void createFurniture(FunctionContext &ctx);
			static void createBigForest(FunctionContext &ctx);
			static void createNewBigForest(FunctionContext &ctx);
			static void createInfluenceRing(FunctionContext &ctx);
			static void createWeatherClimate(FunctionContext &ctx);
			static void createWeatherClimateRain(FunctionContext &ctx);
			static void createWeatherClimateTemp(FunctionContext &ctx);
			static void createWeatherClimateWind(FunctionContext &ctx);
			static void createWeatherStorm(FunctionContext &ctx);
			static void brushSize(FunctionContext &ctx);
			static void createStream(FunctionContext &ctx);
			static void createStreamPoint(FunctionContext &ctx);
			static void createWaterfall(FunctionContext &ctx);
			static void createArena(FunctionContext &ctx);
			static void createFootpath(FunctionContext &ctx);
			static void createFootpathNode(FunctionContext &ctx);
			static void linkFootpath(FunctionContext &ctx);
			static void createBonfire(FunctionContext &ctx);
			static void createBase(FunctionContext &ctx);
			static void createNewFeature(FunctionContext &ctx);
			static void setInteractDesire(FunctionContext &ctx);
			static void toggleComputerPlayer(FunctionContext &ctx);
			static void setComputerPlayerCreatureLike(FunctionContext &ctx);
			static void multiplayerDebug(FunctionContext &ctx);
			static void createStreetLantern(FunctionContext &ctx);
			static void createStreetLight(FunctionContext &ctx);
			static void setLandNumber(FunctionContext &ctx);
			static void createOneShotSpell(FunctionContext &ctx);
			static void createOneShotSpellPu(FunctionContext &ctx);
			static void createFireFly(FunctionContext &ctx);
			static void townDesireBoost(FunctionContext &ctx);
			static void createAnimatedStatic(FunctionContext &ctx);
			static void fireFlySpellRewardProb(FunctionContext &ctx);
			static void createNewTownField(FunctionContext &ctx);
			static void createSpellDispenser(FunctionContext &ctx);
			static void loadComputerPlayerPersonallty(FunctionContext &ctx);
			static void setComputerPlayerPersonallty(FunctionContext &ctx);
			static void setGlobalLandBalance(FunctionContext &ctx);
			static void setLandBalance(FunctionContext &ctx);
			static void createDrinkWaypoint(FunctionContext &ctx);
			static void setTownInfluenceMultiplier(FunctionContext &ctx);
			static void setPlayerInfluenceMultiplier(FunctionContext &ctx);
			static void setTownBalanceBeliefScale(FunctionContext &ctx);
			static void startGameMessage(FunctionContext &ctx);
			static void addGameMessageLine(FunctionContext &ctx);
			static void editLevel(FunctionContext &ctx);
			static void setNighttime(FunctionContext &ctx);
			static void makeLastObjectArtifact(FunctionContext &ctx);
			static void setLostTownScale(FunctionContext &ctx);

			/* Map functions */
			static void setNoPlayers(FunctionContext &ctx);
			static void loadTribeDance(FunctionContext &ctx);
			static void setDate(FunctionContext &ctx);
			static void setTime(FunctionContext &ctx);
			static void setTurnsPerYear(FunctionContext &ctx);
			static void setGameTickTime(FunctionContext &ctx);
			static void loadFeatureScript(FunctionContext &ctx);
			static void pauseGame(FunctionContext &ctx);
			//static void createCreature(FunctionContext &ctx);
			static void outputVillagers(FunctionContext &ctx);
			static void outputTown(FunctionContext &ctx);
			static void outputCreatures(FunctionContext &ctx);
			static void outputCollide(FunctionContext &ctx);
			static void outputAlloc(FunctionContext &ctx);
			static void saveForNetDebug(FunctionContext &ctx);
			static void loadForNetDebug(FunctionContext &ctx);
			//static void loadLandscape(FunctionContext &ctx);
			static void loadGameScript(FunctionContext &ctx);
			static void loadRawGameScript(FunctionContext &ctx);
			static void loadLanguage(FunctionContext &ctx);

			static const FunctionPointer kLandFunctionPointers[];
			static const FunctionPointer kMapFunctionPointers[];
		public:
			static FunctionPointer* GetFunctionFromName(std::string name);
		};
	}
}

#endif

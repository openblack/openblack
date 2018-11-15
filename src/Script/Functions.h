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
			typedef void (Functions::*funcPtr)(FunctionContext &ctx);

			struct FunctionPointer {
				uint32_t id;
				const char *name;
				Type parameters[12];
				funcPtr func;
			};

			//static const FunctionPointer kFunctionPointers[];

			void createMist(FunctionContext &ctx);
			void createPath(FunctionContext &ctx);
			void createTown(FunctionContext &ctx);
			void setTownBelief(FunctionContext &ctx);
			void setTownBeliefCap(FunctionContext &ctx);
			void setTownUninhabitable(FunctionContext &ctx);
			void setTownCongregationPos(FunctionContext &ctx);
			void createAbode(FunctionContext &ctx);
			void createPlannedAbode(FunctionContext &ctx);
			void createTownCentre(FunctionContext &ctx);
			void createTownSpell(FunctionContext &ctx);
			void createNewTownSpell(FunctionContext &ctx);
			void createTownCentreSpellIcon(FunctionContext &ctx);
			void createSpellIcon(FunctionContext &ctx);
			void createPlannedSpellIcon(FunctionContext &ctx);
			void createVillager(FunctionContext &ctx);
			void createTownVillager(FunctionContext &ctx);
			void createSpecialTownVillager(FunctionContext &ctx);
			void createVillagerPos(FunctionContext &ctx);
			void createCitadel(FunctionContext &ctx);
			void createPlannedCitadel(FunctionContext &ctx);
			void createCreaturePen(FunctionContext &ctx);
			void createWorshipSite(FunctionContext &ctx);
			void createPlannedWorshipSite(FunctionContext &ctx);
			void createAnimal(FunctionContext &ctx);
			void createNewAnimal(FunctionContext &ctx);
			void createForest(FunctionContext &ctx);
			void createTree(FunctionContext &ctx);
			void createNewTree(FunctionContext &ctx);
			void createField(FunctionContext &ctx);
			void createTownField(FunctionContext &ctx);
			void createFishFarm(FunctionContext &ctx);
			void createTownFishFarm(FunctionContext &ctx);
			void createFeature(FunctionContext &ctx);
			void createFlowers(FunctionContext &ctx);
			void createWallSection(FunctionContext &ctx);
			void createPlannedWallSection(FunctionContext &ctx);
			void createPitch(FunctionContext &ctx);
			void createPot(FunctionContext &ctx);
			void createTownTemporaryPots(FunctionContext &ctx);
			void createMobileobject(FunctionContext &ctx);
			void createMobilestatic(FunctionContext &ctx);
			void createMobileStatic(FunctionContext &ctx);
			void createDeadTree(FunctionContext &ctx);
			void createScaffold(FunctionContext &ctx);
			void countryChange(FunctionContext &ctx);
			void heightChange(FunctionContext &ctx);
			void createCreature(FunctionContext &ctx);
			void createCreatureFromFile(FunctionContext &ctx);
			void createFlock(FunctionContext &ctx);
			void loadLandscape(FunctionContext &ctx);
			void version(FunctionContext &ctx);
			void createArea(FunctionContext &ctx);
			void startCameraPos(FunctionContext &ctx);
			void flyByFile(FunctionContext &ctx);
			void townNeedsPos(FunctionContext &ctx);
			void createFurniture(FunctionContext &ctx);
			void createBigForest(FunctionContext &ctx);
			void createNewBigForest(FunctionContext &ctx);
			void createInfluenceRing(FunctionContext &ctx);
			void createWeatherClimate(FunctionContext &ctx);
			void createWeatherClimateRain(FunctionContext &ctx);
			void createWeatherClimateTemp(FunctionContext &ctx);
			void createWeatherClimateWind(FunctionContext &ctx);
			void createWeatherStorm(FunctionContext &ctx);
			void brushSize(FunctionContext &ctx);
			void createStream(FunctionContext &ctx);
			void createStreamPoint(FunctionContext &ctx);
			void createWaterfall(FunctionContext &ctx);
			void createArena(FunctionContext &ctx);
			void createFootpath(FunctionContext &ctx);
			void createFootpathNode(FunctionContext &ctx);
			void linkFootpath(FunctionContext &ctx);
			void createBonfire(FunctionContext &ctx);
			void createBase(FunctionContext &ctx);
			void createNewFeature(FunctionContext &ctx);
			void setInteractDesire(FunctionContext &ctx);
			void toggleComputerPlayer(FunctionContext &ctx);
			void setComputerPlayerCreatureLike(FunctionContext &ctx);
			void multiplayerDebug(FunctionContext &ctx);
			void createStreetLantern(FunctionContext &ctx);
			void createStreetLight(FunctionContext &ctx);
			void setLandNumber(FunctionContext &ctx);
			void createOneShotSpell(FunctionContext &ctx);
			void createOneShotSpellPu(FunctionContext &ctx);
			void createFireFly(FunctionContext &ctx);
			void townDesireBoost(FunctionContext &ctx);
			void createAnimatedStatic(FunctionContext &ctx);
			void fireFlySpellRewardProb(FunctionContext &ctx);
			void createNewTownField(FunctionContext &ctx);
			void createSpellDispenser(FunctionContext &ctx);
			void loadComputerPlayerPersonallty(FunctionContext &ctx);
			void setComputerPlayerPersonallty(FunctionContext &ctx);
			void setGlobalLandBalance(FunctionContext &ctx);
			void setLandBalance(FunctionContext &ctx);
			void createDrinkWaypoint(FunctionContext &ctx);
			void setTownInfluenceMultiplier(FunctionContext &ctx);
			void setPlayerInfluenceMultiplier(FunctionContext &ctx);
			void setTownBalanceBeliefScale(FunctionContext &ctx);
			void startGameMessage(FunctionContext &ctx);
			void addGameMessageLine(FunctionContext &ctx);
			void editLevel(FunctionContext &ctx);
			void setNighttime(FunctionContext &ctx);
			void makeLastObjectArtifact(FunctionContext &ctx);
			void setLostTownScale(FunctionContext &ctx);

			static const FunctionPointer kFunctionPointers[];


			/*
			void setNoPlayers(FunctionContext &ctx);
			void loadTribeDance(FunctionContext &ctx);
			void setDate(FunctionContext &ctx);
			void setTime(FunctionContext &ctx);
			void setTurnsPerYear(FunctionContext &ctx);
			void setGameTickTime(FunctionContext &ctx);
			void loadFeatureScript(FunctionContext &ctx);
			void pauseGame(FunctionContext &ctx);
			void createCreature(FunctionContext &ctx);
			void outputVillagers(FunctionContext &ctx);
			void outputTown(FunctionContext &ctx);
			void outputCreatures(FunctionContext &ctx);
			void outputCollide(FunctionContext &ctx);
			void outputAlloc(FunctionContext &ctx);
			void saveForNetDebug(FunctionContext &ctx);
			void loadForNetDebug(FunctionContext &ctx);
			void loadLandscape(FunctionContext &ctx);
			void loadGameScript(FunctionContext &ctx);
			void loadRawGameScript(FunctionContext &ctx);
			void loadLanguage(FunctionContext &ctx);

			const FunctionPointer kFunctionPointers[] = {
				{ 0,   "SET_NO_PLAYERS",       { kTypeNumber },                                        &Functions::setNoPlayers },
				{ 1,   "LOAD_TRIBE_DANCE",     { kTypeVector, kTypeNumber },                           &Functions::loadTribeDance },
				{ 2,   "SET_DATE",             { kTypeNumber, kTypeNumber, kTypeNumber },              &Functions::setDate },
				{ 3,   "SET_TIME",             { kTypeNumber, kTypeNumber, kTypeNumber },              &Functions::setTime },
				{ 4,   "SET_TURNS_PER_YEAR",   { kTypeNumber },                                        &Functions::setTurnsPerYear },
				{ 5,   "SET_GAME_TICK_TIME",   { kTypeNumber },                                        &Functions::setGameTickTime },
				{ 6,   "LOAD_FEATURE_SCRIPT",  { kTypeVector },                                        &Functions::loadFeatureScript },
				{ 7,   "PAUSE_GAME",           {  },                                                   &Functions::pauseGame },
				{ 8,   "CREATE_CREATURE",      { kTypeNumber, kTypeNumber, kTypeNumber, kTypeNumber }, &Functions::createCreature },
				{ 9,   "OUTPUT_VILLAGERS",     {  },                                                   &Functions::outputVillagers },
				{ 10,  "OUTPUT_TOWN",          {  },                                                   &Functions::outputTown },
				{ 11,  "OUTPUT_CREATURES",     {  },                                                   &Functions::outputCreatures },
				{ 12,  "OUTPUT_COLLIDE",       {  },                                                   &Functions::outputCollide },
				{ 13,  "OUTPUT_ALLOC",         {  },                                                   &Functions::outputAlloc },
				{ 14,  "SAVE_FOR_NET_DEBUG",   { kTypeNumber, kTypeNumber },                           &Functions::saveForNetDebug },
				{ 15,  "LOAD_FOR_NET_DEBUG",   { kTypeNumber },                                        &Functions::loadForNetDebug },
				{ 16,  "LOAD_LANDSCAPE",       { kTypeVector },                                        &Functions::loadLandscape },
				{ 17,  "LOAD_GAME_SCRIPT",     { kTypeVector },                                        &Functions::loadGameScript },
				{ 18,  "LOAD_RAW_GAME_SCRIPT", { kTypeVector },                                        &Functions::loadRawGameScript },
				{ 19,  "LOAD_LANGUAGE",        {  },                                                   &Functions::loadLanguage },
			};
			*/
		};
	}
}

#endif

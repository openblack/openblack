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
#include <cstdint>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "AllMeshes.h"
#include "Enums.h"

namespace openblack
{
#pragma pack(push, 1)
struct GTribeInfo
{
	Tribe tribeType;
	WorshipSiteInfo worshipSiteInfo;
	float percentFemaleInTown;
};

struct GObjectInfo
{
	ObjectType type;
	AlignmentType alignmentType;
	std::array<char, 0x30> debugString;
	SoundCollisionType collideSound;
	ImmersionEffectType immersion;
	HelpText helpStartEnum;
	HelpText helpEndEnum;
	HelpSystemMessageSet helpMessage;
	HelpSystemCondition helpCondition;
	HelpSystemMessageSet helpInHand;
	HelpSystemCondition handCondition;
	float foodValue;
	uint32_t woodValue;
	FoodType foodType;
	float defenceEffectBurn;
	float defenceEffectCrush;
	float defenceEffectHit;
	float defenceEffectHeal;
	float defenceEffectFlyAway;
	float defenceEffectAlignmentModification;
	float defenceEffectBeliefModification;
	float defenceMultiplierBurn;
	float defenceMultiplierCrush;
	float defenceMultiplierHit;
	float defenceMultiplierHeal;
	float defenceMultiplierFlyAway;
	float defenceMultiplierAlignmentModification;
	float defenceMultiplierBeliefModification;
	float weight;
	float heatCapacity;
	float combustionTemperature;
	float burningPriority;
	uint32_t canCreatureUseForBuilding;
	uint32_t canCreatureInteractWithMe;
	uint32_t canCreatureAttackMe;
	uint32_t canCreaturePlayWithMe;
	uint32_t canCreatureInspectMe;
	uint32_t canCreatureGiveMeToLiving;
	uint32_t canCreatureBringMeBackToTheCitadel;
	VillagerStates villagerInteractState;
	VillagerStates villagerInteractStateForBuilding;
	float villagerInteractDesire;
	float sacrificeValue;
	float impressiveValue;
	float aggressorValue;
	float villagerImpressiveValue;
	float artifactMultiplier;
	float drawImportance;
	float computerAttackDesire;
};

struct GMiscInfo: GObjectInfo
{
	MeshId normal;
};

struct GMobileInfo: GObjectInfo
{
	uint32_t dummy;
};

struct SpeedGroup
{
	SpeedState speedDefault;
	SpeedState speedFleeing;
	SpeedState speed2;
	SpeedState speed3;
	SpeedState speed4;
	SpeedState speed5;
};

struct GMobileWallHugInfo: GMobileInfo
{
	SpeedGroup speedGroup;
	CollideType collideType;
};

struct IsReacting
{
	uint32_t isFleeingFromObject;
	uint32_t isLookingAtObject;
	uint32_t isFollowingObject;
	uint32_t isFleeingFromSpell;
	uint32_t isLookingAtSpell;
	uint32_t isFollowingSpell;
	uint32_t isReactingToCreature;
	uint32_t isReactingToFood;
	uint32_t isReactingToMagicTree;
	uint32_t isReactingToFlyingObject;
	uint32_t isReactingToAbodeBurning;
	uint32_t isReactingToBall;
	uint32_t isReactingToWood;
	uint32_t isReactingToMagicShield;
	uint32_t isReactingToGift;
	uint32_t isReactingToNewBuilding;
	uint32_t isReactingToHandPickUp;
	uint32_t isReactingToHandUsingTotem;
	uint32_t isReactingToObjectCrushed;
	uint32_t isReactingToFight;
	uint32_t isReactingToTeleport;
	uint32_t isReactingToNiceSpell;
	uint32_t isReactingToHandPuttingStuffInStoragePit;
	uint32_t isReactingToDeath;
	uint32_t isReactingToDroppedByHand;
	uint32_t isReactingToFainting;
	uint32_t isReactingToConfused;
	uint32_t isReactingToFallingTree;
	uint32_t isFleeingFromPredator;
	uint32_t isReactingInCrowd;
	uint32_t isReactingToBreeder;
	uint32_t isReactingToTownCelebration;
	uint32_t isReactingToVillagerInHand;
	uint32_t isReactingToBurningThingInHand;
	uint32_t isReactingToMagicWaterPuttingOutFire;
	uint32_t isReactingToMagicShieldStruck;
	uint32_t isReactingToMagicShieldDestroyed;
	uint32_t isReactingToImpressiveSpell;
	uint32_t isReactingToScaffold;
	uint32_t isReactingToMissionary;
	uint32_t isReactingToFightWon;
};

struct GLivingInfo: GMobileWallHugInfo
{
	LivingType creatureType;
	LivingStates moveState;
	float life;
	float strength;
	float defence;
	uint32_t startAge;
	uint32_t grownUpAge;
	uint32_t oldAge;
	uint32_t retirementAge;
	IsReacting isReacting;
	uint32_t isShepherdable;
	uint32_t minFlockingValue;
	uint32_t maxFlockingValue;
};

struct AgeToScale
{
	float values[20];
};

struct GAnimalInfo: GLivingInfo
{
	AnimalInfo animalInfo;
	SexType sex;
	MeshId high;
	MeshId std;
	MeshId low;
	AnimId defaultAnim;
	uint32_t hunger;
	uint32_t thirst;
	uint32_t sleep;
	uint32_t needToBreed;
	float flockDistance;
	TurnangleStates turnAngle;
	TurnangleStates viewAngle;
	uint32_t playerCanPickUp;
	uint32_t needsTown;
	FoodType needsFoodTypes;
	uint32_t flocksCanMerge;
	float stalkingDistance;
	float attackDistance;
	float huntingDistance;
	uint32_t siSightDistance;
	uint32_t farSightDistance;
	uint32_t nearSightDistance;
	uint32_t chaseTime;
	uint32_t environment;
	uint32_t domainInnerRadius;
	uint32_t domainRadius;
	uint32_t maxFlockSize;
	uint32_t stayTime;
	float altitudeMin;
	float altitudeMax;
	float altitudeVariance;
	float altitudeMovementChange;
	float altitudeNormal;
	AgeToScale ageToScale;
};

struct GMobileObjectInfo: GMobileInfo
{
	MobileObjectInfo mobileType;
	MeshId meshId;
	float startScale;
	float finalScale;
};

struct GPotInfo: GMobileObjectInfo
{
	PotType potType;
	ResourceType resourceType;
	uint32_t maxAmountInPot;
	uint32_t scaleEvery;
	PotInfo nextPotForResource;
	Reaction associatedReaction;
	uint32_t canBecomeAPhysicsObject;
	uint32_t amountPickedUpInitially;
	uint32_t amountPickedUpPerTurn;
	uint32_t amountPickedUpPerTurnEnd;
	uint32_t maxAmountCanBePickedUp;
	float multiPickUpRampTime;
};

struct GFootballPositionInfo
{
	glm::vec2 offset;
	VillagerStates startState;
	VillagerStates state;
	uint32_t side;
};

struct GBallInfo: GMobileObjectInfo
{
	uint32_t defaultSpeed;
	CollideType collideType;
	uint32_t strength;
	float defence;
	float kickXZVelocity;
	float kickYVelocity;
	float shootXZVelocity;
	float shootYVelocity;
	float dribbleXZVelocity;
	float dribbleYVelocity;
	float minDistanceToFindPlayer;
};

struct GSoundInfo
{
	float weatherPercentageForMaxFade;
	float weatherPercentageForMaxWeatherVolume;
	float radiusForMaxAtmosVolume;
	float radiusForMinAtmosVolume;
	float fadeOutStart;
	float normalAtmosFadeStartHeight;
	float normalAtmosFadeEndHeight;
	float atmosphereHeight;
	float atmosphereMaxVolHeight;
	float spaceHeight; // AtmosphereNoVolHeight
	float atmosVolumeStep;
	float townTriggerDistance;
	float townTriggerOffDistance;
	float noBlocksForFullAtmos;
	float distanceFromNearestAtmosForFadeStart;
};

struct GMagicFireBallInfo: GObjectInfo
{
	float initialTemperature;
	float deletionTemperature;
	float catchIncreaseFactor;
};

struct GDanceInfo
{
	uint32_t duration;
	uint32_t totalNumBeats;
	std::array<char, 0x40> debugText;
	std::array<char, 0x40> fileName;
	uint32_t startsAutomatically;
	glm::vec2 areaRequired;
	uint32_t minimumDancers;
	uint32_t maximumDancers;
	float impressiveness;
};

struct GSpecialVillagerInfo
{
	std::array<char, 0x30> name;
	uint32_t age;
	SexType sex;
	VillagerJob job;
	uint32_t married;
	Tribe tribe;
	AnimalInfo pet;
	uint32_t faceNumber;
};

struct CreatureDevelopmentPhaseEntry
{
	std::array<char, 0x30> name;
	float field_0x30;
	float field_0x34;
	float field_0x38;
	uint32_t field_0x3c;
	uint32_t field_0x40;
	uint32_t field_0x44;
	uint32_t field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
	uint32_t field_0x58;
	uint32_t field_0x5c;
	uint32_t field_0x60;
	uint32_t field_0x64;
	uint32_t field_0x68;
	uint32_t field_0x6c;
	uint32_t field_0x70;
};

struct CreatureInitialDesireInfo
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
	uint32_t field_0x28;
	uint32_t field_0x2c;
	uint32_t field_0x30;
	uint32_t field_0x34;
	uint32_t field_0x38;
	float field_0x3c;
	float field_0x40;
	float field_0x44;
	float field_0x48;
	float field_0x4c;
	float field_0x50;
	float field_0x54;
	float field_0x58;
	uint32_t field_0x5c;
	float field_0x60;
	float field_0x64;
	float field_0x68;
	uint32_t field_0x6c;
	std::array<char, 0x40> field_0x70;
	std::array<char, 0x40> field_0xb0;
	std::array<char, 0x40> field_0xf0;
	std::array<char, 0x40> field_0x130;
	std::array<char, 0x40> field_0x170;
};

struct CreatureActionKnownAboutEntry
{
	std::array<char, 0x40> field_0x0;
	uint32_t field_0x40;
	uint32_t field_0x44;
	float field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
};

struct CreatureDesireAttributeEntry
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
};

struct GMultiMapFixedInfo: GObjectInfo
{
	GroundInfo groundInfo;
	uint32_t woodRequiredPerBuild;
	uint32_t timeToBuild;
	uint32_t scaffoldsRequired;
	uint32_t maxVillagerNeededToBuild;
	float desireToBeBuilt;
	float desireToBeRepaired;
	float influence;
};

struct GCitadelPartInfo: GMultiMapFixedInfo
{
	CitadelTypes citadelType;
	MeshId meshType;
	float startLife;
	uint32_t startStrength;
	float startDefence;
};

struct GWorshipSiteInfo: GCitadelPartInfo
{
	float radiusFromCitadel;
	MeshId baseMesh;
	PotInfo potForResourceFood;
	PotInfo potForResourceWood;
	float chantsPerVillager;
	float prayerSiteDistance;
	uint32_t maxDancersVisible;
	float chantsToFillBattery;
	float eachVillagerAddToFillBattery;
	uint32_t chantsToReserveForMaintaining;
	float artifactPowerupMultiplier;
};

struct GAbodeInfo: GMultiMapFixedInfo
{
	AbodeType abodeType;
	AbodeNumber abodeNumber;
	std::array<char, 0x30> debugString;
	Tribe tribeType;
	MeshId meshId;
	uint32_t canBePhysicallyDamaged;
	float startLife;
	uint32_t startStrength;
	float startDefence;
	uint32_t startInfluence;
	uint32_t maxVillagersInAbode;
	uint32_t maxChildrenInAbode;
	uint32_t startVillagersInAbode;
	uint32_t startChildrenInAbode;
	uint32_t startFood;
	uint32_t startFoodRAnd;
	uint32_t startWood;
	uint32_t startWoodRAnd;
	uint32_t howLongRuinLastsFor;
	PotInfo potForResourceFood;
	PotInfo potForResourceWood;
	float percentTooCrowded;
	MobileObjectInfo producesMobileObject;
	float maxNumMobileObjectsToProduce;
	float timeEachMobileObjectTakesToProduce;
	float emptyAbodeLifeReducer;
	int populationWhenNeeded;
	float thresholdForStopBeingFunctional;
	HelpText toolTipsForBuild;
	HelpText didYouKnow;
	DykCategory dykCategory;

	static AbodeInfo Find(const std::string& name);
	static AbodeInfo Find(Tribe tribe, AbodeNumber abodeNumber);
};

struct GSingleMapFixedInfo: GObjectInfo
{
	MeshId normal;
};

struct GMapShieldInfo: GSingleMapFixedInfo
{
	float dummy;
};

struct GRewardInfo: GMobileObjectInfo
{
	SpellSeedType seed;
	GestureType gestureType;
	MagicType powerUp;
	HelpText helpTextEnum;
	float beliefValue;
	MobileStaticInfo mobileStatic;
	uint32_t scaffold;
};

struct GContainerInfo
{
	ContainerInfoType containerType;
};

struct GCitadelInfo: GContainerInfo
{
	CitadelHeartInfoType heartInfo;
	glm::ivec2 pensStart;
	uint32_t pensMaxSpiral;
	glm::ivec2 worshipSiteStart;
	uint32_t maxNo;
	uint32_t maxDistanceOfTownFromCitadelOfPygmyTown;
	uint32_t pygmyTownMaxSpiral;
	uint32_t housesInPygmyTown;
	uint32_t importanceOfBeingFarAwayFromOtherTowns;
	int prayerSiteAngle;
	float prayerSiteDistance;
	float virtualInfluenceMaxDistance;
	uint32_t virtualInfluenceMaxGameTicks;
	float virtualInfluenceChantsToDouble;
};

struct CreatureDevelopmentDurationEntry
{
	int field_0x0;
	int field_0x4;
	int field_0x8;
	int field_0xc;
	int field_0x10;
	int field_0x14;
	int field_0x18;
	int field_0x1c;
	int field_0x20;
	int field_0x24;
	int field_0x28;
	int field_0x2c;
	int field_0x30;
	int field_0x34;
};

struct GTownInfo: GContainerInfo
{
	TownInfo townType;
	uint32_t size;
	uint32_t minHousesToSetup;
	uint32_t maxHousesToSetup;
	uint32_t maxSizeInCells;
	uint32_t distance;
	uint32_t split;
	TurnangleStates aroundCitadelStartAngle;
	uint32_t aroundCitadelStepAngle;
	uint32_t aroundCitadelStartDistance;
	uint32_t aroundCitadelEndDistance;
	AbodeInfo townCentre;
	AbodeInfo storagePit;
	uint32_t percentFemale;
	uint32_t processAbodeEvery;
	uint32_t newBuildingEvery;
	uint32_t profession[static_cast<size_t>(VillagerJob::_COUNT)];
	float happyFoodMultiplier;
	float influence;
	float numberOfDaysFoodInStoreToBeHappy;
	float happyWoodPerBuildingSite;
	uint32_t playtimeEvery;
	float maxDistanceWallBeBuilt;
	float beliefNeededForWonder;
	float proportionDeadForGraveyard;
	float playerInteactDecay;
	float howImpressedWithFlyingThing;
	float impressedWithFlyingThingRemovedAsSeen;
	float impressedWithFlyingThingAddPerGameTurn;
	float baseBeliefDesireValue;
	float totalWoodNeededToStartBuilding;
	float firstTimeDamageDoneAddition;
	uint32_t totalScaffoldNeededToStartBuilding;
	float builtPercentBeforeWhichYouCanAddScaffolds;
	float beliefInNeutralPlayer;
	float maxAlignmentChangePerGameTurnForDesires;
	float storyInfluence[5];
	float maxForTimeWillWorkUntil;
	float bedTimeMod;
	float foodWantedMultiplier;
	float minimumWoodForDesire;
	float maximumWoodForDesire;
	float numOfBuildingsForDesiredWood;
	float relaxationMod;
	float startingBeliefForCreatedTown;
	uint32_t emptyTownAllowedThisManyVillagers;
	float multiplierForAddingResourceToTown;
	float multiplierForNonOwnerAddingResource;
	uint32_t maxGameturnsForBeliefAfterRemovingFromStoragePit;
	float footballMatchDuration;
	float beliefForFooballMatch;
	float thresholdToStartRepairing;
	uint32_t gameTurnsAfterEmergencyVillagersReact;
	uint32_t numSecondsToIgnoreCreature;
	uint32_t numSecondsBeforeTownForgetsAboutCreature;
	uint32_t maxDistanceForVillagersToGoToTheWorshipsite;
	float minLifeForVillagersToGoToTheWorshipsite;
	float consideredEvilIfMakingDisciplesAndDesireIsLessThan;
	float consideredGoodIfMakingDisciplesAndDesireIsGreaterThan;
	uint32_t numGameTurnsBetweenGettingBeliefForArtifacts;
	float populationForUnmodifiedBelief;
	float multiplierForBeliefFromVillagersDancingAroundArtifact;
	float giveResourceAligmnetChangeMultiplier;
	float takeResourceAligmnetChangeMultiplier;
	float maxDistanceFromCongreationPosThatPeopleChillOut;
	float maxDistanceFromHouseThatPeopleChillOut;
	float maxDistanceThatVillagersWillGoToWorship;
	float amountOverCapacityThatDiscipleBreedersBreedUntil;
	uint32_t populationUnderWhichHelpSpritesWarn;
	float oneOffBeliefForCreature;
	float divisorForAverageDesires;
	float thresholdForAverageDesiresHelpSprites;
	float alignmentChangePerDisciple;
	float maxDistanceForTownForest;
	uint32_t shuffleVillagersEvery;
	float beliefSpeedScaleMultiPlayer;
	float beliefSpeedScaleStory[6];
	float fightWonImpressiveValue;
};

struct GJobInfo
{
	JobInfo job;
	uint32_t restAtHomeAfterWork;
	uint32_t subVisitsAtWhileWork;
	uint32_t timeAtEachSubVisit;
	uint32_t maxGameTurnsToSpendAtJob;
	float minResourceToGet;
	JobActivity spring;
	JobActivity summer;
	JobActivity autumn;
	JobActivity winter;
	uint32_t bestResourceCollectedPerSubVisit;
	uint32_t inJobSpiralArea;
	uint32_t nutureValue;
	float lookForJobLocationMaxDistance;
	uint32_t maxNoLookForsPerTurn;
	EffectInfo nurtureEffect;
	EffectInfo harvestEffect;
	TownDesireInfo satisfyWhichTownDesire;
};

struct GMagicInfo
{
	MagicType magicType;
	ImmersionEffectType immersion;
	uint32_t stopImmersion;
	float perceivedPower;
	ParticleType particleType;
	ImpressiveType impressiveType;
	SpellSeedType spellSeedType;
	GestureType gestureType;
	PowerUpType powerupType;
	CastRuleType castRuleType;
	uint32_t isSpellSeedDrawnInHand;
	uint32_t isSpellRecharged;
	uint32_t isCreatureCastFromAbove;
	uint32_t oneOffSpellIsPlayful;
	uint32_t oneOffSpellIsAggressive;
	uint32_t oneOffSpellIsCompassionate;
	uint32_t oneOffSpellIsToRestoreHealth;
	ParticleType particleTypeInHand;
};

struct GMagicGeneralInfo: GMagicInfo
{
};

struct GMagicResourceInfo: GMagicInfo
{
	ResourceType resourceType;
	uint32_t resourceAmountFirstEvent;
	uint32_t resourceAmountPerEvent;
	uint32_t costPerUnit;
	uint32_t poisoned;
};

struct GPBallInfo: GMobileObjectInfo
{
	float kickXZVelocity;
	float kickYVelocity;
};

struct GShotInfo: GMobileObjectInfo
{
	AttackInfo hitEffect;
	int strength;
	int acceleration; // (no longer used)
	int heightAcceleration;
	int velocity; // no longer used
	int finalHeightVelocity;
	SpeedState speed;
};

struct GAlignmentInfo
{
	float animalNice;
	float animalNasty;
	float creature;
	float priest;
	float skeleton;
	float villager;
	float building;
	float plant;
	float field;
	float feature;
	float mobileObject;
	float land;
	float script;
	float unimportant;
};

struct GClimateInfo
{
	std::array<char, 0x30> debugString;
	float rainMinSpring;
	float rainMinSummer;
	float rainMinAutumn;
	float rainMinWinter;
	float rainMaxSpring;
	float rainMaxSummer;
	float rainMaxAutumn;
	float rainMaxWinter;
	float tempMinSpring;
	float tempMinSummer;
	float tempMinAutumn;
	float tempMinWinter;
	float tempMaxSpring;
	float tempMaxSummer;
	float tempMaxAutumn;
	float tempMaxWinter;
	float windMinSpring;
	float windMinSummer;
	float windMinAutumn;
	float windMinWinter;
	float windMaxSpring;
	float windMaxSummer;
	float windMaxAutumn;
	float windMaxWinter;
};

struct GClimateRainInfo
{
	float january;
	float february;
	float march;
	float april;
	float may;
	float june;
	float july;
	float august;
	float september;
	float october;
	float november;
	float december;
	float midnight;
	float one_O_Clock;
	float two_O_Clock;
	float three_O_Clock;
	float four_O_Clock;
	float five_O_Clock;
	float six_O_Clock;
	float seven_O_Clock;
	float height_O_Clock;
	float nine_O_Clock;
	float ten_O_Clock;
	float eleven_O_Clock;
	float midday;
	float thirdten_O_Clock;
	float fourten_O_Clock;
	float fiveten_O_Clock;
	float sixten_O_Clock;
	float seventen_O_Clock;
	float heightten_O_Clock;
	float nineten_O_Clock;
	float twenty_O_Clock;
	float twentyone_O_Clock;
	float twentytwo_O_Clock;
	float twentitree_O_Clock;
	float natureRainDesire;
};
struct GCreaturePenInfo: GCitadelPartInfo
{
	PenInfo info;
	float startPlacementDistance;
	float endPlacementDistance;
	uint32_t placementAngle;
};

struct GScriptOpposingCreature
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
};

struct GTotemStatueInfo: GMultiMapFixedInfo
{
	MeshId plinth;
};

struct CreatureDesireForType
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	float field_0x10;
	float field_0x14;
	float field_0x18;
	float field_0x1c;
	float field_0x20;
	float field_0x24;
	float field_0x28;
	float field_0x2c;
	float field_0x30;
	float field_0x34;
	float field_0x38;
	float field_0x3c;
	float field_0x40;
};

struct GVortexInfo
{
	ParticleType particleTypePreLandscape;
	ParticleType particleTypePostLandscape;
	ParticleType particleTypeObjectMover;
	ParticleType particleTypeLightMap;
	VortexStateType initialState;
	uint32_t fadeWhenDeleted;
	float baseScale;
	VortexStatType maxToCreateTotalObjects;
	VortexStatType maxToCreateResourceFood;
	VortexStatType maxToCreateResourceWood;
	VortexStatType maxToCreateVillager;
	VortexStatType maxToCreateOneShot;
};

struct GScaffoldInfo: GMobileObjectInfo
{
	float phantomBuildingRotationPerGameTurn;
	uint32_t gameTurnsAfterPlacingCanStillPickUp;
	float maxDistanceForImpressingTowns;
	float proportionOfTownToImpress;
	uint16_t maxNumberForCombining;
};

struct GScriptHighlightInfo: GSingleMapFixedInfo
{
	MeshId active;
	ParticleType particleTypeGlints;
	ParticleType particleTypeActive;
};

struct GSpeedThreshold
{
	SpeedState speedMaxWalk;
	SpeedState speedMaxRun;
};

struct GCreatureInfo: GLivingInfo
{
	std::array<uint8_t, 416> field_0x1e4;
};

struct GMagicRadiusSpellInfo: GMagicInfo
{
	float minRadius;
	float maxRadius;
	float radiusForNormalCost;
};

struct GMagicStormAndTornadoInfo: GMagicRadiusSpellInfo
{
	float maxWindSpeed;
	float rainAmount;
};

struct GHelpSpritesGuidance
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
	uint32_t field_0x28;
	uint32_t field_0x2c;
	uint32_t field_0x30;
	uint32_t field_0x34;
	uint32_t field_0x38;
	uint32_t field_0x3c;
	uint32_t field_0x40;
	uint32_t field_0x44;
	uint32_t field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
	uint32_t field_0x58;
	uint32_t field_0x5c;
	uint32_t field_0x60;
	uint32_t field_0x64;
	uint32_t field_0x68;
	uint32_t field_0x6c;
	uint32_t field_0x70;
	uint32_t field_0x74;
	uint32_t field_0x78;
	uint32_t field_0x7c;
	uint32_t field_0x80;
	uint32_t field_0x84;
};

struct GFeatureInfo: GMultiMapFixedInfo
{
	MeshId meshId;

	static FeatureInfo Find(const std::string& name);
};

struct GBigForestInfo: GFeatureInfo
{
	CarriedTreeType carriedType;
};

struct GForestInfo: GContainerInfo
{
	uint32_t defaultNoTrees;
	HelpText helpStartEnum;
	SoundCollisionType collideSound;
	ImmersionEffectType immersion;
	HelpText helpEndEnum;
	HelpSystemMessageSet helpMessage;
	HelpSystemCondition helpCondition;
};

struct GAnimatedStaticInfo: GFeatureInfo
{
	uint32_t dummy;
	AnimId defaultAnim;

	static AnimatedStaticInfo Find(const std::string& name);
};

struct GWorshipSiteUpgradeInfo: GFeatureInfo
{
	float scale;
};

struct GWallSectionInfo: GFeatureInfo
{
	float nearEnoughToImprove;
	uint32_t gameTurnsBeforeImprove;
	float heightIncreasePerImprove;
	float distanceToConsiderSeperateWall;
};

struct GArrowInfo: GMobileObjectInfo
{
	uint32_t field_0x104;
	uint32_t field_0x108;
	uint32_t field_0x10c;
	uint32_t field_0x110;
	uint32_t field_0x114;
	uint32_t field_0x118;
	uint32_t field_0x11c;
};

struct GFieldInfo: GMultiMapFixedInfo
{
	glm::vec2 size;
	uint32_t maxNoFarmersPerField;
	glm::uvec2 gateNumberMissing;
	MobileStaticInfo fence;
	uint32_t deleteIfSame;
	MobileObjectInfo cropPlanted;
	uint32_t maxNumFarmers;
	EffectInfo nurtureEffect;
	float rainEffectMultiplier;
};

struct GBeliefInfo
{
	float townDesireBeliefThresholds;
	float claimedTownBeliefMultiplier;
	float lostATownBeliefInPlayerMultiplier;
	float minimumThreshold;
	float defaultBoredomOfMe;
	float beliefLeftWhenHelpSpritesWarn;
};

struct GShowNeedsInfo: GObjectInfo
{
	MeshId mesh;
	AnimId animId;
	float maxNeedValue;
	float showNeedGreater;
	float maxHieght;
};

struct GTerrainMaterialInfo
{
	SoundSurfaceType SurfaceSound;
	int immersion;
	float surfaceFriction;
	std::array<char, 0x30> debugString;
	HelpText HelpStartEnum;
	HelpText HelpEndEnum;
	glm::uvec3 tornadoDustColorRGB;
	TreeInfo magicTreeTypes[4];
};

struct GFlowersInfo: GFeatureInfo
{
	float growthSpeed;
};

struct HelpSystemInfo
{
	float maxDistanceForFOVobject;
	float wideScreenTime;
	uint32_t readDefaultAdjustGTTime;
	uint32_t readDefaultWordGTTime;
};

struct GMagicForestInfo: GMagicInfo
{
	uint32_t finalNoTrees;
	float startLife;
	float growSpeed;
	float decaySpeed;
	float woodValueMultiplier;
};

struct GVillagerStateTableInfo
{
	uint32_t field_0x0;
	int field_0x4;
	float field_0x8;
	int field_0xc;
	int field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	int field_0x20;
	uint32_t field_0x24;
	std::array<char, 0x80> name;
	int field_0xa8;
	uint32_t field_0xac;
	uint32_t field_0xb0;
	uint32_t field_0xb4;
	int field_0xb8;
	uint32_t field_0xbc;
	int field_0xc0;
	int field_0xc4;
	float field_0xc8;
	float field_0xcc;
	uint32_t field_0xd0;
	uint32_t field_0xd4;
	uint32_t field_0xd8;
	int field_0xdc;
	uint32_t field_0xe0;
	uint32_t field_0xe4;
	uint32_t field_0xe8;
	uint32_t field_0xec;
	uint32_t field_0xf0;
	uint32_t field_0xf4;
	float field_0xf8;
	uint32_t field_0xfc;
	uint32_t field_0x100;
};

struct GVillagerInfo: GLivingInfo
{
	Tribe tribeType;
	SexType sex;
	VillagerNumber villagerNumber;
	MeshId superMesh;
	MeshId childMeshHigh;
	MeshId childMeshMedium;
	MeshId childMeshLow;
	MeshId highDetail;
	MeshId stdDetail;
	MeshId lowDetail;
	JobInfo primaryJob;
	JobInfo secondaryJob;
	float jobEfficency;
	uint32_t startHavingSexAge;
	uint32_t stopHavingSexAge;
	int wantSexSpring;
	int wantSexSummer;
	int wantSexAutumn; // always -1 ?
	int wantSexWinter;
	uint32_t timePregnantFor;
	uint32_t lookForJobDelay;
	uint32_t waitAtHome;
	uint32_t restAtHomeTime;
	float restAtHomeRestoresLifeBy;
	float pregnancyChance;
	uint32_t inheritence;
	uint32_t boyGirlChance;
	float aveageHeightOfAdult;
	uint32_t maxFoodCarried;
	uint32_t maxWoodCarried;
	uint32_t minWoodToShowGraphic;
	uint32_t minFoodToShowGraphic;
	uint32_t maxTraderFoodCarried;
	uint32_t maxTraderWoodCarried;
	float woodUsedPerBuildCycle;
	float repairAmount;
	uint32_t jobSpeed;
	float percentLazy;
	float chantLifeRate;
	uint32_t dyingTimeWithoutGraveyard;
	uint32_t dyingTimeWithGraveyard;
	uint32_t timeForFootballChance;
	uint32_t footballWinGoalTime;
	uint32_t footballLostGoalTime;
	float lookAheadDistance;
	uint32_t scaredStiffTime;
	uint32_t waitForDinner;
	uint32_t housewifeGossipTime;
	uint32_t housewifeWaits;
	float foodNurishmentMultiplier;
	float gameTurnReducesFoodInBellyBy;
	float hungryForFood;
	float starvingForFood;
	float happyWithFood;
	float fullWithFood;
	float hungerToLifeMultiplier;
	float starvingToLifeMultiplier;
	uint32_t foodReqiredForDinner;
	uint32_t processChecksEvery;
	uint32_t havingSexTime;
	uint32_t dancingSpeed;
	AgeToScale ageToScale;
	AnimId lAttractAnim[3];
	AnimId lNeedAnim[3];
	float maxDistCarryFoodPit;
	float maxDistCarryWoodPit;
	uint32_t allowFaceSubstitution;
	float damageThresholdToGoHome;
	float damageThresholdToSleepUntil;
	uint32_t numGameTurnsAfterAggressionInterestedInShield;
	float pauseForASecondChance;
	float baseForTownNeedsSpeedMod;
	float divisorForTownNeedsSpeedMod;
	float speedModWhenFullLoadOfWood;
	float speedModWhenFullLoadOfFood;
	float lifeWhenWalksWounded;
	float lifeWhenCrawlsWounded;
	uint32_t amountOfWoodPerBuilderWanted;
	uint32_t keepMeshWhenChangeTown;
	float ownDesireThreshold;
	float maxDistanceFromArtifactDanceCanBe;
	uint16_t initialChillOutTime;
	uint16_t subsequentChillOutTime;
	uint16_t timeToWaitForArtifactDance;
	uint16_t drowningTime;
	float foodPowerupIncrease;
	float missionaryImpressiveValue;

	static VillagerInfo Find(Tribe tribe, VillagerNumber villagerNumber);
};

struct CreatureDesireActionEntry
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	uint32_t field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
	uint32_t field_0x28;
	uint32_t field_0x2c;
	uint32_t field_0x30;
	uint32_t field_0x34;
	uint32_t field_0x38;
	uint32_t field_0x3c;
	uint32_t field_0x40;
	uint32_t field_0x44;
	uint32_t field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
	uint32_t field_0x58;
	uint32_t field_0x5c;
	uint32_t field_0x60;
	uint32_t field_0x64;
	uint32_t field_0x68;
	uint32_t field_0x6c;
	uint32_t field_0x70;
	uint32_t field_0x74;
};

struct GMagicTeleportInfo: GMagicInfo
{
	float costPerKilometer;
};

struct GPlaytimeInfo
{
	ObjectType AssociatedObject;
	float Priority;
	AbodeNumber AssociatedStructure;
	AbodeNumber AssociatedAbodeNumber;
	DanceInfo AssociatedDance;
};

struct CreatureMagicActionKnownAboutEntry: CreatureActionKnownAboutEntry
{
	uint32_t field_0x58;
	float field_0x5c;
};

struct CreatureSourceBoundsInfo
{
	glm::vec3 field_0x0;
};

struct GPlayerInfo
{
	float maxAlignmentChangePerGameTurn;
	float maxScriptAlignmentChange;
	float treePullPutAlignmentChange;
	float applyEffectAlignmentChangeAddition;
	float dealthReason[static_cast<size_t>(DeathReason::_COUNT)];
	float computerPlayerBeliefChangeDecay;
	float averageTownPopulationForCredits;
};

struct ReactionInfo
{
	uint32_t priority;
	uint32_t numGameTurnsForNormalThingsToReact;
	uint32_t numGameTurnsForNormalThingsBeforeReactingAgain;
	uint32_t numGameTurnsForCreatureToReact;
	uint32_t numGameTurnsForCreatureBeforeReactingAgain;
	uint32_t whetherItImpresses;
	uint32_t whetherReactionFinishesIfInitiatorInHand;
	float maxReactionDistance;
	float howImportantIsDistance;
	uint32_t whetherReactionGrows;
	float reactionGrowthPerGameTurn;
	uint32_t stealthRandomChance;
	float minDistanceToRunAwayFromObject;
	float maxDistanceToRunAwayFromObject;
	float defaultReactionImpressiveMultiplier;
	float additionToTownBoredomMultipliers;
	TownDesireInfo correspondingTownDesire;
	TownDesireInfo correspondingTownDesireForAlignment;
	float alignmentModifier;
	GuidanceAlignment alignmentForSFX;
};

struct GMagicHealInfo: GMagicInfo
{
	float dummyVar;
	uint32_t maxToHeal;
};

struct GMagicFlockGroundInfo: GMagicInfo
{
	uint32_t numberToCreate;
	float alignmentSwitch;
	float distanceToTravel;
	float huntingRadius;
};

struct GAnimalStateTableInfo
{
	uint32_t field_0x0;
	int field_0x4;
	float field_0x8;
	uint32_t field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	std::array<char, 0x80> name;
	uint32_t field_0xa4;
};

struct GLeashSelectorInfo: GObjectInfo
{
	uint32_t dummy;
};

struct CreatureInitialSourceInfo
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	float field_0x10;
	float field_0x14;
	float field_0x18;
	float field_0x1c;
	float field_0x20;
	float field_0x24;
	float field_0x28;
	float field_0x2c;
	float field_0x30;
	float field_0x34;
	float field_0x38;
	float field_0x3c;
	float field_0x40;
};

struct GMagicWaterInfo: GMagicInfo
{
};

struct GMagicFoodInfo: GMagicResourceInfo
{
};

struct GMagicWoodInfo: GMagicResourceInfo
{
};

struct GMobileStaticInfo: GFeatureInfo
{
	float maxScale;
	MobileStaticInfo mobileType;
};

struct GHelpSystemTooltipsInfo
{
	float priority;
	float displayTime;
	float displayTimeAfterFocus;
};

struct GWeatherInfo
{
	std::array<char, 0x30> debugString;
	float fadeInOutTime;
	float lastsFor;
	float strength;
	int temperature;
	int wetness;
	int snowFall;
	uint32_t overCast;
	glm::ivec2 wind;
};

struct GPrayerIconInfo: GSingleMapFixedInfo
{
	uint32_t dummy;
};

struct GInfluenceInfo
{
	float percentageFullInfluence;
	float percentageDistanceForDecreasingGradient;
	float valueOfSmall;
};

struct GTreeInfo: GSingleMapFixedInfo
{
	MeshId growing;
	MeshId burning;
	uint32_t strength;
	float defence;
	float startLife;
	uint32_t growsAfterNumGameTurns;
	float growthAmount;
	uint32_t burnCheckAroundEvery;
	uint32_t randomBurnChance;
	float minSize;
	float maxSize;
	float rainingAcceleratorMultiplier;
	float waterSpellAcceleratorMultiplier;
	CarriedTreeType carriedType;
	uint16_t maxNumTreesCanProduce;
};

struct DifferentCreatureInfo
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	float field_0x10;
	float field_0x14;
	float field_0x18;
	float field_0x1c;
	float field_0x20;
	float field_0x24;
	float field_0x28;
	float field_0x2c;
	float field_0x30;
	float field_0x34;
	float field_0x38;
	float field_0x3c;
	float field_0x40;
	float field_0x44;
	float field_0x48;
	uint32_t field_0x4c;
	float field_0x50;
	float field_0x54;
	float field_0x58;
};

struct GEffectInfo
{
	float effectBurn;
	float effectCrush;
	float effectHit;
	float effectHeal;
	float effectFlyAway;
	float effectAlignmentModification;
	float effectBeliefModification;
	float radius;
	MeshId testMesh;
};

struct GMagicEffectInfo: GEffectInfo
{
	std::array<char, 0x30> debugString;
	uint32_t timerWhenOneShot;
	uint32_t timerWhenPlayerCasting;
	uint32_t timerWhenCreatureCasting;
	uint32_t timerWhenComputerPlayerCasting;
	float initialChants;
	float costToCreate;
	float costPerEvent;
	float costPerGameTurn;
	float costPerShieldCollide;
	uint32_t divideCostsByTribalPower;
	uint32_t createReactionOnCast;
	uint32_t createReactionOnEvent;
	Reaction reactionType;
	CreatureDesires perceivedPlayerDesire[2];
	TownDesireInfo townDesireBeingHelped;
	float agressiveRangeMin;
	float agressiveRangeMax;
	float calculatedCostForOneTurn;
	float costForOneAppliedEffect;
	float costInGameTurnsToCreateForOneVillager;
	float tempNoVillagers;
	float costForTempVillagers;
	float costInRealTimeToCreate;
	float impressiveValue;
	float cpImpressiveBalance;
	uint32_t useTribalPowerMultiplier[static_cast<int>(Tribe::_COUNT)];
	uint32_t isAggressiveSpellWhichIsUsedInCreatureFightArena;
	uint32_t isDefensiveSpellWhichIsUsedInCreatureFightArena;
	HelpText helpStartEnum;
	HelpText helpEndEnum;
	HelpText creatureNearlyLearntEnum;
	HelpText creatureLearntEnum;
	HelpSystemMessageSet helpMessage;
	HelpSystemCondition helpCondition;
	HelpText toolTipsEnum;
	float aggressiveAttackValue;
	float computerCastDuration;
};

struct GTownDesireInfo
{
	PrayerIconInfo associatedPrayerSite;
	float showsAfterPercent;
	float desireTriggersVillagerAction;
	float desireTriggersVillagerEmergencyAction;
	VillagerBasicInfo associatedVillagerBirth[3];
	MeshId worshipSiteMesh;
	int worshipSiteSlot;
	float worshipSiteScale;
	float desireToBeliefScale;
	float desireAffectsBeliefAfter;
	float desireBuildWonderReducer;
	float desireValueGreaterCausesDecayInBelief;
	float desireToBeliefThresholdDecay;
	float desireAffectsAlignmentAfter;
	float howImportantDesireIsToAlignment;
	uint32_t maxTimeForAlignmentChange;
	float tribeMultiplier[static_cast<int>(Tribe::_COUNT)];
	HelpText helpStartEnum;
	HelpText helpEndEnum;
	HelpText helpStatEnum;
	HelpSystemMessageSet helpMessage;
	HelpSystemCondition helpCondition;
};

struct GMagicFlockFlyingInfo: GMagicInfo
{
	uint32_t numberToCreate;
	float alignmentSwitch;
	float distanceToTravel;
};

struct GSpotVisualInfo
{
	std::array<char, 0x30> debugString;
	ParticleType particleType;
	uint32_t life;
	Reaction reactionType;
	uint32_t singleZSort;
	uint32_t targetOwnerObject;
};

struct GPFootballInfo: GMultiMapFixedInfo
{
	MeshId meshId;
};

struct GSpellSeedInfo: GObjectInfo
{
	uint32_t field_0xf0;
	uint32_t field_0xf4;
	uint32_t field_0xf8;
	uint32_t field_0xfc;
	uint32_t field_0x100;
	uint32_t field_0x104;
	uint32_t field_0x108;
	uint32_t field_0x10c;
	uint32_t field_0x110;
	uint32_t field_0x114;
	uint32_t field_0x118;
	uint32_t field_0x11c;
	uint32_t field_0x120;
	uint32_t field_0x124;
	uint32_t field_0x128;
	uint32_t field_0x12c;
	uint32_t field_0x130;
	float field_0x134;
	float field_0x138;
	float field_0x13c;
	float field_0x140;
	float field_0x144;
	uint32_t field_0x148;
	uint32_t field_0x14c;
	float field_0x150;
	float field_0x154;
	uint32_t field_0x158;
	float field_0x15c;
	uint32_t field_0x160;
	uint32_t field_0x164;
	uint32_t field_0x168;
	uint32_t field_0x16c;
	uint32_t field_0x170;
	uint32_t field_0x174;
	uint32_t field_0x178;
	uint32_t field_0x17c;
};

struct GCitadelHeartInfo: GCitadelPartInfo
{
	int startGoodness;
	uint32_t startFollowers;
	uint32_t maxFlockCount;
	float storyInfluence[5];
	float transferedDamageMultiplier;
};

struct GMagicCreatureSpellInfo: GMagicInfo
{
	CreatureReceiveSpellType creatureReceiveSpellType;
	std::array<char, 0x30> text;
	float startTransitionDuration;
	float finishTransitionDuration;
	float totalDuration;
	float maxDirnChangeWhenCtrCasting;
};

struct GRewardProgress
{
	MagicType magicType;
	uint32_t onLand[6];
};

struct GRewardProgressGood: GRewardProgress
{
};

struct GRewardProgressEvil: GRewardProgress
{
};

struct GFurnitureInfo: GObjectInfo
{
	MeshId mesh;
};

struct GSpookyVoiceInfo
{
	uint32_t soundNumber;
	std::array<char, 30> name;
};

struct GFieldTypeInfo: GMultiMapFixedInfo
{
	float ageGrowth;
	float ageRecolt;
	float timesToSow;
	float foodValueTakenWithHand;
	float totalFoodInField;
	uint32_t maxFarmerInFarm;
	float effectSunWhenGrowing;
	float effectSunWhenRipening;
	float effectRainWhenGrowing;
	float effectRainWhenRipening;
	MobileStaticInfo fence;
	float ratioBeforeRipe;
	float effectOfWaterSpell;
};

struct CreatureActionInfo
{
	float field_0x0;
	float field_0x4;
	float field_0x8;
	uint32_t field_0xc;
	float field_0x10;
	std::array<char, 0x20> name;
	uint32_t field_0x34;
	uint32_t field_0x38;
	uint32_t field_0x3c;
	uint32_t field_0x40;
	uint32_t field_0x44;
	uint32_t field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
	uint32_t field_0x58;
	uint32_t field_0x5c;
	uint32_t field_0x60;
	uint32_t field_0x64;
	uint32_t field_0x68;
	uint32_t field_0x6c;
	uint32_t field_0x70;
	uint32_t field_0x74;
	uint32_t field_0x78;
	uint32_t field_0x7c;
	uint32_t field_0x80;
	uint32_t field_0x84;
	uint32_t field_0x88;
	uint32_t field_0x8c;
	uint32_t field_0x90;
	uint32_t field_0x94;
	uint32_t field_0x98;
	uint32_t field_0x9c;
	uint32_t field_0xa0;
	uint32_t field_0xa4;
	uint32_t field_0xa8;
	uint32_t field_0xac;
	uint32_t field_0xb0;
	uint32_t field_0xb4;
	uint32_t field_0xb8;
	uint32_t field_0xbc;
	float field_0xc0;
	float field_0xc4;
	uint32_t field_0xc8;
	float field_0xcc;
	float field_0xd0;
	float field_0xd4;
	uint32_t field_0xd8;
	uint32_t field_0xdc;
	uint32_t field_0xe0;
	uint32_t field_0xe4;
	float field_0xe8;
	float field_0xec;
	uint32_t field_0xf0;
	uint32_t field_0xf4;
	uint32_t field_0xf8;
	uint32_t field_0xfc;
};

struct GSpellSystemInfo
{
	GestureType selectionSystemGestureR;
	GestureType selectionSystemGestureNonCreature;
	GestureType selectionSystemGestureCreature;
	float selectionSystemTimeOut;
	float selectionSystemRepeatTimeOut;
	float delayBeforeSeedActive;
	GestureType creatureSpecialMoveGesture;
	float leashSelectionSystemTimeOut;
	GestureType leashSelectionStart;
	GestureType leashSelectionGestures[4];
	GestureType creatureZoomTo;
	GestureType creatureEndGive;
};

struct GFishFarmInfo: GMultiMapFixedInfo
{
	uint32_t maxNoFishermanPerFishFarm;
	uint32_t numGameTurnsAfterWhichFoodIsIncreased;
};

struct CreatureDesireDependency
{
	uint32_t field_0x0;
	float field_0x4;
	float field_0x8;
	float field_0xc;
	uint32_t field_0x10;
	uint32_t field_0x14;
	uint32_t field_0x18;
	uint32_t field_0x1c;
	uint32_t field_0x20;
	uint32_t field_0x24;
	uint32_t field_0x28;
	uint32_t field_0x2c;
	uint32_t field_0x30;
	uint32_t field_0x34;
	uint32_t field_0x38;
	uint32_t field_0x3c;
	float field_0x40;
	uint32_t field_0x44;
	uint32_t field_0x48;
	uint32_t field_0x4c;
	uint32_t field_0x50;
	uint32_t field_0x54;
	uint32_t field_0x58;
	float field_0x5c;
	float field_0x60;
	float field_0x64;
	float field_0x68;
	float field_0x6c;
	float field_0x70;
	float field_0x74;
	float field_0x78;
	float field_0x7c;
	float field_0x80;
	float field_0x84;
	float field_0x88;
	float field_0x8c;
	float field_0x90;
	float field_0x94;
	uint32_t field_0x98;
	uint32_t field_0x9c;
};

struct GPFootballPositionInfo: GFootballPositionInfo
{
};

struct CreatureMimicInfo
{
	std::array<char, 0x80> name;
	float field_0x80;
	uint32_t field_0x84;
	uint32_t field_0x88;
	uint32_t field_0x8c;
	uint32_t field_0x90;
	uint32_t field_0x94;
	uint32_t field_0x98;
	uint32_t field_0x9c;
	uint32_t field_0xa0;
	uint32_t field_0xa4;
	uint32_t field_0xa8;
	uint32_t field_0xac;
};

struct GPrayerSiteInfo: GFeatureInfo
{
	float radius;
};

struct HelpSpiritInfo: GLivingInfo
{
	MeshId meshId;
};

struct CreatureDesireSourceTable
{
	uint32_t field_0x0;
	uint32_t field_0x4;
	float field_0x8;
	float field_0xc;
	float field_0x10;
	std::array<char, 0x40> field_0x14;
	std::array<char, 0x40> field_0x54;
};

struct GSpellIconInfo: GMultiMapFixedInfo
{
	MeshId meshId;
	float radiusFromCitadel;
	float gatheringChantAddPerGameTurn;
};

struct GMagicShieldInfo: GMagicRadiusSpellInfo
{
	float chantCostPerImpactMomentum;
	float shieldHeight;
	float raiseWithScale;
	float bobMagnitude;
};

struct GGroundInfo: GObjectInfo
{
	CountryList country;
	MeshId editorMesh;
};

struct InfoConstants
{
	std::array<GMagicGeneralInfo, 10> magicGeneral;
	std::array<GMagicHealInfo, 2> magicHeal;
	std::array<GMagicTeleportInfo, 1> magicTeleport;
	std::array<GMagicForestInfo, 1> magicForest;
	std::array<GMagicFoodInfo, 2> magicFood;
	std::array<GMagicStormAndTornadoInfo, 3> magicStormAndTornado;
	std::array<GMagicShieldInfo, 2> magicShield;
	std::array<GMagicWoodInfo, 1> magicWood;
	std::array<GMagicWaterInfo, 2> magicWater;
	std::array<GMagicFlockFlyingInfo, 1> magicFlockFlying;
	std::array<GMagicFlockGroundInfo, 1> magicFlockGround;
	std::array<GMagicCreatureSpellInfo, 0x10> magicCreatureSpell;
	std::array<GMagicEffectInfo, 42> magicEffect;
	std::array<GSpellSeedInfo, 30> spellSeed;
	std::array<GAnimalInfo, static_cast<size_t>(AnimalInfo::_COUNT)> animal;
	std::array<GCreatureInfo, 17> creature;
	std::array<DifferentCreatureInfo, 17> differentCreature;
	std::array<CreatureDesireForType, 40> creatureDesireForType;
	std::array<CreatureDevelopmentPhaseEntry, 14> creatureDevelopmentPhaseEntry;
	std::array<CreatureDevelopmentDurationEntry, 17> creatureDevelopmentDurationEntry;
	GCitadelInfo citadel;
	GCitadelHeartInfo citadelHeart;
	std::array<GCreaturePenInfo, 5> creaturePen;
	std::array<GWorshipSiteInfo, 9> worshipSite;
	std::array<GSpellIconInfo, 2> spellIcon;
	std::array<GAbodeInfo, static_cast<size_t>(AbodeInfo::_COUNT)> abode;
	std::array<GVillagerInfo, static_cast<size_t>(VillagerInfo::_COUNT)> villager;
	std::array<GSpecialVillagerInfo, 0x30> specialVillager;
	std::array<GTreeInfo, static_cast<size_t>(TreeInfo::_COUNT)> tree;
	std::array<GSingleMapFixedInfo, 4> singleMapFixed;
	std::array<GScriptHighlightInfo, 4> scriptHighlight;
	std::array<GMapShieldInfo, 2> mapShield;
	GBallInfo ball;
	GTownInfo town;
	std::array<GJobInfo, static_cast<size_t>(JobInfo::_COUNT)> job;
	std::array<GFeatureInfo, static_cast<size_t>(FeatureInfo::_COUNT)> feature;
	std::array<GFlowersInfo, 2> flowers;
	std::array<GAnimatedStaticInfo, static_cast<size_t>(AnimatedStaticInfo::_COUNT)> animatedStatic;
	std::array<GMobileObjectInfo, static_cast<size_t>(MobileObjectInfo::_COUNT)> mobileObject;
	GScaffoldInfo scaffold;
	std::array<GMobileStaticInfo, static_cast<size_t>(MobileStaticInfo::_COUNT)> mobileStatic;
	std::array<GPotInfo, static_cast<size_t>(PotInfo::_COUNT)> pot;
	GPrayerIconInfo prayerIcon;
	GPrayerSiteInfo prayerSite;
	std::array<GShowNeedsInfo, 4> showNeeds;
	std::array<GTownDesireInfo, 17> townDesire;
	GWorshipSiteUpgradeInfo worshipSiteUpgrade;
	std::array<HelpSpiritInfo, 3> helpSpirit;
	GArrowInfo arrow;
	std::array<GSpotVisualInfo, 50> spotVisual;
	std::array<GEffectInfo, 12> effect;
	std::array<GFieldInfo, 2> field;
	std::array<GFieldTypeInfo, static_cast<size_t>(FieldTypeInfo::_COUNT)> fieldType;
	GFishFarmInfo fishFarm;
	std::array<GFootballPositionInfo, 10> footballPosition;
	std::array<GPlaytimeInfo, 5> playtime;
	GPlayerInfo player;
	GSoundInfo sound;
	GBeliefInfo belief;
	std::array<GHelpSpritesGuidance, 22> helpSpritesGuidance;
	GInfluenceInfo influence;
	HelpSystemInfo helpSystem;
	std::array<GAlignmentInfo, 7> alignment;
	std::array<ReactionInfo, 41> reaction;
	std::array<CreatureActionInfo, 328> creatureAction;
	std::array<CreatureDesireActionEntry, 40> creatureDesireAction_1;
	std::array<CreatureDesireActionEntry, 17> creatureDesireAction_2;
	std::array<CreatureDesireActionEntry, 40> creatureDesireAction_3;
	std::array<CreatureDesireDependency, 40> creatureDesireDependency;
	std::array<CreatureInitialDesireInfo, 40> creatureInitialDesire;
	std::array<CreatureDesireSourceTable, 61> desireSourceTable;
	std::array<CreatureInitialSourceInfo, 61> creatureInitialSource_1;
	std::array<CreatureInitialSourceInfo, 61> creatureInitialSource_2;
	std::array<CreatureSourceBoundsInfo, 61> creatureSourceBounds;
	std::array<CreatureDesireAttributeEntry, 40> creatureDesireAttributeEntry;
	std::array<CreatureActionKnownAboutEntry, 6> creatireActionKnownAboutEntry;
	std::array<CreatureMagicActionKnownAboutEntry, 42> creatureMagicActionKnownAboutEntry;
	std::array<CreatureMimicInfo, 46> creatureMimic;
	std::array<GTerrainMaterialInfo, 43> terrainMaterial;
	std::array<GTribeInfo, static_cast<size_t>(Tribe::_COUNT)> tribe;
	std::array<GSpeedThreshold, 10> speedThreshold;
	GPBallInfo pBall;
	GPFootballInfo pFootball;
	std::array<GPFootballPositionInfo, 10> pFootballPosition;
	std::array<GDanceInfo, 25> dance;
	std::array<GVillagerStateTableInfo, 0xFF> villagerStateTable;
	std::array<GAnimalStateTableInfo, 53> animalStateTable;
	std::array<GTotemStatueInfo, 9> totemStatue;
	std::array<GBigForestInfo, static_cast<size_t>(BigForestInfo::_COUNT)> bigForest;
	std::array<GFurnitureInfo, 31> furniture;
	std::array<GWeatherInfo, 7> weather;
	std::array<GClimateInfo, 7> climate;
	GSpellSystemInfo spellSystem;
	std::array<GVortexInfo, 3> vortex;
	std::array<GMagicFireBallInfo, 3> magicFireBall;
	std::array<GRewardInfo, 61> reward;
	GLeashSelectorInfo leashSelector;
	std::array<GRewardProgressGood, 30> rewardProgressGood;
	std::array<GRewardProgressEvil, 30> rewardProgressEvil;
	std::array<GSpookyVoiceInfo, 5> spookyVoice;
	std::array<GScriptOpposingCreature, 17> scriptOpposingCreature;
	std::array<GHelpSystemTooltipsInfo, 170> toolTips;
};
#pragma pack(pop)
static_assert(std::is_trivial<InfoConstants>::value, "GInfo must be trivial to be read properly");
static_assert(sizeof(InfoConstants) == 0x8E186);

} // namespace openblack

/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <string_view>

// Enums translated over from ones found in the Creature Isle modding tools.
// Creature Isle specific enums will be commented out with: CREATUREISLE
// Enum version: Last Saved,#2001-11-15 15:39:30#,"Jonty Barnes"

namespace openblack
{

constexpr unsigned int k_CellSizeMetres = 10;
constexpr unsigned int k_GameTicksPerSecond = 10;
constexpr unsigned int k_OneMetre = 6554;
constexpr unsigned int k_NumberOfStoryLand = 5;

enum class DetailDistance
{
	Metre0 = 0,
	Metre1 = 6554,
	Metre3 = 19661,
	Metre4 = 26214,
	Metre5 = 32768,
	Metre10 = 65536,
	Metre11 = 72090,
	Metre12 = 78643,
	Metre13 = 85197,
	Metre14 = 91750,
	Metre15 = 98304,
	Metre20 = 131072,
	Metre30 = 196608,
	Metre40 = 262144,
	Metre50 = 327680,
	Metre60 = 393216,
	Metre70 = 458752,
	Metre80 = 524288,
	Metre90 = 589824,
	Metre100 = 655360,
};

enum class ObjectType : int
{
	Invalid = -2,
	Any = -1,
	Abode = 0,
	Creature = 1,
	Villager = 2,
	Magic = 3,
	Animal = 4,
	Lightning = 5,
	ForestTree = 6,
	FirewallSection = 7,
	Citadel = 8,
	Terrain = 9,
	MagicLiving = 10,
	WallSection = 11,
	FakeWallSection = 12,
	Spell = 13,
	Boundry = 14,
	Ball = 15,
	Shot = 16,
	ComputerPlayer = 17,
	Field = 18,
	Feature = 19,
	MobileObject = 20,
	Pot = 21,
	Football = 22,
	Ground = 23,
	Prayer = 24,
	ShowNeed = 25,
	Pfootball = 26,
	Pball = 27,
	MobileStatic = 28,
	Dummy = 29,
	HelpSpirit = 30,
	TotemStatue = 31,
	BigForest = 32,
	FishFarm = 33,
	Reward = 34,
	ScriptHighlight = 35,
	Waterfall = 36,
	HelpOrb = 37,
	LeashSelector = 38,
	OneOffSpell = 39,
	Furniture = 40,
	Flowers = 41,
	Firefly = 42,
	ArenaSpellIcon = 43,
	MapShield = 44,
	// CreatureIslesBuilding = 45, // CREATUREISLE
};

enum class VillagerJob : uint32_t
{
	None = 0,
	Fisherman = 1,
	Forester = 2,
	Farmer = 3,
	Shepherd = 4,
	Leader = 5,
	Housewife = 6,
	Trader = 7,

	_COUNT
};

enum class AbodeType : uint32_t
{
	General = 1 << 0,
	LivingQuarters = 1 << 1,
	Civic = 1 << 2,
	Windmill = 1 << 3 | LivingQuarters,
	Totem = 1 << 4 | Civic,
	StoragePit = 1 << 5 | Civic,
	Creche = 1 << 6 | Civic,
	Workshop = 1 << 7 | Civic,
	Wonder = 1 << 8,
	Graveyard = 1 << 9 | Civic,
	TownCentre = 1 << 10 | Civic,
	Citadel = 1 << 11 | Civic,
	FootballPitch = 1 << 12 | Civic,
	SpellDispenser = 1 << 13 | Civic,
	Field = 1 << 14 | Civic,
	Any = 0x7FFF,
	Last = 0x8000,
};

enum class LivingType : int
{
	Any = -1,
	Lion = 0,
	Tiger = 1,
	Wolf = 2,
	Leopard = 3,
	Sheep = 4,
	Goat = 5,
	Tortoise = 6,
	Zebra = 7,
	Cow = 8,
	Horse = 9,
	Pig = 10,
	Crow = 11,
	Dove = 12,
	Swallow = 13,
	Pigeon = 14,
	Seagull = 15,
	Bat = 16,
	Vulture = 17,
	Apes = 18,
	Bear = 19,
	HelpSpirit = 20,
	Villager = 21,
};

enum class AnimalInfo : int
{
	None = -1,
	Lion = 0,
	Tiger = 1,
	Wolf = 2,
	Leopard = 3,
	Sheep = 4,
	Goat = 5,
	Tortoise = 6,
	Zebra = 7,
	Cow = 8,
	Horse = 9,
	Pig = 10,
	Crow = 11,
	Dove = 12,
	Swallow = 13,
	Pigeon = 14,
	Seagull = 15,
	Bat = 16,
	Vulture = 17,
	CitadelDove = 18,
	CitadelBat = 19,
	SpellDove = 20,
	SpellBat = 21,
	SpellWolf = 22,
	PuzzleLion = 23,
	PuzzleSheep = 24,
	PuzzleWolf = 25,
	PuzzleVillager = 26,
	PuzzleHorse = 27,
	PuzzleCow = 28,
	PuzzleTortoise = 29,
	PuzzlePig = 30,

	_COUNT = 31
};

/// Originally TribeType and VillageEthnicities
enum class Tribe : int32_t
{
	NONE = -1,
	CELTIC = 0,
	AFRICAN = 1,
	AZTEC = 2,
	JAPANESE = 3,
	INDIAN = 4,
	EGYPTIAN = 5,
	GREEK = 6,
	NORSE = 7,
	TIBETAN = 8,

	_COUNT
};
static constexpr std::array<std::string_view, static_cast<uint8_t>(Tribe::_COUNT)> k_TribeStrs = {
    // "NONE",
    "CELTIC",   //
    "AFRICAN",  //
    "AZTEC",    //
    "JAPANESE", //
    "INDIAN",   //
    "EGYPTIAN", //
    "GREEK",    //
    "NORSE",    //
    "TIBETAN",  //
};

enum class AbodeNumber : int32_t
{
	Invalid = -1,
	A = 0,
	B = 1,
	C = 2,
	D = 3,
	E = 4,
	F = 5,
	Totem = 6,
	StoragePit = 7,
	Creche = 8,
	Workshop = 9,
	Wonder = 10,
	Graveyard = 11,
	TownCentre = 12,
	FootballPitch = 13,
	SpellDispenser = 14,
	Field = 15,

	_COUNT
};

static constexpr std::array<std::string_view, static_cast<uint32_t>(AbodeNumber::_COUNT)> k_AbodeNumberStrs = {
    "A",               //
    "B",               //
    "C",               //
    "D",               //
    "E",               //
    "F",               //
    "TOTEM",           //
    "STORAGE_PIT",     //
    "CRECHE",          //
    "WORKSHOP",        //
    "WONDER",          //
    "GRAVEYARD",       //
    "TOWN_CENTRE",     //
    "FOOTBALL_PITCH",  //
    "SPELL_DISPENSER", //
    "FIELD",           //
};

enum class SpellSeedType : int
{
	None = -1,
	Storm = 0,
	Nature = 1,
	Fire = 2,
	Food = 3,
	Shield = 4,
	PhysicalShield = 5,
	LightningBolt = 6,
	Heal = 7,
	Wood = 8,
	Water = 9,
	FlockFlying = 10,
	FlockGround = 11,
	CreatureSpellFreeze = 12,
	CreatureSpellSmall = 13,
	CreatureSpellBig = 14,
	CreatureSpellWeak = 15,
	CreatureSpellStrong = 16,
	CreatureSpellFat = 17,
	CreatureSpellThin = 18,
	CreatureSpellInvisible = 19,
	CreatureSpellCompassion = 20,
	CreatureSpellAngry = 21,
	CreatureSpellHungry = 22,
	CreatureSpellFrightene = 23,
	CreatureSpellTired = 24,
	CreatureSpellIll = 25,
	CreatureSpellThirsty = 26,
	CreatureSpellItchy = 27,
	Teleport = 28,
	BeamExplosion = 29,
	// CreatureSpellAntiSpell  = 30, // CREATUREISLE
	// CreatureSpellFast       = 31, // CREATUREISLE
};

enum class GestureType : uint32_t
{
	None = 0,
	Spiral = 1,
	InverseSpiral = 2,
	SShape = 3,
	Circle = 4,
	Scribble = 5,
	Three = 6,
	VerticalScribble = 7,
	Star = 8,
	ForkRight = 9,
	ForkUp = 10,
	ForkLeft = 11,
	ForkDown = 12,
	Heart = 13,
	RShape = 14,
	SquareSpirial = 15,
	CyrillicL = 16,
	EShape = 17,
	ReverseS = 18,
	Infinity = 19,
	WShape = 20,
	House = 21,
	// InverseSquareSpiral = 22, // CREATUREISLE
	// SquareWave          = 23, // CREATUREISLE
};

enum class FieldTypeInfo
{
	Wheat = 0,
	WheatWithFence = 1,
	Corn = 2,
	CornWithFence = 3,
	Cereal = 4,
	CerealWithFence = 5,

	_COUNT

	// SpecialCrop = 6,
};

enum class MagicFireballType
{
	Normal = 0,
	Pu1 = 1,
	Pu2 = 2,
};

enum class VortexStatType
{
	TotalObjects = 0,
	ResourceFood = 1,
	ResourceWood = 2,
	Villager = 3,
	Oneshot = 4,
};

enum class VortexStateType
{
	Inactive = 0,
	Active = 1,
	FadeIn = 2,
	FadeOut = 3,
};

enum class VortexType
{
	In = 0,
	Out = 1,
	Volcano = 2,
};

enum class ParticleType : uint32_t
{
	None = 0,
	Tornado = 1,
	Firework = 2,
	FireworkSingle = 3,
	Leaves = 4,
	Magicball = 5,
	Lightning = 6,
	LightningBolt = 7,
	Explosion_1 = 8,
	Food = 9,
	FoodPoisoned = 10,
	ExplosionOne = 11,
	ExplosionOnePuOne = 12,
	ExplosionOnePuTwo = 13,
	ExplosionCitadel = 14,
	Wood = 15,
	Water = 16,
	WaterPuOne = 17,
	WaterOnHolder = 18,
	WaterInHand = 19,
	WaterInHandPuOne = 20,
	Heal = 21,
	ManaPath = 22,
	ExplodeObject = 23,
	BeliefSprite = 24,
	TownBelief = 25,
	SpellFail = 26,
	SpellSucceed = 27,
	SpellSelection = 28,
	GripLandscape = 29,
	MagicObjectCreated = 30,
	CreatureGesture = 31,
	VillagerTeleport = 32,
	CreatureTarget = 33,
	CreatureCastVisual = 34,
	Gesture = 35,
	KbTest = 36,
	OnFire = 37,
	MagicFx = 38,
	MagicFxOnObject = 39,
	MagicFxOnCitadel = 40,
	FireFx = 41,
	FireFxOnObject = 42,
	Fireball = 43,
	FireballInHand = 44,
	FireballInHandPuOne = 45,
	FireballInHandPuTwo = 46,
	FireballOnHolder = 47,
	GestureLocal = 48,
	MagicSystem = 49,
	HealInHand = 50,
	HealOnHolder = 51,
	FireballPuOne = 52,
	FireballPuTwo = 53,
	HealPuOne = 54,
	LightningStormInHand = 55,
	LightningStormInHandPuOne = 56,
	LightningStormInHandPuTwo = 57,
	LightningStormOnHolder = 58,
	LightningBoltInHand = 59,
	LightningBoltOnHolder = 60,
	LightningStrike = 61,
	LightningSingleStrike = 62,
	FoodInHand = 63,
	Shield = 64,
	ShieldInHand = 65,
	ShieldOnHolder = 66,
	PhysicalShieldFx = 67,
	LightningBoltPuOne = 68,
	LightningBoltPuTwo = 69,
	LightningBoltInHandPuOne = 70,
	LightningBoltInHandPuTwo = 71,
	Teleport = 72,
	TeleportVortex = 73,
	TeleportOnHolder = 74,
	TeleportInHand = 75,
	LandscapeVortexObjectMover = 76,
	LandscapeVortexLightmap = 77,
	LandscapeVortexInBefore = 78,
	LandscapeVortexInAfter = 79,
	LandscapeVortexOutBefore = 80,
	LandscapeVortexOutAfter = 81,
	VolcanoVortexBefore = 82,
	VolcanoVortexAfter = 83,
	VolcanoVortexLightmap = 84,
	CreatureSpellPhysical = 85,
	CreatureSpellMental = 86,
	CreatureSpellItchy = 87,
	CreatureSpellItchyInHand = 88,
	CreatureSpellItchyOnHolder = 89,
	CreatureSpellFreeze = 90,
	CreatureSpellFreezeInHand = 91,
	CreatureSpellFreezeOnHolder = 92,
	CreatureSpellCompassion = 93,
	CreatureSpellCompassionInHand = 94,
	CreatureSpellCompassionOnHolder = 95,
	CreatureSpellWeak = 96,
	CreatureSpellWeakInHand = 97,
	CreatureSpellWeakOnHolder = 98,
	ScriptHighlightGoldGlints = 99,
	ScriptHighlightSilverGlints = 100,
	ScriptHighlightBronzeGlints = 101,
	ScriptHighlightGoldActive = 102,
	ScriptHighlightSilverActive = 103,
	FlockFlying = 104,
	FlockGround = 105,
	StormCast = 106,
	FoodPickup = 107,
	FoodPickupPoisoned = 108,
	FoodPickupFish = 109,
	WoodPickup = 110,
	FoodPutdownPoisoned = 111,
	WoodPutdown = 112,
	FoodPutdown = 113,
	Steam = 114,
	Smoke = 115,
	Bonfire = 116,
	EvilSmoke = 117,
	Dust = 118,
	MagicBeam = 119,
	MagicBeamOnCitadel = 120,
	MagicBeamCreatureSwap = 121,
	FlockFlyingCastGood = 122,
	FlockFlyingCastEvil = 123,
	FlockFlyingRainGood = 124,
	FlockFlyingRainEvil = 125,
	FlockGroundDust = 126,
	Butterflies = 127,
	ButterfliesOnObject = 128,
	Flies = 129,
	FliesOnObject = 130,
	ObjectAppear = 131,
	ObjectDisappear = 132,
	SingStonesGlow = 133,
	PlayerIconFountain = 134,
	Bang = 135,
	HealFx = 136,
	HighlightOnObject = 137,
	BeamExplosionFx = 138,
	Flash = 139,
	TickerTape = 140,
	ForestCreated = 141,
	SingingStonesHeal = 142,
	PilefoodSpeedup = 143,
	SpelldispenserVortex = 144,
	SeeThisBeam = 145,
	SeeThisBeam2 = 146,
	Test = 147,
	Test2 = 148,
	Test3 = 149,
};

enum class LeashSelectorType
{
	LeashInfoSelector = 0,
};

// CREATUREISLE
// enum class CreatureIslesBuildingsInfo
// {
// 	Dojo    = 0,
// 	Stadium = 1,
// };

enum class LeashType
{
	None = -1,
	Evil = 1,
	Rope = 2,
	Good = 3,
};

enum class MobileStaticInfo : int32_t
{
	None = -1,
	CeltFenceShort = 0,
	CeltFenceTall = 1,
	Rock = 2,
	DeadTree = 3,
	PhysicalShield = 4,
	SingingStone_1 = 5,
	SingingStoneBase = 6,
	StreetLantern = 7,
	Bonfire = 8,
	CreatureCage = 9,
	Idol = 10,
	IdolReward = 11,
	Vortex = 12,
	Teleport = 13,
	WeepingStone = 14,
	WeepingStoneReward = 15,
	Boulder1Chalk = 16,
	Boulder1Lime = 17,
	Boulder1Sand = 18,
	Boulder1Volcanic = 19,
	Boulder2Chalk = 20,
	Boulder2Lime = 21,
	Boulder2Sand = 22,
	Boulder2Volcanic = 23,
	Boulder3Chalk = 24,
	Boulder3Lime = 25,
	Boulder3Sand = 26,
	Boulder3Volcanic = 27,
	FlatrockChalk = 28,
	FlatrockLimestone = 29,
	FlatrockSandstone = 30,
	FlatrockVolcanic = 31,
	LongrockChalk = 32,
	LongrockLimestone = 33,
	LongrockSandstone = 34,
	LongrockVolcanic = 35,
	RockChalk = 36,
	RockLimestone = 37,
	RockSandstone = 38,
	RockVolcanic = 39,
	SharprockChalk = 40,
	SharprockLimestone = 41,
	SharprockSandstone = 42,
	SharprockVolcanic = 43,
	SquarerockChalk = 44,
	SquarerockLimestone = 45,
	SquarerockSandstone = 46,
	SquarerockVolcanic = 47,
	StandaloneAltar = 48,
	GateTotemApe = 49,
	GateTotemBlank = 50,
	GateTotemCow = 51,
	GateTotemTiger = 52,
	ToyBall = 53,
	ToyCuddly = 54,
	ToyDie = 55,
	ToySkittle = 56,
	ToyBowlingBall = 57,
	SculptureInProgress = 58,
	CountryLantern = 59,
	Meteor = 60,

	_COUNT
	// Marble1                  = 61, // CREATUREISLE
	// Marble2                  = 62, // CREATUREISLE
	// Marble3                  = 63, // CREATUREISLE
	// Marble4                  = 64, // CREATUREISLE
	// Marble5                  = 65, // CREATUREISLE
	// Marble6                  = 66, // CREATUREISLE
	// Marble7                  = 67, // CREATUREISLE
	// Marble8                  = 68, // CREATUREISLE
	// Marble9                  = 69, // CREATUREISLE
	// Marble10                 = 70, // CREATUREISLE
	// Marble11                 = 71, // CREATUREISLE
	// Marble12                 = 72, // CREATUREISLE
	// Egg                      = 73, // CREATUREISLE
	// SoftBall                 = 74, // CREATUREISLE
	// BaseOnlyBats             = 75, // CREATUREISLE
	// BaseOnlyCreatureFight    = 76, // CREATUREISLE
	// BaseOnlyCreatureSpeed    = 77, // CREATUREISLE
	// BaseOnlyFungus           = 78, // CREATUREISLE
	// BaseOnlyIce              = 79, // CREATUREISLE
	// BaseOnlyNoEncounter      = 80, // CREATUREISLE
	// BaseOnlySwamp            = 81, // CREATUREISLE
	// BaseOnlyTeleport         = 82, // CREATUREISLE
	// BaseOnlySwap             = 83, // CREATUREISLE
	// BaseOnlyBrotherhood      = 84, // CREATUREISLE
	// BaseOnlyBrotherhoodSmall = 85, // CREATUREISLE
	// WaterCannon              = 86, // CREATUREISLE
	// WaterCannonAmmo          = 87, // CREATUREISLE
	// BaseOnlyMarbleBase       = 88, // CREATUREISLE
};

enum class FeatureInfo : int32_t
{
	None = -1,
	AztcOlmechead = 0,
	AztcStatue = 1,
	AztcSuntemple = 2,
	AztcTemple = 3,
	EgptNeedle = 4,
	EgptPyramid = 5,
	EgptWell = 6,
	GrekAcropolis = 7,
	TibtLargePiller = 8,
	TibtTemple = 9,
	MineEntrance = 10,
	PrisonPillar_1 = 11,
	PrisonPillar_2 = 12,
	PrisonPillar_3 = 13,
	Old_4 = 14,
	Old_5 = 15,
	Old_6 = 16,
	Old_7 = 17,
	Old_8 = 18,
	Old_9 = 19,
	Old_10 = 20,
	Old_11 = 21,
	Old_12 = 22,
	FatPilarChalk = 23,
	FatPilarLime = 24,
	FatPilarSand = 25,
	FatPilarVolcanic = 26,
	FPiperCave = 27,
	FPiperCaveRock = 28,
	Pilar2Chalk = 29,
	Pilar2Lime = 30,
	Pilar2Sand = 31,
	Pilar2Volcanic = 32,
	Pilar3Chalk = 33,
	Pilar3Lime = 34,
	Pilar3Sand = 35,
	Pilar3Volcanic = 36,
	PilarChalk = 37,
	PilarLime = 38,
	PilarSand = 39,
	PilarVolcanic = 40,
	SpikeyPilarChalk = 41,
	SpikeyPilarLime = 42,
	SpikeyPilarSand = 43,
	SpikeyPilarVolcanic = 44,
	Champi = 45,
	Old_13 = 46,
	Old_14 = 47,
	Old_15 = 48,
	Old_16 = 49,
	Old_17 = 50,
	Old_18 = 51,
	Old_19 = 52,
	Old_20 = 53,
	MagicMushroom = 54,
	Old_21 = 55,
	Old_22 = 56,
	Old_23 = 57,
	Old_24 = 58,
	Old_25 = 59,
	Old_26 = 60,
	Old_27 = 61,
	Old_28 = 62,
	Old_29 = 63,
	Old_30 = 64,
	Old_31 = 65,
	Old_32 = 66,
	Toadstool = 67,
	Ark = 68,
	ArkDryDock = 69,
	ArkWreck = 70,
	Whale = 71,
	PhoneBox = 72,
	Pier = 73,
	Crater = 74,
	Tombstone = 75,

	_COUNT

	// StoneIconApe = 76,
	// StoneIconBear = 77,
	// StoneIconCow = 78,
	// StoneIconHorse = 79,
	// StoneIconLeopard = 80,
	// StoneIconLion = 81,
	// StoneIconPolarbear = 82,
	// StoneIconSheep = 83,
	// StoneIconTiger = 84,
	// StoneIconTortoise = 85,
	// StoneIconWolf = 86,
	// StoneIconZebra = 87,

	// MarbleBaseDoNotUse  = 88, // CREATUREISLE
	// EggTree             = 89, // CREATUREISLE
	// CreatureRacePillar  = 90, // CREATUREISLE
	// EggTree_2           = 91, // CREATUREISLE
	// EggTree_3           = 92, // CREATUREISLE
	// EggTree_4           = 93, // CREATUREISLE
	// PointerHand         = 94, // CREATUREISLE
	// BowlingEnd          = 95, // CREATUREISLE
};

enum class FlowersInfo
{
	WhiteOne = 0,
	Pink = 1,
	// Sun      = 2, // CREATUREISLE
};

enum class AnimatedStaticInfo : int32_t
{
	None = -1,
	NorseGate = 0,
	GateStonePlinth = 1,
	PiperCaveEntrance = 2,
	ChessPionTeamA = 3,
	ChessPionTeamB = 4,
	ChessTowerTeamA = 5,
	ChessTowerTeamB = 6,
	ChessKnightTeamA = 7,
	ChessKnightTeamB = 8,
	ChessMadTeamA = 9,
	ChessMadTeamB = 10,
	ChessQueenTeamA = 11,
	ChessQueenTeamB = 12,
	ChessKingTeamA = 13,
	ChessKingTeamB = 14,
	PhoneBox = 15,

	_COUNT
	// Balista           = 16, // CREATUREISLE
};

enum class MagicType : uint32_t
{
	None = 0,
	Fireball = 1,
	FireballPowerUpOne = 2,
	FireballPowerUpTwo = 3,
	LightningBolt = 4,
	LightningBoltPowerUpOne = 5,
	LightningBoltPowerUpTwo = 6,
	ExplosionOne = 7,
	ExplosionOnePuOne = 8,
	ExplosionOnePuTwo = 9,
	Heal = 10,
	HealPowerUpOne = 11,
	Teleport = 12,
	Forest = 13,
	Food = 14,
	FoodPowerUpOne = 15,
	StormWindRain = 16,
	StormWindRainLightning = 17,
	Tornado = 18,
	Shield = 19,
	PhysicalShield = 20,
	Wood = 21,
	Water = 22,
	WaterPowerUpOne = 23,
	FlockFlying = 24,
	FlockGround = 25,
	CreatureSpellFreeze = 26,
	CreatureSpellSmall = 27,
	CreatureSpellBig = 28,
	CreatureSpellWeak = 29,
	CreatureSpellStrong = 30,
	CreatureSpellFat = 31,
	CreatureSpellThin = 32,
	CreatureSpellInvisible = 33,
	CreatureSpellCompassion = 34,
	CreatureSpellAngry = 35,
	CreatureSpellHungry = 36,
	CreatureSpellFrightene = 37,
	CreatureSpellTired = 38,
	CreatureSpellIll = 39,
	CreatureSpellThirsty = 40,
	CreatureSpellItchy = 41,
	// CreatureSpellAntiSpell  = 42, // CREATUREISLE
	// CreatureSpellFast       = 43, // CREATUREISLE
};

enum class CreatureReceiveSpellType : uint32_t
{
	CreatureReceiveSpellFreeze = 0,
	CreatureReceiveSpellSmall = 1,
	CreatureReceiveSpellBig = 2,
	CreatureReceiveSpellWeak = 3,
	CreatureReceiveSpellStrong = 4,
	CreatureReceiveSpellFat = 5,
	CreatureReceiveSpellThin = 6,
	CreatureReceiveSpellInvisible = 7,
	CreatureReceiveSpellCompassionate = 8,
	CreatureReceiveSpellAngry = 9,
	CreatureReceiveSpellHungry = 10,
	CreatureReceiveSpellFrightened = 11,
	CreatureReceiveSpellTired = 12,
	CreatureReceiveSpellIll = 13,
	CreatureReceiveSpellThirsty = 14,
	CreatureReceiveSpellItchy = 15,
	// CreatureReceiveSpellAntiSpell     = 16, // CREATUREISLE
	// CreatureReceiveSpellFast          = 17, // CREATUREISLE
};

enum class TerrainMaterialType
{
	None = 0,
	DeepWater = 1,
	ShallowWater = 2,
	RiverMud = 3,
	WetEarth = 4,
	Earth = 5,
	HardEarth = 6,
	Sand = 7,
	WetSand = 8,
	Gravel = 9,
	WetGravel = 10,
	SmallRocks = 11,
	SmallRocksInShallowWater = 12,
	SmallLooseRocksOnSolidRock = 13,
	LargeRocks = 14,
	SolidRock = 15,
	WetSolidRock = 16,
	UnstableSolidRock = 17,
	Grass = 18,
	WetGrass = 19,
	DryGrass = 20,
	Rushes = 21,
	Ferns = 22,
	Corn = 23,
	Scrub = 24,
	Heather = 25,
	ForestUndergrowth = 26,
	Snow = 27,
	PackIce = 28,
	SolidIce = 29,
	SnowyMountainTops = 30,
	Pavement = 31,
	WetPavement = 32,
	Crystal = 33,
	Metal = 34,
	HollowMetal = 35,
	Flesh = 36,
	GroaningFlesh = 37,
	Timber = 38,
	Scales = 39,
	Cloud = 40,
	Hair = 41,
	JungleFloor = 42,
};

enum class SoundSurfaceType
{
	None = 0,
	Grass = 1,
	Gravel = 2,
	Hard = 3,
	Mud = 4,
	Snow = 5,
	DeepWater = 6,
	ShallowWater = 7,
	LooseFoliage = 8,
};

enum class Reaction : int
{
	None = -1,
	FleeFromObject = 0,
	LookAtObject = 1,
	FollowObject = 2,
	FleeFromSpell = 3,
	LookAtSpell = 4,
	FollowSpell = 5,
	ReactToCreature = 6,
	ReactToFood = 7,
	ReactToMagicTree = 8,
	ReactToFlyingObject = 9,
	ReactToFire = 10,
	ReactToBall = 11,
	ReactToWood = 12,
	ReactToMagicShield = 13,
	ReactToGiftFromCreature = 14,
	ReactToNewBuilding = 15,
	ReactToHandPickUp = 16,
	ReactToHandUsingTotem = 17,
	ReactToObjectCrushed = 18,
	ReactToFight = 19,
	ReactToTeleport = 20,
	LookAtNiceSpell = 21,
	ReactToHandPuttingStuffInStoragePit = 22,
	ReactToDeath = 23,
	ReactToDroppedByHand = 24,
	Fainting = 25,
	Confused = 26,
	AvoidFallingTree = 27,
	FleeFromPredator = 28,
	CrowdAround = 29,
	ReactToBreeder = 30,
	ReactToTownCelebration = 31,
	ReactToVillagerInHand = 32,
	ReactToBurningObjectInHand = 33,
	ReactToMagicWaterPuttingOutFire = 34,
	ReactToMagicShieldStruck = 35,
	ReactToMagicShieldDestroyed = 36,
	ReactToImpressiveSpell = 37,
	ReactToScaffold = 38,
	ReactToMissionary = 39,
	ReactToFightWon = 40,
};

enum class ImpressiveType
{
	Fireball = 0,
	FireballPuOne = 1,
	FireballPuTwo = 2,
	LightningStorm = 3,
	LightningBolt = 4,
	LightningBoltPuOne = 5,
	LightningBoltPuTwo = 6,
	ExplosionOne = 7,
	Football = 8,
	Heal = 9,
	HealPuOne = 10,
	Tornado = 11,
	Wall = 12,
	Firework = 13,
	MagicTree = 14,
	Priest = 15,
	MagicFood = 16,
	MagicFoodPuOne = 17,
	MagicFoodPuTwo = 18,
	ShieldOne = 19,
	MagicWood = 20,
	MagicWater = 21,
	GiveVillagersFood = 22,
	GiveVillagersWood = 23,
	CreatureNotImpressive = 24,
	CreatureQuiteImpressive = 25,
	CreatureVeryImpressive = 26,
	NotImpressiveAtAll = 27,
	SkeletonArmy = 28,
};

enum class EffectType
{
	Burn = 0,
	Crush = 1,
	Hit = 2,
	Heal = 3,
	ApplyForce = 4,
	AlignmentModification = 5,
	BeliefModification = 6,
};

enum class LivingStates : uint32_t
{
	LivingInvalid = 0,
	LivingMoveToPos = 1,
	LivingMoveToObject = 2,
	LivingMoveOnStructure = 3,
	LivingInScript = 4,
	LivingInDance = 5,
	LivingFleeFromObjectReaction = 6,
	LivingLookAtObjectReaction = 7,
	LivingFollowObjectReaction = 8,
	LivingInspectObjectReaction = 9,
	LivingFlying = 10,
	LivingLanded = 11,
	LivingLookAtFlyingObjectReaction = 12,
	LivingSetDying = 13,
	LivingDying = 14,
	LivingDead = 15,
	LivingDrowning = 16,
	LivingDowned = 17,
	LivingBeingEaten = 18,
	LivingGotoFoodReaction = 19,
	LivingArrivesAtFoodReaction = 20,
	LivingGotoWoodReaction = 21,
	LivingArrivesAtWoodReaction = 22,
	LivingWaitForAnimation = 23,
	LivingInHand = 24,
	LivingGotoPickupBallReaction = 25,
	LivingArrivesAtPickupBallReaction = 26,
	LivingMoveInFlock = 27,
	LivingMoveAlongPath = 28,
	LivingMoveOnPath = 29,
	LivingFleeingLookAtObjectReaction = 30,
};

enum class VillagerStates : uint32_t
{
	InvalidState = 0,
	MoveToPos = 1,
	MoveToObject = 2,
	MoveOnStructure = 3,
	InScript = 4,
	InDance = 5,
	FleeingFromObjectReaction = 6,
	LookingAtObjectReaction = 7,
	FollowingObjectReaction = 8,
	InspectObjectReaction = 9,
	Flying = 10,
	Landed = 11,
	LookAtFlyingObjectReaction = 12,
	SetDying = 13,
	Dying = 14,
	Dead = 15,
	Drowning = 16,
	Downed = 17,
	BeingEaten = 18,
	GotoFoodReaction = 19,
	ArrivesAtFoodReaction = 20,
	GotoWoodReaction = 21,
	ArrivesAtWoodReaction = 22,
	WaitForAnimation = 23,
	InHand = 24,
	GotoPickupBallReaction = 25,
	ArrivesAtPickupBallReaction = 26,
	MoveInFlock = 27,
	MoveAlongPath = 28,
	MoveOnPath = 29,
	FleeingAndLookingAtObjectReaction = 30,
	GotoStoragePitForDropOff = 31,
	ArrivesAtStoragePitForDropOff = 32,
	GotoStoragePitForFood = 33,
	ArrivesAtStoragePitForFood = 34,
	ArrivesAtHomeWithFood = 35,
	GoHome = 36,
	ArrivesHome = 37,
	AtHome = 38,
	ArrivesAtStoragePitForBuildingMaterials = 39,
	ArrivesAtBuildingSite = 40,
	Building = 41,
	GotoStoragePitForWorshipSupplies = 42,
	ArrivesAtStoragePitForWorshipSupplies = 43,
	GotoWorshipSiteWithSupplies = 44,
	MoveToWorshipSiteWithSupplies = 45,
	ArrivesAtWorshipSiteWithSupplies = 46,
	ForesterMoveToForest = 47,
	ForesterGotoForest = 48,
	ForesterArrivesAtForest = 49,
	ForesterChopsTree = 50,
	ForesterChopsTreeForBuilding = 51,
	ForesterFinishedForestering = 52,
	ArrivesAtBigForest = 53,
	ArrivesAtBigForestForBuilding = 54,
	FishermanArrivesAtFishing = 55,
	Fishing = 56,
	WaitForCounter = 57,
	GotoWorshipSiteForWorship = 58,
	ArrivesAtWorshipSiteForWorship = 59,
	WorshippingAtWorshipSite = 60,
	GotoAltarForRest = 61,
	ArrivesAtAltarForRest = 62,
	AtAltarRest = 63,
	AtAltarFinishedRest = 64,
	RestartWorshippingAtWorshipSite = 65,
	RestartWorshippingCreature = 66,
	FarmerArrivesAtFarm = 67,
	FarmerPlantsCrop = 68,
	FarmerDigsUpCrop = 69,
	MoveToFootballPitchConstruction = 70,
	FootballWalkToPosition = 71,
	FootballWaitForKickOff = 72,
	FootballAttacker = 73,
	FootballGoalie = 74,
	FootballDefender = 75,
	FootballWonGoal = 76,
	FootballLostGoal = 77,
	StartMoveToPickUpBallForDeadBall = 78,
	ArrivedAtPickUpBallForDeadBall = 79,
	ArrivedAtPutDownBallForDeadBallStart = 80,
	ArrivedAtPutDownBallForDeadBallEnd = 81,
	FootballMatchPaused = 82,
	FootballWatchMatch = 83,
	FootballMexicanWave = 84,
	Created = 85,
	ArrivesInAbodeToTrade = 86,
	ArrivesInAbodeToPickUpExcess = 87,
	MakeScaredStiff = 88,
	ScaredStiff = 89,
	WorshippingCreature = 90,
	ShepherdLookForFlock = 91,
	ShepherdMoveFlockToWater = 92,
	ShepherdMoveFlockToFood = 93,
	ShepherdMoveFlockBack = 94,
	ShepherdDecideWhatToDoWithFlock = 95,
	ShepherdWaitForFlock = 96,
	ShepherdSlaughterAnimal = 97,
	ShepherdFetchStray = 98,
	ShepherdGotoFlock = 99,
	HousewifeAtHome = 100,
	HousewifeGotoStoragePit = 101,
	HousewifeArrivesAtStoragePit = 102,
	HousewifePickupFromStoragePit = 103,
	HousewifeReturnHomeWithFood = 104,
	HousewifeMakeDinner = 105,
	HousewifeServesDinner = 106,
	HousewifeClearsAwayDinner = 107,
	HousewifeDoesHousework = 108,
	HousewifeGossipsAroundStoragePit = 109,
	HousewifeStartsGivingBirth = 110,
	HousewifeGivingBirth = 111,
	HousewifeGivenBirth = 112,
	ChildAtCreche = 113,
	ChildFollowsMother = 114,
	ChildBecomesAdult = 115,
	SitsDownToDinner = 116,
	EatFood = 117,
	EatFoodAtHome = 118,
	GotoBedAtHome = 119,
	SleepingAtHome = 120,
	WakeUpAtHome = 121,
	StartHavingSex = 122,
	HavingSex = 123,
	StopHavingSex = 124,
	StartHavingSexAtHome = 125,
	HavingSexAtHome = 126,
	StopHavingSexAtHome = 127,
	WaitForDinner = 128,
	HomelessStart = 129,
	VagrantStart = 130,
	MornDeath = 131,
	PerformInspectionReaction = 132,
	ApproachObjectReaction = 133,
	InitialiseTellOthersAboutObject = 134,
	TellOthersAboutInterestingObject = 135,
	ApproachVillagerToTalkTo = 136,
	TellParticularVillagerAboutObject = 137,
	InitialiseLookAroundForVillagerToTell = 138,
	LookAroundForVillagerToTell = 139,
	MoveTowardsObjectToLookAt = 140,
	InitialiseImpressedReaction = 141,
	PerformImpressedReaction = 142,
	InitialiseFightReaction = 143,
	PerformFightReaction = 144,
	HomelessEatDinner = 145,
	InspectCreatureReaction = 146,
	PerformInspectCreatureReaction = 147,
	ApproachCreatureReaction = 148,
	InitialiseBewilderedByMagicTreeReaction = 149,
	PerformBewilderedByMagicTreeReaction = 150,
	TurnToFaceMagicTree = 151,
	LookAtMagicTree = 152,
	DanceForEditingPurposes = 153,
	MoveToDancePos = 154,
	InitialiseRespectCreatureReaction = 155,
	PerformRespectCreatureReaction = 156,
	FinishRespectCreatureReaction = 157,
	ApproachHandReaction = 158,
	FleeingFromCreatureReaction = 159,
	TurnToFaceCreatureReaction = 160,
	WatchFlyingObjectReaction = 161,
	PointAtFlyingObjectReaction = 162,
	DecideWhatToDo = 163,
	InteractDecideWhatToDo = 164,
	EatOutside = 165,
	RunAwayFromObjectReaction = 166,
	MoveTowardsCreatureReaction = 167,
	AmazedByMagicShieldReaction = 168,
	VillagerGossips = 169,
	CheckInteractWithAnimal = 170,
	CheckInteractWithWorshipSite = 171,
	CheckInteractWithAbode = 172,
	CheckInteractWithField = 173,
	CheckInteractWithFishFarm = 174,
	CheckInteractWithTree = 175,
	CheckInteractWithBall = 176,
	CheckInteractWithPot = 177,
	CheckInteractWithFootball = 178,
	CheckInteractWithVillager = 179,
	CheckInteractWithMagicLiving = 180,
	CheckInteractWithRock = 181,
	ArrivesAtRockForWood = 182,
	GotWoodFromRock = 183,
	ReenterBuildingState = 184,
	ArriveAtPushObject = 185,
	TakeWoodFromTree = 186,
	TakeWoodFromPot = 187,
	TakeWoodFromTreeForBuilding = 188,
	TakeWoodFromPotForBuilding = 189,
	ShepherdTakeAnimalForSlaughter = 190,
	ShepherdTakesControlOfFlock = 191,
	ShepherdReleasesControlOfFlock = 192,
	DanceButNotWorship = 193,
	FaintingReaction = 194,
	StartConfusedReaction = 195,
	ConfusedReaction = 196,
	AfterTapOnAbode = 197,
	WeakOnGround = 198,
	ScriptWanderAroundPosition = 199,
	ScriptPlayAnim = 200,
	GoTowardsTeleportReaction = 201,
	TeleportReaction = 202,
	DanceWhileReacting = 203,
	ControlledByCreature = 204,
	PointAtDeadPerson = 205,
	GoTowardsDeadPerson = 206,
	LookAtDeadPerson = 207,
	MournDeadPerson = 208,
	NothingToDo = 209,
	ArrivesAtWorkshopForDropOff = 210,
	ArrivesAtStoragePitForWorkshopMaterials = 211,
	ShowPoisoned = 212,
	HidingAtWorshipSite = 213,
	CrowdReaction = 214,
	ReactToFire = 215,
	PutOutFireByBeating = 216,
	PutOutFireWithWater = 217,
	GetWaterToPutOutFire = 218,
	OnFire = 219,
	MoveAroundFire = 220,
	DiscipleNothingToDo = 221,
	FootballMoveToBall = 222,
	ArrivesAtStoragePitForTraderPickUp = 223,
	ArrivesAtStoragePitForTraderDropOff = 224,
	BreederDisciple = 225,
	MissionaryDisciple = 226,
	ReactToBreeder = 227,
	ShepherdCheckAnimalForSlaughter = 228,
	InteractDecideWhatToDoForOtherVillager = 229,
	ArtifactDance = 230,
	FleeingFromPredatorReaction = 231,
	WaitForWood = 232,
	InspectObject = 233,
	GoHomeAndChange = 234,
	WaitForMate = 235,
	GoAndHideInNearbyBuilding = 236,
	LookToSeeIfItIsSafe = 237,
	SleepInTent = 238,
	PauseForASecond = 239,
	PanicReaction = 240,
	GetFoodAtWorshipSite = 241,
	GotoCongregateInTownAfterEmergency = 242,
	CongregateInTownAfterEmergency = 243,
	ScriptInCrowd = 244,
	GoAndChilloutOutsideHome = 245,
	SitAndChillout = 246,
	ScriptGoAndMoveAlongPath = 247,
	GoHomeFromWorship = 248,
	ArrivesHomeFromWorship = 249,
	SleepInTentFromWorship = 250,
	GoTowardsTeleportReactionQuickly = 251,
	GoAndChilloutInTown = 252,
	WaitForArtifactDance = 253,
	BreederJustLanded = 254,
	LastState = 255,

	_COUNT = LastState
};

static constexpr std::array<std::string_view, static_cast<size_t>(VillagerStates::_COUNT)> k_VillagerStateStrings = {
    "INVALID_STATE",
    "MOVE_TO_POS",
    "MOVE_TO_OBJECT",
    "MOVE_ON_STRUCTURE",
    "IN_SCRIPT",
    "IN_DANCE",
    "FLEEING_FROM_OBJECT_REACTION",
    "LOOKING_AT_OBJECT_REACTION",
    "FOLLOWING_OBJECT_REACTION",
    "INSPECT_OBJECT_REACTION",
    "FLYING",
    "LANDED",
    "LOOK_AT_FLYING_OBJECT_REACTION",
    "SET_DYING",
    "DYING",
    "DEAD",
    "DROWNING",
    "DOWNED",
    "BEING_EATEN",
    "GOTO_FOOD_REACTION",
    "ARRIVES_AT_FOOD_REACTION",
    "GOTO_WOOD_REACTION",
    "ARRIVES_AT_WOOD_REACTION",
    "WAIT_FOR_ANIMATION",
    "IN_HAND",
    "GOTO_PICKUP_BALL_REACTION",
    "ARRIVES_AT_PICKUP_BALL_REACTION",
    "MOVE_IN_FLOCK",
    "MOVE_ALONG_PATH",
    "MOVE_ON_PATH",
    "FLEEING_AND_LOOKING_AT_OBJECT_REACTION",
    "GOTO_STORAGE_PIT_FOR_DROP_OFF",
    "ARRIVES_AT_STORAGE_PIT_FOR_DROP_OFF",
    "GOTO_STORAGE_PIT_FOR_FOOD",
    "ARRIVES_AT_STORAGE_PIT_FOR_FOOD",
    "ARRIVES_AT_HOME_WITH_FOOD",
    "GO_HOME",
    "ARRIVES_HOME",
    "AT_HOME",
    "ARRIVES_AT_STORAGE_PIT_FOR_BUILDING_MATERIALS",
    "ARRIVES_AT_BUILDING_SITE",
    "BUILDING",
    "GOTO_STORAGE_PIT_FOR_WORSHIP_SUPPLIES",
    "ARRIVES_AT_STORAGE_PIT_FOR_WORSHIP_SUPPLIES",
    "GOTO_WORSHIP_SITE_WITH_SUPPLIES",
    "MOVE_TO_WORSHIP_SITE_WITH_SUPPLIES",
    "ARRIVES_AT_WORSHIP_SITE_WITH_SUPPLIES",
    "FORESTER_MOVE_TO_FOREST",
    "FORESTER_GOTO_FOREST",
    "FORESTER_ARRIVES_AT_FOREST",
    "FORESTER_CHOPS_TREE",
    "FORESTER_CHOPS_TREE_FOR_BUILDING",
    "FORESTER_FINISHED_FORESTERING",
    "ARRIVES_AT_BIG_FOREST",
    "ARRIVES_AT_BIG_FOREST_FOR_BUILDING",
    "FISHERMAN_ARRIVES_AT_FISHING",
    "FISHING",
    "WAIT_FOR_COUNTER",
    "GOTO_WORSHIP_SITE_FOR_WORSHIP",
    "ARRIVES_AT_WORSHIP_SITE_FOR_WORSHIP",
    "WORSHIPPING_AT_WORSHIP_SITE",
    "GOTO_ALTAR_FOR_REST",
    "ARRIVES_AT_ALTAR_FOR_REST",
    "AT_ALTAR_REST",
    "AT_ALTAR_FINISHED_REST",
    "RESTART_WORSHIPPING_AT_WORSHIP_SITE",
    "RESTART_WORSHIPPING_CREATURE",
    "FARMER_ARRIVES_AT_FARM",
    "FARMER_PLANTS_CROP",
    "FARMER_DIGS_UP_CROP",
    "MOVE_TO_FOOTBALL_PITCH_CONSTRUCTION",
    "FOOTBALL_WALK_TO_POSITION",
    "FOOTBALL_WAIT_FOR_KICK_OFF",
    "FOOTBALL_ATTACKER",
    "FOOTBALL_GOALIE",
    "FOOTBALL_DEFENDER",
    "FOOTBALL_WON_GOAL",
    "FOOTBALL_LOST_GOAL",
    "START_MOVE_TO_PICK_UP_BALL_FOR_DEAD_BALL",
    "ARRIVED_AT_PICK_UP_BALL_FOR_DEAD_BALL",
    "ARRIVED_AT_PUT_DOWN_BALL_FOR_DEAD_BALL_START",
    "ARRIVED_AT_PUT_DOWN_BALL_FOR_DEAD_BALL_END",
    "FOOTBALL_MATCH_PAUSED",
    "FOOTBALL_WATCH_MATCH",
    "FOOTBALL_MEXICAN_WAVE",
    "CREATED",
    "ARRIVES_IN_ABODE_TO_TRADE",
    "ARRIVES_IN_ABODE_TO_PICK_UP_EXCESS",
    "MAKE_SCARED_STIFF",
    "SCARED_STIFF",
    "WORSHIPPING_CREATURE",
    "SHEPHERD_LOOK_FOR_FLOCK",
    "SHEPHERD_MOVE_FLOCK_TO_WATER",
    "SHEPHERD_MOVE_FLOCK_TO_FOOD",
    "SHEPHERD_MOVE_FLOCK_BACK",
    "SHEPHERD_DECIDE_WHAT_TO_DO_WITH_FLOCK",
    "SHEPHERD_WAIT_FOR_FLOCK",
    "SHEPHERD_SLAUGHTER_ANIMAL",
    "SHEPHERD_FETCH_STRAY",
    "SHEPHERD_GOTO_FLOCK",
    "HOUSEWIFE_AT_HOME",
    "HOUSEWIFE_GOTO_STORAGE_PIT",
    "HOUSEWIFE_ARRIVES_AT_STORAGE_PIT",
    "HOUSEWIFE_PICKUP_FROM_STORAGE_PIT",
    "HOUSEWIFE_RETURN_HOME_WITH_FOOD",
    "HOUSEWIFE_MAKE_DINNER",
    "HOUSEWIFE_SERVES_DINNER",
    "HOUSEWIFE_CLEARS_AWAY_DINNER",
    "HOUSEWIFE_DOES_HOUSEWORK",
    "HOUSEWIFE_GOSSIPS_AROUND_STORAGE_PIT",
    "HOUSEWIFE_STARTS_GIVING_BIRTH",
    "HOUSEWIFE_GIVING_BIRTH",
    "HOUSEWIFE_GIVEN_BIRTH",
    "CHILD_AT_CRECHE",
    "CHILD_FOLLOWS_MOTHER",
    "CHILD_BECOMES_ADULT",
    "SITS_DOWN_TO_DINNER",
    "EAT_FOOD",
    "EAT_FOOD_AT_HOME",
    "GOTO_BED_AT_HOME",
    "SLEEPING_AT_HOME",
    "WAKE_UP_AT_HOME",
    "START_HAVING_SEX",
    "HAVING_SEX",
    "STOP_HAVING_SEX",
    "START_HAVING_SEX_AT_HOME",
    "HAVING_SEX_AT_HOME",
    "STOP_HAVING_SEX_AT_HOME",
    "WAIT_FOR_DINNER",
    "HOMELESS_START",
    "VAGRANT_START",
    "MORN_DEATH",
    "PERFORM_INSPECTION_REACTION",
    "APPROACH_OBJECT_REACTION",
    "INITIALISE_TELL_OTHERS_ABOUT_OBJECT",
    "TELL_OTHERS_ABOUT_INTERESTING_OBJECT",
    "APPROACH_VILLAGER_TO_TALK_TO",
    "TELL_PARTICULAR_VILLAGER_ABOUT_OBJECT",
    "INITIALISE_LOOK_AROUND_FOR_VILLAGER_TO_TELL",
    "LOOK_AROUND_FOR_VILLAGER_TO_TELL",
    "MOVE_TOWARDS_OBJECT_TO_LOOK_AT",
    "INITIALISE_IMPRESSED_REACTION",
    "PERFORM_IMPRESSED_REACTION",
    "INITIALISE_FIGHT_REACTION",
    "PERFORM_FIGHT_REACTION",
    "HOMELESS_EAT_DINNER",
    "INSPECT_CREATURE_REACTION",
    "PERFORM_INSPECT_CREATURE_REACTION",
    "APPROACH_CREATURE_REACTION",
    "INITIALISE_BEWILDERED_BY_MAGIC_TREE_REACTION",
    "PERFORM_BEWILDERED_BY_MAGIC_TREE_REACTION",
    "TURN_TO_FACE_MAGIC_TREE",
    "LOOK_AT_MAGIC_TREE",
    "DANCE_FOR_EDITING_PURPOSES",
    "MOVE_TO_DANCE_POS",
    "INITIALISE_RESPECT_CREATURE_REACTION",
    "PERFORM_RESPECT_CREATURE_REACTION",
    "FINISH_RESPECT_CREATURE_REACTION",
    "APPROACH_HAND_REACTION",
    "FLEEING_FROM_CREATURE_REACTION",
    "TURN_TO_FACE_CREATURE_REACTION",
    "WATCH_FLYING_OBJECT_REACTION",
    "POINT_AT_FLYING_OBJECT_REACTION",
    "DECIDE_WHAT_TO_DO",
    "INTERACT_DECIDE_WHAT_TO_DO",
    "EAT_OUTSIDE",
    "RUN_AWAY_FROM_OBJECT_REACTION",
    "MOVE_TOWARDS_CREATURE_REACTION",
    "AMAZED_BY_MAGIC_SHIELD_REACTION",
    "VILLAGER_GOSSIPS",
    "CHECK_INTERACT_WITH_ANIMAL",
    "CHECK_INTERACT_WITH_WORSHIP_SITE",
    "CHECK_INTERACT_WITH_ABODE",
    "CHECK_INTERACT_WITH_FIELD",
    "CHECK_INTERACT_WITH_FISH_FARM",
    "CHECK_INTERACT_WITH_TREE",
    "CHECK_INTERACT_WITH_BALL",
    "CHECK_INTERACT_WITH_POT",
    "CHECK_INTERACT_WITH_FOOTBALL",
    "CHECK_INTERACT_WITH_VILLAGER",
    "CHECK_INTERACT_WITH_MAGIC_LIVING",
    "CHECK_INTERACT_WITH_ROCK",
    "ARRIVES_AT_ROCK_FOR_WOOD",
    "GOT_WOOD_FROM_ROCK",
    "REENTER_BUILDING_STATE",
    "ARRIVE_AT_PUSH_OBJECT",
    "TAKE_WOOD_FROM_TREE",
    "TAKE_WOOD_FROM_POT",
    "TAKE_WOOD_FROM_TREE_FOR_BUILDING",
    "TAKE_WOOD_FROM_POT_FOR_BUILDING",
    "SHEPHERD_TAKE_ANIMAL_FOR_SLAUGHTER",
    "SHEPHERD_TAKES_CONTROL_OF_FLOCK",
    "SHEPHERD_RELEASES_CONTROL_OF_FLOCK",
    "DANCE_BUT_NOT_WORSHIP",
    "FAINTING_REACTION",
    "START_CONFUSED_REACTION",
    "CONFUSED_REACTION",
    "AFTER_TAP_ON_ABODE",
    "WEAK_ON_GROUND",
    "SCRIPT_WANDER_AROUND_POSITION",
    "SCRIPT_PLAY_ANIM",
    "GO_TOWARDS_TELEPORT_REACTION",
    "TELEPORT_REACTION",
    "DANCE_WHILE_REACTING",
    "CONTROLLED_BY_CREATURE",
    "POINT_AT_DEAD_PERSON",
    "GO_TOWARDS_DEAD_PERSON",
    "LOOK_AT_DEAD_PERSON",
    "MOURN_DEAD_PERSON",
    "NOTHING_TO_DO",
    "ARRIVES_AT_WORKSHOP_FOR_DROP_OFF",
    "ARRIVES_AT_STORAGE_PIT_FOR_WORKSHOP_MATERIALS",
    "SHOW_POISONED",
    "HIDING_AT_WORSHIP_SITE",
    "CROWD_REACTION",
    "REACT_TO_FIRE",
    "PUT_OUT_FIRE_BY_BEATING",
    "PUT_OUT_FIRE_WITH_WATER",
    "GET_WATER_TO_PUT_OUT_FIRE",
    "ON_FIRE",
    "MOVE_AROUND_FIRE",
    "DISCIPLE_NOTHING_TO_DO",
    "FOOTBALL_MOVE_TO_BALL",
    "ARRIVES_AT_STORAGE_PIT_FOR_TRADER_PICK_UP",
    "ARRIVES_AT_STORAGE_PIT_FOR_TRADER_DROP_OFF",
    "BREEDER_DISCIPLE",
    "MISSIONARY_DISCIPLE",
    "REACT_TO_BREEDER",
    "SHEPHERD_CHECK_ANIMAL_FOR_SLAUGHTER",
    "INTERACT_DECIDE_WHAT_TO_DO_FOR_OTHER_VILLAGER",
    "ARTIFACT_DANCE",
    "FLEEING_FROM_PREDATOR_REACTION",
    "WAIT_FOR_WOOD",
    "INSPECT_OBJECT",
    "GO_HOME_AND_CHANGE",
    "WAIT_FOR_MATE",
    "GO_AND_HIDE_IN_NEARBY_BUILDING",
    "LOOK_TO_SEE_IF_IT_IS_SAFE",
    "SLEEP_IN_TENT",
    "PAUSE_FOR_A_SECOND",
    "PANIC_REACTION",
    "GET_FOOD_AT_WORSHIP_SITE",
    "GOTO_CONGREGATE_IN_TOWN_AFTER_EMERGENCY",
    "CONGREGATE_IN_TOWN_AFTER_EMERGENCY",
    "SCRIPT_IN_CROWD",
    "GO_AND_CHILLOUT_OUTSIDE_HOME",
    "SIT_AND_CHILLOUT",
    "SCRIPT_GO_AND_MOVE_ALONG_PATH",
    "RESTART_MEETING",
    "ARRIVES_AT_SCAFFOLD_FOR_PICKUP",
    "ARRIVES_AT_BUILDING_SITE_WITH_SCAFFOLD",
    "MOVE_SCAFFOLD_TO_BUILDING_SITE",
    "GOTO_ABODE_BURNING_REACTION",
    "ARRIVES_AT_ABODE_BURNING_REACTION",
    "REPAIRS_ABODE",
};
static_assert(k_VillagerStateStrings.size() == static_cast<size_t>(VillagerStates::_COUNT),
              "The number of state type strings should be the same as the number of state types");

enum class AlignmentType : uint32_t
{
	AnimalNice = 0,
	AnimalNasty = 1,
	Creature = 2,
	Priest = 3,
	Skeleton = 4,
	Villager = 5,
	Building = 6,
	Plant = 7,
	Field = 8,
	Feature = 9,
	MobileObject = 10,
	Land = 11,
	Script = 12,
	Unimportant = 13,
};

enum class SpellIconInfo
{
	Basic = 0,
	TownCentre = 1,
};

enum class WorshipSiteInfo
{
	Celtic = 0,
	African = 1,
	Aztec = 2,
	Japanese = 3,
	Indian = 4,
	Egyptian = 5,
	Greek = 6,
	Norse = 7,
	Tibetan = 8,
};

enum class PotInfo : int32_t
{
	FoodPot = 0,
	WoodPot = 1,
	StoragePitFoodPile = 2,
	WoodPile_1 = 3,
	WoodPile_2 = 4,
	WoodPile_3 = 5,
	WoodPile_4 = 6,
	WoodPile_5 = 7,
	FoodPile = 8,
	MagicWood = 9,
	MagicFood = 10,
	HandWood = 11,
	HandFood = 12,
	TreeBranchEvergreenInHand = 13,
	TreeBranchHardwoodInHand = 14,
	TreeFruitInHand = 15,
	TreeLogsInHand = 16,
	WheatInHand = 17,
	Fish = 18,

	_COUNT
};

enum class PotType : uint32_t
{
	Pot = 0,
	PileFood = 1,
	PileWood = 2,
};

enum class MiscInfo
{
	HelpOrb = 0,
	HelpOrbHolder = 1,
	Firefly = 2,
	ArenaSpellIcon = 3,
};

enum class SpookyEnum
{
	SpookyName_01 = 0,
	SpookyName_02 = 1,
	SpookyName_03 = 2,
	SpookyName_04 = 3,
	SpookyName_05 = 4,
};

enum class HighlightInfo
{
	ScriptHighlightInfoScriptBronze = 0,
	ScriptHighlightInfoScriptSilver = 1,
	ScriptHighlightInfoScriptGold = 2,
	// ScriptHighlightInfoScoreboard          = 3, // CREATUREISLE
	// ScriptHighlightInfoScoreboardBig       = 4, // CREATUREISLE
	// ScriptHighlightInfoScriptSilverCancel  = 5, // CREATUREISLE
	// ScriptHighlightInfoScriptGoldCancel    = 6, // CREATUREISLE
	// ScriptHighlightInfoScriptSwapCreature  = 7, // CREATUREISLE
	// ScriptHighlightInfoScriptFightCreature = 8, // CREATUREISLE
	// ScriptHighlightInfoBrotherhood         = 9, // CREATUREISLE
};

enum class MapShieldInfo
{
	Magic = 0,
	Physical = 1,
};

enum class TreeInfo
{
	Beech = 0,
	Birch = 1,
	Cedar = 2,
	Conifer = 3,
	ConiferA = 4,
	Oak = 5,
	OakA = 6,
	Olive = 7,
	Palm = 8,
	PalmA = 9,
	PalmB = 10,
	PalmC = 11,
	Pine = 12,
	Bush = 13,
	BushA = 14,
	BushB = 15,
	Cypress = 16,
	CypressA = 17,
	Copse = 18,
	CopseA = 19,
	Hedge = 20,
	HedgeA = 21,
	Burnt = 22,

	_COUNT
};

enum class ResourceType : int
{
	Any = -2,
	None = -1,
	Food = 0,
	Wood = 1,
};

enum class AbodeInfo : int32_t
{
	None = -1,
	CelticHut = 0,
	CelticTent = 1,
	CelticShackX = 2,
	CelticShackY = 3,
	CelticTempleX = 4,
	CelticTempleY = 5,
	CelticTotem = 6,
	CelticStoragePit = 7,
	CelticCreche = 8,
	CelticWorkshop = 9,
	CelticWonder = 10,
	CelticGraveyard = 11,
	CelticTownCentre = 12,
	CelticFootballPitch = 13,
	CelticSpellDispenser = 14,
	CelticField = 15,
	AfricanHut = 16,
	AfricanTent = 17,
	AfricanShackX = 18,
	AfricanShackY = 19,
	AfricanTempleX = 20,
	AfricanTempleY = 21,
	AfricanTotem = 22,
	AfricanStoragePit = 23,
	AfricanCreche = 24,
	AfricanWorkshop = 25,
	AfricanWonder = 26,
	AfricanGraveyard = 27,
	AfricanTownCentre = 28,
	AfricanFootballPitch = 29,
	AfricanSpellDispenser = 30,
	AfricanField = 31,
	AztecHut = 32,
	AztecTent = 33,
	AztecShackX = 34,
	AztecShackY = 35,
	AztecTempleX = 36,
	AztecTempleY = 37,
	AztecTotem = 38,
	AztecStoragePit = 39,
	AztecCreche = 40,
	AztecWorkshop = 41,
	AztecWonder = 42,
	AztecGraveyard = 43,
	AztecTownCentre = 44,
	AztecFootballPitch = 45,
	AztecSpellDispenser = 46,
	AztecField = 47,
	JapaneseHut = 48,
	JapaneseTent = 49,
	JapaneseShackX = 50,
	JapaneseShackY = 51,
	JapaneseTempleX = 52,
	JapaneseTempleY = 53,
	JapaneseTotem = 54,
	JapaneseStoragePit = 55,
	JapaneseCreche = 56,
	JapaneseWorkshop = 57,
	JapaneseWonder = 58,
	JapaneseGraveyard = 59,
	JapaneseTownCentre = 60,
	JapaneseFootballPitch = 61,
	JapaneseSpellDispenser = 62,
	JapaneseField = 63,
	IndianHut = 64,
	IndianTent = 65,
	IndianShackX = 66,
	IndianShackY = 67,
	IndianTempleX = 68,
	IndianTempleY = 69,
	IndianTotem = 70,
	IndianStoragePit = 71,
	IndianCreche = 72,
	IndianWorkshop = 73,
	IndianWonder = 74,
	IndianGraveyard = 75,
	IndianTownCentre = 76,
	IndianFootballPitch = 77,
	IndianSpellDispenser = 78,
	IndianField = 79,
	EgyptianHut = 80,
	EgyptianTent = 81,
	EgyptianShackX = 82,
	EgyptianShackY = 83,
	EgyptianTempleX = 84,
	EgyptianTempleY = 85,
	EgyptianTotem = 86,
	EgyptianStoragePit = 87,
	EgyptianCreche = 88,
	EgyptianWorkshop = 89,
	EgyptianWonder = 90,
	EgyptianGraveyard = 91,
	EgyptianTownCentre = 92,
	EgyptianFootballPitch = 93,
	EgyptianSpellDispenser = 94,
	EgyptianField = 95,
	GreekHut = 96,
	GreekTent = 97,
	GreekShackX = 98,
	GreekShackY = 99,
	GreekTempleX = 100,
	GreekTempleY = 101,
	GreekTotem = 102,
	GreekStoragePit = 103,
	GreekCreche = 104,
	GreekWorkshop = 105,
	GreekWonder = 106,
	GreekGraveyard = 107,
	GreekTownCentre = 108,
	GreekFootballPitch = 109,
	GreekSpellDispenser = 110,
	GreekField = 111,
	NorseHut = 112,
	NorseTent = 113,
	NorseShackX = 114,
	NorseShackY = 115,
	NorseTempleX = 116,
	NorseTempleY = 117,
	NorseTotem = 118,
	NorseStoragePit = 119,
	NorseCreche = 120,
	NorseWorkshop = 121,
	NorseWonder = 122,
	NorseGraveyard = 123,
	NorseTownCentre = 124,
	NorseFootballPitch = 125,
	NorseSpellDispenser = 126,
	NorseField = 127,
	TibetanHut = 128,
	TibetanTent = 129,
	TibetanShackX = 130,
	TibetanShackY = 131,
	TibetanTempleX = 132,
	TibetanTempleY = 133,
	TibetanTotem = 134,
	TibetanStoragePit = 135,
	TibetanCreche = 136,
	TibetanWorkshop = 137,
	TibetanWonder = 138,
	TibetanGraveyard = 139,
	TibetanTownCentre = 140,
	TibetanFootballPitch = 141,
	TibetanSpellDispenser = 142,
	TibetanField = 143,
	ArkDryDock = 144,
	ArkWreck = 145,
	Belltower = 146,

	_COUNT
};

enum class FootballInfo
{
	FootballPitch = 0,
};

enum class FootballPositionInfo
{
	NoneFree = -1,
	LeftAttack_1 = 0,
	LeftAttack_2 = 1,
	Goal_1 = 2,
	Goal_2 = 3,
	LeftDefence_1 = 4,
	LeftDefence_2 = 5,
	RightAttack_1 = 6,
	RightAttack_2 = 7,
	RightDefence_1 = 8,
	RightDefence_2 = 9,
};

enum class PfootballPositionInfo
{
	NoneFree = -1,
	LeftAttack_1 = 0,
	LeftAttack_2 = 1,
	Goal_1 = 2,
	Goal_2 = 3,
	LeftDefence_1 = 4,
	LeftDefence_2 = 5,
	RightAttack_1 = 6,
	RightAttack_2 = 7,
	RightDefence_1 = 8,
	RightDefence_2 = 9,
};

enum class PlaytimeInfo
{
	None = -1,
	Marrage = 0,
	Funeral = 1,
	Maypole = 2,
	Football = 3,
	Pfootball = 4,
};

enum class GroundInfo : int
{
	None = -1,
	Normal = 0,
	Abode = 1,
};

enum class CountryList : uint32_t
{
	None = 0,
	Country1 = 1,
	Country2 = 2,
	Country3 = 3,
	Country4 = 4,
	Country5 = 5,
	Country6 = 6,
	Country7 = 7,
	Country8 = 8,

	_COUNT
};

enum class PrayerIconInfo : int
{
	None = -1,
	Normal = 0,
};

enum class PrayerSiteInfo
{
	Normal = 0,
};

enum class LeaderAnimation
{
	Normal = 0,
	Fast = 1,
	Frantic = 2,
};

enum class SpellCastType
{
	SpellCastInHand = 0,
	SpellCastHandGesture = 1,
	SpellCastHandPosition = 2,
};

enum class JobActivity : uint32_t
{
	None = 0,
	Nurture = 1,
	Harvest = 2,
	Plan = 3,
	Execute = 4,
};

enum class JobInfo : uint32_t
{
	PoorFisherman = 0,
	NormalFisherman = 1,
	GoodFisherman = 2,
	PoorForester = 3,
	NormalForester = 4,
	GoodForester = 5,
	PoorFarmer = 6,
	NormalFarmer = 7,
	GoodFarmer = 8,
	PoorShepherd = 9,
	NormalShepherd = 10,
	GoodShepherd = 11,
	NormalLeader = 12,
	NormalHousewife = 13,
	NormalTrader = 14,
	NormalNoJob = 15,

	_COUNT
};

enum class EffectInfo : int
{
	None = -1,
	Hit = 0,
	Arrow = 1,
	Burn = 2,
	Crush = 3,
	WaterPail = 4,
	Build = 5,
	Chop = 6,
	Dig = 7,
	CitadelHit = 8,
	Nurture = 9,
	FieldCropGrow = 10,
	WeatherLightning = 11,
};

enum class SexType : int
{
	None = -1,
	Male = 0,
	Female = 1,
};

enum class Season
{
	Spring = 0,
	Summer = 1,
	Autumn = 2,
	Winter = 3,
};

enum class ShowNeedsInfo
{
	Hunger = 0,
	Life = 1,
	Hiding = 2,
	ShowWorshipNeedsInfoLast = 3,
	WorkshopWood = 3,
};

enum class TownHappinessConsideration
{
	AmountOfFood = 0,
	AmountOfWood = 1,
	NumberOfBuildings = 2,
	NumberOfPeople = 3,
};

enum class TownDesireInfo : int
{
	None = -1,
	ForFood = 0,
	ForWood = 1,
	ForPlaytime = 2,
	ForProtection = 3,
	ForMercy = 4,
	ForAbodes = 5,
	ForCivicBuilding = 6,
	ForSupplyWorship = 7,
	ForChildren = 8,
	ToBuild = 9,
	ForRain = 10,
	ForSun = 11,
	ToRepair = 12,
	ToSupplyWorkshop = 13,
	ToBuildWonder = 14,
	ForRelaxation = 15,
	ForSleep = 16,
};

enum class WorshipSiteUpgradeInfo
{
	FastFood = 0,
};

enum class VillagerInfo
{
	None = -1,
	CelticHousewifeFemale = 0,
	CelticForesterMale = 1,
	CelticFishermanMale = 2,
	CelticFarmerMale = 3,
	CelticShepherdMale = 4,
	CelticLeaderMale = 5,
	CelticTraderMale = 6,
	AfricanHousewifeFemale = 7,
	AfricanForesterMale = 8,
	AfricanFishermanMale = 9,
	AfricanFarmerMale = 10,
	AfricanShepherdMale = 11,
	AfricanLeaderMale = 12,
	AfricanTraderMale = 13,
	AztecHousewifeFemale = 14,
	AztecForesterMale = 15,
	AztecFishermanMale = 16,
	AztecFarmerMale = 17,
	AztecShepherdMale = 18,
	AztecLeaderMale = 19,
	AztecTraderMale = 20,
	JapaneseHousewifeFemale = 21,
	JapaneseForesterMale = 22,
	JapaneseFishermanMale = 23,
	JapaneseFarmerMale = 24,
	JapaneseShepherdMale = 25,
	JapaneseLeaderMale = 26,
	JapaneseTraderMale = 27,
	IndianHousewifeFemale = 28,
	IndianForesterMale = 29,
	IndianFishermanMale = 30,
	IndianFarmerMale = 31,
	IndianShepherdMale = 32,
	IndianLeaderMale = 33,
	IndianTraderMale = 34,
	EgyptianHousewifeFemale = 35,
	EgyptianForesterMale = 36,
	EgyptianFishermanMale = 37,
	EgyptianFarmerMale = 38,
	EgyptianShepherdMale = 39,
	EgyptianLeaderMale = 40,
	EgyptianTraderMale = 41,
	GreekHousewifeFemale = 42,
	GreekForesterMale = 43,
	GreekFishermanMale = 44,
	GreekFarmerMale = 45,
	GreekShepherdMale = 46,
	GreekLeaderMale = 47,
	GreekTraderMale = 48,
	NorseHousewifeFemale = 49,
	NorseForesterMale = 50,
	NorseFishermanMale = 51,
	NorseFarmerMale = 52,
	NorseShepherdMale = 53,
	NorseLeaderMale = 54,
	NorseTraderMale = 55,
	TibetanHousewifeFemale = 56,
	TibetanForesterMale = 57,
	TibetanFishermanMale = 58,
	TibetanFarmerMale = 59,
	TibetanShepherdMale = 60,
	TibetanLeaderMale = 61,
	TibetanTraderMale = 62,
	PiedPiper = 63,
	Shaolin = 64,
	IdolBuilder = 65,
	Hermit = 66,
	Hippy = 67,
	Priest = 68,
	Priestess = 69,
	Marauder = 70,
	Footballer_1 = 71,
	Footballer_2 = 72,
	Engineer = 73,
	Shepered = 74,
	Nomade = 75,
	AztecLeader = 76,
	CreatureTrainer = 77,
	NorseSailor = 78,
	Breeder = 79,
	Healer = 80,
	Sculptor = 81,
	Crusader = 82,
	SailorAccordian = 83,

	_COUNT
	// MuleHead                = 84, // CREATUREISLE
	// EgyptianBigEyes         = 85, // CREATUREISLE
	// Giant                   = 86, // CREATUREISLE
};

enum class VillagerBasicInfo : int
{
	None = -1,
	HousewifeFemale = 0,
	ForesterMale = 1,
	FishermanMale = 2,
	FarmerMale = 3,
	ShepherdMale = 4,
	LeaderMale = 5,
	TraderMale = 6,
};

enum class SpotVisualType
{
	None = 0,
	ApplySpellEffect = 1,
	GripLandscape = 2,
	SpellSucceedCast = 3,
	SpellFailCast = 4,
	SingleFirework = 5,
	TownFireworks = 6,
	TownFireworksPu1 = 7,
	TownFireworksPu2 = 8,
	MagicObjectCreated = 9,
	CommandSucceed = 10,
	CommandFail = 11,
	CreatureTarget = 12,
	CreatureCastVisual = 13,
	VillagerTeleport = 14,
	FireFx = 15,
	FireFxOnObject = 16,
	MagicFx = 17,
	MagicFxOnObject = 18,
	MagicFxOnCitadel = 19,
	MagicBeam = 20,
	MagicBeamOnCitadel = 21,
	Steam = 22,
	Smoke = 23,
	Dust = 24,
	Bonfire = 25,
	EvilSmoke = 26,
	ObjectAppear = 27,
	ObjectDisappear = 28,
	Bang = 29,
	SingStonesGlow = 30,
	PlayerIconFountain = 31,
	ExplosionCitadel = 32,
	HealFx = 33,
	HighlightOnObject = 34,
	LightningSingleStrike = 35,
	BeamExplosionFx = 36,
	Butterflies = 37,
	ButterfliesOnObject = 38,
	Flies = 39,
	FliesOnObject = 40,
	MagicBeamCreatureSwap = 41,
	Flash = 42,
	TickerTape = 43,
	ForestCreated = 44,
	SingingStonesHeal = 45,
	PilefoodSpeedup = 46,
	SeeThisBeam = 47,
	SeeThisBeam2 = 48,
	Test = 49,
};

enum class WallSectionType
{
	None = -1,
	Basic = 0,
};

enum class CastRuleType
{
	Anywhere = 0,
	OnLand = 1,
	InInfluence = 2,
	OnLandInInfluence = 3,
};

enum class VillagerNumber : uint32_t
{
	Housewife = 0,
	Forester = 1,
	Fisherman = 2,
	Farmer = 3,
	Shepherd = 4,
	Leader = 5,
	Trader = 6,

	_COUNT
};
static constexpr std::array<std::string_view, static_cast<uint32_t>(VillagerNumber::_COUNT)> k_VillagerNumberStrs = {
    "HOUSEWIFE", //
    "FORESTER",  //
    "FISHERMAN", //
    "FARMER",    //
    "SHEPHERD",  //
    "LEADER",    //
    "TRADER",    //
};

enum class CarriedObject
{
	NoChange = 0,
	None = 1,
	Axe = 2,
	FishingRod = 3,
	Crook = 4,
	Saw = 5,
	Bag = 6,
	Ball = 7,
	Hammer = 8,
	MalletHeavy = 9,
	Scythe = 10,
	Spade = 11,
	Wood = 12,
	Tree_1 = 13,
	Tree_2 = 14,
	Tree_3 = 15,
};

enum class DiscreteAlignmentValues
{
	Devilish = 0,
	Evil = 1,
	Bad = 2,
	Neutral = 3,
	Nice = 4,
	Good = 5,
	Angelic = 6,
};

enum class HelpSpiritType
{
	None = 0,
	Good = 1,
	Evil = 2,
};

enum class DetailLevel
{
	High = 0,
	Normal = 1,
	Low = 2,
};

enum class MobileObjectInfo : int32_t
{
	None = -1,
	EgyptBarrel = 0,
	EgyptCart = 1,
	EgyptPotA = 2,
	EgyptPotB = 3,
	MagicFood = 4,
	LumpOfPoo = 5,
	WaterJug = 6,
	Arrow = 7,
	Ball = 8,
	Cart = 9,
	FoodPot = 10,
	WoodPot = 11,
	PileFood = 12,
	PileWood = 13,
	MagicWood = 14,
	Crop = 15,
	OldScaffold = 16,
	Champi = 17,
	MagicMushroom = 18,
	Toadstool = 19,
	CreatureSwapOrb = 20,
	Creed = 21,
	Eyes = 22,
	Ark = 23,
	Whale = 24,
	OneOffSpellSeed = 25,
	HanoiPuzzleBase = 26,
	HanoiPuzzlePart1 = 27,
	HanoiPuzzlePart2 = 28,
	HanoiPuzzlePart3 = 29,
	HanoiPuzzlePart4 = 30,
	Cauldron = 31,

	_COUNT
	// MobileObjectDolphin           = 32, // CREATUREISLE
	// MobileObjectCroc_1            = 33, // CREATUREISLE
	// MobileObjectCroc_2            = 34, // CREATUREISLE
	// MobileObjectCroc_3            = 35, // CREATUREISLE
	// MobileObjectBrotherhoodRing_1 = 36, // CREATUREISLE
	// MobileObjectBrotherhoodRing_2 = 37, // CREATUREISLE
	// MobileObjectBrotherhoodRing_3 = 38, // CREATUREISLE
	// MobileObjectMosesBasket       = 39, // CREATUREISLE
	// MobileObjectBrotherhoodPuck   = 40, // CREATUREISLE
};

enum class SpeedThreshold
{
	VillagerNormal = 0,
	VillagerMan = 1,
	AnimalCow = 2,
	AnimalHorse = 3,
	AnimalSheep = 4,
	AnimalPig = 5,
	AnimalLion = 6,
	AnimalTiger = 7,
	AnimalLeopard = 8,
	AnimalWolf = 9,
};

enum class SpeedMax
{
	Walk = 0,
	Run = 1,
};

enum class SpeedId
{
	Default = 0,
	Fleeing = 1,
	BriskWalk = 2,
	Dancing = 3,
	Stalking = 2,
	Attack = 3,
	Hunting = 4,
	Wander = 4,
	Running = 5,
};

enum class DanceInfo
{
	None = -1,
	Worship = 0,
	Creature = 1,
	Meeting = 2,
	Housewife = 3,
	BuildingSmall = 4,
	BuildingLarge = 5,
	DanceAroundPerson = 6,
	DancePrayInwards = 7,
	DanceTownCelebration = 8,
	CreatureTellingStory = 9,
	CreaturePlayingGameWithVillagers = 10,
	CreatureDanceWithVillagersWatching = 11,
	CreatureDanceToImpressVillagers = 12,
	CreatureDanceAmorous = 13,
	NewDanceAroundPerson = 14,
	DanceAroundObjectDiameterLess_7 = 15,
	DanceAroundObjectDiameterLess_10 = 16,
	DanceAroundObjectDiameterLess_20 = 17,
	DanceAroundObjectDiameterGreater_20 = 18,
	CitadelDance_1 = 19,
	CitadelDance_2 = 20,
	CitadelDance_3 = 21,
	CitadelDance_4 = 22,
	CitadelDance_5 = 23,
	CitadelDance_6 = 24,
};

enum class VillagerAvailable
{
	Free = 1,
	AtHome = 3,
	NotFree = 4,
};

enum class VillagerMustBe
{
	DontCare = 0,
	AtHome = 1,
	NotAtHome = 2,
};

enum class VillagerMakeDinner
{
	Yes = 0,
	Making = 1,
	No = 2,
	NotYet = 3,
};

enum class PowerUpType
{
	One = 0,
	Two = 1,
	Three = 2,
};

enum class BigForestInfo : int32_t
{
	None = -1,
	Info1 = 0,
	Info2 = 1,
	Info3 = 2,
	Info4 = 3,

	_COUNT
};

enum class FurnitureInfo
{
	None = -1,
	Axe = 0,
	Bag = 1,
	Ball = 2,
	Bench = 3,
	Bucket = 4,
	Bucket02 = 5,
	Chest = 6,
	ChestTop = 7,
	Crook = 8,
	FishingRod = 9,
	Frame = 10,
	FrameSkin = 11,
	Hammer = 12,
	MalletHeavy = 13,
	Saw = 14,
	Scythe = 15,
	Spade = 16,
	Stool = 17,
	Stool01 = 18,
	Table = 19,
	Trough = 20,
	WashingLineAmcn = 21,
	WashingLineAztc = 22,
	WashingLineCelt = 23,
	WashingLineEgpt = 24,
	WashingLineGrek = 25,
	WashingLineJapn = 26,
	WashingLineNors = 27,
	WashingLineTibt = 28,
	Well = 29,
	WellCovered = 30,
};

enum class FishFarmInfo
{
	None = -1,
	Normal = 0,
};

enum class WeatherInfo
{
	None = -1,
	Monsoon = 0,
	Rain = 1,
	Drizzle = 2,
	Sleet = 3,
	SnowNoSettle = 4,
	Snow = 5,
	Blizzard = 6,
};

enum class ClimateInfo
{
	None = -1,
	WorldDefault = 0,
	Desert = 1,
	Polar = 2,
	Jungle = 3,
	GeneralSnow = 4,
	ColdRain = 5,
	Internet = 6,
};

enum class RewardObjectInfo
{
	None = 0,
	HealPu1 = 1,
	HealPu2 = 2,
	SmallFood = 3,
	MediumFood = 4,
	LargeFood = 5,
	SmallWood = 6,
	MediumWood = 7,
	LargeWood = 8,
	SpellSeedFireball = 9,
	SpellFireballPu1 = 10,
	SpellFireballPu2 = 11,
	SpellSeedBolt = 12,
	SpellBoltPu1 = 13,
	SpellBoltPu2 = 14,
	SpellSeedExplosion = 15,
	SpellExplosionPu1 = 16,
	SpellSeedHeal = 17,
	SpellHealPu1 = 18,
	SpellSeedTeleport = 19,
	SpellSeedFood = 20,
	SpellSeedStorm = 21,
	SpellStormPu1 = 22,
	SpellStormPu2 = 23,
	SpellStormPu3 = 24,
	SpellSeedShield = 25,
	SpellShieldPu1 = 26,
	SpellSeedPhysicalShield = 27,
	SpellSeedWood = 28,
	SpellSeedSkeleton = 29,
	SpellSkeletonPu1 = 30,
	SpellSeedCreatureFreeze = 31,
	SpellSeedCreatureSmall = 32,
	SpellSeedCreatureBig = 33,
	SpellSeedCreatureWeak = 34,
	SpellSeedCreatureStrong = 35,
	SpellSeedCreatureFat = 36,
	SpellSeedCreatureThin = 37,
	SpellSeedCreatureInvisible = 38,
	SpellSeedCreatureCompassion = 39,
	SpellSeedCreatureAngry = 40,
	SpellSeedCreatureHungry = 41,
	SpellSeedCreatureFrightene = 42,
	SpellSeedCreatureTried = 43,
	SpellSeedCreatureIll = 44,
	SpellSeedCreatureThirsty = 45,
	SpellSeedCreatureItchy = 46,
	SmallBelief = 47,
	MediumBelief = 48,
	LargeBelief = 49,
	Increment = 50,
	SingingStone = 51,
	Idol = 52,
	WeepingStone = 53,
	Scaffold = 54,
	SpellSeedWater = 55,
	SpellSeedFlockFlying = 56,
	SpellSeedFlockGround = 57,
	RewardObjectToyBall = 58,
	RewardObjectToyCuddly = 59,
	RewardObjectToyDie = 60,
};

enum class RewardProgressGood
{
	RewardInfoProgressGood_1 = 0,
	RewardInfoProgressGood_2 = 1,
	RewardInfoProgressGood_3 = 2,
	RewardInfoProgressGood_4 = 3,
	RewardInfoProgressGood_5 = 4,
	RewardInfoProgressGood_6 = 5,
	RewardInfoProgressGood_7 = 6,
	RewardInfoProgressGood_8 = 7,
	RewardInfoProgressGood_9 = 8,
	RewardInfoProgressGood_10 = 9,
	RewardInfoProgressGood_11 = 10,
	RewardInfoProgressGood_12 = 11,
	RewardInfoProgressGood_13 = 12,
	RewardInfoProgressGood_14 = 13,
	RewardInfoProgressGood_15 = 14,
	RewardInfoProgressGood_16 = 15,
	RewardInfoProgressGood_17 = 16,
	RewardInfoProgressGood_18 = 17,
	RewardInfoProgressGood_19 = 18,
	RewardInfoProgressGood_20 = 19,
	RewardInfoProgressGood_21 = 20,
	RewardInfoProgressGood_22 = 21,
	RewardInfoProgressGood_23 = 22,
	RewardInfoProgressGood_24 = 23,
	RewardInfoProgressGood_25 = 24,
	RewardInfoProgressGood_26 = 25,
	RewardInfoProgressGood_27 = 26,
	RewardInfoProgressGood_28 = 27,
	RewardInfoProgressGood_29 = 28,
	RewardInfoProgressGood_30 = 29,
};

enum class RewardProgressEvil
{
	RewardInfoProgressEvil_1 = 0,
	RewardInfoProgressEvil_2 = 1,
	RewardInfoProgressEvil_3 = 2,
	RewardInfoProgressEvil_4 = 3,
	RewardInfoProgressEvil_5 = 4,
	RewardInfoProgressEvil_6 = 5,
	RewardInfoProgressEvil_7 = 6,
	RewardInfoProgressEvil_8 = 7,
	RewardInfoProgressEvil_9 = 8,
	RewardInfoProgressEvil_10 = 9,
	RewardInfoProgressEvil_11 = 10,
	RewardInfoProgressEvil_12 = 11,
	RewardInfoProgressEvil_13 = 12,
	RewardInfoProgressEvil_14 = 13,
	RewardInfoProgressEvil_15 = 14,
	RewardInfoProgressEvil_16 = 15,
	RewardInfoProgressEvil_17 = 16,
	RewardInfoProgressEvil_18 = 17,
	RewardInfoProgressEvil_19 = 18,
	RewardInfoProgressEvil_20 = 19,
	RewardInfoProgressEvil_21 = 20,
	RewardInfoProgressEvil_22 = 21,
	RewardInfoProgressEvil_23 = 22,
	RewardInfoProgressEvil_24 = 23,
	RewardInfoProgressEvil_25 = 24,
	RewardInfoProgressEvil_26 = 25,
	RewardInfoProgressEvil_27 = 26,
	RewardInfoProgressEvil_28 = 27,
	RewardInfoProgressEvil_29 = 28,
	RewardInfoProgressEvil_30 = 29,
};

enum class CreatureActionLearningType
{
	Normal = 0,
	Magic = 1,
	None = 2,
};

enum class CreatureActionKnownAbout
{
	Build = 0,
	UseField = 1,
	UseTotem = 2,
	UseStoragePit = 3,
	Fish = 4,
	Dance = 5,
	None = 6,
};

enum class LandedTypes
{
	Standing = 0,
	OnFront = 1,
	OnBack = 2,
	OnRightSide = 1,
	OnLeftSide = 2,
	Default = 3,
};

enum class MagicLivingInfo
{
	Priest = 0,
	Skeleton = 1,
};

enum class PlayerInfo
{
	Normal = 0,
};

enum class CreatureDesires : uint32_t
{
	ToImpress = 0,
	Compassion = 1,
	Anger = 2,
	ToPlay = 3,
	Hunger = 4,
	Fear = 5,
	Curiosity = 6,
	ToPoo = 7,
	Tiredness = 8,
	ToIdleAroundWithPlayer = 9,
	Wanderlust = 10,
	ToPuke = 11,
	ToBuildHome = 12,
	ToBringStuffHome = 13,
	ForWater = 14,
	ToRestoreHealth = 15,
	ToBeFriends = 16,
	ToAttractPlayersAttention = 17,
	ToManifestState = 18,
	ToGetWarmer = 19,
	ToGetColder = 20,
	ToScratch = 21,
	ToRunAwayFromPlayer = 22,
	ToRest = 23,
	ToObeyPlayer = 24,
	Illness = 25,
	ToObeyCreature = 26,
	Sadness = 27,
	ToStayNearHome = 28,
	ToTellPlayerWhatYouThinkOfHim = 29,
	ToPlayWithPlayer = 30,
	ToTellCreatureWhatYouThinkOfHim = 31,
	ToEducateFriend = 32,
	ToFollowPlayerDesire = 33,
	ToGetHigh = 34,
	ToHangAroundAtHome = 35,
	MentalIllness = 36,
	MissFriend = 37,
	ToLookAround = 38,
	ToSteal = 39,
	Invalid = 40,
	Any = 41,
	None = 42,
};

enum class SoundCollisionType : int
{
	Default = 0,
	Flesh = 1,
	Bush = 2,
	FirTree = 3,
	EnglishTree = 4,
	BurntTree = 5,
	PalmTree = 6,
	FabricBuilding = 7,
	MudBuilding = 8,
	WoodenBuilding = 9,
	BrickBuilding = 10,
	Grain = 11,
	SolidWood = 12,
	SolidMetal = 13,
	SolidStone = 14,
	BrittleStone = 15,
	FlintStone = 16,
	ChalkStone = 17,
	HollowWood = 18,
	HollowMetal = 19,
	Fungus = 20,
	Cart = 21,
	Muck = 22,
	Pottery = 23,
	Football = 24,
	Furniture = 25,
	Tools = 26,
	Ground = 27,
	PhysicalShield = 28,
	Water = 29,
	SpellBubble = 30,
	Fragment = 31,
	Scaffold = 32,
	// Skittle        = 33, // CREATURE_ISLE
};

enum class SpecialVillagerInfo
{

};

enum class TooltipsType
{
	None = -1,
	Type1 = 0,
	Type2 = 1,
	Type3 = 2,
	Type4 = 3,
	Type5 = 4,
	Type6 = 5,
	Type7 = 6,
	Type8 = 7,
	Type9 = 8,
	Type10 = 9,
	Type11 = 10,
	Type12 = 11,
	Type13 = 12,
	Type14 = 13,
	Type15 = 14,
	Type16 = 15,
	Type17 = 16,
	Type18 = 17,
	Type19 = 18,
	Type20 = 19,
	Type21 = 20,
	Type22 = 21,
	Type23 = 22,
	Type24 = 23,
	Type25 = 24,
	Type26 = 25,
	Type27 = 26,
	Type28 = 27,
	Type29 = 28,
	Type30 = 29,
	Type31 = 30,
	Type32 = 31,
	Type33 = 32,
	Type34 = 33,
	Type35 = 34,
	Type36 = 35,
	Type37 = 36,
	Type38 = 37,
	Type39 = 38,
	Type40 = 39,
	Type41 = 40,
	Type42 = 41,
	Type43 = 42,
	Type44 = 43,
	Type45 = 44,
	Type46 = 45,
	Type47 = 46,
	Type48 = 47,
	Type49 = 48,
	Type50 = 49,
	Type51 = 50,
	Type52 = 51,
	Type53 = 52,
	Type54 = 53,
	Type55 = 54,
	Type56 = 55,
	Type57 = 56,
	Type58 = 57,
	Type59 = 58,
	Type60 = 59,
	Type61 = 60,
	Type62 = 61,
	Type63 = 62,
	Type64 = 63,
	Type65 = 64,
	Type66 = 65,
	Type67 = 66,
	Type68 = 67,
	Type69 = 68,
	Type70 = 69,
	Type71 = 70,
	Type72 = 71,
	Type73 = 72,
	Type74 = 73,
	Type75 = 74,
	Type76 = 75,
	Type77 = 76,
	Type78 = 77,
	Type79 = 78,
	Type80 = 79,
	Type81 = 80,
	Type82 = 81,
	Type83 = 82,
	Type84 = 83,
	Type85 = 84,
	Type86 = 85,
	Type87 = 86,
	Type88 = 87,
	Type89 = 88,
	Type90 = 89,
	Type91 = 90,
	Type92 = 91,
	Type93 = 92,
	Type94 = 93,
	Type95 = 94,
	Type96 = 95,
	Type97 = 96,
	Type98 = 97,
	Type99 = 98,
	Type100 = 99,
	Type101 = 100,
	Type102 = 101,
	Type103 = 102,
	Type104 = 103,
	Type105 = 104,
	Type106 = 105,
	Type107 = 106,
	Type108 = 107,
	Type109 = 108,
	Type110 = 109,
	Type111 = 110,
	Type112 = 111,
	Type113 = 112,
	Type114 = 113,
	Type115 = 114,
	Type116 = 115,
	Type117 = 116,
	Type118 = 117,
	Type119 = 118,
	Type120 = 119,
	Type121 = 120,
	Type122 = 121,
	Type123 = 122,
	Type124 = 123,
	Type125 = 124,
	Type126 = 125,
	Type127 = 126,
	Type128 = 127,
	Type129 = 128,
	Type130 = 129,
	Type131 = 130,
	Type132 = 131,
	Type133 = 132,
	Type134 = 133,
	Type135 = 134,
	Type136 = 135,
	Type137 = 136,
	Type138 = 137,
	Type139 = 138,
	Type140 = 139,
	Type141 = 140,
	Type142 = 141,
	Type143 = 142,
	Type144 = 143,
	Type145 = 144,
	Type146 = 145,
	Type147 = 146,
	Type148 = 147,
	Type149 = 148,
	Type150 = 149,
	Type151 = 150,
	Type152 = 151,
	Type153 = 152,
	Type154 = 153,
	Type155 = 154,
	Type156 = 155,
	Type157 = 156,
	Type158 = 157,
	Type159 = 158,
	Type160 = 159,
	Type161 = 160,
	Type162 = 161,
	Type163 = 162,
	Type164 = 163,
	Type165 = 164,
	Type166 = 165,
	Type167 = 166,
	Type168 = 167,
	Type169 = 168,
	Type170 = 169,
};

enum class CarriedTreeType : uint32_t
{
	None = 0,
	Evergreen = 1,
	Fruit = 2,
	Hardwood = 3,
};

enum class VillagerDisciple
{
	None = 0,
	Farmer = 1,
	Forester = 2,
	Fisherman = 3,
	Builder = 4,
	Breeder = 5,
	Protection = 6,
	Missionary = 7,
	Craftsman = 8,
	Trader = 9,
	ChangeHouse = 10,
	Worship = 11,
	FromVortex = 12,
};

enum class HoldType
{
	None = 0,
	Above = 1,
	Magic = 2,
	Grain = 3,
	Fingers = 4,
	Tree = 5,
	Side = 6,
	Villager = 7,
};

enum class ScaffoldInfo
{
	Normal = 0,
};

enum class HelpSpritesGuidance
{
	TownBeingAttacked = 0,
	CreatureBeingAttacked = 1,
	CreatureAttackingThem = 2,
	LosingVillagers = 3,
	AttackingTown = 4,
	LowOnFood = 5,
	LowOnWood = 6,
	InjuredPeople = 7,
	LowOnPeople = 8,
	VillagersUnhappy = 9,
	LosingBelief = 10,
	OtherVillages = 11,
	CreatureFight = 12,
	GeneralBad = 13,
	GeneralGood = 14,
	KillingPeople = 15,
	GoodBeingEvil = 16,
	EvilBeingGood = 17,
	WorshippersDying = 18,
	VeryGood = 19,
	VeryEvil = 20,
	DestroyBuilding = 21,
};

enum class DykCategory : uint32_t
{
	Navigation = 0,
	Creature = 1,
	VillageLife = 2,
	Miracles = 3,
	Misc = 4,
};

enum class DeathReason : uint32_t
{
	None = 0,
	Starving = 1,
	Spell = 2,
	Animal = 3,
	Chant = 4,
	PlayerInteraction = 5,
	PlayerInteractionDrown = 6,
	Sacrifice = 7,
	Exhaustion = 8,
	OldAge = 9,

	_COUNT = 10
};

enum class ImmersionEffectType : int
{
	None = -1,
	Fur = 0,
	Slap = 1,
	Hit = 2,
	GestureSuccess = 3,
	GripForce = 4,
	Uproot = 5,
	HandHitInfluence = 6,
	LeashPull = 7,
	GestureTrail = 8,
	SpellCrackle = 9,
	MiscHeart = 10,
	SprinkleFood = 11,
	SprinkleWood = 12,
	Burning = 13,
	OneShotSpellPop = 14,
	Heavy = 15,
	Trademark = 16,
	MushroomChallenge = 17,
	FireballL1 = 18,
	FireballL2 = 19,
	FireballL3 = 20,
	LightningBolt = 21,
	AnimatingInHand = 22,
	GeneralSpellCast = 23,
	Shield = 24,
	SprinkleWater = 25,
	FlockFlying = 26,
	FlockOnGround = 27,
	CreatureSpell = 28,
	HandOverSpell = 29,
	HandOverGeneral = 30,
	HandOverStraw = 31,
	HandOverSmooth = 32,
	HandOverWood = 33,
	HandOverCanvas = 34,
	HandOverChallengeScroll = 35,
	HandOverStoryScroll = 36,
	HandOverFoliage = 37,
	HandOverCreed = 38,
	HandOverSingingStones = 39,
	HandOverVortex = 40,
	HandOverCitadel = 41,
	HitExclusion = 42,
	MovingFish = 43,
	CommandSuccess = 44,
	Hanoi = 45,
	PlantObject = 46,
	HandOverTeleport = 47,
	HandOverSpecialCrops = 48,
	// CannonFire              = 49, // CREATUREISLE
};

enum class GuidanceAlignment : uint32_t
{
	None = 0,
	Good = 1,
	Evil = 2,
};

enum class VillagerRoles : uint8_t
{
	NONE,
	HOUSEWIFE,
	FARMER,
	FISHERMAN,
	FORESTER,
	BREEDER,
	SHEPHERD,
	MISSIONARY,
	LEADER, // No idea what a leader is but they are spawned in land 2 and so on
	TRADER, // Exists on one of the MP land scripts
	PiedPiper,
	Shaolin,
	IdolBuilder,
	Hermit,
	Hippy,
	Priest,
	Priestess,
	Marauder,
	Footballer_1,
	Footballer_2,
	Engineer,
	Shepered,
	Nomade,
	AztecLeader,
	CreatureTrainer,
	NorseSailor,
	Breeder,
	Healer,
	Sculptor,
	Crusader,
	SailorAccordian,

	_COUNT
};

static constexpr std::array<std::string_view, static_cast<uint8_t>(VillagerRoles::_COUNT)> k_VillagerRoleStrs = {
    "NONE",            //
    "HOUSEWIFE",       //
    "FARMER",          //
    "FISHERMAN",       //
    "FORESTER",        //
    "BREEDER",         //
    "SHEPHERD",        //
    "MISSIONARY",      //
    "LEADER",          //
    "TRADER",          //
    "PiedPiper",       //
    "Shaolin",         //
    "IdolBuilder",     //
    "Hermit",          //
    "Hippy",           //
    "Priest",          //
    "Priestess",       //
    "Marauder",        //
    "Footballer_1",    //
    "Footballer_2",    //
    "Engineer",        //
    "Shepered",        //
    "Nomade",          //
    "AztecLeader",     //
    "CreatureTrainer", //
    "NorseSailor",     //
    "Breeder",         //
    "Healer",          //
    "Sculptor",        //
    "Crusader",        //
    "SailorAccordian"  //
};

enum class PlayerNames : uint8_t
{
	PLAYER_ONE = 0,
	PLAYER_TWO = 1,
	PLAYER_THREE = 2,
	PLAYER_FOUR = 3,
	PLAYER_FIVE = 4,
	PLAYER_SIX = 5,
	PLAYER_SEVEN = 6,
	NEUTRAL = 7,

	_COUNT
};

static constexpr std::array<std::string_view, static_cast<uint8_t>(PlayerNames::_COUNT)> k_PlayerNamesStrs = {
    "PLAYER_ONE",   //
    "PLAYER_TWO",   //
    "PLAYER_THREE", //
    "PLAYER_FOUR",  //
    "PLAYER_FIVE",  //
    "PLAYER_SIX",   //
    "PLAYER_SEVEN", //
    "NEUTRAL",      //
};

enum class ContainerInfoType : uint32_t
{
	Town = 0,
	Prayer = 1,
	Citadel = 2,
	Forest = 3,
};

enum class CitadelHeartInfoType : uint32_t
{
	Normal = 0,
};

enum class HelpText : int
{
};

enum class FoodType : uint32_t
{
	None = 0,
	Meat = 1 << 0,
	Vegetable = 1 << 1,
	Graze = 1 << 2,
	Any = Meat | Vegetable,
	MeatVegetable = Meat | Vegetable,
	VegetableGraze = Vegetable | Graze,
};

enum class CitadelTypes : uint32_t
{
	Defence = 0,
	Civic = 1,
	Pen = 2,
	Worship = 3,
};

enum class HelpSystemMessageSet : uint32_t
{
};

enum class HelpSystemCondition : uint32_t
{
};

constexpr uint32_t MakeSpeedState(float ms)
{
	constexpr uint32_t ms100 = 0x10000;
	constexpr float ms1 = static_cast<float>(ms100) * 0.01f;
	float scaled = ms * ms1;
	// Speed needs to be positive. Round cast to int won't work in negative.
	// NOLINTNEXTLINE(bugprone-incorrect-roundings): can't use lroundf in constexpr
	return static_cast<uint32_t>(scaled + 0.5f);
}

enum class SpeedState : uint32_t
{
	MetresSec_0 = MakeSpeedState(0.0f),
	MetresSec_0_1 = MakeSpeedState(0.1f),
	MetresSec_0_25 = MakeSpeedState(0.25f),
	MetresSec_0_5 = MakeSpeedState(0.5f),
	MetresSec_0_75 = MakeSpeedState(0.75f),
	MetresSec_1 = MakeSpeedState(1.0f),
	MetresSec_1_25 = MakeSpeedState(1.25f),
	MetresSec_1_5 = MakeSpeedState(1.5f),
	MetresSec_1_75 = MakeSpeedState(1.75f),
	MetresSec_2 = MakeSpeedState(2.0f),
	MetresSec_2_25 = MakeSpeedState(2.25f),
	MetresSec_2_5 = MakeSpeedState(2.5f),
	MetresSec_2_75 = MakeSpeedState(2.75f),
	MetresSec_3 = MakeSpeedState(3.0f),
	MetresSec_4 = MakeSpeedState(4.0f),
	MetresSec_5 = MakeSpeedState(5.0f),
	MetresSec_6 = MakeSpeedState(6.0f),
	MetresSec_6_5 = MakeSpeedState(6.5f),
	MetresSec_7 = MakeSpeedState(7.0f),
	MetresSec_8 = MakeSpeedState(8.0f),
	MetresSec_9 = MakeSpeedState(9.0f),
	MetresSec_10 = MakeSpeedState(10.0f),
	MetresSec_100 = MakeSpeedState(100.0f),
};

constexpr float GetSpeedStateSpeed(SpeedState state)
{
	constexpr uint32_t ms100 = 0x10000;
	constexpr float ms1 = static_cast<float>(ms100) * 0.01f;
	float scaled = static_cast<uint32_t>(state) - 0.5f;
	return scaled / ms1;
}

constexpr uint32_t MakeTurnAngleState(float degrees)
{
	constexpr uint32_t a360 = 0x800;
	constexpr float a1 = static_cast<float>(a360) / 360.0f;
	float scaled = degrees * a1;
	// Speed needs to be positive. Round cast to int won't work in negative.
	// NOLINTNEXTLINE(bugprone-incorrect-roundings): can't use lroundf in constexpr
	return static_cast<uint32_t>(scaled + 0.5f);
}

enum class TurnangleStates : int
{
	Degrees_360 = MakeTurnAngleState(360.0f / 1.0f),
	Degrees_180 = MakeTurnAngleState(360.0f / 2.0f),
	Degrees_90 = MakeTurnAngleState(360.0f / 4.0f),
	Degrees_45 = MakeTurnAngleState(360.0f / 8.0f),
	Degrees_22_5 = MakeTurnAngleState(360.0f / 16.0f),
	Degrees_11_25 = MakeTurnAngleState(360.0f / 32.0f),
	Degrees_5_625 = MakeTurnAngleState(360.0f / 64.0f),
	Degrees_2_8125 = MakeTurnAngleState(360.0f / 128.0f),
	Degrees_1_40625 = MakeTurnAngleState(360.0f / 256.0f),
	Degrees_6 = MakeTurnAngleState(6.0f),
	Degrees_3 = MakeTurnAngleState(3.0f),
};

enum class CollideType : uint32_t
{
	Clear = 0,
	Water = 1 << 0,
	Land = 1 << 1,
	Field = 1 << 2,
	Fixed = 1 << 3,
	Edge = 1 << 4,
	Tree = 1 << 5,
	Wall = 1 << 6,

	// Collision and Water have same values as Villager and Animal
	Villager = 1,
	Airbourne = 0,
	Animal = 1,
	Creature = 9,
	Ball = 8,
};

enum class TownInfo : uint32_t
{
	RoundVillageSmall = 0,
	RoundVillageMedium = 1,
	RoundVillageLarge = 2,
	SquareVillageSmall = 3,
	SquareVillageMedium = 4,
	SquareVillageLarge = 5,
	MazeVillageSmall = 6,
	MazeVillageMedium = 7,
	MazeVillageLarge = 8,
	CornerVillageSmall = 9,
	CornerVillageMedium = 10,
	CornerVillageLarge = 11,
	OctagonVillageSmall = 12,
	OctagonVillageMedium = 13,
	OctagonVillageLarge = 14,
	SquaresVillageSmall = 15,
	SquaresVillageMedium = 16,
	SquaresVillageLarge = 17,
};

enum class PenInfo : uint32_t
{
	Feed = 0,
	Train = 1,
	Sleep = 2,
	Discipline = 3,
	Pleasure = 4,
	Walkway = 5,
};

enum class AttackInfo : int
{
	None = -1,
	HandToHand = 0,
	Arrow = 1,

	_COUNT = 2
};

enum class CreatureType : int32_t
{
	Unknown = 0,
	Cow = 1,
	Tiger = 2,
	Leopard = 3,
	Wolf = 4,
	Lion = 5,
	Horse = 6,
	Tortoise = 7,
	Zebra = 8,
	BrownBear = 9,
	PolarBear = 10,
	Sheep = 11,
	Chimp = 12,
	Ogre = 13,
	Mandrill = 14,
	Rhino = 15,
	Gorilla = 16,
	GiantApe = 17,

	_COUNT = 18
};

} // namespace openblack

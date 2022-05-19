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
#include <string>

#include <fmt/format.h>

//  626 Meshes.
//  441 Anims.
//  0 with substitution.
//  0 with dummy object.

namespace openblack
{

enum class MeshId : uint32_t
{
	Dummy = 0,
	AnimalBat1 = 1,
	AnimalBat2 = 2,
	AnimalCrow1 = 3,
	AnimalCrow2 = 4,
	AnimalDove1 = 5,
	AnimalDove2 = 6,
	AnimalPigeon1 = 7,
	AnimalPigeon2 = 8,
	AnimalSeagull1 = 9,
	AnimalSeagull2 = 10,
	AnimalSpellDove = 11,
	AnimalSwallow1 = 12,
	AnimalSwallow2 = 13,
	AnimalVulture1 = 14,
	AnimalVulture2 = 15,
	AnimalCow1 = 16,
	AnimalCow2 = 17,
	AnimalCow3 = 18,
	AnimalHorse1 = 19,
	AnimalHorse2 = 20,
	AnimalHorse3 = 21,
	AnimalLeopard1 = 22,
	AnimalLeopard2 = 23,
	AnimalLeopard3 = 24,
	AnimalLion1 = 25,
	AnimalLion2 = 26,
	AnimalLion3 = 27,
	AnimalPig1 = 28,
	AnimalPig2 = 29,
	AnimalPig3 = 30,
	SharkBoned = 31,
	AnimalSheep1 = 32,
	AnimalSheep2 = 33,
	AnimalSheep3 = 34,
	AnimalTiger1 = 35,
	AnimalTiger2 = 36,
	AnimalTiger3 = 37,
	AnimalTortoise1 = 38,
	AnimalTortoise2 = 39,
	AnimalTortoise3 = 40,
	AnimalWolf1 = 41,
	AnimalWolf2 = 42,
	AnimalWolf3 = 43,
	AnimalZebra1 = 44,
	AnimalZebra2 = 45,
	AnimalZebra3 = 46,
	BuildingAmerican1 = 47,
	BuildingAmerican1A = 48,
	BuildingAmerican2 = 49,
	BuildingAmerican2A = 50,
	BuildingAmerican3 = 51,
	BuildingAmerican3A = 52,
	BuildingAmerican4 = 53,
	BuildingAmerican5 = 54,
	BuildingAmericanCreche = 55,
	BuildingAmericanFence = 56,
	BuildingAmericanGraveyard = 57,
	BuildingAmericanTotem = 58,
	BuildingAmericanWonder = 59,
	BuildingAmericanWorkshop = 60,
	BuildingAztec1 = 61,
	BuildingAztec2 = 62,
	BuildingAztec3 = 63,
	BuildingAztec4 = 64,
	BuildingAztec5 = 65,
	BuildingAztecCreche = 66,
	BuildingAztecGraveyard = 67,
	BuildingAztecOlmecHead = 68,
	BuildingAztecStatue = 69,
	BuildingAztecTemple = 70,
	BuildingAztecTotem = 71,
	BuildingAztecWonder = 72,
	BuildingAztecWorkshop = 73,
	BuildingCeltic1 = 74,
	BuildingCeltic2 = 75,
	BuildingCeltic3 = 76,
	BuildingCeltic4 = 77,
	BuildingCeltic5 = 78,
	BuildingCeltic5A = 79,
	BuildingCelticCreche = 80,
	BuildingCelticFenceShort = 81,
	BuildingCelticFenceTall = 82,
	BuildingCelticGraveyard = 83,
	BuildingCelticGrave01 = 84,
	BuildingCelticGrave02 = 85,
	BuildingCelticGrave03 = 86,
	BuildingCelticTotem = 87,
	BuildingCelticWallGateSpell = 88,
	BuildingCelticWallTowerSpell = 89,
	BuildingCelticWallSpell = 90,
	BuildingCelticWonder = 91,
	BuildingCelticWorkshop = 92,
	BuildingCitadelAmericanAltar = 93,
	BuildingCitadelAztecAltar = 94,
	BuildingCitadelCelticAltar = 95,
	BuildingCitadelDesireHungry = 96,
	BuildingCitadelDesireSleep = 97,
	BuildingCitadelEgyptianAltar = 98,
	BuildingCitadelGreekAltar = 99,
	BuildingCitadelJapaneseAltar = 100,
	BuildingCitadelNorseAltar = 101,
	BuildingCitadelPopCounterBead = 102,
	BuildingCitadelTibetanAltar = 103,
	BuildingEgyptian1 = 104,
	BuildingEgyptian1A = 105,
	BuildingEgyptian2 = 106,
	BuildingEgyptian2A = 107,
	BuildingEgyptian3 = 108,
	BuildingEgyptian4 = 109,
	BuildingEgyptian5 = 110,
	BuildingEgyptian5A = 111,
	BuildingEgyptianBarrel = 112,
	BuildingEgyptianCart = 113,
	BuildingEgyptianCreche = 114,
	BuildingEgyptianGraveyard = 115,
	BuildingEgyptianNeedle = 116,
	BuildingEgyptianPotA = 117,
	BuildingEgyptianPotB = 118,
	BuildingEgyptianTotem = 119,
	BuildingEgyptianWell = 120,
	BuildingEgyptianWindmill = 121,
	BuildingEgyptianWonder = 122,
	BuildingEgyptianWorkshop = 123,
	BuildingGreek1 = 124,
	BuildingGreek2 = 125,
	BuildingGreek2A = 126,
	BuildingGreek3 = 127,
	BuildingGreek4 = 128,
	BuildingGreek5 = 129,
	BuildingGreek5A = 130,
	BuildingGreekCreche = 131,
	BuildingGreekGraveyard = 132,
	BuildingGreekSail = 133,
	BuildingGreekStatue = 134,
	BuildingGreekTotem = 135,
	BuildingGreekWonder = 136,
	BuildingGreekWorkshop = 137,
	BuildingJapanese1 = 138,
	BuildingJapanese2 = 139,
	BuildingJapanese3 = 140,
	BuildingJapanese4 = 141,
	BuildingJapanese5 = 142,
	BuildingJapaneseCreche = 143,
	BuildingJapaneseGraveyard = 144,
	BuildingJapaneseTotem = 145,
	BuildingJapaneseWonder = 146,
	BuildingJapaneseWorkshop = 147,
	BuildingCampfire = 148,
	BuildingCreaturePenLesahPole = 149,
	BuildingFootballCentreCircle = 150,
	BuildingFootballCornerFlag = 151,
	BuildingFootballGoal = 152,
	BuildingFootballPitch = 153,
	BuildingFootballPitchBig = 154,
	BuildingMineEntrance = 155,
	BuildingPier = 156,
	BuildingScaffold01 = 157,
	BuildingScaffold02 = 158,
	BuildingScaffold03 = 159,
	BuildingXandoBase = 160,
	BuildingXandoO = 161,
	BuildingXandoX = 162,
	PiperEntrancePhys1 = 163,
	BuildingAmericanVillageCentre = 164,
	BuildingAztecVillageCentre = 165,
	BuildingCelticVillageCentre = 166,
	BuildingDesireFlagABODE = 167,
	BuildingDesireFlagCHILDREN = 168,
	BuildingDesireFlagCIVIC_BUILDING = 169,
	BuildingDesireFlagFOOD = 170,
	BuildingDesireFlagPLAYTIME = 171,
	BuildingDesireFlagPROTECTION = 172,
	BuildingDesireFlagRAIN = 173,
	BuildingDesireFlagSUN = 174,
	BuildingDesireFlagWood = 175,
	BuildingEgyptianVillageCentre = 176,
	BuildingGreekVillageCentre = 177,
	BuildingJapaneseVillageCentre = 178,
	BuildingNorseVillageCentre = 179,
	BuildingPlayerIcon01 = 180,
	BuildingPlayerIcon01Plinth = 181,
	BuildingPlayerIconApe = 182,
	BuildingPlayerIconBear = 183,
	BuildingPlayerIconCow = 184,
	BuildingPlayerIconHorse = 185,
	BuildingPlayerIconLeopard = 186,
	BuildingPlayerIconLion = 187,
	BuildingPlayerIconPlinthAmerican = 188,
	BuildingPlayerIconPlinthAztec = 189,
	BuildingPlayerIconPlinthCeltic = 190,
	BuildingPlayerIconPlinthEgyptian = 191,
	BuildingPlayerIconPlinthGreek = 192,
	BuildingPlayerIconPlinthJapanese = 193,
	BuildingPlayerIconPlinthNorse = 194,
	BuildingPlayerIconPlinthTibetan = 195,
	BuildingPlayerIconPolarBear = 196,
	BuildingPlayerIconSheep = 197,
	BuildingPlayerIconTiger = 198,
	BuildingPlayerIconTortoise = 199,
	BuildingPlayerIconWolf = 200,
	BuildingPlayerIconZebra = 201,
	BuildingTibetanVillageCentre = 202,
	BuildingVillageCentreSpellHand = 203,
	BuildingNorse1 = 204,
	BuildingNorse2 = 205,
	BuildingNorse2A = 206,
	BuildingNorse3 = 207,
	BuildingNorse3A = 208,
	BuildingNorse4 = 209,
	BuildingNorse5 = 210,
	BuildingNorseCreche = 211,
	BuildingNorseGate = 212,
	BuildingNorseGraveyard = 213,
	BuildingNorsePigPen = 214,
	BuildingNorseTemple = 215,
	BuildingNorseTotem = 216,
	BuildingNorseWonder = 217,
	BuildingNorseWorkshop = 218,
	NorseGatePhys1 = 219,
	NorseGatePhys2 = 220,
	BuildingAmericanStoragePit = 221,
	BuildingAztecStoragePit = 222,
	BuildingCelticStoragePit = 223,
	BuildingEgyptianStoragePit = 224,
	BuildingGrain = 225,
	BuildingGreekStoragePit = 226,
	BuildingJapaneseStoragePit = 227,
	BuildingNorseStoragePit = 228,
	BuildingTibetanStoragePit = 229,
	BuildingWood00 = 230,
	BuildingWood01 = 231,
	BuildingWood02 = 232,
	BuildingWood03 = 233,
	BuildingWood04 = 234,
	BuildingWoodOutside = 235,
	BuildingTibetan1 = 236,
	BuildingTibetan2 = 237,
	BuildingTibetan3 = 238,
	BuildingTibetan3A = 239,
	BuildingTibetan4 = 240,
	BuildingTibetan5 = 241,
	BuildingTibetan5A = 242,
	BuildingTibetanCreche = 243,
	BuildingTibetanGraveyard = 244,
	BuildingTibetanLargePillar = 245,
	BuildingTibetanMiniTemple = 246,
	BuildingTibetanTotem = 247,
	BuildingTibetanWonder = 248,
	BuildingTibetanWorkshop = 249,
	BuildingSpellHand = 250,
	BuildingWorshipGrain = 251,
	BuildingWorshipSite01Neutral = 252,
	Boulder1Chalk = 253,
	Boulder1Lime = 254,
	Boulder1Sand = 255,
	Boulder1Volcanic = 256,
	Boulder2Chalk = 257,
	Boulder2Lime = 258,
	Boulder2Sand = 259,
	Boulder2Volcanic = 260,
	Boulder3Chalk = 261,
	Boulder3Lime = 262,
	Boulder3Sand = 263,
	Boulder3Volcanic = 264,
	FatPilarChalk = 265,
	FatPilarLime = 266,
	FatPilarSand = 267,
	FatPilarVolcanic = 268,
	FeatureCreatureCave = 269,
	FeatureForest = 270,
	FeatureForest01 = 271,
	FeatureForest02 = 272,
	FeatureForest03 = 273,
	FeatureForestBook01 = 274,
	FeatureGiantSkeleton = 275,
	FeatureGiantSkull = 276,
	FeatureIdol = 277,
	FeaturePiperCave = 278,
	FeatureRib1 = 279,
	FeatureRib2 = 280,
	FeatureSpoutHole = 281,
	FeatureVertabrae = 282,
	Pilar2Chalk = 283,
	Pilar2Lime = 284,
	Pilar2Sand = 285,
	Pilar2Volcanic = 286,
	Pilar3Chalk = 287,
	Pilar3Lime = 288,
	Pilar3Sand = 289,
	Pilar3Volcanic = 290,
	PilarChalk = 291,
	PilarLime = 292,
	PilarSand = 293,
	PilarVolcanic = 294,
	Puzzle1 = 295,
	Puzzle2 = 296,
	Puzzle3 = 297,
	Puzzle4 = 298,
	Puzzle5 = 299,
	SpikeyPilarChalk = 300,
	SpikeyPilarLime = 301,
	SpikeyPilarSand = 302,
	SpikeyPilarVolcanic = 303,
	Z_PhoneBox = 304,
	I_Blast = 305,
	I_Fireball = 306,
	I_Food = 307,
	I_Football = 308,
	I_Forest = 309,
	I_Heal = 310,
	I_Lightning = 311,
	I_Lightning2 = 312,
	I_Miniscroll = 313,
	I_MiniscrollActive = 314,
	I_MiniscrollBronze = 315,
	I_MiniscrollBronzeActive = 316,
	I_MiniscrollGold = 317,
	I_MiniscrollGoldActive = 318,
	I_MiniscrollSilver = 319,
	I_MiniscrollSilverActive = 320,
	I_OfferingBedroll = 321,
	I_OfferingFertility = 322,
	I_OfferingFood = 323,
	I_OfferingPlay = 324,
	I_OfferingProtection = 325,
	I_OfferingSkull = 326,
	I_OfferingVillageImprove = 327,
	I_OfferingWater = 328,
	I_OfferingWood = 329,
	I_Priest = 330,
	I_Shield = 331,
	I_Skeleton = 332,
	I_SpellGem = 333,
	I_Trees = 334,
	I_Wall = 335,
	GateTotemPlinthePhys1 = 336,
	GateTotemPlinthePhys2 = 337,
	GateTotemPlinthePhys3 = 338,
	ObjectArk = 339,
	ObjectArkDryDock = 340,
	ObjectArkWreck = 341,
	ObjectAxe = 342,
	ObjectBag = 343,
	ObjectBall = 344,
	ObjectBellTower = 345,
	ObjectBoxFrame = 346,
	ObjectBranchEvergreenInHand = 347,
	ObjectBranchFruitInHand = 348,
	ObjectBranchHardWoodInHand = 349,
	ObjectBurntTree = 350,
	ObjectCauldron = 351,
	ObjectChampi = 352,
	ObjectCreatureTurd = 353,
	ObjectCrook = 354,
	ObjectFishingRod = 355,
	ObjectFlatRockChalk = 356,
	ObjectFlatRockLimeStone = 357,
	ObjectFlatRockSandStone = 358,
	ObjectFlatRockVolcanic = 359,
	ObjectFlowerBed1 = 360,
	ObjectFlowerBed2 = 361,
	ObjectGateTotemApe = 362,
	ObjectGateTotemBlank = 363,
	ObjectGateTotemCow = 364,
	ObjectGateTotemPlinthe = 365,
	ObjectGateTotemTiger = 366,
	ObjectHammer = 367,
	ObjectInfoSign = 368,
	ObjectJapaneseStoneThing = 369,
	ObjectLargeFishDead = 370,
	ObjectLightBulb = 371,
	ObjectLogsInHand = 372,
	ObjectLongRockChalk = 373,
	ObjectLongRockLimeStone = 374,
	ObjectLongRockSandStone = 375,
	ObjectLongRockVolcanic = 376,
	ObjectMagicMushroom = 377,
	ObjectMalletHeavy = 378,
	ObjectRockChalk = 379,
	ObjectRockLimeStone = 380,
	ObjectRockSandStone = 381,
	ObjectRockVolcanic = 382,
	ObjectSaw = 383,
	ObjectScythe = 384,
	ObjectSharpRockChalk = 385,
	ObjectSharpRockLimeStone = 386,
	ObjectSharpRockSandStone = 387,
	ObjectSharpRockVolcanic = 388,
	ObjectSkullTotem = 389,
	ObjectSpade = 390,
	ObjectSquareRockChalk = 391,
	ObjectSquareRockLimeStone = 392,
	ObjectSquareRockSandStone = 393,
	ObjectSquareRockVolcanic = 394,
	ObjectTickertApe = 395,
	ObjectToadstool = 396,
	ObjectTombStone = 397,
	ObjectTownLight = 398,
	ObjectToyBall = 399,
	ObjectToyBowlingBall = 400,
	ObjectToyCuddly = 401,
	ObjectToyDice = 402,
	ObjectToySkittle = 403,
	ObjectWheatEnclosure = 404,
	ObjectWheatInHand = 405,
	ObjectWoodInHand = 406,
	RewardChestExplode = 407,
	SculptureInProgress = 408,
	SingingStoneCenter = 409,
	SingingStone = 410,
	IdolBuilder = 411,
	NewShaolin = 412,
	PersonAmericanFemaleA1 = 413,
	PersonAmericanFemaleA2 = 414,
	PersonAmericanFemaleA3 = 415,
	PersonAmericanMaleA1 = 416,
	PersonAmericanMaleA2 = 417,
	PersonAmericanMaleA3 = 418,
	PersonAnimalBreeder = 419,
	PersonAnimalTrainer = 420,
	PersonAztecFemaleA1 = 421,
	PersonAztecFemaleA2 = 422,
	PersonAztecFemaleA3 = 423,
	PersonAztecLeader = 424,
	PersonAztecMaleA1 = 425,
	PersonAztecMaleA2 = 426,
	PersonAztecMaleA3 = 427,
	PersonBandit = 428,
	PersonBoyBlack1 = 429,
	PersonBoyBlack2 = 430,
	PersonBoyBlack3 = 431,
	PersonBoyOriental1 = 432,
	PersonBoyOriental2 = 433,
	PersonBoyOriental3 = 434,
	PersonBoyTan1 = 435,
	PersonBoyTan2 = 436,
	PersonBoyTan3 = 437,
	PersonBoyWhite = 438,
	PersonBoyWhite1 = 439,
	PersonBoyWhite2 = 440,
	PersonBoyWhite3 = 441,
	PersonCelticFootballer = 442,
	PersonCelticFemaleA1 = 443,
	PersonCelticFemaleA2 = 444,
	PersonCelticFemaleA3 = 445,
	PersonCelticMaleA1 = 446,
	PersonCelticMaleA2 = 447,
	PersonCelticMaleA3 = 448,
	PersonCrusader = 449,
	PersonCultPriest = 450,
	PersonCultPriestess = 451,
	PersonEgyptianFemaleA1 = 452,
	PersonEgyptianFemaleA2 = 453,
	PersonEgyptianFemaleA3 = 454,
	PersonEgyptianMaleA1 = 455,
	PersonEgyptianMaleA2 = 456,
	PersonEgyptianMaleA3 = 457,
	PersonEgyptianMaleB1 = 458,
	PersonEgyptianMaleB2 = 459,
	PersonEngineer = 460,
	PersonGirlBlack1 = 461,
	PersonGirlBlack2 = 462,
	PersonGirlBlack3 = 463,
	PersonGirlOriental1 = 464,
	PersonGirlOriental2 = 465,
	PersonGirlOriental3 = 466,
	PersonGirlTan1 = 467,
	PersonGirlTan2 = 468,
	PersonGirlTan3 = 469,
	PersonGirlWhite = 470,
	PersonGirlWhite1 = 471,
	PersonGirlWhite2 = 472,
	PersonGirlWhite3 = 473,
	PersonGreekFemaleA1 = 474,
	PersonGreekFemaleA2 = 475,
	PersonGreekFemaleA3 = 476,
	PersonGreekMaleA1 = 477,
	PersonGreekMaleA2 = 478,
	PersonGreekMaleA3 = 479,
	PersonHermit = 480,
	PersonHippy = 481,
	PersonIdolBuilder = 482,
	PersonIntroFemale = 483,
	PersonIntroMale = 484,
	PersonJapaneseFemaleA2 = 485,
	PersonJapaneseFemaleA3 = 486,
	PersonJapaneseMaleA1 = 487,
	PersonJapaneseMaleA2 = 488,
	PersonJapaneseMaleA3 = 489,
	PersonJapnFemaleA1 = 490,
	PersonJapnFemaleA2 = 491,
	PersonJapnFemaleA3 = 492,
	PersonJapnMaleA1 = 493,
	PersonJapnMaleA2 = 494,
	PersonJapnMaleA3 = 495,
	PersonNomad = 496,
	PersonNorseFootballer = 497,
	PersonNorseFemaleA1 = 498,
	PersonNorseFemaleA2 = 499,
	PersonNorseFemaleA3 = 500,
	PersonNorseMaleA1 = 501,
	PersonNorseMaleA2 = 502,
	PersonNorseMaleA3 = 503,
	PersonNorseSailor = 504,
	PersonPiedPiper = 505,
	PersonSailorAccordian = 506,
	PersonSculptor = 507,
	PersonShaolinMonk = 508,
	PersonShepard = 509,
	PersonSkeletonFemale = 510,
	PersonSkeletonMale = 511,
	PersonTibetanFemaleA2 = 512,
	PersonTibetanFemaleA3 = 513,
	PersonTibetanMaleA1 = 514,
	PersonTibetanMaleA2 = 515,
	PersonTibetanMaleA3 = 516,
	PersonTibtFemaleA1 = 517,
	PersonTibtFemaleA2 = 518,
	PersonTibtFemaleA3 = 519,
	PersonTibtMaleA1 = 520,
	PersonTibtMaleA2 = 521,
	PersonTibtMaleA3 = 522,
	PersonTibtMaleB1 = 523,
	PersonTibtMaleB2 = 524,
	Creed = 525,
	SpellBlastCone = 526,
	SpellBlastCentre = 527,
	SpellBlastInner = 528,
	SpellBlastOuter = 529,
	SpellColumn = 530,
	SpellFireballTail = 531,
	SpellHealMesh = 532,
	SpellGrainPile = 533,
	SpellHornOfPlenty = 534,
	SpellPhile = 535,
	SpellPhileAngry = 536,
	SpellPhileBig = 537,
	SpellPhileFat = 538,
	SpellPhileFreeze = 539,
	SpellPhileFrighten = 540,
	SpellPhileHungry = 541,
	SpellPhileIll = 542,
	SpellPhileInvisible = 543,
	SpellPhileItchy = 544,
	SpellPhileLove = 545,
	SpellPhileSmall = 546,
	SpellPhileStrong = 547,
	SpellPhileThin = 548,
	SpellPhileThirsty = 549,
	SpellPhileTired = 550,
	SpellPhileWeak = 551,
	SpellRainCone = 552,
	SpellShieldMagicLowPoly = 553,
	SpellSolidShield = 554,
	SpellSpellBallSurface02 = 555,
	SpellSkullIcon = 556,
	SpellSpellCreator = 557,
	SpellSpellDispenser = 558,
	SpellVortexCylinder = 559,
	SpellZCheatBox = 560,
	SpellPulseIn = 561,
	SpellPulseOut = 562,
	AnimalWorm = 563,
	BuildingDummyCitadel = 564,
	Z_AppleBarrel = 565,
	Z_Arrow = 566,
	Z_SpellRock01 = 567,
	Z_SpellRock02 = 568,
	TreeCopse = 569,
	TreeCopseA = 570,
	TreeBeech = 571,
	TreeBirch = 572,
	TreeBush = 573,
	TreeBushA = 574,
	TreeBushB = 575,
	TreeCedar = 576,
	TreeConifer = 577,
	TreeConiferA = 578,
	TreeCypress = 579,
	TreeCypressA = 580,
	TreeHedge = 581,
	TreeHedgeA = 582,
	TreeOak = 583,
	TreeOakA = 584,
	TreeOlive = 585,
	TreePalm = 586,
	TreePalmA = 587,
	TreePalmB = 588,
	TreePalmC = 589,
	TreePine = 590,
	TreeRice = 591,
	TreeRoots = 592,
	TreeRootsPile = 593,
	TreeWheat = 594,
	TreeWheatInField = 595,
	U_Axe = 596,
	U_Bag = 597,
	U_Ball = 598,
	U_Bench = 599,
	U_Bucket = 600,
	U_Bucket2 = 601,
	U_Cart = 602,
	U_Chest = 603,
	U_ChestTop = 604,
	U_Crook = 605,
	U_FishingRod = 606,
	U_Frame = 607,
	U_FrameSkin = 608,
	U_Hammer = 609,
	U_MalletHeavy = 610,
	U_Saw = 611,
	U_Scythe = 612,
	U_Spade = 613,
	U_Stool = 614,
	U_Stool01 = 615,
	U_Table = 616,
	U_Trough = 617,
	U_WashingLineAmerican = 618,
	U_WashingLineAztec = 619,
	U_WashingLineCeltic = 620,
	U_WashingLineEgyptian = 621,
	U_WashingLineGreek = 622,
	U_WashingLineJapanese = 623,
	U_WashingLineNorse = 624,
	U_WashingLineTibetan = 625,

	_COUNT
};

// just generated, mainly for debug
extern const std::array<std::string, static_cast<uint32_t>(MeshId::_COUNT)> k_MeshNames;

enum class AnimId : int
{
	DontDraw = -4,
	NoMoveAnimSet = -3,
	NoAnimSet = -2,
	Invalid = -1,
	BatEraticglide = 0,
	BatFlap = 1,
	BatGlide = 2,
	CrowFlap = 3,
	CrowGlide = 4,
	CrowLanding = 5,
	CrowSit = 6,
	CrowTakeoff = 7,
	DoveFlap = 8,
	DoveGlide = 9,
	DoveLanding = 10,
	DoveSit = 11,
	DoveTakeoff = 12,
	PigeonFlap = 13,
	PigeonGlide = 14,
	PigeonLanding = 15,
	PigeonSit = 16,
	PigeonTakeoff = 17,
	SeagullFlap = 18,
	SeagullGentleflap = 19,
	SeagullGlide = 20,
	SeagullLanding = 21,
	SeagullTakeoff = 22,
	SpellDoveFlap = 23,
	SpellDoveGlide = 24,
	SwallowCalmglide = 25,
	SwallowEraticglide = 26,
	SwallowFlap = 27,
	ACowDead = 28,
	ACowDeadOnLhs = 29,
	ACowDeadOnRhs = 30,
	ACowDie = 31,
	ACowDownedOnLhs = 32,
	ACowDownedOnRhs = 33,
	ACowDrowning = 34,
	ACowEat_1 = 35,
	ACowEat_2 = 36,
	ACowGotoEat = 37,
	ACowInHand = 38,
	ACowLandedLeftSide = 39,
	ACowLandedRightSide = 40,
	ACowRun = 41,
	ACowStand = 42,
	ACowThrown = 43,
	ACowUpfromEat = 44,
	ACowWalk = 45,
	AHorseDeadlhs = 46,
	AHorseDeadrhs = 47,
	AHorseEat1 = 48,
	AHorseEat2 = 49,
	AHorseFallDeadlhs = 50,
	AHorseFallDeadrhs = 51,
	AHorseGotoEat = 52,
	AHorseInHand = 53,
	AHorseLandGetUplhs = 54,
	AHorseLandGetUprhs = 55,
	AHorseRun = 56,
	AHorseStand = 57,
	AHorseThrown = 58,
	AHorseTrot = 59,
	AHorseUpFromEat = 60,
	AHorseWalk = 61,
	ALeopardDead = 62,
	ALeopardDeadOnLeft = 63,
	ALeopardDeadOnRight = 64,
	ALeopardDie = 65,
	ALeopardEat = 66,
	ALeopardGetUpFromLeft = 67,
	ALeopardGetUpFromRight = 68,
	ALeopardGotoEat = 69,
	ALeopardGotoSit = 70,
	ALeopardGotoSleep = 71,
	ALeopardInHand = 72,
	ALeopardPounce = 73,
	ALeopardPounceHi = 74,
	ALeopardPounceLo = 75,
	ALeopardProwl = 76,
	ALeopardRun = 77,
	ALeopardSit = 78,
	ALeopardSleep = 79,
	ALeopardStand = 80,
	ALeopardThrown = 81,
	ALeopardUpfromEat = 82,
	ALeopardUpfromSit = 83,
	ALeopardUpfromSleep = 84,
	ALeopardWalk = 85,
	ALionDead = 86,
	ALionDeadOnLeft = 87,
	ALionDeadOnRight = 88,
	ALionDie = 89,
	ALionDownedOnLhs = 90,
	ALionDownedOnRhs = 91,
	ALionEat = 92,
	ALionGetUpFromLeft = 93,
	ALionGetUpFromRight = 94,
	ALionGotoEat = 95,
	ALionGotoSit = 96,
	ALionGotoSleep = 97,
	ALionInHand = 98,
	ALionPounce = 99,
	ALionPounceHi = 100,
	ALionPounceLo = 101,
	ALionProwl = 102,
	ALionRun = 103,
	ALionSit = 104,
	ALionSleep = 105,
	ALionStand = 106,
	ALionThrown = 107,
	ALionTurnLeft_90Degrees = 108,
	ALionTurnRight_90Degrees = 109,
	ALionUpfromEat = 110,
	ALionUpfromSit = 111,
	ALionUpfromSleep = 112,
	ALionWalk = 113,
	APigDeadlhs = 114,
	APigDeadrhs = 115,
	APigEat1 = 116,
	APigEat2 = 117,
	APigFallDeadlhs = 118,
	APigFallDeadrhs = 119,
	APigGotoEat = 120,
	APigInHand = 121,
	APigLandGetUplhs = 122,
	APigLandGetUprhs = 123,
	APigOutOfEat = 124,
	APigRun = 125,
	APigStand = 126,
	APigThrown = 127,
	APigWalk = 128,
	SharkBonedSwim = 129,
	ASheepDeadLhs = 130,
	ASheepDeadRhs = 131,
	ASheepEat1 = 132,
	ASheepEat2 = 133,
	ASheepFallDeadlhs = 134,
	ASheepFallDeadrhs = 135,
	ASheepGotoEat = 136,
	ASheepInHand = 137,
	ASheepLandedGetuplhs = 138,
	ASheepLandedGetuprhs = 139,
	ASheepOutOfEat = 140,
	ASheepRun = 141,
	ASheepStand = 142,
	ASheepThrown = 143,
	ASheepTrot = 144,
	ASheepWalk = 145,
	ATigerDead = 146,
	ATigerDeadOnLeft = 147,
	ATigerDeadOnRight = 148,
	ATigerDie = 149,
	ATigerEat = 150,
	ATigerGetUpFromLeft = 151,
	ATigerGetUpFromRight = 152,
	ATigerGotoEat = 153,
	ATigerGotoSit = 154,
	ATigerGotoSleep = 155,
	ATigerInHand = 156,
	ATigerPounce = 157,
	ATigerPounceHi = 158,
	ATigerPounceLo = 159,
	ATigerProwl = 160,
	ATigerRun = 161,
	ATigerSit = 162,
	ATigerSleep = 163,
	ATigerStand = 164,
	ATigerThrown = 165,
	ATigerUpfromEat = 166,
	ATigerUpfromSit = 167,
	ATigerUpfromSleep = 168,
	ATigerWalk = 169,
	ATortoiseDead = 170,
	ATortoiseStand = 171,
	ATortoiseWalk = 172,
	AWolfDie = 173,
	AWolfEat = 174,
	AWolfGotoEat = 175,
	AWolfGotoSit = 176,
	AWolfGotoSleep = 177,
	AWolfHowl = 178,
	AWolfPounce = 179,
	AWolfProwl = 180,
	AWolfRun = 181,
	AWolfSit = 182,
	AWolfSleep = 183,
	AWolfStand = 184,
	AWolfUpfromEat = 185,
	AWolfUpfromSit = 186,
	AWolfUpfromSleep = 187,
	AWolfWalk = 188,
	MineEntrance = 189,
	PlayerIcon01Plinth = 190,
	NorsGate = 191,
	Phonebox = 192,
	PiperCaveClosing = 193,
	PiperCaveOpening = 194,
	GateTotemPlinthe = 195,
	PiedPiperDanceStand = 196,
	PiedPiperDanceStandSpin = 197,
	PiedPiperDanceWalk = 198,
	PiedPiperDanceWalkSpin = 199,
	PiedPiperSummon = 200,
	PiedPiperWalk = 201,
	PAfricanDrummer = 202,
	PAmbient1 = 203,
	PAmbient2 = 204,
	PArsonist = 205,
	PAttackedByLion = 206,
	PAttractYourAttention = 207,
	PAvoidObjectDive = 208,
	PAvoidObjectDodge = 209,
	PBeckon = 210,
	PBlowRaspberry = 211,
	PCarryingBag = 212,
	PCarryingPot = 213,
	PCarryingSticks = 214,
	PCarryAxe = 215,
	PCarryObjectRun = 216,
	PChoppingTree = 217,
	PClap_1 = 218,
	PClap_2 = 219,
	PClap_3 = 220,
	PClap_4 = 221,
	PConductMeeting = 222,
	PCoupleKissMan = 223,
	PCoupleKissWoman = 224,
	PCrawlInjured = 225,
	PCrawlInjuredInto = 226,
	PCrowdImpressed_1 = 227,
	PCrowdImpressed_2 = 228,
	PCrowdLost = 229,
	PCrowdLost_2 = 230,
	PCrowdMexicanWave = 231,
	PCrowdUnimpressed_1 = 232,
	PCrowdUnimpressed_2 = 233,
	PCrowdWon = 234,
	PCrowdWon_2 = 235,
	PDancePissTake = 236,
	PDanceStand = 237,
	PDanceTogetherArmlink = 238,
	PDanceTogetherCircling = 239,
	PDanceTogetherCirclingPartner = 240,
	PDanceX = 241,
	PDanceXPartner = 242,
	PDead1 = 243,
	PDead1GotoHeaven = 244,
	PDead1GotoHell = 245,
	PDead2 = 246,
	PDead2GotoHeaven = 247,
	PDead2GotoHell = 248,
	PDeadDrowned = 249,
	PDeadInHand = 250,
	PDribbleBall = 251,
	PDrowning = 252,
	PDying = 253,
	PEat = 254,
	PFakeFoulFall = 255,
	PFarmer = 256,
	PFarmerHarvesting = 257,
	PFarmerShooSheep = 258,
	PFarmerSowingSeeds = 259,
	PFarmerStokeSheep = 260,
	PFarmerTurningSoil = 261,
	PFisherman = 262,
	PFrightDuck = 263,
	PFrightJump = 264,
	PFDanceA = 265,
	PFDanceB = 266,
	PFDanceC = 267,
	PGiveBirth = 268,
	PGoalkeeper = 269,
	PGoalkeeper_2 = 270,
	PGoalkeeper_3 = 271,
	PGossipMan = 272,
	PGossipWoman_1 = 273,
	PGossipWoman_2 = 274,
	PGossipWoman_3 = 275,
	PHammering = 276,
	PHelpSomeoneHelpee = 277,
	PHelpSomeoneHelper = 278,
	PHug = 279,
	PInspectObject_1 = 280,
	PInspectObject_2 = 281,
	PInspectObject_3 = 282,
	PIntoDeadDrowned = 283,
	PIntoHammering = 284,
	PIntoMourning = 285,
	PIntoPointing = 286,
	PIntoPray = 287,
	PIntoPullObject = 288,
	PIntoPushObject = 289,
	PIntoRunMan = 290,
	PIntoRunWoman = 291,
	PIntoSawWood = 292,
	PIntoSledgehammer = 293,
	PIntoSleep = 294,
	PIntoSprintMan = 295,
	PIntoSprintWoman = 296,
	PIntoWalkMan = 297,
	PIntoWalkWoman = 298,
	PJumpA = 299,
	PJumpAtHand = 300,
	PJumpAtHand_2 = 301,
	PKickBall = 302,
	PKickBallPass = 303,
	PKickBallPassPt1 = 304,
	PKickBallPassPt2 = 305,
	PLanded = 306,
	PLandedFromBack = 307,
	PLandedFromFeet = 308,
	PLandedFromFeetCarryObject = 309,
	PLookingForSomething = 310,
	PLookAtHand = 311,
	PLovingTouch = 312,
	PMourning = 313,
	PMDanceA = 314,
	PMDanceB = 315,
	PMDanceC = 316,
	POnFireDie = 317,
	POnFireDoused = 318,
	POnFireRun = 319,
	POutOfHammering = 320,
	POutOfMourning = 321,
	POutOfPointing = 322,
	POutOfPray = 323,
	POutOfPullObject = 324,
	POutOfPushObject = 325,
	POutOfRunMan = 326,
	POutOfRunWoman = 327,
	POutOfSawWood = 328,
	POutOfSledgehammer = 329,
	POutOfSleep = 330,
	POutOfWalkWoman = 331,
	POverworked1 = 332,
	POverworked2 = 333,
	PPanicMan = 334,
	PPanicWoman = 335,
	PPickupBallPt1 = 336,
	PPickupBallPt2 = 337,
	PPickUpBag = 338,
	PPickUpPotOnHead = 339,
	PPickUpSticks = 340,
	PPlayAccordian = 341,
	PPoisoned = 342,
	PPray = 343,
	PProdCampfire = 344,
	PPullObject = 345,
	PPushObject = 346,
	PPutDownBag = 347,
	PPutDownSticks = 348,
	PPutOutFire = 349,
	PPuzzled = 350,
	PRunMan = 351,
	PRunWithBall = 352,
	PRunWoman = 353,
	PSawWood = 354,
	PScaredStiff = 355,
	PScaredStiff_2 = 356,
	PScoreGoal = 357,
	PScript1 = 358,
	PScrubbs = 359,
	PSculptor = 360,
	PShakeFistFemale = 361,
	PShootAtGoal = 362,
	PShootAtGoal_2 = 363,
	PShootAtGoalPt_1 = 364,
	PShootAtGoalPt_2 = 365,
	PShrugShoulders = 366,
	PSittingDown1Into = 367,
	PSittingDown1OutOf = 368,
	PSittingDown1Sitting = 369,
	PSittingDown2Into = 370,
	PSittingDown2OutOf = 371,
	PSittingDown2Sitting = 372,
	PSittingDownDanceSailing1 = 373,
	PSittingDownDanceSailing2 = 374,
	PSittingDownDanceSailing3 = 375,
	PSittingDownMale1 = 376,
	PSittingDownMale2 = 377,
	PSittingSwingingLegs = 378,
	PSkip02 = 379,
	PSledgehammer = 380,
	PSleep = 381,
	PSleeping = 382,
	PSprintRunMan = 383,
	PSprintRunWoman = 384,
	PStand = 385,
	PStandDespair_1 = 386,
	PStandDespair_2 = 387,
	PStandDespair_3 = 388,
	PStirPot01 = 389,
	PSwim2 = 390,
	PTalk1Female = 391,
	PTalk1Male = 392,
	PTalk2Female = 393,
	PTalk2Male = 394,
	PTalkingAndPointing = 395,
	PTalkToNeighbour = 396,
	PThankYou = 397,
	PThankYouFemale = 398,
	PThrown = 399,
	PThrownDead = 400,
	PThrownVortex = 401,
	PThrowBallPt_1 = 402,
	PThrowBallPt_2 = 403,
	PThrowRock = 404,
	PThrowWater = 405,
	PTitanic = 406,
	PTurn_180 = 407,
	PTurn_90Left = 408,
	PTurn_90Right = 409,
	PTurnOnTheSpotLeft = 410,
	PTurnOnTheSpotRight = 411,
	PWaitingForBall = 412,
	PWaitingForBall_2 = 413,
	PWaitingForKickOff = 414,
	PWaitingForKickOff_2 = 415,
	PWaitingForKickOff_3 = 416,
	PWaitingForKickOff_4 = 417,
	PWaitingImpateintly = 418,
	PWalkBeckon02 = 419,
	PWalkDespairAway_1 = 420,
	PWalkDespairAway_2 = 421,
	PWalkDespairAway_3 = 422,
	PWalkDespairTo_1 = 423,
	PWalkDespairTo_2 = 424,
	PWalkDespairTo_3 = 425,
	PWalkInjured = 426,
	PWalkMan = 427,
	PWalkPoint = 428,
	PWalkTiredEyes = 429,
	PWalkToStandWoman = 430,
	PWalkWoman = 431,
	PWellDoneFemale = 432,
	PWhittlingAtStick = 433,
	PYankedUnderground = 434,
	PYankedUnderground2 = 435,
	PYankedUnderground3 = 436,
	PYawn = 437,
	PYawn2 = 438,
	SPriestSuckLifeForce = 439,
	AWorm = 440,

	_count
};

} // namespace openblack

/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

namespace openblack
{

struct MobileStatic
{
	/// Originally MobileStaticInfo
	enum class Info
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

	Info type;
};

struct MobileObject
{
	/// Originally MobileObjectInfo
	enum class Info
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

	Info type;
};

} // namespace openblack

/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

namespace openblack::entities::components
{

struct Feature
{
	/// Originally FeatureInfo
	enum class Info
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
		StoneIconApe = 76,
		StoneIconBear = 77,
		StoneIconCow = 78,
		StoneIconHorse = 79,
		StoneIconLeopard = 80,
		StoneIconLion = 81,
		StoneIconPolarbear = 82,
		StoneIconSheep = 83,
		StoneIconTiger = 84,
		StoneIconTortoise = 85,
		StoneIconWolf = 86,
		StoneIconZebra = 87,
		// MarbleBaseDoNotUse  = 88, // CREATUREISLE
		// EggTree             = 89, // CREATUREISLE
		// CreatureRacePillar  = 90, // CREATUREISLE
		// EggTree_2           = 91, // CREATUREISLE
		// EggTree_3           = 92, // CREATUREISLE
		// EggTree_4           = 93, // CREATUREISLE
		// PointerHand         = 94, // CREATUREISLE
		// BowlingEnd          = 95, // CREATUREISLE
	};

	Info type;
};

} // namespace openblack::entities::components

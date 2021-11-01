/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

namespace openblack::ecs::components
{

struct MobileStatic
{
	MobileStaticInfo type;
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

} // namespace openblack::ecs::components

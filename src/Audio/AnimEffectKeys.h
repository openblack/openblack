/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>

namespace openblack::audio
{

// Values of the keys of Black & White's animation effects (see AnimEffectTable), named as the bank's editor text names
// them after the game's SoundSize.h, SoundAlignment.h, SoundObject.h, SoundSurface.h and SoundAction.h. Only the
// values the game plays so far are here, the creatures' among them. 0 leaves a key unset.

enum class SoundSize : int32_t
{
	None = 0,
	Large = 1,
	Medium = 2,
	Small = 3,
};

enum class SoundObject : int32_t
{
	None = 0,
	Human = 1,
	Lion = 2,
	Ape = 3,
	Wolf = 4,
	Tiger = 5,
	Chimp = 6,
	Horse = 7,
	Zebra = 8,
	Leopard = 9,
	BrownBear = 10,
	PolarBear = 11,
	Buffalo = 12,
	Cow = 13,
	Tree = 20,
	Tortoise = 28,
	Sheep = 29,
	Greek = 42,
	Rhino = 43,
};

/// What the sound is made on. The game's sound surfaces end at loose foliage; Tree is a value of the editor's banks.
enum class SoundSurface : int32_t
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
	Tree = 10,
};

enum class SoundAction : int32_t
{
	None = 0,
	BreatheIn = 1,
	BreatheOut = 2,
	FootstepLight = 3,
	FootstepNormal = 4,
	FootStamp = 5,
	Scream = 6,
	RoarShort = 7,
	SnoreIn = 8,
	SnoreOut = 9,
	SneezeIn = 10,
	SneezeOut = 11,
	GrowlShort = 12,
	Scratch = 13,
	Vomit = 14,
	Pooh = 15,
	Sniff = 16,
	Bite = 17,
	Chew = 18,
	Yawn = 19,
	Grunt = 20,
	Pant = 21,
	FacePunch = 22,
	SlapFace = 23,
	SlapSide = 24,
	ReactToHit = 25,
	HitGround = 26,
	RewardedShort = 27,
	Swipe = 54,
	Acknowledge = 55,
	Refusal = 56,
	Whistle = 61,
	Tree = 70,
	Collide = 75,
	TauntGrunt = 79,
	HappyGrunt = 80,
	Scared = 81,
	PlayfulGrunt = 82,
	PickMeScream = 83,
	AngryScream = 84,
	FightGrunt = 85,
	BodyPunch = 86,
	RewardedMedium = 88,
	RewardedLong = 89,
	GrowlMedium = 90,
	GrowlLong = 91,
	RoarMedium = 92,
	RoarLong = 93,
	Death = 94,
	CreedGlowLeft0 = 95,
	CreedGlowCentre100 = 109,
	Drink = 127,
	DrinkFinish = 128,
	KissShort = 129,
	KissLong = 130,
	Huff = 131,
};

/// What an animation effect is for, in the order of the keys of the banks
struct AnimEffectKeys
{
	SoundSize size {SoundSize::None};
	int32_t alignment {0};
	SoundObject object {SoundObject::None};
	SoundSurface surface {SoundSurface::None};
	SoundAction action {SoundAction::None};

	[[nodiscard]] constexpr std::array<int32_t, 5> ToArray() const noexcept
	{
		return {static_cast<int32_t>(size), alignment, static_cast<int32_t>(object), static_cast<int32_t>(surface),
		        static_cast<int32_t>(action)};
	}

	constexpr bool operator==(const AnimEffectKeys&) const noexcept = default;
};

} // namespace openblack::audio

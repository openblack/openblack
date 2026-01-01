/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

/* This file is based on the ScriptEnums.h and ScriptEnumstwo.h from the
 * Black & White Scripting Tools for Creature Isle that can be downloaded from here:
 * https://forum.bwgame.net/downloads/black-white-scripting-tools.1456/
 */

#pragma once

#include <cstdint>

namespace openblack::script
{

/// enums from ScriptEnums.h

enum class FindType : uint16_t
{
	Any = 5000,
};

enum class SpiritType : uint8_t
{
	None,
	Good,
	Evil,
	Alignment,
	AntiAlignment,
	Random,
	Last,
};

enum class PuzzleGameType : uint8_t
{
	None,
	Tree1,
	Tree2,
	Hanoi,
	Maze1,
	Maze2,
	Totem1,
	Totem2,
	Totem3,
	Totem4,
	Thesius1,
	Thesius2,
	Tilt1,
	Tilt2,
	Fishes1,
	Fishes2,
	Immersion,
	Immersion2,
	Lionsheep,
	Chess,
	Last,
};

enum class PuzzleGameStatus : uint8_t
{
	Won,
	Lost,
	WonGoodway,
	WonEvilway,
	GameInProgress,
	GameNotYetBegun,
	Last,
};

enum class ObjectPropertyType : uint8_t
{
	None,
	Health,
	Angle,
	XAngle,
	ZAngle,
	Flying,
	Drowning,
	Moving,
	Scale,
	InHand,
	// InHandAny,	<-- CI
	InHandGrabypeSpeed,
	RunningSpeed,
	DefaultSpeed,
	Death,
	ThingType,
	Age,
	Strength,
	Alignment,
	// ActualHeight,	<-- CI
	Height,
	MaxHeight,
	Player,
	BuiltPercentage,
	XPos,
	YPos,
	ZPos,
	// Radius,		<-- CI
	// Density,		<-- CI

	// Enums for creature properties
	CreatureWarmth,
	CreatureFatness,
	CreatureEnergy,
	CreatureItchiness,
	CreatureAmountOfPoo,
	CreatureExhaustion,
	CreatureDehydration,
	CreatureFightHealth,
	CreatureMinSize,
	CreatureMaxSize,
	CreatureCanGoThroughVortex,
	CreatureFightPower,

	Last,
};

enum class ObjectType : uint8_t
{
	None,
	Marker,
	Abode,
	Feature,
	Villager,
	VillagerChild,
	Animal,
	Reward,
	MobileStatic,
	Town,
	Dance,
	Flock,
	Creature,
	DeadTree,
	InfluenceRing,
	WeatherThing,
	Store,
	Timer,
	Citadel,
	WorshipSite,
	MobileObject,
	Bird,
	Tree,
	Vortex,
	SpellSeed,
	Poo,
	Whale,
	Ark,
	Ball,
	Mist,
	OneShotSpell,
	OneShotSpellInHand,
	PuzzleGame,
	Rock,
	Totem,
	Field,
	SpellDispenser,
	Highlight,
	ComputerPlayer,
	Scaffold,
	TotemStatue,
	AnimatedStatic,
	SpecialField,
	Die,
	CreatureIsleBuilding,
	Dolphin,
	Cannon,
	DumbCreature,
	Flowers,
	FemaleCreature,
	Last,
};

enum class InterfaceSet : uint8_t
{
	SetNone,
	SetNoInterface,
	SetInteraction,
	SetPickUp,
	SetNormal,
	ActivateGestures,
};

enum class AviSequence : uint8_t
{
	None,
	Intro,
	Falling,
	Last,
};

enum class InterfaceLevel : uint8_t
{
	Normal,
	JustGrab,
	JustGrabFarToCitadel,
	JustRotate,
	JustDoubleClickAndDrag,
	JustZoom,
	JustRotateInteract,
	JustRotateInteractAndZoom,
	JustHandMove,
	Just9, // <- missing in CI
	JustRotateAndDrag,
	JustPitch,
	JustHandInteraction,
	JustGrabDoubleClickAndRotate,
	JustGrabDoubleClickAndRotateAndPitch,
	JustGrabDoubleClickAndRotatePitchAndZoom,
};

enum class CountDownDraw : uint8_t
{
	None,
	TopLeft,
	BottomLeft,
	TopRight,
	BottomRight,
	Centre,
	TopCentre,
	BottomCentre,
	Last,
};

enum class DevFunction : uint8_t
{
	None,
	StartDevelopmentScripts,
	RopeLeashEnabled,
	OtherLeashesEnabled,
	LoadMyCreature,
	ResetEscapeState,
	MyCreaturePointOutHighlight,
	ClearInteractionMagnitude,
	MyCreatureCanDie,
	MyCreatureCannotDie,
	CreatureHelpOn,
	CreatureHelpOff,
	EnterSavegameroom,

	Last,
};

/// enums from ScriptEnumstwo.h

enum class JcSpecial : uint8_t
{
	CreateFireBall,
	CameraFollowFireBall,
	CancelCameraFollowFireBall,
	DansLesNuages,
	CreateHandWithKid,
	ActivatedHand,
	IlEtaitUnPetitNavire,
	IAmTheLittleBoy,
	BlendAngle,
	PrepareRotate180,
	PrepareRotateLeft90,
	PrepareRotateRight90,
	FadeInHand,
	StartHandWithBoy,
	SwitchOnBookmark,
	SwitchOffBookmark,
	PrepareTurnLeft,
	PrepareTurnRight,
	CleanIntro,
	AlwaysVisible,
	CiBoatArrival,
	CiBoatArrivalEnd,
	RaiseTotem,

	StartEye,
	EndEye,
	ZoomEye,

	CiBoatLaunch,
	CiBoatLaunchEnd,

	CiBoatCrash,
	CiBoatCrash2,
	CiBoatCrashEnd,

	SwitchOnInfluence,
	SwitchOffInfluence,
};

enum class CreatureTattooType : uint8_t
{
	None,
	TheBrotherhood,
	SuperGods,
	Fighter,
	FighterAlways,
	Last,
};

enum class SpotVisualAlex : uint8_t
{
	Hulahoop,
	Fence,
	Last,
};

} // namespace openblack::script

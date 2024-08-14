/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

/* This file is based on the ScriptEnums.h from the Black & White Scripting Tools
 * for Creature Isle that can be downloaded from here:
 * https://forum.bwgame.net/downloads/black-white-scripting-tools.1456/
 */

#pragma once

#include <cstdint>

namespace openblack::script
{

enum class FindType
{
	Any = 5000,
};

enum class SpiritType
{
	None,
	Good,
	Evil,
	Alignment,
	AntiAlignment,
	Random,
	Last,
};

enum class PuzzleGameType
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

enum class PuzzleGameStatus
{
	Won,
	Lost,
	WonGoodway,
	WonEvilway,
	GameInProgress,
	GameNotYetBegun,
	Last,
};

enum class ObjectPropertyType
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

enum class ObjectType
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

enum class InterfaceSet
{
	SetNone,
	SetNoInterface,
	SetInteraction,
	SetPickUp,
	SetNormal,
	ActivateGestures,
};

enum class AviSequence
{
	None,
	Intro,
	Falling,
	Last,
};

enum class InterfaceLevel
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

enum class CountDownDraw
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

enum class DevFunction
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

} // namespace openblack::script

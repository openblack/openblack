/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

namespace openblack::script
{

enum class JcSpecial
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

enum class CreatureTattooType
{
	None,
	TheBrotherhood,
	SuperGods,
	Fighter,
	FighterAlways,
	Last,
};

enum class SpotVisualAlex
{
	Hulahoop,
	Fence,
	Last,
};

} // namespace openblack::script

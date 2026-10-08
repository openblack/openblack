/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LivingRules.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include "Body.h"

using namespace openblack;
using namespace openblack::physics;
using namespace openblack::physics::living;

namespace
{
constexpr float k_Pi = std::numbers::pi_v<float>;
constexpr float k_TwoPi = 2.0f * k_Pi;
/// A direction shorter than this across the land has no heading
constexpr float k_HeadingLeast = 1e-6f;
/// A thing nearer than it flies in this many seconds is too near to stand and look at
constexpr float k_RunSeconds = 2.0f;
} // namespace

LandingPose living::VillagerLandingPose(float sideUp)
{
	if (sideUp < -k_LyingLean)
	{
		return LandingPose::Front;
	}
	if (sideUp > k_LyingLean)
	{
		return LandingPose::Back;
	}
	return LandingPose::Feet;
}

LandingPose living::AnimalLandingPose(float sideUp)
{
	if (sideUp > k_LyingLean)
	{
		return LandingPose::Front;
	}
	if (sideUp < -k_LyingLean)
	{
		return LandingPose::Back;
	}
	return LandingPose::Feet;
}

float living::HeadingOf(glm::vec3 axis)
{
	if (!(axis.x * axis.x + axis.z * axis.z > k_HeadingLeast))
	{
		return 0.0f;
	}
	return std::atan2(axis.x, -axis.z);
}

float living::WrapHeading(float radians)
{
	if (radians > k_Pi)
	{
		return radians - k_TwoPi;
	}
	if (radians < -k_Pi)
	{
		return radians + k_TwoPi;
	}
	return radians;
}

float living::VillagerLandingHeading(LandingPose pose, const glm::mat3& axes)
{
	switch (pose)
	{
	case LandingPose::Front:
		return HeadingOf(axes[1]);
	case LandingPose::Back:
		return WrapHeading(HeadingOf(axes[1]) + k_Pi);
	default:
		return WrapHeading(HeadingOf(axes[2]) + k_Pi);
	}
}

float living::AnimalLandingHeading(const glm::mat3& axes)
{
	return WrapHeading(HeadingOf(axes[2]) + k_Pi);
}

AnimId living::VillagerThrownClip(bool alive, bool inVortex)
{
	if (!alive)
	{
		return AnimId::PThrownDead;
	}
	return inVortex ? AnimId::PThrownVortex : AnimId::PThrown;
}

AnimId living::VillagerLandedClip(LandingPose pose, bool carrying)
{
	switch (pose)
	{
	case LandingPose::Feet:
		return carrying ? AnimId::PLandedFromFeetCarryObject : AnimId::PLandedFromFeet;
	case LandingPose::Front:
		return AnimId::PLanded;
	default:
		return AnimId::PLandedFromBack;
	}
}

AnimalClips living::ClipsOf(AnimalInfo kind)
{
	switch (kind)
	{
	case AnimalInfo::Lion:
		return {AnimId::ALionStand, AnimId::ALionInHand, AnimId::ALionUpfromSleep, AnimId::ALionUpfromSleep,
		        AnimId::ALionUpfromSleep};
	case AnimalInfo::Leopard:
		return {AnimId::ALeopardStand, AnimId::ALeopardInHand, AnimId::ALeopardUpfromSleep, AnimId::ALeopardUpfromSleep,
		        AnimId::ALeopardUpfromSleep};
	case AnimalInfo::Tiger:
		return {AnimId::ATigerStand, AnimId::ATigerStand, AnimId::ATigerUpfromSleep, AnimId::ATigerUpfromSleep,
		        AnimId::ATigerUpfromSleep};
	case AnimalInfo::Wolf:
	case AnimalInfo::SpellWolf:
		return {AnimId::AWolfStand, AnimId::AWolfStand, AnimId::AWolfUpfromSleep, AnimId::AWolfUpfromSleep,
		        AnimId::AWolfUpfromSleep};
	case AnimalInfo::Cow:
		return {AnimId::ACowThrown, AnimId::ACowInHand, AnimId::ACowStand, AnimId::ACowLandedRightSide,
		        AnimId::ACowLandedLeftSide};
	case AnimalInfo::Sheep:
		return {AnimId::ASheepThrown, AnimId::ASheepInHand, AnimId::ASheepStand, AnimId::ASheepLandedGetuprhs,
		        AnimId::ASheepLandedGetuplhs};
	case AnimalInfo::Horse:
		return {AnimId::AHorseThrown, AnimId::AHorseInHand, AnimId::AHorseStand, AnimId::AHorseLandGetUprhs,
		        AnimId::AHorseLandGetUplhs};
	case AnimalInfo::Pig:
		return {AnimId::APigThrown, AnimId::APigInHand, AnimId::APigStand, AnimId::APigLandGetUprhs, AnimId::APigLandGetUplhs};
	case AnimalInfo::Tortoise:
		return {AnimId::ATortoiseStand, AnimId::ATortoiseStand, AnimId::ATortoiseStand, AnimId::ATortoiseStand,
		        AnimId::ATortoiseStand};
	case AnimalInfo::Dove:
		return {.thrown = AnimId::DoveFlap};
	case AnimalInfo::SpellDove:
		return {.thrown = AnimId::SpellDoveFlap};
	case AnimalInfo::Bat:
	case AnimalInfo::SpellBat:
		return {.thrown = AnimId::BatGlide};
	case AnimalInfo::Crow:
		return {.thrown = AnimId::CrowTakeoff};
	case AnimalInfo::Pigeon:
		return {.thrown = AnimId::PigeonTakeoff};
	case AnimalInfo::Seagull:
		return {.thrown = AnimId::SeagullTakeoff};
	case AnimalInfo::Swallow:
		return {.thrown = AnimId::SwallowFlap};
	default:
		return {};
	}
}

std::optional<AnimId> living::AnimalLandedClip(AnimalInfo kind, LandingPose pose)
{
	const auto clips = ClipsOf(kind);
	switch (pose)
	{
	case LandingPose::Front:
		return clips.landedRight;
	case LandingPose::Back:
		return clips.landedLeft;
	default:
		return clips.landedFeet;
	}
}

std::optional<float> living::LivingCrush(float knock)
{
	if (!(knock > k_HurtingKnock))
	{
		return std::nullopt;
	}
	return (knock - k_HurtingKnock) * k_CrushPerKnock;
}

float living::CreatureMass(float scale, float thinFat, float weakStrong)
{
	constexpr float k_SizeToLength = 8.333334f;
	constexpr float k_BuildWeight = 0.15f;
	constexpr float k_MassScale = 100.0f;
	const float length = scale * k_SizeToLength;
	return ((weakStrong + thinFat) * k_BuildWeight + 1.0f) * k_MassScale * length * length * length;
}

std::optional<float> living::CreatureCrush(float impact, float creatureMass)
{
	const float knock = std::min(impact / (creatureMass * k_Gravity), k_CreatureKnockCap);
	const float crush = knock * k_CreatureCrushPerKnock;
	if (!(crush > k_CreatureLeastCrush))
	{
		return std::nullopt;
	}
	return crush;
}

DrowningStep living::StepDrowning(uint16_t left, bool indestructible)
{
	if (indestructible)
	{
		left = k_IndestructibleDrowning;
	}
	// The count wraps past nothing, as the game's does: one started at nothing struggles a long time
	--left;
	return {.left = left, .dies = left == 0};
}

FlyingObjectResponse living::RespondToFlyingObject(float distance, float speed)
{
	return distance < k_RunSeconds * speed ? FlyingObjectResponse::Run : FlyingObjectResponse::Point;
}

bool living::AnimalFleesFlyingObject(float distance, float speed)
{
	return distance < k_RunSeconds * speed;
}

AnimId living::PointingClip(bool womanOrChild, uint32_t firstRoll, uint32_t secondRoll)
{
	if (womanOrChild && firstRoll == 0)
	{
		return AnimId::PScaredStiff;
	}
	switch (secondRoll)
	{
	case 0:
		return AnimId::PLookingForSomething;
	case 1:
		return AnimId::PStand;
	default:
		return AnimId::PTalkingAndPointing;
	}
}

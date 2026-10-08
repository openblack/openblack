/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <numbers>

#include <gtest/gtest.h>

#include "Physics/LivingRules.h"

using namespace openblack;
using namespace openblack::physics::living;

namespace
{
constexpr float k_Pi = std::numbers::pi_v<float>;
}

TEST(PhysicsLiving, VillagersLieByHowTheirSideLeans)
{
	EXPECT_EQ(VillagerLandingPose(-0.6f), LandingPose::Front);
	EXPECT_EQ(VillagerLandingPose(0.6f), LandingPose::Back);
	EXPECT_EQ(VillagerLandingPose(0.5f), LandingPose::Feet);
	EXPECT_EQ(VillagerLandingPose(-0.5f), LandingPose::Feet);
}

TEST(PhysicsLiving, AnimalsLieTheOtherWayRound)
{
	EXPECT_EQ(AnimalLandingPose(0.6f), LandingPose::Front);
	EXPECT_EQ(AnimalLandingPose(-0.6f), LandingPose::Back);
	EXPECT_EQ(AnimalLandingPose(0.0f), LandingPose::Feet);
}

TEST(PhysicsLiving, HeadingsAreMeasuredFromAnAxisAcrossTheLand)
{
	EXPECT_FLOAT_EQ(HeadingOf({0.0f, 0.0f, -1.0f}), 0.0f);
	EXPECT_FLOAT_EQ(HeadingOf({1.0f, 0.0f, 0.0f}), k_Pi * 0.5f);
	// Pointing straight up, it has none
	EXPECT_FLOAT_EQ(HeadingOf({0.0f, 1.0f, 0.0f}), 0.0f);
	EXPECT_FLOAT_EQ(WrapHeading(k_Pi + 1.0f), 1.0f - k_Pi);
	EXPECT_FLOAT_EQ(WrapHeading(-k_Pi - 1.0f), k_Pi - 1.0f);
}

TEST(PhysicsLiving, AVillagerOnItsFeetKeepsItsHeading)
{
	// The body faces along its forward axis turned opposite the game's: standing up it keeps that heading
	const float heading = 0.7f;
	const glm::mat3 axes(glm::vec3(std::cos(heading), 0.0f, std::sin(heading)), glm::vec3(0.0f, 1.0f, 0.0f),
	                     glm::vec3(-std::sin(heading), 0.0f, std::cos(heading)));
	EXPECT_NEAR(VillagerLandingHeading(LandingPose::Feet, axes), heading, 1e-5f);
	EXPECT_NEAR(AnimalLandingHeading(axes), heading, 1e-5f);
}

TEST(PhysicsLiving, AVillagerOnItsFrontFacesAlongItsUpAxis)
{
	const glm::mat3 axes(glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	EXPECT_NEAR(VillagerLandingHeading(LandingPose::Front, axes), k_Pi * 0.5f, 1e-5f);
	EXPECT_NEAR(VillagerLandingHeading(LandingPose::Back, axes), -k_Pi * 0.5f, 1e-5f);
}

TEST(PhysicsLiving, ThrownAndLandedClips)
{
	EXPECT_EQ(VillagerThrownClip(false, true), AnimId::PThrownDead);
	EXPECT_EQ(VillagerThrownClip(true, true), AnimId::PThrownVortex);
	EXPECT_EQ(VillagerThrownClip(true, false), AnimId::PThrown);
	EXPECT_EQ(VillagerLandedClip(LandingPose::Feet, false), AnimId::PLandedFromFeet);
	EXPECT_EQ(VillagerLandedClip(LandingPose::Feet, true), AnimId::PLandedFromFeetCarryObject);
	EXPECT_EQ(VillagerLandedClip(LandingPose::Front, true), AnimId::PLanded);
	EXPECT_EQ(VillagerLandedClip(LandingPose::Back, false), AnimId::PLandedFromBack);
	EXPECT_EQ(VillagerLandedClip(LandingPose::None, false), AnimId::PLandedFromBack);
}

TEST(PhysicsLiving, AnimalsHaveTheirKindsClips)
{
	EXPECT_EQ(AnimalLandedClip(AnimalInfo::Cow, LandingPose::Front), AnimId::ACowLandedRightSide);
	EXPECT_EQ(AnimalLandedClip(AnimalInfo::Cow, LandingPose::Back), AnimId::ACowLandedLeftSide);
	EXPECT_EQ(AnimalLandedClip(AnimalInfo::Pig, LandingPose::Feet), AnimId::APigStand);
	EXPECT_EQ(ClipsOf(AnimalInfo::Lion).inHand, AnimId::ALionInHand);
	EXPECT_EQ(ClipsOf(AnimalInfo::Crow).thrown, AnimId::CrowTakeoff);
	// Goats and zebras keep whatever clip they play
	EXPECT_FALSE(ClipsOf(AnimalInfo::Goat).thrown.has_value());
	EXPECT_FALSE(AnimalLandedClip(AnimalInfo::Zebra, LandingPose::Feet).has_value());
}

TEST(PhysicsLiving, OnlyKnocksPastTwiceTheirWeightHurtTheLiving)
{
	EXPECT_FALSE(LivingCrush(2.0f).has_value());
	ASSERT_TRUE(LivingCrush(12.0f).has_value());
	EXPECT_NEAR(*LivingCrush(12.0f), 0.3f, 1e-6f);
}

TEST(PhysicsLiving, ACreatureWeighsByItsSizeAndBuild)
{
	const float length = 8.333334f;
	EXPECT_NEAR(CreatureMass(1.0f, 0.0f, 0.0f), 100.0f * length * length * length, 1.0f);
	EXPECT_NEAR(CreatureMass(1.0f, 1.0f, 1.0f), 1.3f * 100.0f * length * length * length, 1.0f);
}

TEST(PhysicsLiving, ACreatureTakesAHundredthOfItsKnockUpToAHundred)
{
	const float mass = 1000.0f;
	const float weight = mass * 9.81f;
	EXPECT_FALSE(CreatureCrush(weight * 0.4f, mass).has_value());
	ASSERT_TRUE(CreatureCrush(weight * 50.0f, mass).has_value());
	EXPECT_NEAR(*CreatureCrush(weight * 50.0f, mass), 0.5f, 1e-5f);
	EXPECT_NEAR(*CreatureCrush(weight * 500.0f, mass), 1.0f, 1e-5f);
}

TEST(PhysicsLiving, DrowningCountsDownToDeath)
{
	auto step = StepDrowning(2, false);
	EXPECT_EQ(step.left, 1);
	EXPECT_FALSE(step.dies);
	step = StepDrowning(step.left, false);
	EXPECT_TRUE(step.dies);
	// An indestructible villager never gets there
	step = StepDrowning(1, true);
	EXPECT_EQ(step.left, 9);
	EXPECT_FALSE(step.dies);
}

TEST(PhysicsLiving, PeopleRunFromWhatComesFastAndPointAtTheRest)
{
	EXPECT_EQ(RespondToFlyingObject(19.0f, 10.0f), FlyingObjectResponse::Run);
	EXPECT_EQ(RespondToFlyingObject(20.0f, 10.0f), FlyingObjectResponse::Point);
	EXPECT_TRUE(AnimalFleesFlyingObject(5.0f, 3.0f));
	EXPECT_FALSE(AnimalFleesFlyingObject(7.0f, 3.0f));
	EXPECT_EQ(PointingClip(true, 0, 2), AnimId::PScaredStiff);
	EXPECT_EQ(PointingClip(false, 0, 0), AnimId::PLookingForSomething);
	EXPECT_EQ(PointingClip(true, 1, 1), AnimId::PStand);
	EXPECT_EQ(PointingClip(false, 2, 2), AnimId::PTalkingAndPointing);
}

TEST(PhysicsLiving, FlyingThingsAreMeasuredAcrossTheMap)
{
	// A thing high overhead is as near as the ground under it
	EXPECT_FLOAT_EQ(MapDistance({0.0f, 0.0f, 0.0f}, {3.0f, 100.0f, 4.0f}), 5.0f);
}

TEST(PhysicsLiving, AThingIsStillInTheAirWhileFastOrAboveTheLand)
{
	EXPECT_TRUE(IsActuallyInTheAir(3.1f, 1.0f, 1.0f, 0.0f));
	EXPECT_FALSE(IsActuallyInTheAir(3.0f, 1.0f, 1.0f, 0.1f));
	// Its middle less the land, plus its reach, must be over a fifth of a metre
	EXPECT_TRUE(IsActuallyInTheAir(0.0f, 10.0f, 9.0f, 0.0f));
	EXPECT_FALSE(IsActuallyInTheAir(0.0f, 10.0f, 10.0f, 0.2f));
	EXPECT_TRUE(IsActuallyInTheAir(0.0f, 10.0f, 10.0f, 0.25f));
}

TEST(PhysicsLiving, TownsCountTheirInjuredAcrossSevenTenths)
{
	EXPECT_EQ(InjuredChange(0.8f, 0.6f), 1);
	EXPECT_EQ(InjuredChange(0.6f, 0.8f), -1);
	EXPECT_EQ(InjuredChange(0.6f, 0.5f), 0);
	// Landing on the mark either way changes nothing
	EXPECT_EQ(InjuredChange(0.8f, 0.7f), 0);
	EXPECT_EQ(InjuredChange(0.7f, 0.6f), 0);
}

TEST(PhysicsLiving, LandedAnimalsMoveTheirFlocksHomeByKind)
{
	EXPECT_EQ(LairOnLanding(AnimalInfo::Cow, false), LandedLair::WhereItLanded);
	EXPECT_EQ(LairOnLanding(AnimalInfo::Lion, true), LandedLair::WhereItLanded);
	EXPECT_EQ(LairOnLanding(AnimalInfo::Lion, false), LandedLair::Unchanged);
	EXPECT_EQ(LairOnLanding(AnimalInfo::SpellWolf, false), LandedLair::Unchanged);
	EXPECT_EQ(LairOnLanding(AnimalInfo::Tiger, true), LandedLair::ForestOfItsKind);
	EXPECT_EQ(LairOnLanding(AnimalInfo::Tiger, false), LandedLair::Unchanged);
	EXPECT_EQ(LairOnLanding(AnimalInfo::Wolf, false), LandedLair::ForestOfItsKind);
}

TEST(PhysicsLiving, AHeadingStandsAThingUpright)
{
	const auto axes = HeadingAxes(k_Pi * 0.5f);
	EXPECT_NEAR(axes[0].z, 1.0f, 1e-6f);
	EXPECT_NEAR(axes[1].y, 1.0f, 1e-6f);
	EXPECT_NEAR(axes[2].x, -1.0f, 1e-6f);
}

TEST(PhysicsLiving, AFleeingThingGivesUpWatchesOrRunsByDistanceAndHeading)
{
	using enum FleeStep;
	// Beyond the reaction's furthest it gives up
	EXPECT_EQ(FleeFromObject(51.0f, 10.0f, 50.0f, true), GiveUp);
	// Between its nearest and furthest it watches a thing not coming at it, and runs from one that is
	EXPECT_EQ(FleeFromObject(30.0f, 10.0f, 50.0f, false), Watch);
	EXPECT_EQ(FleeFromObject(30.0f, 10.0f, 50.0f, true), Run);
	// Within its nearest it runs whatever the thing does
	EXPECT_EQ(FleeFromObject(5.0f, 10.0f, 50.0f, false), Run);
	EXPECT_EQ(FleeFromObject(50.0f, 10.0f, 50.0f, false), Watch);
}

TEST(PhysicsLiving, AVillagersStateChoosesItsClipOrTakesItsRow)
{
	// No state plays scared stiff
	EXPECT_EQ(VillagerStateClip(0, std::nullopt, 385), AnimId::PScaredStiff);
	EXPECT_EQ(VillagerStateClip(0xFF, std::nullopt, 385), AnimId::PScaredStiff);
	// Flying and landing choose their own clips
	EXPECT_TRUE(VillagerStateChoosesClip(10));
	EXPECT_TRUE(VillagerStateChoosesClip(11));
	EXPECT_EQ(VillagerStateClip(10, AnimId::PThrownDead, 399), AnimId::PThrownDead);
	// A choosing state whose choice isn't made yet rests in its model's pose
	EXPECT_EQ(VillagerStateClip(1, std::nullopt, 0), AnimId::Invalid);
	// Drowning and being held play their rows
	EXPECT_FALSE(VillagerStateChoosesClip(16));
	EXPECT_FALSE(VillagerStateChoosesClip(24));
	EXPECT_EQ(VillagerStateClip(16, AnimId::PThrown, 252), static_cast<AnimId>(252));
	EXPECT_EQ(VillagerStateClip(24, std::nullopt, 355), AnimId::PScaredStiff);
	// A negative row keeps the clip playing
	EXPECT_EQ(VillagerStateClip(24, std::nullopt, -1), std::nullopt);
}

TEST(PhysicsLiving, AStatesClipChoiceIsNoChoiceOfTheNextState)
{
	// Landed chose its landing clip; walking on afterwards chooses afresh and rests until it does
	EXPECT_EQ(ChoiceOfState(11, 11, AnimId::PLanded), AnimId::PLanded);
	EXPECT_EQ(ChoiceOfState(1, 11, AnimId::PLanded), std::nullopt);
	EXPECT_EQ(VillagerStateClip(1, ChoiceOfState(1, 11, AnimId::PLanded), 0), AnimId::Invalid);
}

TEST(PhysicsLiving, ADancerNoticesOnlyWhatFliesAtIt)
{
	using openblack::physics::living::FlyingAt;
	const glm::vec3 villager(0.0f, 0.0f, 0.0f);
	const glm::vec3 object(10.0f, 0.0f, 0.0f);
	EXPECT_TRUE(FlyingAt(villager, object, glm::vec3(-5.0f, 0.0f, 0.0f)));
	// Turned more than about 37 degrees away it isn't coming at it
	EXPECT_TRUE(FlyingAt(villager, object, glm::vec3(-1.0f, 0.0f, 0.7f)));
	EXPECT_FALSE(FlyingAt(villager, object, glm::vec3(-1.0f, 0.0f, 0.8f)));
	EXPECT_FALSE(FlyingAt(villager, object, glm::vec3(5.0f, 0.0f, 0.0f)));
}

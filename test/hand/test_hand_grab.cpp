/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <glm/geometric.hpp>
#include <gtest/gtest.h>

#include "Hand/HandGrabRules.h"

using namespace openblack;
using namespace openblack::hand_grab;

TEST(HandGrab, VillagersAtHomeHeldOrHidingCantBePickedUp)
{
	Holdable villager {.kind = GrabKind::Villager};
	EXPECT_TRUE(ValidForPlaceInHand(villager));
	villager.atHome = true;
	EXPECT_FALSE(ValidForPlaceInHand(villager));
	villager.atHome = false;
	villager.inHand = true;
	EXPECT_FALSE(ValidForPlaceInHand(villager));
	villager.inHand = false;
	villager.hiding = true;
	EXPECT_FALSE(ValidForPlaceInHand(villager));
	villager.hiding = false;
	villager.available = false;
	EXPECT_FALSE(ValidForPlaceInHand(villager));
}

TEST(HandGrab, AnimalsOnlyWhenTheirKindAllows)
{
	Holdable animal {.kind = GrabKind::Animal};
	EXPECT_FALSE(ValidForPlaceInHand(animal));
	animal.speciesAllows = true;
	EXPECT_TRUE(ValidForPlaceInHand(animal));
}

TEST(HandGrab, RocksWiderThanTheHandCanLiftStay)
{
	Holdable rock {.kind = GrabKind::Rock, .radius = 3.6f, .isRock = true};
	EXPECT_TRUE(ValidForPlaceInHand(rock));
	rock.radius = 3.61f;
	EXPECT_FALSE(ValidForPlaceInHand(rock));
	// A dead tree of any size can be lifted
	EXPECT_TRUE(ValidForPlaceInHand({.kind = GrabKind::DeadTree, .radius = 10.0f}));
	EXPECT_TRUE(ValidForPlaceInHand({.kind = GrabKind::Tree}));
	EXPECT_TRUE(ValidForPlaceInHand({.kind = GrabKind::MobileStatic, .radius = 10.0f}));
	EXPECT_FALSE(ValidForPlaceInHand({.kind = GrabKind::None}));
}

TEST(HandGrab, AnyFailedCheckMakesThePressATap)
{
	const Gate pass {.valid = true};
	EXPECT_TRUE(PassesGate(pass));
	EXPECT_FALSE(PassesGate({.spaceInHand = false, .valid = true}));
	EXPECT_FALSE(PassesGate({.alreadyInHand = true, .valid = true}));
	EXPECT_FALSE(PassesGate({.valid = false}));
	EXPECT_FALSE(PassesGate({.valid = true, .cannotBePickedUp = true}));
	EXPECT_FALSE(PassesGate({.valid = true, .carried = true}));
	EXPECT_FALSE(PassesGate({.valid = true, .inInfluence = false}));
}

TEST(HandGrab, APressIsTimedByTheClockButNoLongerThanTheTurnsAllow)
{
	// By the clock 500 ms, but the game has only moved on from turn 10 to turn 11
	EXPECT_EQ(ElapsedMs(1500, 1000, 11, 10), 200u);
	EXPECT_EQ(ElapsedMs(1100, 1000, 20, 10), 100u);
}

TEST(HandGrab, EachKindHangsItsOwnWay)
{
	const auto villager = HoldOf(GrabKind::Villager, MobileStaticInfo::None, MeshId::Dummy, 2.0f, 0.5f);
	EXPECT_EQ(villager.type, HoldType::Villager);
	EXPECT_FLOAT_EQ(villager.loweringMultiplier, 0.65f);
	EXPECT_FLOAT_EQ(villager.holdRadius, 0.5f);

	const auto tree = HoldOf(GrabKind::Tree, MobileStaticInfo::None, MeshId::Dummy, 8.0f, 3.0f);
	EXPECT_EQ(tree.type, HoldType::Tree);
	EXPECT_FLOAT_EQ(tree.loweringMultiplier, 0.1f);
	EXPECT_FLOAT_EQ(tree.holdRadius, 0.6f);

	const auto pot = HoldOf(GrabKind::MobileObject, MobileStaticInfo::None, MeshId::Dummy, 1.0f, 0.4f);
	EXPECT_EQ(pot.type, HoldType::Side);
	EXPECT_FLOAT_EQ(pot.loweringMultiplier, 0.7f);

	const auto poo = HoldOf(GrabKind::Poo, MobileStaticInfo::None, MeshId::Dummy, 1.0f, 0.4f);
	EXPECT_EQ(poo.type, HoldType::Above);
	EXPECT_FLOAT_EQ(poo.loweringMultiplier, -0.3f);
	EXPECT_FLOAT_EQ(poo.holdRadius, 0.75f);

	const auto rock = HoldOf(GrabKind::Rock, MobileStaticInfo::RockChalk, MeshId::Dummy, 2.0f, 1.0f);
	EXPECT_EQ(rock.type, HoldType::Above);
	EXPECT_FLOAT_EQ(rock.loweringMultiplier, 0.0f);
	EXPECT_FLOAT_EQ(rock.holdRadius, 1.5f);

	const auto totem = HoldOf(GrabKind::MobileStatic, MobileStaticInfo::GateTotemApe, MeshId::Dummy, 4.0f, 1.0f);
	EXPECT_EQ(totem.type, HoldType::Side);
	EXPECT_FLOAT_EQ(totem.loweringMultiplier, 0.7f);
	const auto singing = HoldOf(GrabKind::MobileStatic, MobileStaticInfo::SingingStone_1, MeshId::Dummy, 4.0f, 1.0f);
	EXPECT_FLOAT_EQ(singing.loweringMultiplier, 0.4f);
	const auto cuddly = HoldOf(GrabKind::MobileStatic, MobileStaticInfo::ToyCuddly, MeshId::ObjectToyCuddly, 1.0f, 1.0f);
	EXPECT_EQ(cuddly.type, HoldType::Side);
	EXPECT_FLOAT_EQ(cuddly.loweringMultiplier, 0.0f);
}

TEST(HandGrab, TheHandRisesForWhatItHolds)
{
	// Taken to be pulled free, a thing is gripped no closer than a share of the hand's height
	EXPECT_FLOAT_EQ(HoldDistance(0.65f, 2.0f, 1.0f), 1.3f);
	EXPECT_FLOAT_EQ(HoldDistance(0.0f, 2.0f, 1.0f), 0.3f * 3.2f);
	// On the palm a little; any other way its hang, but at least 1.9, whatever the hand's size; a standing tree a tenth
	// of its height more
	EXPECT_FLOAT_EQ(HandRise(HoldType::Above, 5.0f, 1.0f, std::nullopt), 0.2f);
	EXPECT_FLOAT_EQ(HandRise(HoldType::Villager, 1.3f, 1.0f, std::nullopt), 1.9f);
	EXPECT_FLOAT_EQ(HandRise(HoldType::Villager, 1.3f, 2.0f, std::nullopt), 1.9f);
	EXPECT_FLOAT_EQ(HandRise(HoldType::Side, 2.5f, 1.0f, std::nullopt), 2.5f);
	EXPECT_FLOAT_EQ(HandRise(HoldType::Tree, 2.0f, 1.0f, 10.0f), 3.0f);
	EXPECT_FLOAT_EQ(CursorRaise(3.0f), 1.8f);
}

TEST(HandGrab, TheSpringStepsAtLeastOnceAFrame)
{
	HandSpring spring;
	spring.Start({0.0f, 0.0f, 0.0f});
	spring.Count(0);
	spring.Step({1.0f, 0.0f, 0.0f});
	// One step of 10 ms: v = 260 * 1 * 0.01, then x = v * 0.01
	EXPECT_EQ(spring.StepsTaken(), 1u);
	EXPECT_FLOAT_EQ(spring.Velocity().x, 2.6f);
	EXPECT_FLOAT_EQ(spring.Position().x, 0.026f);
}

TEST(HandGrab, TheSpringTakesAsManyStepsAsTheGameTimeAllows)
{
	const auto frame = [](HandSpring& spring, uint32_t ms) {
		spring.Count(ms);
		spring.Step({1.0f, 0.0f, 0.0f});
	};
	HandSpring once;
	once.Start({0.0f, 0.0f, 0.0f});
	frame(once, 30);
	HandSpring stepped;
	stepped.Start({0.0f, 0.0f, 0.0f});
	frame(stepped, 0);
	frame(stepped, 0);
	frame(stepped, 0);
	EXPECT_FLOAT_EQ(once.Position().x, stepped.Position().x);
	// A frame of 15 ms after one of 0 ms takes one more step, not two
	HandSpring uneven;
	uneven.Start({0.0f, 0.0f, 0.0f});
	frame(uneven, 0);
	frame(uneven, 15);
	HandSpring two;
	two.Start({0.0f, 0.0f, 0.0f});
	frame(two, 20);
	EXPECT_FLOAT_EQ(uneven.Position().x, two.Position().x);
}

TEST(HandGrab, TheSpringCatchesUpWithTheTimeHeldBeforeItTookHold)
{
	// Half a second holding with the spring off, then the first frame with it on takes all of that time's steps
	HandSpring spring;
	for (int i = 0; i < 50; ++i)
	{
		spring.Count(10);
	}
	spring.Start({0.0f, 0.0f, 0.0f});
	spring.Count(10);
	spring.Step({1.0f, 0.0f, 0.0f});
	EXPECT_EQ(spring.StepsTaken(), 51u);
	// Taking hold again keeps the time counted
	spring.Start({0.0f, 0.0f, 0.0f});
	spring.Count(10);
	spring.Step({1.0f, 0.0f, 0.0f});
	EXPECT_EQ(spring.StepsTaken(), 1u);
}

TEST(HandGrab, TheSpringSettlesOnItsTargetAndIsCapped)
{
	HandSpring spring;
	spring.Start({0.0f, 0.0f, 0.0f});
	for (int i = 0; i < 200; ++i)
	{
		spring.Count(10);
		spring.Step({3.0f, 1.0f, -2.0f});
	}
	EXPECT_NEAR(spring.Position().x, 3.0f, 1e-3f);
	EXPECT_NEAR(spring.Position().z, -2.0f, 1e-3f);
	HandSpring far;
	far.Start({0.0f, 0.0f, 0.0f});
	far.Count(10);
	far.Step({10000.0f, 0.0f, 0.0f});
	EXPECT_FLOAT_EQ(glm::length(far.Velocity()), k_MaxThrowSpeed);
}

TEST(HandGrab, TheTwistIsCancelledWhenItsCountdownEndsOnNothing)
{
	int32_t ms = 180;
	EXPECT_EQ(CountDown(ms, 100), Countdown::Running);
	EXPECT_EQ(CountDown(ms, 100), Countdown::RunOut);
	// A countdown that lands on exactly nothing gives no twist
	ms = 180;
	EXPECT_EQ(CountDown(ms, 90), Countdown::Running);
	EXPECT_EQ(CountDown(ms, 90), Countdown::Cancelled);
	ms = 0;
	EXPECT_EQ(CountDown(ms, 10), Countdown::Cancelled);
}

TEST(HandGrab, OnlyATreeTheHandCanLiftIsPulledFree)
{
	Tug tug {.axes = glm::mat3(1.0f), .base = glm::vec3(0.0f)};
	TugFrame frame {
	    .hand = {0.0f, 2.0f, 0.0f}, .holdDistance = 1.0f, .weight = 50.0f, .height = 10.0f, .tree = true, .seconds = 0.01f};
	// A pull of 1000 a unit of stretch, one unit away: more than the tree weighs
	auto result = PullAt(tug, frame);
	EXPECT_FLOAT_EQ(result.force.y, 1000.0f);
	EXPECT_TRUE(result.comesFree);
	// Pulled no harder than it weighs, it stays
	tug = {.axes = glm::mat3(1.0f), .base = glm::vec3(0.0f)};
	frame.weight = 2000.0f;
	EXPECT_FALSE(PullAt(tug, frame).comesFree);
	// Too heavy for the hand's greatest pull it never comes free, however far the hand pulls
	tug = {.axes = glm::mat3(1.0f), .base = glm::vec3(0.0f)};
	frame.weight = MaxForce(0.0f) / 9.81f + 1.0f;
	frame.hand = {0.0f, 100000.0f, 0.0f};
	result = PullAt(tug, frame);
	EXPECT_FLOAT_EQ(glm::length(result.force), MaxForce(0.0f));
	EXPECT_FALSE(result.comesFree);
	// Anything but a tree comes free at once
	tug = {.axes = glm::mat3(1.0f), .base = glm::vec3(0.0f)};
	frame.tree = false;
	EXPECT_TRUE(PullAt(tug, frame).comesFree);
}

TEST(HandGrab, APulledTreeLeansAndStretchesTowardsTheHand)
{
	Tug tug {.axes = glm::mat3(1.0f), .base = glm::vec3(0.0f)};
	const TugFrame frame {
	    .hand = {1.0f, 1.0f, 0.0f}, .holdDistance = 1.0f, .weight = 1.0e6f, .height = 10.0f, .tree = true, .seconds = 0.01f};
	TugResult result;
	for (int i = 0; i < 20; ++i)
	{
		result = PullAt(tug, frame);
	}
	// Its top leans towards the hand, which is off to its side
	EXPECT_GT(tug.axes[1].x, 0.0f);
	EXPECT_GT(glm::length(tug.pullVelocity), 0.0f);
	// It stretches by how far the hand is from its grip, no more than 1.3 times
	EXPECT_GT(result.stretchTarget, 1.0f);
	EXPECT_LE(result.stretchTarget, 1.3f);
	// Rock-like things neither lean nor stretch
	Tug rock {.axes = glm::mat3(1.0f), .base = glm::vec3(0.0f)};
	auto still = frame;
	still.leans = false;
	static_cast<void>(PullAt(rock, still));
	EXPECT_FLOAT_EQ(rock.axes[1].x, 0.0f);
	EXPECT_EQ(rock.pullVelocity, glm::vec3(0.0f));
}

TEST(HandGrab, AThrowIsFastAcrossTheGround)
{
	EXPECT_FALSE(IsThrow({2.0f, 50.0f, 0.0f}, false));
	EXPECT_TRUE(IsThrow({2.0f, 0.0f, 0.1f}, false));
	EXPECT_FALSE(IsThrow({1.0f, 0.0f, 0.0f}, true));
	EXPECT_TRUE(IsThrow({1.0f, 0.0f, 0.1f}, true));
	EXPECT_FALSE(IsFastRelease({1.0f, 0.0f, 0.0f}));
	EXPECT_TRUE(IsFastRelease({1.0f, 0.1f, 0.0f}));
	EXPECT_TRUE(PotPours({2.0f, 1.0f, 0.0f}));
	EXPECT_FALSE(PotPours({2.0f, 1.0f, 0.1f}));
}

TEST(HandGrab, ATwistTurnsAboutTheLevelAxisAcrossTheHandsMotion)
{
	const auto torque = ReleaseSpinTorque(2.0f, 10.0f, {1.0f, 5.0f, 0.0f});
	EXPECT_FLOAT_EQ(torque.x, 0.0f);
	EXPECT_FLOAT_EQ(torque.y, 0.0f);
	EXPECT_FLOAT_EQ(torque.z, -32.0f);
	const auto forward = ReleaseSpinTorque(1.0f, 1.0f, {0.0f, 0.0f, 2.0f});
	EXPECT_FLOAT_EQ(forward.x, 3.2f);
}

TEST(HandGrab, APutDownStandsOnlyOnLandWithoutBeingRaised)
{
	const Landing gentle {.dryLand = true};
	EXPECT_TRUE(LandsOnRelease(gentle));
	EXPECT_FALSE(LandsOnRelease({.thrown = true, .dryLand = true}));
	EXPECT_FALSE(LandsOnRelease({.raised = true, .dryLand = true}));
	EXPECT_TRUE(LandsOnRelease({.raised = true, .computerVillager = true, .dryLand = true}));
	// Over the shore, a cell higher than 1 is enough
	EXPECT_TRUE(LandsOnRelease({.nearestAltitude = 2}));
	EXPECT_FALSE(LandsOnRelease({.nearestAltitude = 1}));
	EXPECT_FALSE(LandsOnRelease({}));
	// People and fences need gentle ground
	EXPECT_FALSE(LandsOnRelease({.dryLand = true, .needsGentleSlope = true, .normalY = 0.69f}));
	EXPECT_TRUE(LandsOnRelease({.dryLand = true, .needsGentleSlope = true, .normalY = 0.7f}));
}

TEST(HandGrab, PeopleFencesAndUprightTreesLeaveThePhysicsOnLanding)
{
	EXPECT_EQ(OutcomeOfLanding({.living = true}), LandedOutcome::LeavesPhysics);
	EXPECT_EQ(OutcomeOfLanding({.fence = true}), LandedOutcome::LeavesPhysics);
	EXPECT_EQ(OutcomeOfLanding({.tree = true, .onLand = true}), LandedOutcome::LeavesPhysics);
	EXPECT_EQ(OutcomeOfLanding({.tree = true, .onLand = true, .tiltX = 0.21f}), LandedOutcome::Falls);
	EXPECT_EQ(OutcomeOfLanding({.tree = true, .onLand = true, .dontReplant = true}), LandedOutcome::Falls);
	// A creature's put-down tree isn't asked its tilt
	EXPECT_EQ(OutcomeOfLanding({.tree = true, .onLand = true, .tiltZ = 1.0f, .byCreature = true}),
	          LandedOutcome::LeavesPhysics);
	EXPECT_EQ(OutcomeOfLanding({.tree = true, .burning = true, .onLand = true}), LandedOutcome::Settles);
	EXPECT_EQ(OutcomeOfLanding({.tree = true}), LandedOutcome::Settles);
	EXPECT_EQ(OutcomeOfLanding({}), LandedOutcome::Settles);
}

TEST(HandGrab, AScoopRampsUpOverItsTimeAsASquare)
{
	const ScoopFacts facts {.initial = 25, .perTurn = 8, .perTurnEnd = 70, .maxPickedUp = 20000, .rampSeconds = 6.0f};
	EXPECT_EQ(ScoopAmount(0, facts), 8u);
	// Half way through its 60 turns, a quarter of the rise
	EXPECT_EQ(ScoopAmount(30, facts), 8u + 62u / 4u);
	EXPECT_EQ(ScoopAmount(60, facts), 70u);
	EXPECT_EQ(ScoopAmount(600, facts), 70u);
	EXPECT_FLOAT_EQ(ScoopRamp(30, facts), 0.25f);
	// No more than the source has, nor than the handful has room for
	EXPECT_EQ(ScoopTaken(70, 10, 0, facts), 10u);
	EXPECT_EQ(ScoopTaken(70, 1000, 19990, facts), 10u);
	EXPECT_EQ(ScoopTaken(70, 1000, 30000, facts), 0u);
}

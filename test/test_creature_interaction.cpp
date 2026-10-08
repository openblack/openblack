/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <array>
#include <numeric>
#include <vector>

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <gtest/gtest.h>

#include "3D/SkeletalAnimation.h"
#include "Creature/CreatureDesires.h"
#include "Creature/CreatureFeedback.h"
#include "Creature/CreatureIdleMind.h"
#include "Creature/CreatureObjectActions.h"
#include "Creature/CreatureReach.h"
#include "Creature/CreatureThrow.h"

using namespace openblack;

namespace
{
constexpr float k_Tolerance = 1e-4f;

/// Four made-up reach corners: back left, back right, front left, front right, the left being +x
creature_reach::Points FakeReach()
{
	return {glm::vec3(2.0f, 3.0f, -1.0f), glm::vec3(-2.0f, 3.0f, -1.0f), glm::vec3(2.0f, 2.0f, -7.0f),
	        glm::vec3(-2.0f, 2.0f, -7.0f)};
}

float Sum(const std::array<float, 4>& weights)
{
	return std::accumulate(weights.begin(), weights.end(), 0.0f);
}

/// The same random number every time
uint32_t First(uint32_t /*range*/)
{
	return 0;
}
} // namespace

TEST(CreatureReach, ACornerTakesAllTheWeight)
{
	const auto points = FakeReach();
	for (size_t i = 0; i < points.size(); ++i)
	{
		const auto blend = creature_reach::Solve(points, points.at(i));
		EXPECT_TRUE(blend.inRange);
		for (size_t j = 0; j < points.size(); ++j)
		{
			EXPECT_NEAR(blend.weights.at(j), i == j ? 1.0f : 0.0f, k_Tolerance) << i << " " << j;
		}
	}
}

TEST(CreatureReach, TheMiddleBlendsAllFourEvenly)
{
	const auto blend = creature_reach::Solve(FakeReach(), glm::vec3(0.0f, 0.0f, -4.0f));
	EXPECT_TRUE(blend.inRange);
	for (const auto weight : blend.weights)
	{
		EXPECT_NEAR(weight, 0.25f, k_Tolerance);
	}
}

TEST(CreatureReach, StretchingPastTheCornersExtrapolatesThenFails)
{
	const auto points = FakeReach();
	// Half again as far forward as the front corners: still in reach, the back animations counting against
	const auto stretched = creature_reach::Solve(points, glm::vec3(0.0f, 0.0f, -10.0f));
	EXPECT_TRUE(stretched.inRange);
	EXPECT_NEAR(stretched.backToFront, 1.5f, k_Tolerance);
	EXPECT_NEAR(Sum(stretched.weights), 1.0f, k_Tolerance);
	EXPECT_LT(stretched.weights.at(0), 0.0f);
	// Past 1.8 of the depth, or more than half the width beyond either side, it can't reach
	EXPECT_FALSE(creature_reach::Solve(points, glm::vec3(0.0f, 0.0f, -12.0f)).inRange);
	EXPECT_FALSE(creature_reach::Solve(points, glm::vec3(4.5f, 0.0f, -4.0f)).inRange);
	EXPECT_FALSE(creature_reach::Solve(points, glm::vec3(0.0f, 0.0f, 4.0f)).inRange);
}

TEST(CreatureReach, HeightDoesNotMatter)
{
	const auto low = creature_reach::Solve(FakeReach(), glm::vec3(1.0f, 0.0f, -3.0f));
	const auto high = creature_reach::Solve(FakeReach(), glm::vec3(1.0f, 50.0f, -3.0f));
	for (size_t i = 0; i < low.weights.size(); ++i)
	{
		EXPECT_NEAR(low.weights.at(i), high.weights.at(i), k_Tolerance);
	}
}

TEST(CreatureReach, TheOtherSideIsReachedMirrored)
{
	// Corners biased to the right (-x), as a right hand's are
	auto points = FakeReach();
	for (auto& point : points)
	{
		point.x -= 1.0f;
	}
	EXPECT_FALSE(creature_reach::ReachesMirrored(points, glm::vec3(-2.0f, 0.0f, -4.0f)));
	EXPECT_TRUE(creature_reach::ReachesMirrored(points, glm::vec3(2.0f, 0.0f, -4.0f)));
	const auto mirrored = creature_reach::Mirrored(points);
	for (size_t i = 0; i < points.size(); ++i)
	{
		EXPECT_FLOAT_EQ(mirrored.at(i).x, -points.at(i).x);
		EXPECT_FLOAT_EQ(mirrored.at(i).z, points.at(i).z);
	}
	// A point and its reflection blend the same on their own sides
	const auto right = creature_reach::Solve(points, glm::vec3(-1.5f, 0.0f, -3.0f));
	const auto left = creature_reach::Solve(mirrored, glm::vec3(1.5f, 0.0f, -3.0f));
	for (size_t i = 0; i < right.weights.size(); ++i)
	{
		EXPECT_NEAR(right.weights.at(i), left.weights.at(i), k_Tolerance);
	}
}

TEST(CreatureReach, MaxReachIsTheFrontStretchedAsFarAsItGoes)
{
	// -0.4 of the back pair and 0.9 of the front pair: z = 0.8 - 12.6
	EXPECT_NEAR(creature_reach::MaxReach(FakeReach()), 11.8f, k_Tolerance);
	EXPECT_NEAR(creature_reach::Centre(FakeReach()).z, -4.0f, k_Tolerance);
}

TEST(CreatureThrow, FlightTimeIsTheTimeToFallTheDistance)
{
	EXPECT_NEAR(creature_throw::FlightTime(9.81f), 1.0f, k_Tolerance);
	EXPECT_NEAR(creature_throw::FlightTime(4.0f * 9.81f), 2.0f, k_Tolerance);
	EXPECT_EQ(creature_throw::FlightTime(-1.0f), 0.0f);
}

TEST(CreatureThrow, TheReleaseVelocityLandsOnTheTarget)
{
	const glm::vec3 release {1.0f, 12.0f, -3.0f};
	const glm::vec3 target {20.0f, 0.0f, -60.0f};
	const auto seconds = creature_throw::FlightTime(glm::distance(release, target));
	const auto velocity = creature_throw::ReleaseVelocity(target, release, seconds);
	// Gravity's half g t squared is made up in the upward speed
	EXPECT_NEAR(velocity.y, (target.y - release.y) / seconds + 0.5f * creature_throw::k_Gravity * seconds, k_Tolerance);
	// Its curve reaches the target at that time
	const auto reached =
	    release + velocity * seconds + glm::vec3(0.0f, -0.5f * creature_throw::k_Gravity * seconds * seconds, 0.0f);
	EXPECT_NEAR(reached.x, target.x, 1e-3f);
	EXPECT_NEAR(reached.y, target.y, 1e-3f);
	EXPECT_NEAR(reached.z, target.z, 1e-3f);
}

TEST(CreatureThrow, ItThrowsAtNothingTooClose)
{
	// Two thirds of its height, 15 units at size 1
	EXPECT_FALSE(creature_throw::FarEnoughToThrow(9.8f, 1.0f));
	EXPECT_TRUE(creature_throw::FarEnoughToThrow(10.0f, 1.0f));
	EXPECT_FALSE(creature_throw::FarEnoughToThrow(19.0f, 2.0f));
}

TEST(CreatureThrow, HighTargetsBlendInTheHighThrow)
{
	EXPECT_FLOAT_EQ(creature_throw::HighThrowWeight(0.5f, 0.5f, 1.5f), 0.0f);
	EXPECT_FLOAT_EQ(creature_throw::HighThrowWeight(1.0f, 0.5f, 1.5f), 0.5f);
	EXPECT_FLOAT_EQ(creature_throw::HighThrowWeight(3.0f, 0.5f, 1.5f), 1.0f);
	EXPECT_FLOAT_EQ(creature_throw::HighThrowWeight(-2.0f, 0.5f, 1.5f), 0.0f);
	EXPECT_FLOAT_EQ(creature_throw::HighThrowWeight(1.0f, 0.5f, 0.5f), 0.0f);
}

TEST(CreatureThrow, TossingKeepsSomeOfTheHandsSpeed)
{
	// The hand moved 1 unit ahead in the last 100 ms: 10 units a second
	const auto hand = creature_throw::HandVelocity(glm::vec3(0.0f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, -1.0f), 100.0f);
	EXPECT_NEAR(hand.z, -10.0f, k_Tolerance);
	const auto plain = creature_throw::TossVelocity(glm::vec3(3.0f, 1.0f, -10.0f), false, glm::mat3(1.0f), 0.6f);
	EXPECT_NEAR(plain.x, 1.8f, k_Tolerance);
	EXPECT_NEAR(plain.z, -6.0f, k_Tolerance);
	// Tossed with the other hand it goes the other way sideways; turned with the creature
	const auto mirrored = creature_throw::TossVelocity(glm::vec3(3.0f, 1.0f, -10.0f), true, glm::mat3(1.0f), 0.6f);
	EXPECT_NEAR(mirrored.x, -1.8f, k_Tolerance);
	const auto turned = creature_throw::TossVelocity(glm::vec3(0.0f, 0.0f, -10.0f), false,
	                                                 glm::mat3(glm::eulerAngleY(glm::half_pi<float>())), 1.0f);
	EXPECT_NEAR(turned.x, -10.0f, k_Tolerance);
	EXPECT_NEAR(turned.z, 0.0f, k_Tolerance);
}

TEST(CreatureFeedback, AStrokeLandsOnTheNearestPart)
{
	std::array<glm::vec3, creature_feedback::k_BodyPartCount> parts {};
	for (size_t i = 0; i < parts.size(); ++i)
	{
		parts.at(i) = glm::vec3(static_cast<float>(i) * 10.0f, 0.0f, 0.0f);
	}
	EXPECT_EQ(creature_feedback::NearestPart(glm::vec3(1.0f, 2.0f, 0.0f), parts), creature_feedback::BodyPart::Head);
	EXPECT_EQ(creature_feedback::NearestPart(glm::vec3(31.0f, 0.0f, 0.0f), parts), creature_feedback::BodyPart::Belly);
	EXPECT_EQ(creature_feedback::NearestPart(glm::vec3(200.0f, 0.0f, 0.0f), parts), creature_feedback::BodyPart::LeftHand);
	// The left side's rewards are the right's mirrored
	EXPECT_EQ(creature_feedback::k_RewardAnimations.at(1), creature_feedback::k_RewardAnimations.at(2));
	EXPECT_TRUE(creature_feedback::k_RewardMirrored.at(2));
	EXPECT_FALSE(creature_feedback::k_RewardMirrored.at(1));
}

TEST(CreatureFeedback, StrokesNeedANewPartAndTime)
{
	using creature_feedback::BodyPart;
	EXPECT_TRUE(creature_feedback::StrokeDue(std::nullopt, BodyPart::Head, 2000.0f));
	EXPECT_FALSE(creature_feedback::StrokeDue(BodyPart::Head, BodyPart::Head, 5000.0f));
	EXPECT_FALSE(creature_feedback::StrokeDue(BodyPart::Head, BodyPart::Belly, 1999.0f));
	EXPECT_TRUE(creature_feedback::StrokeDue(BodyPart::Head, BodyPart::Belly, 2000.0f));
}

TEST(CreatureFeedback, SlapsAreClassedByHeightAndSpeed)
{
	constexpr float k_Height = 15.0f;
	// Too slow, or too high or under the ground, is no slap
	EXPECT_FALSE(creature_feedback::ClassifySlap(7.0f, 5.0f * k_Height, k_Height, false).has_value());
	EXPECT_FALSE(creature_feedback::ClassifySlap(17.0f, 100.0f, k_Height, false).has_value());
	EXPECT_FALSE(creature_feedback::ClassifySlap(-1.0f, 100.0f, k_Height, false).has_value());
	// Feet, waist and head, gently below nine heights a second
	const auto feet = creature_feedback::ClassifySlap(3.0f, 100.0f, k_Height, false);
	ASSERT_TRUE(feet.has_value());
	EXPECT_EQ(feet->animation, 175u);
	EXPECT_TRUE(feet->gentle);
	const auto waist = creature_feedback::ClassifySlap(9.0f, 200.0f, k_Height, true);
	ASSERT_TRUE(waist.has_value());
	EXPECT_EQ(waist->animation, 167u);
	EXPECT_FALSE(waist->gentle);
	EXPECT_TRUE(waist->mirrored);
	const auto head = creature_feedback::ClassifySlap(12.0f, 200.0f, k_Height, false);
	ASSERT_TRUE(head.has_value());
	EXPECT_EQ(head->animation, 165u);
}

TEST(CreatureFeedback, StrokesAndSlapsAddUp)
{
	auto sum = 0.0f;
	for (int i = 0; i < 3; ++i)
	{
		sum = creature_feedback::AfterStroke(sum);
	}
	EXPECT_NEAR(sum, 0.3f, k_Tolerance);
	// Enjoying it, a slap takes twice as much off
	sum = creature_feedback::AfterSlap(sum, true);
	EXPECT_NEAR(sum, 0.1f, k_Tolerance);
	sum = creature_feedback::AfterSlap(sum, false);
	EXPECT_NEAR(sum, -0.1f, k_Tolerance);
	for (int i = 0; i < 20; ++i)
	{
		sum = creature_feedback::AfterSlap(sum, false);
	}
	EXPECT_FLOAT_EQ(sum, -1.0f);
	for (int i = 0; i < 40; ++i)
	{
		sum = creature_feedback::AfterStroke(sum);
	}
	EXPECT_FLOAT_EQ(sum, 1.0f);
	EXPECT_FLOAT_EQ(creature_feedback::Delivered(1.5f), 1.0f);
	EXPECT_NEAR(creature_feedback::AttitudeAfter(0.5f, 1.0f), 0.4f, k_Tolerance);
	EXPECT_NEAR(creature_feedback::AverageAfter(0.5f, -1.0f), -0.4f, k_Tolerance);
}

TEST(CreatureFeedback, TheHandTouchesTheBodyWhereTheLineOfSightMeetsIt)
{
	const std::array<creature_feedback::Capsule, 2> body {{
	    {.from = glm::vec3(0.0f, 0.0f, 0.0f), .to = glm::vec3(0.0f, 10.0f, 0.0f), .radius = 1.0f},
	    {.from = glm::vec3(5.0f, 0.0f, 0.0f), .to = glm::vec3(5.0f, 10.0f, 0.0f), .radius = 1.0f},
	}};
	const auto hit = creature_feedback::RayHit(glm::vec3(0.0f, 5.0f, 20.0f), glm::vec3(0.0f, 0.0f, -2.0f), body);
	ASSERT_TRUE(hit.has_value());
	// It enters the near side of the first capsule, 19 units along: 9.5 lengths of the direction
	EXPECT_NEAR(*hit, 9.5f, k_Tolerance);
	EXPECT_FALSE(creature_feedback::RayHit(glm::vec3(2.5f, 5.0f, 20.0f), glm::vec3(0.0f, 0.0f, -1.0f), body).has_value());
	EXPECT_FALSE(creature_feedback::RayHit(glm::vec3(0.0f, 5.0f, 20.0f), glm::vec3(0.0f, 0.0f, 1.0f), body).has_value());
}

TEST(CreatureFeedback, TheBodyIsACapsuleFromEachJointToItsParentsPlaced)
{
	// A root and two bones hanging off it, the last off the first, its own parent out of range
	const std::array<uint32_t, 3> parents {0xFFFFFFFF, 0, 1};
	const std::array<glm::mat4, 3> bones {
	    glm::translate(glm::vec3(0.0f, 1.0f, 0.0f)),
	    glm::translate(glm::vec3(0.0f, 3.0f, 0.0f)) * glm::eulerAngleY(0.5f),
	    glm::translate(glm::vec3(2.0f, 3.0f, 0.0f)),
	};
	const auto placement = glm::translate(glm::vec3(10.0f, 0.0f, -4.0f)) * glm::scale(glm::vec3(2.0f));
	const auto capsules = creature_feedback::BodyCapsules(parents, bones, placement, 0.5f);
	ASSERT_EQ(capsules.size(), 3u);
	// The root's capsule is only the point where it is
	EXPECT_EQ(capsules[0].from, glm::vec3(10.0f, 2.0f, -4.0f));
	EXPECT_EQ(capsules[0].to, glm::vec3(10.0f, 2.0f, -4.0f));
	EXPECT_EQ(capsules[1].from, glm::vec3(10.0f, 2.0f, -4.0f));
	EXPECT_EQ(capsules[1].to, glm::vec3(10.0f, 6.0f, -4.0f));
	EXPECT_EQ(capsules[2].from, glm::vec3(10.0f, 6.0f, -4.0f));
	EXPECT_EQ(capsules[2].to, glm::vec3(14.0f, 6.0f, -4.0f));
	for (const auto& capsule : capsules)
	{
		EXPECT_FLOAT_EQ(capsule.radius, 0.5f);
	}
	// Each joint is where its whole posed matrix puts the bone's origin
	EXPECT_EQ(capsules[1].to, glm::vec3((placement * bones[1])[3]));
}

TEST(CreatureObjectActions, TheTownFearsThrowingAndEatingVillagers)
{
	using creature_object_actions::AttitudeTo;
	using creature_object_actions::Kind;
	using creature_object_actions::TownAttitude;
	EXPECT_EQ(AttitudeTo(Kind::Throw, false, false), TownAttitude::Fear);
	EXPECT_EQ(AttitudeTo(Kind::Eat, false, false), TownAttitude::None);
	EXPECT_EQ(AttitudeTo(Kind::Eat, true, false), TownAttitude::Fear);
	EXPECT_EQ(AttitudeTo(Kind::Keep, false, true), TownAttitude::Fear);
	EXPECT_EQ(AttitudeTo(Kind::PickUp, false, false), TownAttitude::None);
	EXPECT_FLOAT_EQ(creature_object_actions::AttitudeSeconds(TownAttitude::Fear), 30.0f);
	EXPECT_FLOAT_EQ(creature_object_actions::AttitudeSeconds(TownAttitude::Respect), 10.0f);
	EXPECT_EQ(creature_object_actions::HandsFor(Kind::PickUp), creature_object_actions::Hands::Empty);
	EXPECT_EQ(creature_object_actions::HandsFor(Kind::Throw), creature_object_actions::Hands::Holding);
}

TEST(CreatureDesires, FeedbackPushesEverySourceOfAType)
{
	creature_desires::Desires desires;
	desires[creature_desires::Desire::Play].sources = {{.type = 12, .value = 0.2f, .threshold = 0.5f, .multiplier = 1.0f}};
	desires[creature_desires::Desire::Anger].sources = {{.type = 9, .value = 0.9f, .threshold = 0.5f, .multiplier = 1.0f}};
	creature_desires::ChangeSource(desires, 12, 0.5f);
	creature_desires::ChangeSource(desires, 9, 0.5f);
	EXPECT_NEAR(desires[creature_desires::Desire::Play].sources.front().value, 0.7f, k_Tolerance);
	EXPECT_FLOAT_EQ(desires[creature_desires::Desire::Anger].sources.front().value, 1.0f);
}

TEST(CreatureIdleMind, ACuriousCreatureLooksSomethingOver)
{
	creature_mind::Wants wants {.curiosity = 0.5f, .object = 7u};
	const auto plan = creature_mind::ChooseObjectActivity(wants, First);
	ASSERT_TRUE(plan.has_value());
	EXPECT_EQ(plan->activity, creature_mind::Activity::Examine);
	ASSERT_EQ(plan->agenda.size(), 3u);
	EXPECT_EQ(plan->agenda[0].order.kind, creature_mind::ObjectOrder::Kind::PickUp);
	EXPECT_EQ(plan->agenda[0].order.object, 7u);
	EXPECT_EQ(plan->agenda[1].order.kind, creature_mind::ObjectOrder::Kind::Keep);
	EXPECT_EQ(plan->agenda[1].order.animation, creature_object_actions::k_FirstKeepAnimation);
	// rand(10) of 0 tosses it away rather than putting it down
	EXPECT_EQ(plan->agenda[2].order.kind, creature_mind::ObjectOrder::Kind::Discard);
	// Not curious enough, or nothing at hand, it doesn't
	wants.curiosity = 0.1f;
	EXPECT_FALSE(creature_mind::ChooseObjectActivity(wants, First).has_value());
	wants = {.anger = 0.9f, .object = 7u};
	EXPECT_FALSE(creature_mind::ChooseObjectActivity(wants, First).has_value());
	wants.hurlTarget = glm::vec2(10.0f, 20.0f);
	const auto hurl = creature_mind::ChooseObjectActivity(wants, First);
	ASSERT_TRUE(hurl.has_value());
	EXPECT_EQ(hurl->activity, creature_mind::Activity::Hurl);
	EXPECT_EQ(hurl->agenda.back().order.kind, creature_mind::ObjectOrder::Kind::Throw);
	// Holding something it has no use for, it puts it down
	const auto putDown = creature_mind::ChooseObjectActivity({.holding = true}, First);
	ASSERT_TRUE(putDown.has_value());
	EXPECT_EQ(putDown->activity, creature_mind::Activity::PutDown);
}

TEST(CreatureIdleMind, EatingIsPickingUpExaminingAndEating)
{
	const auto agenda = creature_mind::Eat(3u);
	ASSERT_EQ(agenda.size(), 3u);
	EXPECT_EQ(agenda[0].order.kind, creature_mind::ObjectOrder::Kind::PickUp);
	EXPECT_EQ(agenda[1].order.animation, creature_object_actions::k_ExamineObject);
	EXPECT_EQ(agenda[2].order.kind, creature_mind::ObjectOrder::Kind::Eat);
	EXPECT_EQ(agenda[2].effect, creature_mind::Effect::Eat);
}

TEST(CreatureIdleMind, AnObjectStepWaitsForTheHandsAndGivesUpWhenTheyFail)
{
	creature_mind::IdleMind mind;
	creature_mind::Plan(mind, creature_mind::Activity::Examine, creature_mind::ExamineByPickingUp(4u, First));
	creature_mind::Senses senses {.seconds = 0.1f};
	auto commands = creature_mind::Think(mind, senses, First);
	ASSERT_TRUE(commands.object.has_value());
	EXPECT_EQ(commands.object->kind, creature_mind::ObjectOrder::Kind::PickUp);
	senses.hands = creature_mind::HandsState::Busy;
	commands = creature_mind::Think(mind, senses, First);
	EXPECT_EQ(mind.step, 0u);
	senses.hands = creature_mind::HandsState::Done;
	commands = creature_mind::Think(mind, senses, First);
	EXPECT_EQ(mind.step, 1u);
	// The next step starts, then the hands fail: the rest is given up
	commands = creature_mind::Think(mind, senses, First);
	ASSERT_TRUE(commands.object.has_value());
	EXPECT_EQ(commands.object->kind, creature_mind::ObjectOrder::Kind::Keep);
	senses.hands = creature_mind::HandsState::Failed;
	commands = creature_mind::Think(mind, senses, First);
	EXPECT_GE(mind.step, mind.agenda.size());
}

TEST(CreatureIdleMind, ThrowingAboutAimsFromWhereItStands)
{
	creature_mind::IdleMind mind;
	creature_mind::Plan(mind, creature_mind::Activity::PlayWithObject, creature_mind::ThrowAbout(4u, First));
	mind.step = 1;
	const creature_mind::Senses senses {.seconds = 0.1f, .position = glm::vec2(100.0f, 200.0f)};
	const auto commands = creature_mind::Think(mind, senses, First);
	ASSERT_TRUE(commands.object.has_value());
	EXPECT_EQ(commands.object->kind, creature_mind::ObjectOrder::Kind::Throw);
	// 30 units away at an angle of 0
	EXPECT_NEAR(commands.object->point.x, 130.0f, k_Tolerance);
	EXPECT_NEAR(commands.object->point.y, 200.0f, k_Tolerance);
}

TEST(SkeletalMirror, AMirroredPoseIsTheExactReflection)
{
	// A middle bone and a pair of arms whose rest frames are not mirror images of each other
	const std::array<glm::mat4, 3> rest {
	    glm::mat4(1.0f),
	    glm::translate(glm::vec3(2.0f, 1.0f, 0.0f)) * glm::eulerAngleZ(0.4f),
	    glm::translate(glm::vec3(-2.0f, 1.0f, 0.0f)) * glm::eulerAngleX(1.1f),
	};
	const std::array<uint32_t, 3> parents {skeletal_animation::k_NoParent, 0, 0};
	const auto skeleton = skeletal_animation::Skeleton::FromRestMatrices(parents, rest);
	const std::vector<uint32_t> mirror {0, 2, 1};
	std::vector<skeletal_animation::Pose> poses {
	    {.rotation = skeletal_animation::RotationYXZ({0.1f, 0.3f, -0.2f}), .translation = glm::vec3(0.5f, 3.0f, 1.0f)},
	    {.rotation = skeletal_animation::RotationYXZ({0.7f, -0.4f, 0.9f}), .translation = glm::vec3(2.0f, 1.0f, 0.0f)},
	    {.rotation = skeletal_animation::RotationYXZ({-0.2f, 0.5f, 0.1f}), .translation = glm::vec3(-2.0f, 1.0f, 0.0f)},
	};
	const auto plain = skeletal_animation::ComposeBoneMatrices(poses, parents);
	const auto mirrored =
	    skeletal_animation::ComposeBoneMatrices(skeletal_animation::MirrorPoses(poses, skeleton, mirror), parents);
	const glm::mat4 reflect = glm::scale(glm::vec3(-1.0f, 1.0f, 1.0f));
	for (size_t bone = 0; bone < 3; ++bone)
	{
		// Each bone stands where its mirror bone stood, reflected, and bends away from its rest pose the same way
		const auto& other = plain[mirror[bone]];
		const auto place = glm::vec3(mirrored[bone][3]);
		EXPECT_NEAR(place.x, -other[3].x, k_Tolerance);
		EXPECT_NEAR(place.y, other[3].y, k_Tolerance);
		EXPECT_NEAR(place.z, other[3].z, k_Tolerance);
		const auto bend = glm::mat3(mirrored[bone]) * glm::inverse(glm::mat3(rest[bone]));
		const auto expected =
		    glm::mat3(reflect) * glm::mat3(other) * glm::inverse(glm::mat3(rest[mirror[bone]])) * glm::mat3(reflect);
		for (glm::length_t c = 0; c < 3; ++c)
		{
			for (glm::length_t r = 0; r < 3; ++r)
			{
				EXPECT_NEAR(bend[c][r], expected[c][r], 1e-3f) << bone;
			}
		}
	}
}

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
#include <numbers>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <gtest/gtest.h>

#include "3D/SkeletalAnimation.h"
#include "Creature/CreatureLayers.h"

using namespace openblack;
using namespace openblack::creature_layers;
using skeletal_animation::Animation;
using skeletal_animation::Matrix;
using skeletal_animation::Pose;

namespace
{
constexpr float k_Tolerance = 1e-4f;
constexpr float k_Pi = std::numbers::pi_v<float>;

void ExpectMatrixNear(const Matrix& a, const Matrix& b)
{
	for (size_t r = 0; r < 3; ++r)
	{
		for (size_t c = 0; c < 3; ++c)
		{
			EXPECT_NEAR(a.at(r).at(c), b.at(r).at(c), k_Tolerance) << "row " << r << " column " << c;
		}
	}
}

/// A skeleton of a root with a bone either side of it, all with no rest rotation
skeletal_animation::Skeleton Symmetric(std::array<glm::mat4, 3>& rest)
{
	rest = {glm::mat4(1.0f), glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 0.0f)),
	        glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 2.0f, 0.0f))};
	const std::array<uint32_t, 3> parents {skeletal_animation::k_NoParent, 0, 0};
	return skeletal_animation::Skeleton::FromRestMatrices(parents, rest);
}

/// A still animation of one keyframe per entry, moving the given bones
Animation Frames(std::vector<uint32_t> rotated, std::vector<std::vector<glm::vec3>> eulers,
                 std::vector<uint32_t> translated = {}, std::vector<std::vector<glm::vec3>> translations = {},
                 uint32_t duration = 1000)
{
	Animation animation {.duration = duration,
	                     .looping = true,
	                     .rotatedJoints = std::move(rotated),
	                     .translatedJoints = std::move(translated),
	                     .frames = {}};
	for (size_t i = 0; i < eulers.size(); ++i)
	{
		animation.frames.push_back(
		    {.eulerAngles = eulers[i], .translations = i < translations.size() ? translations[i] : std::vector<glm::vec3> {}});
	}
	return animation;
}
} // namespace

TEST(CreatureLayers, BiggerCreaturesPlayMoreSlowly)
{
	EXPECT_NEAR(PlaybackRate(0.0f), 1.6f, k_Tolerance);
	EXPECT_NEAR(PlaybackRate(1.0f), 1.175f, k_Tolerance);
	EXPECT_NEAR(PlaybackRate(2.0f), 0.75f, k_Tolerance);
}

TEST(CreatureLayers, AnActionPlaysOnceThenTheBodyStands)
{
	const BodyAction stand {};
	EXPECT_FALSE(IsPlaying(stand));
	EXPECT_EQ(CurrentAnimation(stand), animations::k_Stand);

	auto body = PlayOnce(stand, animations::k_Tired, true);
	ASSERT_TRUE(body.has_value());
	EXPECT_TRUE(body->mirrored);
	EXPECT_EQ(CurrentAnimation(*body), animations::k_Tired);
	// Nothing else starts while it plays
	EXPECT_FALSE(PlayOnce(*body, animations::k_Happy, false).has_value());
	EXPECT_FALSE(PlaySequence(*body, 1, 2, 3).has_value());

	auto played = AdvanceBody(*body, 600.0f, 1000);
	EXPECT_TRUE(IsPlaying(played));
	EXPECT_NEAR(played.timeMs, 600.0f, k_Tolerance);
	played = AdvanceBody(played, 400.0f, 1000);
	EXPECT_FALSE(IsPlaying(played));
	EXPECT_FALSE(played.mirrored);
}

TEST(CreatureLayers, AMissingAnimationEndsAtOnce)
{
	const auto body = PlayOnce({}, animations::k_Tired, false);
	ASSERT_TRUE(body.has_value());
	EXPECT_FALSE(IsPlaying(AdvanceBody(*body, 1.0f, std::nullopt)));
}

TEST(CreatureLayers, ASitStartsLoopsUntilToldThenEnds)
{
	auto body = *PlaySequence({}, animations::k_StartSit, animations::k_Sit, animations::k_EndSit);
	EXPECT_EQ(CurrentAnimation(body), animations::k_StartSit);
	body = AdvanceBody(body, 500.0f, 500);
	EXPECT_TRUE(IsLooping(body));
	EXPECT_EQ(CurrentAnimation(body), animations::k_Sit);
	EXPECT_NEAR(body.timeMs, 0.0f, k_Tolerance);
	// The loop wraps round for as long as it isn't told to end
	body = AdvanceBody(body, 700.0f, 400);
	EXPECT_TRUE(IsLooping(body));
	EXPECT_NEAR(body.timeMs, 300.0f, k_Tolerance);
	body = AdvanceBody(EndLoop(body), 10.0f, 400);
	EXPECT_EQ(CurrentAnimation(body), animations::k_EndSit);
	body = AdvanceBody(body, 800.0f, 800);
	EXPECT_FALSE(IsPlaying(body));
}

TEST(CreatureLayers, ABodyTimedByWhatPlaysItStaysWherePut)
{
	// A fight sets its fighter's move at the move's own time each frame; the frame mustn't move it on as well, or the
	// time shown would run ahead by however long each frame took, and go back when a frame takes less
	const BodyAction move {
	    .kind = BodyAction::Kind::Sequence,
	    .phase = BodyAction::Phase::Loop,
	    .animations = {120, 120, 120},
	    .timeMs = 70.0f,
	    .timedByPlayer = true,
	};
	for (const auto milliseconds : {0.0f, 1.0f, 16.0f, 700.0f})
	{
		const auto played = AdvanceBody(move, milliseconds, 500);
		EXPECT_EQ(played.phase, BodyAction::Phase::Loop);
		EXPECT_FLOAT_EQ(played.timeMs, 70.0f);
	}
}

TEST(CreatureLayers, AFaceRunsBackBeforeTheNextStarts)
{
	auto face = PullFace({}, 16, 5000.0f);
	face = AdvanceFace(face, 100.0f, std::nullopt);
	EXPECT_EQ(face.current, 16u);
	face = AdvanceFace(face, 300.0f, 1000);
	EXPECT_NEAR(face.timeMs, 300.0f, k_Tolerance);
	// Another face is pulled: the smile runs back to its start at full speed, then the growl starts
	face = PullFace(face, 18, 5000.0f);
	face = AdvanceFace(face, 200.0f, 1000);
	EXPECT_EQ(face.current, 16u);
	EXPECT_NEAR(face.timeMs, 100.0f, k_Tolerance);
	face = AdvanceFace(face, 200.0f, 1000);
	EXPECT_EQ(face.current, 18u);
	EXPECT_NEAR(face.timeMs, 0.0f, k_Tolerance);
	// Relaxed: it runs back a quarter as fast
	face = AdvanceFace(face, 400.0f, 1000);
	face = RelaxFace(face);
	face = AdvanceFace(face, 400.0f, 1000);
	EXPECT_EQ(face.current, 18u);
	EXPECT_NEAR(face.timeMs, 300.0f, k_Tolerance);
	face = AdvanceFace(face, 1200.0f, 1000);
	EXPECT_FALSE(face.current.has_value());
}

TEST(CreatureLayers, AFaceHoldsItsExpressionRatherThanLooping)
{
	auto face = PullFace({}, 20, 3000.0f);
	face = AdvanceFace(face, 10.0f, 250);
	// Played through, it holds its last frame without starting over: the mouth doesn't open and close again
	for (int frame = 0; frame < 100; ++frame)
	{
		face = AdvanceFace(face, 16.0f, 250);
		EXPECT_EQ(face.current, 20u);
		if (frame > 16)
		{
			EXPECT_NEAR(face.timeMs, 250.0f, k_Tolerance);
		}
	}
}

TEST(CreatureLayers, AFaceRelaxesWhenItsTimeIsUp)
{
	auto face = PullFace({}, 21, 1000.0f, creature_face::Cue::Sad);
	EXPECT_EQ(face.cue, creature_face::Cue::Sad);
	face = AdvanceFace(face, 1.0f, 250);
	face = AdvanceFace(face, 500.0f, 250);
	EXPECT_NEAR(face.timeMs, 250.0f, k_Tolerance);
	face = AdvanceFace(face, 600.0f, 250);
	// Let go of after a second, it runs back at a quarter speed, 150 ms of its expression in the last 600 ms
	EXPECT_FALSE(face.wanted.has_value());
	EXPECT_EQ(face.cue, creature_face::Cue::None);
	EXPECT_EQ(face.current, 21u);
	EXPECT_NEAR(face.timeMs, 100.0f, k_Tolerance);
	face = AdvanceFace(face, 200.0f, 250);
	EXPECT_NEAR(face.timeMs, 50.0f, k_Tolerance);
	face = AdvanceFace(face, 1000.0f, 250);
	EXPECT_FALSE(face.current.has_value());
}

TEST(CreatureLayers, AFacePulledWithoutATimeIsHeldForAnHour)
{
	auto face = PullFace({}, 18, 0.0f);
	face = AdvanceFace(face, 30.0f * 60.0f * 1000.0f, 250);
	EXPECT_EQ(face.wanted, 18u);
	face = AdvanceFace(face, 31.0f * 60.0f * 1000.0f, 250);
	EXPECT_FALSE(face.wanted.has_value());
}

TEST(CreatureLayers, PullingTheHeldFaceAgainKeepsHoldingIt)
{
	auto face = PullFace({}, 16, 500.0f);
	face = AdvanceFace(face, 1.0f, 250);
	face = AdvanceFace(face, 400.0f, 250);
	face = PullFace(face, 16, 500.0f);
	face = AdvanceFace(face, 400.0f, 250);
	EXPECT_EQ(face.wanted, 16u);
	EXPECT_EQ(face.current, 16u);
	EXPECT_NEAR(face.timeMs, 250.0f, k_Tolerance);
}

TEST(CreatureLayers, AFaceTheSpeciesLacksStaysAtItsStart)
{
	auto face = PullFace({}, 26, 1000.0f);
	face = AdvanceFace(face, 1.0f, std::nullopt);
	face = AdvanceFace(face, 100.0f, std::nullopt);
	EXPECT_EQ(face.current, 26u);
	EXPECT_NEAR(face.timeMs, 0.0f, k_Tolerance);
}

TEST(CreatureLayers, AGesturePlaysOnce)
{
	auto gesture = PlayGesture({}, animations::k_Yawn);
	ASSERT_TRUE(gesture.has_value());
	EXPECT_FALSE(PlayGesture(*gesture, animations::k_Yawn).has_value());
	auto played = AdvanceGesture(*gesture, 900.0f, 1000);
	EXPECT_EQ(played.animation, animations::k_Yawn);
	played = AdvanceGesture(played, 100.0f, 1000);
	EXPECT_FALSE(played.animation.has_value());
}

TEST(CreatureLayers, TheHeadSpeedsUpSteadily)
{
	const auto first = TurnHead({}, 1.0f, 2.0f, 0.1f, k_YawLimit);
	EXPECT_NEAR(first.velocity, 0.2f, k_Tolerance);
	EXPECT_NEAR(first.angle, 0.02f, k_Tolerance);
	const auto second = TurnHead(first, 1.0f, 2.0f, 0.1f, k_YawLimit);
	EXPECT_NEAR(second.velocity, 0.4f, k_Tolerance);
}

TEST(CreatureLayers, TheHeadSettlesWithoutOvershooting)
{
	LookAxis axis {};
	float largest = 0.0f;
	for (int i = 0; i < 400; ++i)
	{
		axis = TurnHead(axis, -0.8f, 4.0f, 0.02f, k_YawLimit);
		largest = std::max(largest, -axis.angle);
	}
	EXPECT_FLOAT_EQ(axis.angle, -0.8f);
	EXPECT_FLOAT_EQ(axis.velocity, 0.0f);
	EXPECT_LE(largest, 0.8f + k_Tolerance);
}

TEST(CreatureLayers, TheHeadNeverTurnsPastItsLimit)
{
	LookAxis axis {.angle = 1.5f, .velocity = 5.0f};
	axis = TurnHead(axis, 3.0f, 100.0f, 0.1f, k_PitchLimit);
	EXPECT_LE(axis.angle, k_PitchLimit);
}

TEST(CreatureLayers, AnglesTowardsATarget)
{
	const glm::vec3 head {0.0f, 15.0f, 0.0f};
	const glm::vec3 ahead {0.0f, 0.0f, -1.0f};
	const auto straight = AnglesTowards(head, ahead, {0.0f, 15.0f, -10.0f});
	EXPECT_NEAR(straight.yaw, 0.0f, k_Tolerance);
	EXPECT_NEAR(straight.pitch, 0.0f, k_Tolerance);
	// Facing -z in the left-handed world, +x is on the creature's left
	const auto left = AnglesTowards(head, ahead, {10.0f, 15.0f, -10.0f});
	EXPECT_NEAR(left.yaw, k_Pi / 4.0f, k_Tolerance);
	const auto right = AnglesTowards(head, ahead, {-10.0f, 15.0f, 0.0f});
	EXPECT_NEAR(right.yaw, -k_Pi / 2.0f, k_Tolerance);
	const auto up = AnglesTowards(head, ahead, {0.0f, 25.0f, -10.0f});
	EXPECT_NEAR(up.pitch, k_Pi / 4.0f, k_Tolerance);
}

TEST(CreatureLayers, TheMiddleOfAHeadTurnLooksAhead)
{
	EXPECT_EQ(LookTime(0.0f, k_YawLimit, 1000), 500u);
	EXPECT_EQ(LookTime(k_YawLimit, k_YawLimit, 1000), 1000u);
	EXPECT_EQ(LookTime(-k_YawLimit, k_YawLimit, 1000), 0u);
	EXPECT_EQ(LookTime(k_PitchLimit / 2.0f, k_PitchLimit, 1000), 750u);
}

TEST(CreatureLayers, AnimationNames)
{
	EXPECT_EQ(animations::Name(animations::k_Tired), "tired");
	EXPECT_EQ(animations::Name(animations::k_FriendlyWave), "friendly_wave");
	EXPECT_EQ(animations::Name(animations::k_FirstFace), "smile");
	EXPECT_EQ(animations::Name(animations::k_Yawn), "yawn");
	EXPECT_EQ(animations::Name(animations::k_Sit), "sit");
}

TEST(SkeletalLayers, WeightedPosesSum)
{
	const std::vector<Pose> a {{.rotation = skeletal_animation::k_Identity, .translation = {2.0f, 0.0f, 0.0f}}};
	const std::vector<Pose> b {
	    {.rotation = skeletal_animation::RotationYXZ({0.0f, 0.4f, 0.0f}), .translation = {4.0f, 2.0f, 0.0f}}};
	const std::array<std::vector<Pose>, 2> poses {a, b};
	const std::array<float, 2> weights {0.75f, 0.25f};
	const auto sum = skeletal_animation::WeightedSum(poses, weights);
	ASSERT_EQ(sum.size(), 1u);
	EXPECT_NEAR(sum[0].translation.x, 2.5f, k_Tolerance);
	EXPECT_NEAR(sum[0].translation.y, 0.5f, k_Tolerance);
	ExpectMatrixNear(sum[0].rotation, skeletal_animation::Lerp(a[0].rotation, b[0].rotation, 0.25f));
}

TEST(SkeletalLayers, ALayerAtItsReferenceChangesNothing)
{
	std::array<glm::mat4, 3> rest {};
	const auto skeleton = Symmetric(rest);
	const auto layer = Frames({1}, {{glm::vec3(0.3f, 0.1f, 0.0f)}, {glm::vec3(0.3f, 0.6f, 0.0f)}}, {1},
	                          {{glm::vec3(1.0f, 0.0f, 0.0f)}, {glm::vec3(1.0f, 0.5f, 0.0f)}});
	std::vector<Pose> poses(3, Pose {.rotation = skeletal_animation::RotationYXZ({0.0f, 0.2f, 0.0f}), .translation = {}});
	const auto before = poses;
	skeletal_animation::AddLayer(poses, layer, 0, 0, skeleton);
	ExpectMatrixNear(poses[1].rotation, before[1].rotation);
	EXPECT_NEAR(glm::length(poses[1].translation - before[1].translation), 0.0f, k_Tolerance);
}

TEST(SkeletalLayers, ALayerAddsItsTurnAwayFromTheReference)
{
	std::array<glm::mat4, 3> rest {};
	const auto skeleton = Symmetric(rest);
	// Two keyframes 0.5 apart in y, sampled exactly on the second
	const auto layer = Frames({1}, {{glm::vec3(0.0f, 0.1f, 0.0f)}, {glm::vec3(0.0f, 0.6f, 0.0f)}}, {1},
	                          {{glm::vec3(1.0f, 0.0f, 0.0f)}, {glm::vec3(1.0f, 0.5f, 0.0f)}});
	std::vector<Pose> poses(3, Pose {.rotation = skeletal_animation::RotationYXZ({0.0f, 0.2f, 0.0f}), .translation = {}});
	skeletal_animation::AddLayer(poses, layer, 500, 0, skeleton);
	// Turned on by the layer's half radian: 0.2 + 0.5 about y
	ExpectMatrixNear(poses[1].rotation, skeletal_animation::RotationYXZ({0.0f, 0.7f, 0.0f}));
	EXPECT_NEAR(poses[1].translation.y, 0.5f, k_Tolerance);
	// Bones the layer doesn't move stay as they were
	ExpectMatrixNear(poses[2].rotation, skeletal_animation::RotationYXZ({0.0f, 0.2f, 0.0f}));
}

TEST(SkeletalLayers, MirrorBonesPairAcrossTheBody)
{
	std::array<glm::mat4, 3> rest {};
	[[maybe_unused]] const auto skeleton = Symmetric(rest);
	const auto mirror = skeletal_animation::MirrorJoints(rest);
	ASSERT_EQ(mirror.size(), 3u);
	EXPECT_EQ(mirror[0], 0u);
	EXPECT_EQ(mirror[1], 2u);
	EXPECT_EQ(mirror[2], 1u);
}

TEST(SkeletalLayers, AnUnpairedBoneKeepsItsPlace)
{
	const std::array<glm::mat4, 2> rest {glm::mat4(1.0f), glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 0.0f))};
	const auto mirror = skeletal_animation::MirrorJoints(rest);
	EXPECT_EQ(mirror[1], 1u);
}

TEST(SkeletalLayers, AMirroredAnimationMovesTheOtherSide)
{
	std::array<glm::mat4, 3> rest {};
	const auto skeleton = Symmetric(rest);
	const auto mirror = skeletal_animation::MirrorJoints(rest);
	const glm::vec3 euler {0.1f, 0.2f, 0.3f};
	const auto stand = Frames({0}, {{glm::vec3(0.0f)}}, {0}, {{glm::vec3(0.0f, 1.0f, 0.0f)}});
	const auto animation = Frames({1}, {{euler}, {euler}}, {0}, {{glm::vec3(2.0f, 1.0f, 3.0f)}, {glm::vec3(2.0f, 1.0f, 3.0f)}});

	const auto plain = skeletal_animation::SampleCycle(animation, stand, 0, skeleton);
	ExpectMatrixNear(plain[1].rotation, skeletal_animation::RotationYXZ(euler));
	ExpectMatrixNear(plain[2].rotation, skeletal_animation::k_Identity);
	EXPECT_NEAR(plain[0].translation.x, 2.0f, k_Tolerance);

	const auto mirrored = skeletal_animation::SampleCycle(animation, stand, 0, skeleton, mirror);
	// The right bone's keyframes move the left, their y and z angles negated
	ExpectMatrixNear(mirrored[2].rotation, skeletal_animation::RotationYXZ({0.1f, -0.2f, -0.3f}));
	ExpectMatrixNear(mirrored[1].rotation, skeletal_animation::k_Identity);
	// The root's sideways movement away from the stand is flipped
	EXPECT_NEAR(mirrored[0].translation.x, -2.0f, k_Tolerance);
	EXPECT_NEAR(mirrored[0].translation.y, 1.0f, k_Tolerance);
	EXPECT_NEAR(mirrored[0].translation.z, 3.0f, k_Tolerance);
}

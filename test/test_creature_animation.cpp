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
#include <vector>

#include <glm/mat4x4.hpp>
#include <gtest/gtest.h>

#include "3D/SkeletalAnimation.h"
#include "Creature/CreatureAnimation.h"

using namespace openblack;
using skeletal_animation::Animation;

namespace
{
constexpr float k_Tolerance = 1e-4f;

/// An animation of one bone turned about y and moved along x, a keyframe for each angle and distance
Animation OneBone(const std::vector<float>& yaws, const std::vector<float>& xs, uint32_t duration = 1000)
{
	Animation animation {.duration = duration, .looping = true, .rotatedJoints = {0}, .translatedJoints = {0}, .frames = {}};
	for (size_t i = 0; i < yaws.size(); ++i)
	{
		animation.frames.push_back({.eulerAngles = {glm::vec3(0.0f, yaws[i], 0.0f)}, .translations = {glm::vec3(xs[i], 0, 0)}});
	}
	return animation;
}
} // namespace

TEST(SkeletalAnimation, EulerAnglesRoundTrip)
{
	const glm::vec3 euler {0.3f, -0.7f, 1.1f};
	const auto back = skeletal_animation::EulerYXZ(skeletal_animation::RotationYXZ(euler));
	EXPECT_NEAR(back.x, euler.x, k_Tolerance);
	EXPECT_NEAR(back.y, euler.y, k_Tolerance);
	EXPECT_NEAR(back.z, euler.z, k_Tolerance);
}

TEST(SkeletalAnimation, CyclesWrapFromTheLastFrameToTheFirst)
{
	const auto animation = OneBone({0.0f, 1.0f}, {0.0f, 1.0f}, 1000);
	const auto halfway = skeletal_animation::FindFrames(animation, 250);
	EXPECT_EQ(halfway.from, 0u);
	EXPECT_EQ(halfway.to, 1u);
	EXPECT_NEAR(halfway.t, 0.5f, k_Tolerance);
	const auto wrapping = skeletal_animation::FindFrames(animation, 750);
	EXPECT_EQ(wrapping.from, 1u);
	EXPECT_EQ(wrapping.to, 0u);
}

TEST(SkeletalAnimation, SampledTranslationsReplaceTheRest)
{
	const auto animation = OneBone({0.0f, 0.0f}, {2.0f, 4.0f});
	const std::array<uint32_t, 1> parents {skeletal_animation::k_NoParent};
	const std::array<glm::mat4, 1> rest {glm::mat4(1.0f)};
	const auto skeleton = skeletal_animation::Skeleton::FromRestMatrices(parents, rest);
	const auto poses = skeletal_animation::SampleCycle(animation, animation, 250, skeleton);
	ASSERT_EQ(poses.size(), 1u);
	EXPECT_NEAR(poses[0].translation.x, 3.0f, k_Tolerance);
	const auto matrices = skeletal_animation::ComposeBoneMatrices(poses, parents);
	EXPECT_NEAR(matrices[0][3].x, 3.0f, k_Tolerance);
}

TEST(CreatureAnimation, NoWeightKeepsTheBase)
{
	const auto base = OneBone({0.2f}, {1.0f});
	const auto evil = OneBone({0.8f}, {5.0f});
	const auto blended = creature_animation::Blend(base, {.animation = &evil, .stand = &evil, .weight = 0.0f},
	                                               {.animation = nullptr, .stand = nullptr, .weight = 0.0f});
	EXPECT_NEAR(blended.frames[0].eulerAngles[0].y, 0.2f, k_Tolerance);
	EXPECT_NEAR(blended.frames[0].translations[0].x, 1.0f, k_Tolerance);
}

TEST(CreatureAnimation, FullWeightTakesTheVariant)
{
	const auto base = OneBone({0.2f}, {1.0f});
	const auto evil = OneBone({0.8f}, {5.0f});
	const auto blended = creature_animation::Blend(base, {.animation = &evil, .stand = &evil, .weight = 1.0f},
	                                               {.animation = nullptr, .stand = nullptr, .weight = 0.0f});
	EXPECT_NEAR(blended.frames[0].eulerAngles[0].y, 0.8f, k_Tolerance);
	EXPECT_NEAR(blended.frames[0].translations[0].x, 5.0f, k_Tolerance);
}

TEST(CreatureAnimation, TranslationsBlendLinearlyOnBothAxes)
{
	const auto base = OneBone({0.0f}, {0.0f});
	const auto good = OneBone({0.0f}, {4.0f});
	const auto fat = OneBone({0.0f}, {-2.0f});
	const auto blended = creature_animation::Blend(base, {.animation = &good, .stand = &good, .weight = 0.5f},
	                                               {.animation = &fat, .stand = &fat, .weight = 0.5f});
	EXPECT_NEAR(blended.frames[0].translations[0].x, 1.0f, k_Tolerance);
}

TEST(CreatureAnimation, RotationsBlendOnTheMatrices)
{
	// Halfway between two turns about y on the matrices' elements is the halfway turn, only shorter
	const auto base = OneBone({0.0f}, {0.0f});
	const auto evil = OneBone({0.6f}, {0.0f});
	const auto blended = creature_animation::Blend(base, {.animation = &evil, .stand = &evil, .weight = 0.5f},
	                                               {.animation = nullptr, .stand = nullptr, .weight = 0.0f});
	EXPECT_NEAR(blended.frames[0].eulerAngles[0].y, 0.3f, k_Tolerance);
}

TEST(CreatureAnimation, BonesAVariantDoesNotMoveTakeItsStand)
{
	const auto base = OneBone({0.0f}, {0.0f});
	Animation variant {.duration = 1000, .looping = true, .rotatedJoints = {}, .translatedJoints = {}, .frames = {{}}};
	const auto stand = OneBone({0.0f}, {6.0f});
	const auto blended = creature_animation::Blend(base, {.animation = &variant, .stand = &stand, .weight = 1.0f},
	                                               {.animation = nullptr, .stand = nullptr, .weight = 0.0f});
	EXPECT_NEAR(blended.frames[0].translations[0].x, 6.0f, k_Tolerance);
}

TEST(CreatureAnimation, AVariantWithoutItsOwnMovesByItsStand)
{
	const auto animation = OneBone({0.5f}, {1.0f});
	const auto baseStand = OneBone({0.1f}, {2.0f});
	const auto variantStand = OneBone({0.4f}, {3.5f});
	const auto adjusted = creature_animation::AdjustFromStand(animation, baseStand, variantStand);
	// Turned on by the difference between the stands, moved by the difference between them
	EXPECT_NEAR(adjusted.frames[0].eulerAngles[0].y, 0.8f, k_Tolerance);
	EXPECT_NEAR(adjusted.frames[0].translations[0].x, 2.5f, k_Tolerance);
}

TEST(CreatureAnimation, TheRestPoseBlendsAsTheBody)
{
	const std::array<glm::mat4, 1> base {glm::mat4(1.0f)};
	auto moved = glm::mat4(1.0f);
	moved[3] = glm::vec4(4.0f, 0.0f, 0.0f, 1.0f);
	const std::array<glm::mat4, 1> evil {moved};
	const auto rest = creature_animation::BlendRest(base, evil, 0.25f, base, 1.0f);
	EXPECT_NEAR(rest[0][3].x, 1.0f, k_Tolerance);
	EXPECT_NEAR(rest[0][3].w, 1.0f, k_Tolerance);
}

TEST(CreatureAnimation, BreathingTakesFiveSecondsAtSizeOne)
{
	EXPECT_FLOAT_EQ(creature_animation::BreathPeriod(1.0f), 5.0f);
	EXPECT_FLOAT_EQ(creature_animation::BreathPeriod(4.0f), 10.0f);
	EXPECT_NEAR(creature_animation::AdvanceBreath(0.9f, 1.0f, 5.0f), 0.1f, k_Tolerance);
	EXPECT_EQ(creature_animation::BreathTime(0.5f, 2208), 1104u);
	EXPECT_EQ(creature_animation::BreathTime(1.0f, 2208), 2207u);
}

TEST(CreatureAnimation, TheBreathingPeriodEasesToItsTarget)
{
	constexpr float k_Turn = 0.1f;
	constexpr float k_Resting = 5.0f;
	// A creature not yet breathing starts at its target
	EXPECT_FLOAT_EQ(creature_animation::EaseBreathPeriod(0.0f, k_Resting, k_Resting, k_Turn), k_Resting);
	// Faster breathing is taken up over half a second: five turns, a fifth of the gap a turn
	EXPECT_FLOAT_EQ(creature_animation::EaseBreathPeriod(k_Resting, 2.2f, k_Resting, k_Turn),
	                k_Resting - ((k_Resting - 2.2f) / 5.0f));
	// Back to resting over ten seconds: a hundred turns, a hundredth of the gap a turn
	EXPECT_FLOAT_EQ(creature_animation::EaseBreathPeriod(2.0f, k_Resting, k_Resting, k_Turn),
	                2.0f + ((k_Resting - 2.0f) / 100.0f));
	// Turns longer than the time constant close the whole gap
	EXPECT_FLOAT_EQ(creature_animation::EaseBreathPeriod(k_Resting, 3.0f, k_Resting, 1.0f), 3.0f);
}

TEST(CreatureAnimation, BreathingCalmsSlowlyAndQuickensFast)
{
	constexpr float k_Turn = 0.1f;
	constexpr float k_Resting = 5.0f;
	float quickening = k_Resting;
	float calming = 1.4f;
	for (int turn = 0; turn < 10; ++turn)
	{
		quickening = creature_animation::EaseBreathPeriod(quickening, 1.4f, k_Resting, k_Turn);
		calming = creature_animation::EaseBreathPeriod(calming, k_Resting, k_Resting, k_Turn);
	}
	// A second on, quickening has almost arrived; calming has barely started
	EXPECT_LT(quickening - 1.4f, 0.5f);
	EXPECT_LT(calming - 1.4f, 0.5f);
	EXPECT_GT(calming, 1.4f);
}

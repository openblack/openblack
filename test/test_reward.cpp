/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <numbers>

#include <gtest/gtest.h>

#include "Blast/CameraShake.h"
#include "ECS/RewardRules.h"

using namespace openblack;
using namespace openblack::ecs;

TEST(Reward, AChestFromTheSkyFallsAHundredAndFiftyMetresInThreeSecondsTurningHalfARoundASecond)
{
	EXPECT_FLOAT_EQ(reward::FallHeight(0.0f), 150.0f);
	EXPECT_FLOAT_EQ(reward::FallHeight(1.0f), 100.0f);
	EXPECT_FLOAT_EQ(reward::FallHeight(3.0f), 0.0f);
	EXPECT_FLOAT_EQ(reward::FallYaw(1.0f), std::numbers::pi_v<float>);
}

TEST(Reward, ItsDustSpreadsAboutItInSandColoursAndGrowsAsItFades)
{
	const auto dust = reward::MakeDust([](float a, float b) { return b + 0.0f * a; });
	EXPECT_EQ(dust.size(), 27u);
	EXPECT_FLOAT_EQ(dust[0].offset.x, 2.0f);
	EXPECT_FLOAT_EQ(dust[0].offset.y, 2.0f);
	EXPECT_EQ(dust[0].frame, 15u);
	EXPECT_EQ(dust[0].rgb, (std::array<uint8_t, 3> {212, 180, 140}));
	const auto fresh = reward::LookOfDust(2500.0f);
	EXPECT_EQ(fresh.alpha, 255);
	EXPECT_FLOAT_EQ(fresh.size, 0.7f);
	const auto gone = reward::LookOfDust(0.0f);
	EXPECT_EQ(gone.alpha, 0);
	EXPECT_FLOAT_EQ(gone.size, 2.5f);
}

TEST(Reward, ItsDustIsPlacedAcrossThenUpThenAlong)
{
	// Each draw is told apart by its order
	float next = 0.0f;
	const auto dust = reward::MakeDust([&next](float /*from*/, float /*to*/) { return next += 1.0f; });
	EXPECT_FLOAT_EQ(dust[0].offset.z, 1.0f);
	EXPECT_FLOAT_EQ(dust[0].offset.y, 2.0f);
	EXPECT_FLOAT_EQ(dust[0].offset.x, 3.0f);
}

TEST(Reward, ItsThumpShakesTheCameraNearTheWorldsOrigin)
{
	std::vector<camera_shake::Shake> shakes {{.position = glm::vec3(0.0f),
	                                          .radius = reward::k_ShakeRadius,
	                                          .strength = reward::k_ShakeStrength,
	                                          .milliseconds = 400.0f,
	                                          .millisecondsLeft = 400.0f,
	                                          .verticalOnly = false}};
	EXPECT_FLOAT_EQ(camera_shake::Amplitude(shakes, {100.0f, 0.0f, 100.0f}), 1.0f);
	EXPECT_FLOAT_EQ(camera_shake::Amplitude(shakes, {2560.0f, 0.0f, 2560.0f}), 0.0f);
}

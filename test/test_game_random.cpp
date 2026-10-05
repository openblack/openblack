/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <limits>

#include <gtest/gtest.h>

#define LOCATOR_IMPLEMENTATIONS
#include <Common/GameRandomProduction.h>

using openblack::GameRandomProduction;
using openblack::GameRandomSeeds;
using openblack::ParticleRandomStep;
using openblack::ParticleRandomStream;
namespace game_random = openblack::game_random;

TEST(GameRandom, DrawsAsTheGameDoes)
{
	GameRandomProduction random;
	EXPECT_EQ(random.GameRand(100), 78);
	EXPECT_EQ(random.GameRand(100), 48);
	EXPECT_EQ(random.GameRand(7), 5);
	EXPECT_EQ(random.GetSeeds().synced, 0x47184E08u);
	// the local seed is its own
	EXPECT_EQ(random.GetSeeds().local, game_random::k_InitialSeed);
	EXPECT_EQ(random.LocalRand(100), 78);
}

TEST(GameRandom, ZeroDrawsNothing)
{
	GameRandomProduction random;
	EXPECT_EQ(random.GameRand(0), 0);
	EXPECT_EQ(random.LocalRand(0), 0);
	EXPECT_EQ(random.GameFloatRand(0.0f), 0.0f);
	EXPECT_EQ(random.GameFloatRand(std::numeric_limits<float>::quiet_NaN()), 0.0f);
	EXPECT_EQ(random.LocalFloatRand(-0.0f), 0.0f);
	EXPECT_EQ(random.GetSeeds(), GameRandomSeeds {});
}

TEST(GameRandom, FloatsScaleASixteenBitDraw)
{
	GameRandomProduction random;
	// u = 45003: (u * 10) * k
	EXPECT_EQ(random.GameFloatRand(10.0f), 6.8670177f);
	random.SetSeeds({game_random::k_InitialSeed, game_random::k_InitialSeed});
	EXPECT_EQ(random.GameFloatRand(-10.0f), -6.8670177f);
	EXPECT_EQ(random.LocalFloatRand(10.0f), 6.8670177f);
}

TEST(GameRandom, CrtRandIsMsvcs)
{
	GameRandomProduction random;
	EXPECT_EQ(random.CrtRand(), 41);
	EXPECT_EQ(random.CrtRand(), 18467);
	EXPECT_EQ(random.CrtRand(), 6334);
	random.CrtSrand(1);
	EXPECT_EQ(random.CrtRand(), 41);
}

TEST(GameRandom, ParticlesDrawOnlyInAStep)
{
	GameRandomProduction random;
	EXPECT_EQ(random.ParticleFloatRand(10.0f), 0.0f);
	EXPECT_EQ(random.ParticleRand(100), 0);
	EXPECT_EQ(random.GetSeeds(), GameRandomSeeds {});
	{
		const ParticleRandomStep step(random, true);
		EXPECT_EQ(random.GetParticleStream(), ParticleRandomStream::Synced);
		EXPECT_EQ(random.ParticleFloatRand(10.0f), 6.8670177f);
		EXPECT_EQ(random.GetSeeds().local, game_random::k_InitialSeed);
	}
	EXPECT_EQ(random.GetParticleStream(), ParticleRandomStream::None);
	{
		const ParticleRandomStep step(random, false);
		EXPECT_EQ(random.ParticleRand(100), 78);
		EXPECT_NE(random.GetSeeds().local, game_random::k_InitialSeed);
	}
}

TEST(GameRandom, ParticlePointsAreInTheUnitBall)
{
	GameRandomProduction random;
	EXPECT_EQ(random.ParticleRandR3(), glm::vec3(-1.0f));
	const ParticleRandomStep step(random, true);
	for (int i = 0; i < 100; ++i)
	{
		const auto p = random.ParticleRandR3();
		EXPECT_LE((p.z * p.z + p.y * p.y) + p.x * p.x, 1.0f);
	}
}

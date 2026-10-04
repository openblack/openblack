/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "3D/CreatureCaveEffects.h"

using namespace openblack;

TEST(CreatureCaveEffects, SmokeFadesAsItAges)
{
	// Steady at 79 until a quarter of its life, then fading to nothing at its end
	EXPECT_EQ(CreatureCaveEffects::SmokeAlpha(0, false), 0x4f);
	EXPECT_EQ(CreatureCaveEffects::SmokeAlpha(225, false), 0x4f);
	EXPECT_EQ(CreatureCaveEffects::SmokeAlpha(900, false), 0);
	// Smoke thrown up fades in first
	EXPECT_EQ(CreatureCaveEffects::SmokeAlpha(0, true), 0);
	EXPECT_EQ(CreatureCaveEffects::SmokeAlpha(50, true), 39);
	EXPECT_EQ(CreatureCaveEffects::SmokeAlpha(100, true), 0x4f);
}

TEST(CreatureCaveEffects, FlamesRunBackThroughTheirFramesAQuarterApart)
{
	EXPECT_EQ(CreatureCaveEffects::FlameFrame(0, 0), 31u);
	EXPECT_EQ(CreatureCaveEffects::FlameFrame(32, 0), 30u);
	EXPECT_EQ(CreatureCaveEffects::FlameFrame(0, 1), 23u);
	EXPECT_EQ(CreatureCaveEffects::FlameFrame(32 * 32, 0), 31u);
}

TEST(CreatureCaveEffects, SmokeDriftsUpAndStartsOverAtItsPlace)
{
	CreatureCaveEffects::Smoke smoke;
	smoke.place = {10.0f, 0.0f, 0.0f};
	for (auto& particle : smoke.particles)
	{
		particle.age = 0;
		particle.position = smoke.place;
	}
	// A second: 255 steps of age, rising at 2.55 a second
	CreatureCaveEffects::StepSmoke(smoke, 1000);
	EXPECT_EQ(smoke.particles[0].age, 255);
	EXPECT_NEAR(smoke.particles[0].position.y, 2.55f, 1e-4f);
	EXPECT_TRUE(smoke.particles[0].hidden);
	// Past its life it starts over, seen, from the smoke's place, as far on as it went past
	smoke.particles[0].age = 890;
	smoke.particles[0].position = {50.0f, 50.0f, 50.0f};
	CreatureCaveEffects::StepSmoke(smoke, 100);
	EXPECT_EQ(smoke.particles[0].age, (890 + 25) % 900);
	EXPECT_FALSE(smoke.particles[0].hidden);
	EXPECT_NEAR(smoke.particles[0].position.x, 10.0f, 1e-4f);
	EXPECT_NEAR(smoke.particles[0].position.y, 15.0f / 255.0f * 2.55f, 1e-4f);
}

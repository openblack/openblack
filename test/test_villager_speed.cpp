/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "ECS/VillagerSpeed.h"

using namespace openblack::ecs::villager_speed;

namespace
{
Inputs Fleeing()
{
	Inputs in;
	in.speeds = {1000, 2000, 0, 0, 500, 0};
	in.speedIndex = 1;
	in.lifeWhenWalksWounded = 0.3f;
	in.lifeWhenCrawlsWounded = 0.1f;
	return in;
}
float Zero(float /*max*/)
{
	return 0.0f;
}
} // namespace

TEST(VillagerSpeed, AStatesSpeedScaledByThePlayerAndItsTown)
{
	auto in = Fleeing();
	EXPECT_EQ(StateSpeed(in, Zero), 2000);
	in.landSpeedBalance = 1.5f;
	EXPECT_EQ(StateSpeed(in, Zero), 3000);
	// Believed beyond the neutral belief by 2: a tenth of it, times the story land's scale
	in.landSpeedBalance = 1.0f;
	in.belief = Inputs::Belief {.player = 3.0f, .neutral = 1.0f};
	in.landNumber = 2;
	in.beliefSpeedScaleStory = {0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f};
	EXPECT_EQ(StateSpeed(in, Zero), static_cast<int32_t>(2000.0f * (0.5f * 0.2f + 1.0f)));
	// Not beyond it: the whole belief counts
	in.belief = Inputs::Belief {.player = 0.5f, .neutral = 1.0f};
	EXPECT_EQ(StateSpeed(in, Zero), static_cast<int32_t>(2000.0f * (0.5f * 0.5f + 1.0f)));
}

TEST(VillagerSpeed, WoundedLoadedNeededAndSpedUp)
{
	auto in = Fleeing();
	in.life = 0.05f;
	EXPECT_EQ(StateSpeed(in, Zero), 200);
	in.life = 0.2f;
	EXPECT_EQ(StateSpeed(in, Zero), 500);
	in.life = 1.0f;
	in.townInEmergency = true;
	EXPECT_EQ(StateSpeed(in, Zero), 1500);
	in.townInEmergency = false;
	in.townNeeds = 1.0f;
	in.divisorForTownNeedsSpeedMod = 1.0f;
	in.baseForTownNeedsSpeedMod = 1.0f;
	EXPECT_EQ(StateSpeed(in, Zero), 3000);
	in.townNeeds.reset();
	in.woodHeld = 1.0f;
	in.speedModWhenFullLoadOfWood = 0.5f;
	EXPECT_EQ(StateSpeed(in, Zero), static_cast<int32_t>(2000.0f * 0.75f));
	in.woodHeld = 0.0f;
	in.foodSpeedUp = true;
	in.foodPowerupIncrease = 1.5f;
	EXPECT_EQ(StateSpeed(in, Zero), 3000);
}

TEST(VillagerSpeed, EachVillagersOwnWayOfGoing)
{
	// Made first: 16 hundredths slow
	EXPECT_NEAR(PersonalFactor({.creationIndex = 0, .age = 30, .grownUpAge = 18, .oldAge = 60, .life = 0.0f}), 0.84f, 1e-5f);
	EXPECT_NEAR(PersonalFactor({.creationIndex = 1, .age = 30, .grownUpAge = 18, .oldAge = 60, .life = 1.0f, .female = true}),
	            (47 % 31 - 16) * 0.01f + 1.0f - 0.1f - 0.2f, 1e-5f);
	// Young: slower by a fiftieth a year short of grown, at most four tenths
	EXPECT_NEAR(PersonalFactor({.creationIndex = 0, .age = 8, .grownUpAge = 18, .oldAge = 60}), 0.84f - 0.2f, 1e-5f);
	EXPECT_NEAR(PersonalFactor({.creationIndex = 0, .age = 0, .grownUpAge = 40, .oldAge = 60}), 0.84f - 0.4f, 1e-5f);
	EXPECT_EQ(FinalSpeed(1000, 0.84f), 840);
	EXPECT_EQ(FinalSpeed(-5, 1.0f), 0);
	EXPECT_EQ(FinalSpeed(70000, 1.0f), 0xFFFF);
}

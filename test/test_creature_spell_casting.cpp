/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Creature/CreatureSpellCasting.h"

using namespace openblack::creature_spell_casting;

TEST(CreatureSpellCasting, ItPaysWithItsEnergyAndTires)
{
	Body body {.size = 1.0f, .strength = 0.5f, .energy = 1.0f, .exhaustion = 0.0f};
	const Rates rates {};
	// 2.5 x 36000 chants to an energy of 1
	const float paid = MaintainSpell(body, rates, 9000.0f, 0.3f);
	EXPECT_FLOAT_EQ(paid, 9000.0f);
	EXPECT_NEAR(body.exhaustion, 0.1f, 1e-6f);
	EXPECT_NEAR(body.energy, 1.0f - 0.1f * 0.3f, 1e-6f);
}

TEST(CreatureSpellCasting, NoMoreThanItHasNorTiredByMoreThanSevenTenths)
{
	Body body {.size = 1.0f, .strength = 0.0f, .energy = 0.105f, .exhaustion = 0.0f};
	const Rates rates {};
	const float most = MostChants(body, rates);
	EXPECT_NEAR(most, 2.0f * 0.1f * 36000.0f, 1e-2f);
	EXPECT_NEAR(MaintainSpell(body, rates, 1e9f, 0.3f), most, 1e-2f);
	Body strong {.size = 1.0f, .strength = 1.0f, .energy = 1.0f, .exhaustion = 0.0f};
	MaintainSpell(strong, rates, 200000.0f, 0.3f);
	EXPECT_NEAR(strong.exhaustion, k_MostTiredByPaying, 1e-6f);
	// Nothing below its energy's floor
	Body spent {.size = 1.0f, .strength = 0.5f, .energy = 0.004f, .exhaustion = 0.0f};
	EXPECT_EQ(MaintainSpell(spent, rates, 100.0f, 0.3f), 0.0f);
}

TEST(CreatureSpellCasting, TooExhaustedItCantCast)
{
	const Rates rates {};
	EXPECT_TRUE(CanCast({.size = 1.0f, .strength = 0.5f, .energy = 1.0f, .exhaustion = 0.0f}, rates, 9000.0f));
	// 0.85 less the 0.1 the miracle would cost
	EXPECT_FALSE(CanCast({.size = 1.0f, .strength = 0.5f, .energy = 1.0f, .exhaustion = 0.75f}, rates, 9000.0f));
	EXPECT_TRUE(CanCast({.size = 1.0f, .strength = 0.5f, .energy = 1.0f, .exhaustion = 0.74f}, rates, 9000.0f));
}

TEST(CreatureSpellCasting, ItTriesAtHalfTheSightingsAndFizzlesShortOfAllButOne)
{
	EXPECT_FALSE(MayTry(4.0f, 9.0f));
	EXPECT_TRUE(MayTry(4.5f, 9.0f));
	EXPECT_TRUE(MayTry(12.0f, 9.0f));
	// Against the times needed less one
	EXPECT_FALSE(TrySucceeds(7.0f, 9.0f));
	EXPECT_TRUE(TrySucceeds(8.0f, 9.0f));
	// Never against less than one
	EXPECT_FALSE(TrySucceeds(0.0f, 1.5f));
	EXPECT_TRUE(TrySucceeds(1.0f, 1.5f));
}

TEST(CreatureSpellCasting, FightStaminaAndMagnitude)
{
	EXPECT_FLOAT_EQ(StaminaCost(8000.0f), 0.8f);
	EXPECT_FLOAT_EQ(StaminaCost(20000.0f), 1.0f);
	EXPECT_FLOAT_EQ(CastMagnitude(4.0f, std::nullopt, false, 15.0f), 4.4f);
	EXPECT_FLOAT_EQ(CastMagnitude(4.0f, 2.0f, false, 15.0f), 2.0f * 15.0f * 1.1f);
	EXPECT_FLOAT_EQ(CastMagnitude(4.0f, 2.0f, true, 5.0f), 0.5f);
	EXPECT_FLOAT_EQ(CastMagnitude(4.0f, 2.0f, true, 30.0f), 1.0f);
}

/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "ECS/StoreRules.h"

using namespace openblack::ecs;

TEST(StoreRules, ATreesWoodScalesOnceWithItsSizeAndWithItsLife)
{
	EXPECT_EQ(store_rules::TreeWood(1.0f, 1.0f, 100, 2.0f, 1.0f), 200u);
	EXPECT_EQ(store_rules::TreeWood(0.5f, 1.0f, 100, 2.0f, 1.0f), 100u);
	// A forest miracle's tree and the land's balance multiply it, and it is truncated
	EXPECT_EQ(store_rules::TreeWood(1.0f, 1.5f, 100, 1.0f, 0.999f), 149u);
}

TEST(StoreRules, ADeadTreesWoodIgnoresItsLife)
{
	EXPECT_EQ(store_rules::DeadTreeWood(2.0f, 100, 1.0f), 200u);
}

TEST(StoreRules, AFencesWoodGrowsWithTheCubeOfItsSize)
{
	EXPECT_EQ(store_rules::FenceWood(1.0f, 10, 2.0f), 80u);
	EXPECT_EQ(store_rules::FenceWood(0.5f, 10, 2.0f), 40u);
}

TEST(StoreRules, OnlyMeatAndVegetableAnimalsAreFood)
{
	EXPECT_EQ(store_rules::AnimalFood(300.0f, 1), 300u);
	EXPECT_EQ(store_rules::AnimalFood(300.0f, 2), 300u);
	EXPECT_EQ(store_rules::AnimalFood(300.0f, 4), 0u);
}

TEST(StoreRules, GivingBackWhatWasTakenLatelyCountsForLess)
{
	EXPECT_FLOAT_EQ(store_rules::LastTakenModifier(std::nullopt, 100, 50), 1.0f);
	EXPECT_FLOAT_EQ(store_rules::LastTakenModifier(75, 100, 50), 0.125f);
	EXPECT_FLOAT_EQ(store_rules::LastTakenModifier(0, 100, 50), 1.0f);
}

TEST(StoreRules, APileGivesOnlyWhatIsWithinItsStoresMaximum)
{
	EXPECT_EQ(store_rules::AmountOverMaximum(true, 600, 100), 100);
	EXPECT_EQ(store_rules::AmountOverMaximum(false, 600, 100), 500);
	EXPECT_EQ(store_rules::AskedOfPile(50, 100), 0u);
	EXPECT_EQ(store_rules::AskedOfPile(150, 100), 50u);
	EXPECT_EQ(store_rules::AskedOfPile(150, -10), 150u);
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Magic/ResourcePiles.h"

using namespace openblack;
using namespace openblack::magic::piles;

namespace
{
constexpr float k_Epsilon = 1e-4f;
}

TEST(ResourcePiles, FoodRisesFastAtFirstAndNeverBeyondFull)
{
	EXPECT_NEAR(ProportionRaised(ResourceType::Food, 0, 1000), 0.0f, k_Epsilon);
	// Anything at all shows a twentieth of its fill and more, eased up to nearly a tenth
	const float least = 0.05f + 0.95f * 0.001f;
	EXPECT_NEAR(ProportionRaised(ResourceType::Food, 1, 1000), 1.0f - (1.0f - least) * (1.0f - least), k_Epsilon);
	const float half = 0.05f + 0.95f * 0.5f;
	EXPECT_NEAR(ProportionRaised(ResourceType::Food, 500, 1000), 1.0f - (1.0f - half) * (1.0f - half), k_Epsilon);
	EXPECT_NEAR(ProportionRaised(ResourceType::Food, 1000, 1000), 1.0f, k_Epsilon);
	EXPECT_NEAR(ProportionRaised(ResourceType::Food, 5000, 1000), 1.0f, k_Epsilon);
}

TEST(ResourcePiles, WoodRisesEvenlyUpToFull)
{
	EXPECT_NEAR(ProportionRaised(ResourceType::Wood, 0, 1000), 0.0f, k_Epsilon);
	EXPECT_NEAR(ProportionRaised(ResourceType::Wood, 500, 1000), 0.05f + 0.95f * 0.5f, k_Epsilon);
	EXPECT_NEAR(ProportionRaised(ResourceType::Wood, 1000, 1000), 1.0f, k_Epsilon);
	EXPECT_NEAR(ProportionRaised(ResourceType::Wood, 2000, 1000), 1.0f, k_Epsilon);
	// However much it holds, it never stands above the ground
	EXPECT_FLOAT_EQ(SunkOffset(ProportionRaised(ResourceType::Wood, 2000, 1000), 3.0f), 0.0f);
	EXPECT_NEAR(SunkOffset(0.25f, 4.0f), -3.0f, k_Epsilon);
}

TEST(ResourcePiles, APileRisesOverASecondAndEndsStill)
{
	auto rise = SunkRise(2.0f);
	EXPECT_FALSE(Shown(rise, 2.0f));
	RiseTo(rise, 0.0f);
	StepRise(rise, 0.5f);
	// It starts off accelerating and eases in: past half way at half time (24 t^2/2 - 96 t^3/6 + 144 t^4/24 of 2)
	EXPECT_NEAR(rise.offset, -2.0f + 1.375f, k_Epsilon);
	EXPECT_GT(rise.currentSpeed, 0.0f);
	EXPECT_TRUE(Shown(rise, 2.0f));
	StepRise(rise, 0.25f);
	EXPECT_GT(rise.offset, -0.625f);
	EXPECT_LT(rise.offset, 0.0f);
	StepRise(rise, 0.25f);
	EXPECT_NEAR(rise.offset, 0.0f, k_Epsilon);
	EXPECT_NEAR(rise.currentSpeed, 0.0f, k_Epsilon);
}

TEST(ResourcePiles, ARiseTurnedMidwayKeepsItsSpeed)
{
	auto rise = SunkRise(2.0f);
	RiseTo(rise, 0.0f);
	StepRise(rise, 0.5f);
	const float offset = rise.offset;
	const float speed = rise.currentSpeed;
	RiseTo(rise, -2.0f);
	// Just after turning back it still moves up a little, at its speed, slowing
	EXPECT_GT(speed, 0.0f);
	StepRise(rise, 0.01f);
	EXPECT_GT(rise.offset, offset);
	EXPECT_LT(rise.offset, offset + speed * 0.01f);
	StepRise(rise, 1.0f);
	EXPECT_NEAR(rise.offset, -2.0f, k_Epsilon);
}

TEST(ResourcePiles, GrainFlowsAQuarterWhenSunk)
{
	EXPECT_NEAR(GrainFlow(-2.0f, 2.0f), 0.25f, k_Epsilon);
	EXPECT_NEAR(GrainFlow(-1.0f, 2.0f), 0.125f, k_Epsilon);
	EXPECT_NEAR(GrainFlow(0.0f, 2.0f), 0.0f, k_Epsilon);
	EXPECT_NEAR(GrainFlow(1.0f, 2.0f), 0.0f, k_Epsilon);
	EXPECT_TRUE(GrainFlows(PotInfo::MagicFood));
	EXPECT_TRUE(GrainFlows(PotInfo::StoragePitFoodPile));
	EXPECT_FALSE(GrainFlows(PotInfo::FoodPile));
	EXPECT_FALSE(GrainFlows(PotInfo::MagicWood));
}

TEST(ResourcePiles, ThudsAreSmallUnderTwoHundred)
{
	EXPECT_EQ(PileSoundSample(ResourceType::Food, 199, 7), 0x4Du + 1u);
	EXPECT_EQ(PileSoundSample(ResourceType::Wood, 199, 7), 0x5Cu + 1u);
	EXPECT_EQ(PileSoundSample(ResourceType::Food, 200, 7), 0x4Bu + 1u);
	EXPECT_EQ(PileSoundSample(ResourceType::Wood, 200, 7), 0x56u + 1u);
	EXPECT_FALSE(Thuds(PotInfo::HandFood));
	EXPECT_TRUE(Thuds(PotInfo::MagicFood));
}

TEST(ResourcePiles, PouringSpiralsOutFromItsCell)
{
	EXPECT_EQ(CellOf({25.0f, 37.0f}), glm::ivec2(2, 3));
	EXPECT_EQ(k_SearchCells.front(), glm::ivec2(0, 0));
	EXPECT_EQ(k_SearchCells[1], glm::ivec2(-1, 0));
	EXPECT_EQ(k_SearchCells.back(), glm::ivec2(-1, 1));
	EXPECT_NEAR(RadiusOnGround({2.0f, 3.0f}, 0.5f), 1.5f, k_Epsilon);
}

TEST(ResourcePiles, OnlyPilesLeadingOnHoldNoMoreThanFull)
{
	EXPECT_TRUE(IsCapped(PotInfo::WoodPile_2));
	EXPECT_FALSE(IsCapped(PotInfo::_COUNT));
	EXPECT_EQ(AmountTaken(900, 300, 1000, true), 100u);
	EXPECT_EQ(AmountTaken(900, 300, 1000, false), 300u);
	EXPECT_EQ(AmountTaken(1000, 300, 1000, true), 0u);
}

TEST(ResourcePiles, APotIsDrawnAtTheSizeOfWhatItHolds)
{
	// A quarter, and one more for every so much its kind says, up to five times
	EXPECT_FLOAT_EQ(PotScale(0, 20), 0.25f);
	EXPECT_FLOAT_EQ(PotScale(30, 20), 1.75f);
	EXPECT_FLOAT_EQ(PotScale(1000, 20), 5.0f);
}

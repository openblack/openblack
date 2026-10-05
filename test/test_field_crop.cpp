/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <algorithm>

#include <gtest/gtest.h>

#include "3D/FieldCrop.h"

using namespace openblack;

namespace
{
constexpr field_crop::Type k_Type {
    .ageGrowth = 100.0f,
    .ageRipe = 200.0f,
    .timesToSow = 4.0f,
    .totalFood = 1000.0f,
    .sunWhenGrowing = 1.0f,
    .sunWhenRipening = 2.0f,
    .rainWhenGrowing = 3.0f,
    .rainWhenRipening = 4.0f,
};
} // namespace

TEST(FieldCrop, OnlySownCropsGrow)
{
	field_crop::Crop crop {.timesSown = 3};
	field_crop::Grow(crop, k_Type, 0.0f, false);
	EXPECT_FLOAT_EQ(crop.age, 0.0f);
	crop.timesSown = 4;
	field_crop::Grow(crop, k_Type, 0.0f, false);
	EXPECT_FLOAT_EQ(crop.age, 2.0f);
	// Food in step with the age: the whole field's food by the time it is ripe
	EXPECT_FLOAT_EQ(crop.food, 10.0f);
}

TEST(FieldCrop, GrowsFasterOnGoodLandAndByTheWeather)
{
	field_crop::Crop evil {.timesSown = 4};
	field_crop::Grow(evil, k_Type, -1.0f, false);
	EXPECT_FLOAT_EQ(evil.age, 1.0f);
	field_crop::Crop good {.timesSown = 4};
	field_crop::Grow(good, k_Type, 1.0f, false);
	EXPECT_FLOAT_EQ(good.age, 3.0f);
	field_crop::Crop rained {.timesSown = 4};
	field_crop::Grow(rained, k_Type, 0.0f, true);
	EXPECT_FLOAT_EQ(rained.age, 6.0f);
	// Ripening, at its own rates
	field_crop::Crop ripening {.timesSown = 4, .age = 100.0f};
	field_crop::Grow(ripening, k_Type, 0.0f, false);
	EXPECT_FLOAT_EQ(ripening.age, 104.0f);
	ripening.age = 100.0f;
	field_crop::Grow(ripening, k_Type, 0.0f, true);
	EXPECT_FLOAT_EQ(ripening.age, 108.0f);
}

TEST(FieldCrop, StopsOncePastRipe)
{
	field_crop::Crop crop {.timesSown = 4, .age = 200.0f};
	field_crop::Grow(crop, k_Type, 0.0f, false);
	EXPECT_FLOAT_EQ(crop.age, 204.0f);
	EXPECT_TRUE(field_crop::IsRipe(crop, k_Type));
	field_crop::Grow(crop, k_Type, 0.0f, false);
	EXPECT_FLOAT_EQ(crop.age, 204.0f);
}

TEST(FieldCrop, ShowsOnceGrownALittleWithSomeFood)
{
	EXPECT_FALSE(field_crop::LookOf({.timesSown = 4, .age = 24.0f, .food = 500.0f}, k_Type, 10).has_value());
	EXPECT_FALSE(field_crop::LookOf({.timesSown = 4, .age = 25.0f, .food = 9.0f}, k_Type, 10).has_value());
	EXPECT_TRUE(field_crop::LookOf({.timesSown = 4, .age = 25.0f, .food = 10.0f}, k_Type, 10).has_value());
}

TEST(FieldCrop, ColoursAndHeightAsItRipens)
{
	// Young and empty: green, and sunk all the way
	const auto young = field_crop::LookOf({.timesSown = 4, .age = 50.0f, .food = 0.0f}, k_Type, 0);
	ASSERT_TRUE(young.has_value());
	EXPECT_EQ(young->tint, 0xAAD443u);
	EXPECT_FLOAT_EQ(young->height, -1.0f);
	EXPECT_FALSE(young->sways);
	// Halfway to ripe, halfway to its own colour
	const auto ripening = field_crop::LookOf({.timesSown = 4, .age = 150.0f, .food = 750.0f}, k_Type, 0);
	ASSERT_TRUE(ripening.has_value());
	EXPECT_EQ(ripening->tint, field_crop::Blend(0x799119, 0xFFFFFF, 127));
	EXPECT_FLOAT_EQ(ripening->height, -0.25f);
	// Ripe: its own colour, swaying
	const auto ripe = field_crop::LookOf({.timesSown = 4, .age = 200.0f, .food = 1000.0f}, k_Type, 0);
	ASSERT_TRUE(ripe.has_value());
	EXPECT_EQ(ripe->tint, 0xFFFFFFu);
	EXPECT_FLOAT_EQ(ripe->height, 0.0f);
	EXPECT_TRUE(ripe->sways);
}

TEST(FieldCrop, BlendsInWholeSteps)
{
	EXPECT_EQ(field_crop::Blend(0x000000, 0xFFFFFF, 0), 0x000000u);
	EXPECT_EQ(field_crop::Blend(0x000000, 0xFFFFFF, 255), 0xFFFFFFu);
	// 255 * 100 / 255 in each channel, 10 * 155 / 255 rounded down in the first
	EXPECT_EQ(field_crop::Blend(0x0A0000, 0xFF0000, 100) >> 16u, (10u * 155u + 255u * 100u) / 255u);
}

TEST(FieldCrop, Settles)
{
	// Setting off again each frame, it is most of the way there after a second, overshoots a hair, and comes to rest
	field_crop::Settle settle {.position = -1.0f, .speed = 0.0f};
	float highest = settle.position;
	for (int frame = 0; frame < 60 * 5; ++frame)
	{
		settle = field_crop::Advance(settle, 0.0f, 1.0f / 60.0f);
		highest = std::max(highest, settle.position);
		if (frame == 59)
		{
			EXPECT_NEAR(settle.position, -0.074f, 1e-3f);
		}
	}
	EXPECT_LT(highest, 0.005f);
	EXPECT_NEAR(settle.position, 0.0f, 1e-4f);
	EXPECT_NEAR(settle.speed, 0.0f, 1e-3f);
	// It starts off gently
	const auto first = field_crop::Advance({.position = -1.0f, .speed = 0.0f}, 0.0f, 1.0f / 60.0f);
	EXPECT_GT(first.position, -1.0f);
	EXPECT_LT(first.position, -0.99f);
	// A long frame lands it at rest
	const auto landed = field_crop::Advance({.position = -1.0f, .speed = 0.5f}, 0.0f, 2.0f);
	EXPECT_FLOAT_EQ(landed.position, 0.0f);
	EXPECT_FLOAT_EQ(landed.speed, 0.0f);
}

TEST(FieldCrop, SinksAtMostEightyPercent)
{
	EXPECT_FLOAT_EQ(field_crop::Sink(-1.0f), -0.8f);
	EXPECT_FLOAT_EQ(field_crop::Sink(-0.5f), -0.5f);
	EXPECT_FLOAT_EQ(field_crop::Sink(0.0f), 0.0f);
}

TEST(FieldCrop, AlignmentIsClamped)
{
	EXPECT_FLOAT_EQ(field_crop::ClampAlignment(3.0f), 1.0f);
	EXPECT_FLOAT_EQ(field_crop::ClampAlignment(-3.0f), -1.0f);
	EXPECT_FLOAT_EQ(field_crop::ClampAlignment(0.3f), 0.3f);
}

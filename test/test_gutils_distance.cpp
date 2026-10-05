/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The game's distances, against values worked out with the game's 24-bit float arithmetic. The sigmoid table is
// checked in bits.

#include <bit>
#include <limits>

#include <gtest/gtest.h>

#include "Common/GUtilsDistance.h"

namespace gu = openblack::gutils;
namespace mc = openblack::map_coords;

using mc::MapCoords;

namespace
{
MapCoords Fixed(int32_t x, int32_t z)
{
	return {x, z, 0.0f};
}
} // namespace

TEST(GUtilsDistance, SigmoidTableBits)
{
	// Only the ends are exact: T[0] = 0 and T[37..40] = 1
	ASSERT_EQ(gu::detail::k_SigmoidBits.size(), 41u);
	EXPECT_EQ(gu::detail::k_SigmoidBits[0], 0x00000000u);
	EXPECT_EQ(gu::detail::k_SigmoidBits[1], 0x317763DFu);
	EXPECT_EQ(gu::detail::k_SigmoidBits[10], 0x38172465u); // 3.60351e-5, the floor of GetDistanceModifier
	EXPECT_EQ(gu::detail::k_SigmoidBits[11], 0x38D235BDu); // 1.00235899e-4, not the 0.0001 of a rounded table
	EXPECT_EQ(gu::detail::k_SigmoidBits[20], 0x3F000000u); // 0.5
	EXPECT_EQ(gu::detail::k_SigmoidBits[30], 0x3F7FFDA3u); // 0.999963939, the ceiling of GetDistanceModifier
	EXPECT_EQ(gu::detail::k_SigmoidBits[36], 0x3F7FFFFFu);
	EXPECT_EQ(gu::detail::k_SigmoidBits[37], 0x3F800000u);
	EXPECT_EQ(gu::detail::k_SigmoidBits[40], 0x3F800000u);
	for (size_t i = 0; i < gu::k_Sigmoid.size(); ++i)
	{
		EXPECT_EQ(std::bit_cast<uint32_t>(gu::k_Sigmoid[i]), gu::detail::k_SigmoidBits[i]) << "entry " << i;
	}
}

TEST(GUtilsDistance, InvSqrtTable)
{
	const auto& table = gu::InvSqrtTable();
	ASSERT_EQ(table.size(), 1024u);
	// 1 / sqrt(0.5) = 1.4142, mantissa bits 13..22; entry 511 is the last of [0.5, 1) and 512 the exact 1 (0x7FE000)
	EXPECT_EQ(table[0], 0x350000u);
	EXPECT_EQ(table[1], 0x34C000u);
	EXPECT_EQ(table[2], 0x34A000u);
	EXPECT_EQ(table[511], 0x000000u);
	EXPECT_EQ(table[512], 0x7FE000u); // 1 / sqrt(1) == 1 keeps all ten bits
	EXPECT_EQ(table[513], 0x7FC000u);
	EXPECT_EQ(table[1023], 0x350000u);
}

TEST(GUtilsDistance, InvSqrt)
{
	// 1 / sqrt(1) comes out as 0.99951171875 (0x3F7FE000), not 1: that is the table's trick for [1, 2)
	EXPECT_EQ(std::bit_cast<uint32_t>(gu::InvSqrt(1.0f)), 0x3F7FE000u);
	EXPECT_FLOAT_EQ(gu::InvSqrt(4.0f), 0.499755859375f);
	// 0 gives about 2^63, so 1 / InvSqrt(0) is almost 0 and the truncation of hypotenuse(int, int) makes it 0: neither
	// hypotenuse needs a test for the zero
	EXPECT_FLOAT_EQ(gu::InvSqrt(0.0f), 1.3042424520864956e19f);
	EXPECT_FLOAT_EQ(1.0f / gu::InvSqrt(0.0f), 7.667286378005185e-20f);
	// the length of 100 comes out 0.024 % long: this is what the whole game measures with
	EXPECT_FLOAT_EQ(1.0f / gu::InvSqrt(100.0f), 10.002442359924316f);
}

TEST(GUtilsDistance, HypotenuseWhole)
{
	// 16.16 in and out: 0x10000 units = one cell = 10 m. Truncated, one cell measures 65568 (+0.049 %)
	EXPECT_EQ(gu::Hypotenuse(0, 0), 0);
	EXPECT_EQ(gu::Hypotenuse(0x10000, 0), 65568);
	EXPECT_EQ(gu::Hypotenuse(0, 0x10000), 65568);
	EXPECT_EQ(gu::Hypotenuse(-0x10000, 0), 65568);
	EXPECT_EQ(gu::Hypotenuse(0x10000, 0x10000), 92691);
	EXPECT_EQ(gu::Hypotenuse(6553, 6553), 9269);
	EXPECT_EQ(gu::Hypotenuse(655360, 0), 655520);
	EXPECT_EQ(gu::Hypotenuse(6553600, 6553600), 9271381);
	// the whole map side, 5120 m
	EXPECT_EQ(gu::Hypotenuse(33554432, 0), 33570824);
}

TEST(GUtilsDistance, HypotenuseFloat)
{
	// the 1e-4 cut is on both sides at once, and it is <=
	EXPECT_EQ(gu::Hypotenuse(0.0f, 0.0f), 0.0f);
	EXPECT_EQ(gu::Hypotenuse(1e-4f, 1e-4f), 0.0f);
	EXPECT_FLOAT_EQ(gu::Hypotenuse(1.1e-4f, 0.0f), 0.00011003520921804011f);
	// a NaN side takes the cut
	const float nan = std::numeric_limits<float>::quiet_NaN();
	EXPECT_EQ(gu::Hypotenuse(nan, 0.0f), 0.0f);
	EXPECT_EQ(gu::Hypotenuse(0.0f, nan), 0.0f);
	EXPECT_EQ(std::bit_cast<uint32_t>(gu::Hypotenuse(3.0f, 4.0f)), 0x40A00A01u); // 5.0012212, not 5
	EXPECT_EQ(std::bit_cast<uint32_t>(gu::Hypotenuse(100.0f, 0.0f)), 0x42C81C24u);
	EXPECT_EQ(std::bit_cast<uint32_t>(gu::Hypotenuse(100.0f, 100.0f)), 0x430D7855u);
	// The distance between points is this one on x and z: the y is dropped
	EXPECT_FLOAT_EQ(gu::GetDistance(glm::vec3(0.0f, 500.0f, 0.0f), glm::vec3(3.0f, -7.0f, 4.0f)), 5.001221179962158f);
}

TEST(GUtilsDistance, UnitConversions)
{
	// The integer is exact before the rounding
	EXPECT_EQ(gu::ConvertWholeDistanceToMeters(0x10000), 10.0f);
	EXPECT_FLOAT_EQ(gu::ConvertWholeDistanceToMeters(6554), 1.00006103515625f);
	EXPECT_EQ(gu::ConvertWholeDistanceToMeters(1), 0.000152587890625f);
	EXPECT_EQ(gu::ConvertWholeDistanceToMeters(33554432), 5120.0f);
	EXPECT_FLOAT_EQ(gu::ConvertWholeDistanceToMeters(20000000), 3051.7578125f);
	// m / 10 * 65536, truncated: sometimes one unit below ToFixed's m * 6553.6
	EXPECT_EQ(gu::ConvertMetersToWholeDistance(10.0f), 65536);
	EXPECT_EQ(gu::ConvertMetersToWholeDistance(1.0f), 6553);
	EXPECT_EQ(gu::ConvertMetersToWholeDistance(1234.567f), 8090858);
	EXPECT_EQ(gu::ConvertMetersToWholeDistance(5119.99f), 33554368);
	// 1464 m: 146.4f * 65536 = 9594470, one unit below 1464 * 6553.6f = 9594471
	EXPECT_EQ(gu::ConvertMetersToWholeDistance(1464.0f), 9594470);
	EXPECT_EQ(mc::ToFixed(1464.0f), 9594471);
}

TEST(GUtilsDistance, GetDistanceInMetres)
{
	EXPECT_EQ(gu::GetDistance(Fixed(0, 0), Fixed(655360, 0)), 655520);
	EXPECT_FLOAT_EQ(gu::GetDistanceInMetres(Fixed(0, 0), Fixed(655360, 0)), 100.0244140625f);
	EXPECT_FLOAT_EQ(gu::GetDistanceInMetres(Fixed(0, 0), Fixed(655360, 655360)), 141.48529052734375f);
	EXPECT_FLOAT_EQ(gu::GetDistanceInMetres(Fixed(0, 0), Fixed(2621440, 0)), 400.09765625f); // 400 m -> 400.098
	// 3-4-5 happens to land exactly on 5: 19660^2 + 26214^2 rounds to 0x8000 units
	EXPECT_EQ(gu::GetDistanceInMetres(Fixed(0, 0), Fixed(19660, 26214)), 5.0f);
	// through the world points: x and z are truncated to 16.16 first
	EXPECT_FLOAT_EQ(gu::GetDistanceInMetres(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 90.0f, 0.0f)), 0.999908447265625f);
	EXPECT_FLOAT_EQ(gu::GetDistanceInMetres(glm::vec3(1234.567f, 0.0f, 0.0f), glm::vec3(1334.567f, 0.0f, 0.0f)),
	                100.0244140625f);
	EXPECT_FLOAT_EQ(gu::GetDistanceInMetres(glm::ivec2(0, 0), glm::ivec2(655360, 0)), 100.0244140625f);
}

TEST(GUtilsDistance, DistanceToCell)
{
	// the second point is the centre of the cell, (cell << 16) + 0x8000
	EXPECT_EQ(gu::GetDistanceToCell(Fixed(0, 0), {0, 0}), 46345);
	EXPECT_FLOAT_EQ(gu::GetDistanceInMetresToCell(Fixed(0, 0), {0, 0}), 7.071685791015625f);
	EXPECT_FLOAT_EQ(gu::GetDistanceInMetresToCell(Fixed(0, 0), {1, 0}), 15.814666748046875f);
	EXPECT_EQ(gu::GetDistanceInMetresToCell(Fixed(0x18000, 0x18000), {1, 1}), 0.0f); // the centre of cell (1, 1)
}

TEST(GUtilsDistance, MetresDistanceSqHasNoTable)
{
	// the exact square in float: 100 m x 100 m gives 20000 and not GetDistanceInMetres^2 (20018)
	EXPECT_EQ(gu::GetMetresDistanceSq(Fixed(0, 0), Fixed(655360, 655360)), 20000.0f);
	EXPECT_FLOAT_EQ(gu::GetMetresDistanceSq(Fixed(0, 0), Fixed(6553, 0)), 0.99981689453125f);
}

TEST(GUtilsDistance, FastDistanceAndChebyshev)
{
	// FastDistance is max + min / 2, in map units
	EXPECT_EQ(gu::FastDistance(Fixed(0, 0), Fixed(0x10000, 0x10000)), 98304);
	EXPECT_EQ(gu::FastDistance(Fixed(0, 0), Fixed(100, 300)), 350);
	EXPECT_EQ(gu::FastDistance(Fixed(0, 0), Fixed(-7, -2)), 8); // (2 >> 1) + 7
	// ChebyshevDistance is max(|dx|, |dz|)
	EXPECT_EQ(gu::ChebyshevDistance(Fixed(0, 0), Fixed(100, 300)), 300);
	EXPECT_EQ(gu::ChebyshevDistance(Fixed(0, 0), Fixed(-400, 300)), 400);
}

TEST(GUtilsDistance, SigmoidThreshold)
{
	// the threshold is the first argument: a == 1 gives 0
	EXPECT_EQ(gu::SigmoidThreshold(1.0f, 0.5f), 0.0f);
	EXPECT_EQ(gu::SigmoidThreshold(0.5f, 1.0f), gu::k_Sigmoid[30]);
	EXPECT_EQ(gu::SigmoidThreshold(0.5f, 0.5f), 0.5f);
	EXPECT_EQ(gu::SigmoidThreshold(0.5f, 0.0f), gu::k_Sigmoid[10]);
	EXPECT_EQ(gu::SigmoidThreshold(0.0f, 0.0f), 0.5f);
	// b is clamped to [-1, 1] before the subtraction, and the difference again afterwards
	EXPECT_EQ(gu::SigmoidThreshold(0.5f, 2.0f), gu::SigmoidThreshold(0.5f, 1.0f));
	EXPECT_EQ(gu::SigmoidThreshold(0.5f, -2.0f), gu::k_Sigmoid[0]);
	EXPECT_EQ(gu::SigmoidThreshold(-0.9f, 0.0f), 1.0f); // (0.9 + 1) * 20.5 = 38.95 -> T[38]
	EXPECT_EQ(gu::SigmoidThreshold(-0.9f, -1.0f), gu::k_Sigmoid[18]);
	// the creature's only adds the b <= 0 cut
	EXPECT_EQ(gu::CreatureSigmoidThreshold(0.6f, -0.1f), 0.0f);
	EXPECT_EQ(gu::CreatureSigmoidThreshold(0.6f, 0.0f), 0.0f);
	EXPECT_EQ(gu::CreatureSigmoidThreshold(0.6f, 0.3f), gu::SigmoidThreshold(0.6f, 0.3f));
	EXPECT_EQ(gu::CreatureSigmoidThreshold(0.6f, std::numeric_limits<float>::quiet_NaN()), 0.0f);
}

TEST(GUtilsDistance, GetDistanceModifierFallsOffWithDistance)
{
	// With the 400 m villagers react to fire within: it goes down, from T[30] close by to T[10] at the maximum, in 21
	// steps
	EXPECT_EQ(gu::GetDistanceModifier(0.0f, 400.0f), gu::k_Sigmoid[30]);
	EXPECT_FLOAT_EQ(gu::GetDistanceModifier(50.0f, 400.0f), 0.9997212290763855f);
	EXPECT_FLOAT_EQ(gu::GetDistanceModifier(150.0f, 400.0f), 0.9556082487106323f);
	EXPECT_EQ(gu::GetDistanceModifier(200.0f, 400.0f), 0.5f);
	EXPECT_FLOAT_EQ(gu::GetDistanceModifier(220.0f, 400.0f), 0.2644243538379669f);
	EXPECT_FLOAT_EQ(gu::GetDistanceModifier(250.0f, 400.0f), 0.04439174011349678f);
	EXPECT_FLOAT_EQ(gu::GetDistanceModifier(300.0f, 400.0f), 0.005967208184301853f);
	EXPECT_EQ(gu::GetDistanceModifier(400.0f, 400.0f), gu::k_Sigmoid[10]);
	EXPECT_EQ(gu::GetDistanceModifier(500.0f, 400.0f), gu::k_Sigmoid[10]);
	// a tree's scale against 3, as the water miracle measures it
	EXPECT_EQ(gu::GetDistanceModifier(1.5f, 3.0f), 0.5f);
	EXPECT_EQ(gu::GetDistanceModifier(3.0f, 3.0f), gu::k_Sigmoid[10]);
	// a maximum of 0 divides 0 by 0: the NaN clamps to -1 and the answer is T[0]
	EXPECT_EQ(gu::GetDistanceModifier(0.0f, 0.0f), 0.0f);
	// a NaN distance is kept, and clamps to -1 the same way
	EXPECT_EQ(gu::GetDistanceModifier(std::numeric_limits<float>::quiet_NaN(), 400.0f), 0.0f);
}

TEST(GUtilsDistance, DistanceChangeToBelief)
{
	// SigmoidThreshold(-0.9, -(x / y)): another curve on the same table
	EXPECT_EQ(gu::DistanceChangeToBelief(0.0f, 1.0f), 1.0f);
	EXPECT_EQ(gu::DistanceChangeToBelief(1.0f, 1.0f), gu::k_Sigmoid[18]);
	EXPECT_FLOAT_EQ(gu::DistanceChangeToBelief(0.5f, 1.0f), 0.9997212290763855f);
	EXPECT_EQ(gu::DistanceChangeToBelief(-1.0f, 1.0f), 1.0f);
}

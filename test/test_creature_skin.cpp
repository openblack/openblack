/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdint>

#include <array>

#include <gtest/gtest.h>

#include "Creature/CreatureSkin.h"

using namespace openblack;
using namespace openblack::creature_skin;

TEST(CreatureSkin, NeutralShowsNoVariant)
{
	EXPECT_EQ(BlendWeight(0.0f), 0);
}

TEST(CreatureSkin, WeightIsTheAxisIn256StepsTruncated)
{
	EXPECT_EQ(BlendWeight(0.5f), 128);
	EXPECT_EQ(BlendWeight(-0.5f), 128);
	// 0.03 * 256 is 7.68
	EXPECT_EQ(BlendWeight(0.03f), 7);
	EXPECT_EQ(BlendWeight(-0.999f), 255);
}

TEST(CreatureSkin, WeightIsCappedAt255)
{
	EXPECT_EQ(BlendWeight(1.0f), 255);
	EXPECT_EQ(BlendWeight(-1.0f), 255);
	EXPECT_EQ(BlendWeight(3.0f), 255);
}

TEST(CreatureSkin, ChannelIsAWeightedMeanRoundedDown)
{
	EXPECT_EQ(BlendChannel(0, 15, 0), 0);
	EXPECT_EQ(BlendChannel(0, 15, 255), 15);
	// (0 * 127 + 15 * 128) / 255 = 7.53
	EXPECT_EQ(BlendChannel(0, 15, 128), 7);
	// (15 * 127 + 0 * 128) / 255 = 7.47
	EXPECT_EQ(BlendChannel(15, 0, 128), 7);
	EXPECT_EQ(BlendChannel(9, 9, 77), 9);
}

TEST(CreatureSkin, TexelBlendsEachChannelOnItsOwn)
{
	// Alpha, red, green and blue nibbles
	constexpr uint16_t k_Base = 0xF0A3;
	constexpr uint16_t k_Other = 0xF5AF;
	EXPECT_EQ(BlendTexel(k_Base, k_Other, 0), k_Base);
	EXPECT_EQ(BlendTexel(k_Base, k_Other, 255), k_Other);
	// Red 0 -> 5: 640 / 255 = 2.5; green 10 -> 10; blue 3 -> 15: (381 + 1920) / 255 = 9.02
	EXPECT_EQ(BlendTexel(k_Base, k_Other, 128), 0xF2A9);
}

TEST(CreatureSkin, SkinsArePairedByTheirPlaceInTheList)
{
	constexpr std::array<uint32_t, 2> k_Base {0xA, 0xB};
	constexpr std::array<uint32_t, 2> k_Evil {0x1, 0x2};
	EXPECT_EQ(PairedSkin(k_Base, k_Evil, 0xA), 0x1u);
	EXPECT_EQ(PairedSkin(k_Base, k_Evil, 0xB), 0x2u);
	EXPECT_FALSE(PairedSkin(k_Base, k_Evil, 0xC).has_value());
	EXPECT_FALSE(PairedSkin(k_Base, std::span<const uint32_t>(k_Evil).first(1), 0xB).has_value());
}

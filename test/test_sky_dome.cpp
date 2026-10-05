/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "3D/SkyDome.h"

using namespace openblack;
using sky_dome::Rows;

TEST(SkyDome, SetUpBuildsTheWholeDomeThenFollowsFromDay)
{
	sky_dome::Follow follow(0.0f);
	// The whole dome for the night, then the band following the night from row 0, as it was following the day
	const auto first = follow.Advance(0.0f);
	ASSERT_EQ(first.Get().size(), 2u);
	EXPECT_EQ(first.Get()[0], (Rows {.skyType = 0.0f, .first = 0, .count = 256}));
	EXPECT_EQ(first.Get()[1], (Rows {.skyType = 0.0f, .first = 0, .count = 32}));

	// By day nothing more is needed
	sky_dome::Follow day(2.0f);
	EXPECT_EQ(day.Advance(2.0f).Get().size(), 1u);
	EXPECT_TRUE(day.Advance(2.0f).Get().empty());
}

TEST(SkyDome, FollowsABandAFrame)
{
	sky_dome::Follow follow(2.0f);
	(void)follow.Advance(2.0f);
	// Within the hysteresis the dome stays as it is
	EXPECT_TRUE(follow.Advance(1.98f).Get().empty());
	// Past it, the rows start again with the new sky type, which is kept while they are built
	auto frame = follow.Advance(1.9f);
	ASSERT_EQ(frame.Get().size(), 1u);
	EXPECT_EQ(frame.Get()[0], (Rows {.skyType = 1.9f, .first = 0, .count = 32}));
	for (uint16_t row = 32; row < sky_dome::k_Rows; row += 32)
	{
		frame = follow.Advance(1.5f);
		ASSERT_EQ(frame.Get().size(), 1u);
		EXPECT_EQ(frame.Get()[0], (Rows {.skyType = 1.9f, .first = row, .count = 32}));
	}
	// Once built, the next band follows the sky type of that frame
	frame = follow.Advance(1.5f);
	ASSERT_EQ(frame.Get().size(), 1u);
	EXPECT_EQ(frame.Get()[0], (Rows {.skyType = 1.5f, .first = 0, .count = 32}));
}

TEST(SkyDome, JumpRebuildsTheWholeDome)
{
	sky_dome::Follow follow(2.0f);
	(void)follow.Advance(2.0f);
	follow.Jump(0.5f);
	const auto frame = follow.Advance(0.5f);
	ASSERT_EQ(frame.Get().size(), 2u);
	EXPECT_EQ(frame.Get()[0], (Rows {.skyType = 0.5f, .first = 0, .count = 256}));
	EXPECT_EQ(frame.Get()[1], (Rows {.skyType = 0.5f, .first = 0, .count = 32}));
	EXPECT_FLOAT_EQ(follow.Following(), 0.5f);
}

TEST(SkyDome, TimePairs)
{
	// 0 night, 1 dusk, 2 day
	EXPECT_EQ(sky_dome::TimePair(2.0f), (sky_dome::Pair {.lower = 2, .upper = 1, .weight = 0}));
	EXPECT_EQ(sky_dome::TimePair(1.5f), (sky_dome::Pair {.lower = 2, .upper = 1, .weight = 127}));
	// Full dusk is still day into dusk, all dusk
	EXPECT_EQ(sky_dome::TimePair(1.0f), (sky_dome::Pair {.lower = 2, .upper = 1, .weight = 255}));
	EXPECT_EQ(sky_dome::TimePair(0.75f), (sky_dome::Pair {.lower = 1, .upper = 0, .weight = 63}));
	EXPECT_EQ(sky_dome::TimePair(0.0f), (sky_dome::Pair {.lower = 1, .upper = 0, .weight = 255}));
}

TEST(SkyDome, AlignmentPairs)
{
	EXPECT_EQ(sky_dome::AlignmentPair(-0.5f), (sky_dome::Pair {.lower = 0, .upper = 1, .weight = 0}));
	EXPECT_EQ(sky_dome::AlignmentPair(0.5f), (sky_dome::Pair {.lower = 0, .upper = 1, .weight = 127}));
	EXPECT_EQ(sky_dome::AlignmentPair(1.0f), (sky_dome::Pair {.lower = 0, .upper = 1, .weight = 255}));
	EXPECT_EQ(sky_dome::AlignmentPair(1.25f), (sky_dome::Pair {.lower = 1, .upper = 2, .weight = 63}));
	EXPECT_EQ(sky_dome::AlignmentPair(3.0f), (sky_dome::Pair {.lower = 1, .upper = 2, .weight = 255}));
}

TEST(SkyDome, ChannelsBlendInWholeSteps)
{
	// The weights add up to 255 of 256, so even a whole picture loses a step at the top
	EXPECT_EQ(sky_dome::BlendChannel(31, 0, 0), 30);
	EXPECT_EQ(sky_dome::BlendChannel(0, 31, 255), 30);
	EXPECT_EQ(sky_dome::BlendChannel(31, 31, 127), 30);
	EXPECT_EQ(sky_dome::BlendChannel(16, 8, 128), 11);
	EXPECT_EQ(sky_dome::BlendChannel(1, 1, 128), 0);
}

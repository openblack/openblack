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

TEST(SkyDome, DarknessOfAnEvilSky)
{
	EXPECT_EQ(sky_dome::Darkness(2.0f), 0);
	EXPECT_EQ(sky_dome::Darkness(0.8f), 0);
	// Evil at 0.75 of the way: (0.75 - 0.6) * 225
	EXPECT_EQ(sky_dome::Darkness(0.5f), 33);
	// The float 0.6 is a little over it, so even a wholly evil sky only reaches 89
	EXPECT_EQ(sky_dome::Darkness(0.0f), 89);
	EXPECT_EQ(sky_dome::Darkness(-1.0f), 90);
}

TEST(SkyDome, ClearDayIsUntinted)
{
	const auto tint = sky_dome::TintOf({.hazeColour = glm::vec3(60.0f, 70.0f, 80.0f),
	                                    .overcast = 0.0f,
	                                    .flash = 0,
	                                    .darkness = 0,
	                                    .fog = true,
	                                    .weather = true});
	EXPECT_EQ(tint, (sky_dome::Tint {.modulate = glm::u8vec3(255), .add = glm::u8vec3(0)}));
}

TEST(SkyDome, OvercastTurnsTowardsTheHaze)
{
	// A full overcast of 255: white towards the haze, rounded down, and the haze added, rounded down
	const auto tint = sky_dome::TintOf({.hazeColour = glm::vec3(60.9f, 100.0f, 255.0f),
	                                    .overcast = 1.0f,
	                                    .flash = 0,
	                                    .darkness = 0,
	                                    .fog = true,
	                                    .weather = true});
	EXPECT_EQ(tint.modulate, glm::u8vec3(60, 100, 255));
	EXPECT_EQ(tint.add, glm::u8vec3(59, 99, 254));
	// Without the fog setting the overcast changes nothing
	const auto noFog = sky_dome::TintOf(
	    {.hazeColour = glm::vec3(60.0f), .overcast = 1.0f, .flash = 0, .darkness = 0, .fog = false, .weather = true});
	EXPECT_EQ(noFog, sky_dome::Tint {});
}

TEST(SkyDome, EvilDarkensAndFlashWhitens)
{
	// 90 * 90 / 150 = 54 of 256 off, rounded up: 255 - 54 = 201
	const auto evil = sky_dome::TintOf(
	    {.hazeColour = glm::vec3(0.0f), .overcast = 0.0f, .flash = 0, .darkness = 90, .fog = true, .weather = true});
	EXPECT_EQ(evil.modulate, glm::u8vec3(201));
	EXPECT_EQ(sky_dome::TintOf(
	              {.hazeColour = glm::vec3(0.0f), .overcast = 0.0f, .flash = 0, .darkness = 90, .fog = true, .weather = false})
	              .modulate,
	          glm::u8vec3(255));
	// A full flash: the first nearly white, the second added half as far
	const auto flash = sky_dome::TintOf(
	    {.hazeColour = glm::vec3(0.0f), .overcast = 0.0f, .flash = 255, .darkness = 90, .fog = true, .weather = true});
	EXPECT_EQ(flash.modulate, glm::u8vec3(201 + ((54 * 255) >> 8)));
	EXPECT_EQ(flash.add, glm::u8vec3((255 * 127) >> 8));
}

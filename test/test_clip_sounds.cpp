/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <sstream>
#include <string_view>

#include <SASFile.h>
#include <gtest/gtest.h>

#include "Audio/ClipSounds.h"

using namespace openblack;
using namespace openblack::audio;
using namespace openblack::audio::clip_sounds;

TEST(ClipSounds, TheFileListsEachClipsSoundsUntilEnd)
{
	std::istringstream text("1\nM_A_Cow_Walk 18\n1332 4 0 1\n937 4 0 1\n0 4 0 0\nM_P_Thrown 1\n10 6 0 0\nEND\nM_Ignored 3\n");
	const auto file = sas::Parse(text);
	ASSERT_TRUE(file.has_value());
	EXPECT_EQ(file->version, 1);
	ASSERT_EQ(file->clips.size(), 2);
	EXPECT_EQ(file->clips[0].clip, "M_A_Cow_Walk");
	EXPECT_EQ(file->clips[0].soundType, 18);
	ASSERT_EQ(file->clips[0].sounds.size(), 3);
	EXPECT_EQ(file->clips[0].sounds[1].time, 937);
	EXPECT_EQ(file->clips[0].sounds[1].action, 4);
	EXPECT_EQ(file->clips[1].sounds[0].action, 6);
}

TEST(ClipSounds, AFileWithoutKindsReadsNone)
{
	std::istringstream text("0\nM_Clip\n5 7 0 0\nEND\n");
	const auto file = sas::Parse(text);
	ASSERT_TRUE(file.has_value());
	ASSERT_EQ(file->clips.size(), 1);
	EXPECT_EQ(file->clips[0].soundType, 0);
	EXPECT_EQ(file->clips[0].sounds[0].action, 7);
}

TEST(ClipSounds, ThePlayedStretchPassesItsSounds)
{
	const std::vector<sas::FrameSound> sounds {
	    {.time = 0, .action = 4, .mode = 0}, {.time = 485, .action = 4, .mode = 0}, {.time = 937, .action = 4, .mode = 0}};
	// From the place up to but not including where it plays to
	EXPECT_EQ(Passed(sounds, 400, 100, 1500, true), (std::vector<size_t> {1}));
	EXPECT_TRUE(Passed(sounds, 486, 100, 1500, true).empty());
	// A looping clip comes round and plays from its start
	EXPECT_EQ(Passed(sounds, 1400, 200, 1500, true), (std::vector<size_t> {0}));
	// A clip played once that reaches its end plays all its sounds again
	EXPECT_EQ(Passed(sounds, 900, 700, 1500, false), (std::vector<size_t> {2, 0, 1, 2}));
}

TEST(ClipSounds, APersonsSizeIsByTheirSexAndAge)
{
	EXPECT_EQ(SizeOf(k_PeopleSounds, true, false, false), SoundSize::Large);
	EXPECT_EQ(SizeOf(k_PeopleSounds, true, false, true), SoundSize::Medium);
	EXPECT_EQ(SizeOf(k_PeopleSounds, true, true, false), SoundSize::Small);
	EXPECT_EQ(SizeOf(k_PeopleSounds, false, false, false), SoundSize::Small);
	EXPECT_EQ(SizeOf(18, false, false, false), SoundSize::Medium);
}

TEST(ClipSounds, TheTableFindsAClipByName)
{
	const ClipSoundTable table(
	    sas::SASFile {.version = 1, .clips = {{.clip = "M_P_Drowning", .soundType = 1, .sounds = {{257, 134, 0}}}}});
	ASSERT_NE(table.Find("M_P_Drowning"), nullptr);
	EXPECT_EQ(table.Find("M_P_Drowning")->sounds[0].action, 134);
	EXPECT_EQ(table.Find("M_P_Thrown"), nullptr);
}

TEST(ClipSounds, ANamesSoundsGoToTheFirstPackedClipBearingIt)
{
	ClipSoundTable table(
	    sas::SASFile {.version = 1, .clips = {{.clip = "M_P_Drowning", .soundType = 1, .sounds = {{257, 134, 0}}}}});
	const std::array<std::string_view, 4> names {"M_P_Walk", "M_P_Drowning", "M_P_Run", "M_P_Drowning"};
	table.Attach(names);
	EXPECT_EQ(table.OfClip(0), nullptr);
	ASSERT_NE(table.OfClip(1), nullptr);
	EXPECT_EQ(table.OfClip(1)->sounds[0].action, 134);
	EXPECT_EQ(table.OfClip(3), nullptr);
}

TEST(ClipSounds, ASoundsRouteFollowsTheGamesRules)
{
	// A dead person's clip falls silent for the rest of its sounds
	EXPECT_EQ(RouteOf({.soundType = k_PeopleSounds, .isVillager = true, .alive = false}).outcome, Outcome::Stop);
	// The first banter comes from the villager's home, the others from the villager; both from the banter bank
	const auto home = RouteOf({.soundType = k_PeopleSounds, .action = k_HomeBanter, .isVillager = true});
	EXPECT_EQ(home.outcome, Outcome::Play);
	EXPECT_EQ(home.bank, Bank::Banter);
	EXPECT_TRUE(home.fromHome);
	EXPECT_FALSE(RouteOf({.soundType = k_PeopleSounds, .action = k_LastBanter, .isVillager = true}).fromHome);
	// A thrown person screams only early in its flight
	EXPECT_EQ(RouteOf({.soundType = 1, .clip = k_ThrownClip, .isVillager = true, .turnsInState = 14}).outcome, Outcome::Play);
	EXPECT_EQ(RouteOf({.soundType = 1, .clip = k_ThrownClip, .isVillager = true, .turnsInState = 15}).outcome, Outcome::Skip);
	EXPECT_EQ(RouteOf({.soundType = 1, .clip = k_ThrownVortexClip, .isVillager = true, .turnsInState = 10}).outcome,
	          Outcome::Skip);
	// Anything else playing a thrown clip makes no sound with it
	EXPECT_EQ(RouteOf({.soundType = 3, .clip = k_ThrownClip, .isVillager = false, .turnsInState = 0}).outcome, Outcome::Skip);
	EXPECT_EQ(RouteOf({.soundType = 3, .clip = k_ThrownVortexClip, .isVillager = false, .turnsInState = 0}).outcome,
	          Outcome::Skip);
	// Inside the temple only sounds played another way than the ordinary one are heard
	EXPECT_EQ(RouteOf({.soundType = 3, .mode = 0, .insideTemple = true}).outcome, Outcome::Skip);
	EXPECT_EQ(RouteOf({.soundType = 3, .mode = 1, .insideTemple = true}).outcome, Outcome::Play);
}

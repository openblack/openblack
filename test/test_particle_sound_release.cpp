/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Particles/ParticleSoundRelease.h"

using namespace openblack::particles;

TEST(ParticleSoundRelease, ASoftReleasedLoopPlaysToTheEndOfItsPassFadingByItsStep)
{
	// The lightning's hand crackle: a looping sample the file soft-releases with a fade step of 20
	const auto letGo = LetGoOf(true, true, 20);
	EXPECT_TRUE(letGo.releaseLoop);
	EXPECT_EQ(letGo.fadeStep, 20);
	EXPECT_TRUE(letGo.stopWhenSilent);
}

TEST(ParticleSoundRelease, ASoundTheFileDoesNotSoftReleaseIsStoppedAtOnce)
{
	// The spiritual shield's hum: a looping sample the file doesn't soft-release
	EXPECT_EQ(LetGoOf(false, true, 0), (SoundLetGo {.stopNow = true}));
	// Whatever its fade step, and whether its sample loops or not
	EXPECT_EQ(LetGoOf(false, true, 40), (SoundLetGo {.stopNow = true}));
	EXPECT_EQ(LetGoOf(false, false, 0), (SoundLetGo {.stopNow = true}));
	EXPECT_EQ(LetGoOf(false, false, 20), (SoundLetGo {.stopNow = true}));
}

TEST(ParticleSoundRelease, ASoftReleasedSampleThatPlaysOncePlaysOut)
{
	const auto letGo = LetGoOf(true, false, 0);
	EXPECT_EQ(letGo, (SoundLetGo {.releaseLoop = false, .fadeStep = 0, .stopWhenSilent = false}));
	// With a step it still fades as it plays out
	EXPECT_EQ(LetGoOf(true, false, 40), (SoundLetGo {.releaseLoop = false, .fadeStep = 40, .stopWhenSilent = true}));
	// A soft-released loop without a step plays to the end of its pass at its volume
	EXPECT_EQ(LetGoOf(true, true, 0), (SoundLetGo {.releaseLoop = true, .fadeStep = 0, .stopWhenSilent = false}));
}

TEST(ParticleSoundRelease, TheVolumeFallsByTheStepEachTurnToSilence)
{
	uint32_t volume = k_MaxSoundVolume;
	int turns = 0;
	while (volume > 0)
	{
		volume = FadedVolume(volume, 20);
		++turns;
	}
	// 127 in steps of 20 is silent on the seventh turn, counting the turn it is let go on
	EXPECT_EQ(turns, 7);
	EXPECT_EQ(FadedVolume(50, 0), 50u);
	EXPECT_EQ(FadedVolume(10, -5), 10u);
}

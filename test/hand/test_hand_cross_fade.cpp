/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "3D/HandCrossFade.h"

using openblack::HandCrossFade;

TEST(HandCrossFade, LeavesThePlaceAloneWhenNotFading)
{
	const HandCrossFade fade;
	EXPECT_FALSE(fade.IsActive());
	EXPECT_EQ(fade.Apply({1.0f, 2.0f, 3.0f}), glm::vec3(1.0f, 2.0f, 3.0f));
}

TEST(HandCrossFade, FadesAtAnEvenPaceOverATenthAndAThirdOfASecond)
{
	HandCrossFade fade;
	fade.Start({0.0f, 0.0f, 0.0f});
	// The first frame of the fade is already a frame along it
	fade.Update(0.013f);
	EXPECT_NEAR(fade.Apply({10.0f, 0.0f, 0.0f}).x, 1.0f, 1e-4f);
	fade.Update(0.052f);
	EXPECT_NEAR(fade.Apply({10.0f, 0.0f, 0.0f}).x, 5.0f, 1e-4f);
	// The place faded to may move on while the one faded from stays
	EXPECT_NEAR(fade.Apply({20.0f, 0.0f, 0.0f}).x, 10.0f, 1e-4f);
	fade.Update(0.065f);
	EXPECT_FALSE(fade.IsActive());
	EXPECT_EQ(fade.Apply({10.0f, 0.0f, 0.0f}), glm::vec3(10.0f, 0.0f, 0.0f));
}

TEST(HandCrossFade, StartingAgainFadesFromTheNewPlace)
{
	HandCrossFade fade;
	fade.Start({0.0f, 0.0f, 0.0f});
	fade.Update(0.05f);
	fade.Start({4.0f, 0.0f, 0.0f});
	fade.Update(0.0f);
	EXPECT_EQ(fade.Apply({8.0f, 0.0f, 0.0f}), glm::vec3(4.0f, 0.0f, 0.0f));
}

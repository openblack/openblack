/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Gui/ScreenFade.h"

using namespace openblack::gui;

namespace
{
const glm::vec3 k_White(1.0f);

void Step(ScreenFade& fade, float seconds, int frames)
{
	for (int i = 0; i < frames; ++i)
	{
		fade.Update(seconds);
	}
}
} // namespace

TEST(ScreenFade, StartsClear)
{
	const ScreenFade fade;
	EXPECT_EQ(fade.GetColour().a, 0.0f);
}

TEST(ScreenFade, FadesAwayOverASecond)
{
	ScreenFade fade;
	fade.FadeFrom(1.0f, k_White);
	EXPECT_EQ(fade.GetColour(), glm::vec4(1.0f));
	Step(fade, 0.25f, 2);
	EXPECT_FLOAT_EQ(fade.GetColour().a, 0.5f);
	EXPECT_EQ(glm::vec3(fade.GetColour()), k_White);
	Step(fade, 0.25f, 2);
	EXPECT_EQ(fade.GetColour(), glm::vec4(0.0f));
	EXPECT_EQ(fade.GetTurns(), 1u);
}

TEST(ScreenFade, StaysCoveredBeyondAWhole)
{
	ScreenFade fade;
	fade.FadeFrom(1.2f);
	Step(fade, 0.1f, 1);
	EXPECT_FLOAT_EQ(fade.GetColour().a, 1.0f);
	Step(fade, 0.1f, 2);
	EXPECT_NEAR(fade.GetColour().a, 0.9f, 1e-5f);
}

TEST(ScreenFade, KeepsItsColourUntilFadedAway)
{
	ScreenFade fade;
	fade.FadeFrom(1.0f, k_White);
	Step(fade, 0.5f, 1);
	// A fade without a colour of its own takes on the one still fading
	fade.FadeFrom(1.2f);
	EXPECT_EQ(glm::vec3(fade.GetColour()), k_White);
	Step(fade, 0.5f, 3);
	// Faded away, the colour goes back to black for the next
	fade.FadeFrom(1.0f);
	EXPECT_EQ(fade.GetColour(), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

TEST(ScreenFade, FadesThroughAColour)
{
	ScreenFade fade;
	fade.FadeThrough(k_White);
	Step(fade, 0.5f, 1);
	EXPECT_FLOAT_EQ(fade.GetColour().a, 0.5f);
	EXPECT_EQ(fade.GetTurns(), 0u);
	Step(fade, 0.5f, 1);
	EXPECT_FLOAT_EQ(fade.GetColour().a, 1.0f);
	EXPECT_EQ(fade.GetTurns(), 1u);
	Step(fade, 0.5f, 2);
	EXPECT_EQ(fade.GetColour().a, 0.0f);
	EXPECT_EQ(fade.GetTurns(), 2u);
}

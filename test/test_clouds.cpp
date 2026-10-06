/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "3D/Clouds.h"

namespace clouds = openblack::clouds;

TEST(Clouds, LayoutPinsTwoAtTheEnds)
{
	// A fake random picks the low end every time
	const auto layout = clouds::MakeLayout([](float low, float /*high*/) { return low; });
	ASSERT_EQ(layout.size(), clouds::k_Count);
	EXPECT_TRUE(layout[0].pinned);
	EXPECT_EQ(layout[0].track, glm::vec3(8000.0f, 500.0f, 0.0f));
	EXPECT_EQ(layout[1].track, glm::vec3(-8000.0f, 500.0f, 0.0f));
	EXPECT_FLOAT_EQ(layout[1].size, 300.0f);
	EXPECT_FALSE(layout[2].pinned);
	EXPECT_EQ(layout[2].track, glm::vec3(-8000.0f, 300.0f, -5000.0f));
	EXPECT_FLOAT_EQ(layout[2].size, 13.0f);
	EXPECT_FLOAT_EQ(layout[2].edgeShrink, 2.5f);
}

TEST(Clouds, DriftAlongTheTrack)
{
	// 70 units a second
	EXPECT_FLOAT_EQ(clouds::Move({0.0f, 400.0f, 10.0f}, false, 1000.0f).x, 70.0f);
	EXPECT_EQ(clouds::Move({0.0f, 400.0f, 10.0f}, true, 1000.0f), glm::vec3(0.0f, 400.0f, 10.0f));
	// Past the end, back to the start
	EXPECT_NEAR(clouds::Move({7990.0f, 400.0f, 0.0f}, false, 1000.0f).x, -7940.0f, 1e-2f);
}

TEST(Clouds, FadeAtTheTracksEnds)
{
	EXPECT_EQ(clouds::EdgeAlpha({0.0f, 0.0f, 0.0f}, false), 255);
	EXPECT_EQ(clouds::EdgeAlpha({7000.0f, 0.0f, 0.0f}, false), 128);
	EXPECT_EQ(clouds::EdgeAlpha({-8000.0f, 0.0f, 0.0f}, false), 0);
	EXPECT_EQ(clouds::EdgeAlpha({8000.0f, 0.0f, 0.0f}, true), 192);
}

TEST(Clouds, TurnWithTheWindAboutTheMiddle)
{
	const auto middle = clouds::WorldPosition({0.0f, 400.0f, 0.0f});
	EXPECT_EQ(middle, glm::vec3(1280.0f, 400.0f, 1280.0f));
	const auto along = clouds::WorldPosition({1000.0f, 0.0f, 0.0f});
	EXPECT_NEAR(along.x, 1280.0f - 707.10678f, 1e-2f);
	EXPECT_NEAR(along.z, 1280.0f + 707.10678f, 1e-2f);
}

TEST(Clouds, ColourFollowsTheSkysAlignment)
{
	// Good: clear white, drawn a little towards 35
	EXPECT_EQ(clouds::Colour(1.0f, 0xFFFFFF), 0x00DBDBDBu);
	// Neutral: half opaque
	EXPECT_EQ(clouds::Colour(0.0f, 0xFFFFFF) >> 24, 0xC8u);
	// Evil: opaque, dark orange
	EXPECT_EQ(clouds::Colour(-1.0f, 0xFFFFFF) >> 24, 0xFFu);
	// In no light, only the pull towards 35 is left
	EXPECT_EQ(clouds::Colour(1.0f, 0x000000) & 0xFFFFFFu, 0x00232323u);
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Animals/AnimalAnimation.h"

using namespace openblack::animals;

namespace
{
// A looping flap of ten keyframes over 1066 ms that carries its bird 2.77 of its units each play, and a held pose of
// eleven keyframes over 1100 ms
constexpr ClipTiming k_Flap {.playTime = 1066, .frameCount = 10, .looping = true, .playedByTime = false, .stride = 2.77f};
constexpr ClipTiming k_Held {.playTime = 1100, .frameCount = 11, .looping = false, .playedByTime = false, .stride = 0.0f};
} // namespace

TEST(AnimalAnimation, ALoopingClipComesRoundAndAHeldOneStopsAtItsEnd)
{
	EXPECT_EQ(AdvanceClip(k_Flap, 1000, 50), 1050u);
	EXPECT_EQ(AdvanceClip(k_Flap, 1000, 100), 34u);
	EXPECT_EQ(AdvanceClip(k_Flap, 1000, 66), 0u);
	EXPECT_EQ(AdvanceClip(k_Held, 1000, 50), 1050u);
	EXPECT_EQ(AdvanceClip(k_Held, 1000, 500), 1100u);
	EXPECT_EQ(AdvanceClip(k_Held, 1100, 16), 1100u);
}

TEST(AnimalAnimation, AMovingAnimalPlaysItsClipByTheGroundItCovers)
{
	// 10 metres a second for a tenth of a second is a metre; shrunk by a scale of 2, half a metre, of a stride of 2.77
	const auto speed = static_cast<uint16_t>(6554);
	EXPECT_EQ(MovingPlay(k_Flap, speed, 100, 2.0f), 192);
	// Twice the scale, half the play: a bigger bird flaps slower
	EXPECT_EQ(MovingPlay(k_Flap, speed, 100, 4.0f), 96);
	// A frame too short to cover enough ground plays nothing of the clip
	EXPECT_EQ(MovingPlay(k_Flap, speed, 1, 4.0f), 0);
	// A clip with no stride doesn't play by the ground
	EXPECT_EQ(MovingPlay(k_Held, speed, 100, 1.0f), 0);
}

TEST(AnimalAnimation, TheKeyframesAreSpreadEvenlyOverThePlay)
{
	// Looping: ten keyframes over the whole play, the last blending into the first
	auto span = SpanAt(k_Flap, 0);
	EXPECT_EQ(span.from, 0u);
	EXPECT_EQ(span.to, 1u);
	EXPECT_FLOAT_EQ(span.t, 0.0f);
	span = SpanAt(k_Flap, 160);
	EXPECT_EQ(span.from, 1u);
	EXPECT_NEAR(span.t, 0.5f, 0.01f);
	span = SpanAt(k_Flap, 1065);
	EXPECT_EQ(span.from, 9u);
	EXPECT_EQ(span.to, 0u);
	// Held: from the first keyframe at the start to the last at the end
	span = SpanAt(k_Held, 1100);
	EXPECT_EQ(span.from, 10u);
	EXPECT_NEAR(span.t, 0.0f, 0.001f);
	span = SpanAt(k_Held, 55);
	EXPECT_EQ(span.from, 0u);
	EXPECT_NEAR(span.t, 0.5f, 0.001f);
}

TEST(AnimalAnimation, TheDrawClockCountsMillisecondsIntoTheTurn)
{
	EXPECT_EQ(DrawTime(10, 0.0f), 1000u);
	EXPECT_EQ(DrawTime(10, 0.456f), 1045u);
	EXPECT_EQ(DrawTime(10, 1.0f), 1099u);
}

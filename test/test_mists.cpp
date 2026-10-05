/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "3D/Mists.h"

namespace mists = openblack::mists;

TEST(Mists, FramesFollowTheCounter)
{
	EXPECT_EQ(mists::Frame(0), 0);
	EXPECT_EQ(mists::Frame(20), 1);
	EXPECT_EQ(mists::Frame(319), 15);
	// Past sixteen frames they start again, and the counter's own end is frame 13
	EXPECT_EQ(mists::Frame(320), 0);
	EXPECT_EQ(mists::Frame(900), 13);

	EXPECT_EQ(mists::FrameOffset(9, false), glm::vec2(0.125f, 0.125f));
	EXPECT_EQ(mists::FrameOffset(9, true), glm::vec2(0.125f, 0.375f));
	EXPECT_EQ(mists::StartCounter(15.9f), 15);
}

TEST(Mists, AnimationKeepsItsPaceAtAnyFrameRate)
{
	// A millisecond at a time moves the counter as far as a whole second does, but for rounding
	int counter = 0;
	float remainder = 0.0f;
	for (int i = 0; i < 1000; ++i)
	{
		mists::Advance(counter, remainder, 1.0f);
	}
	int once = 0;
	float onceRemainder = 0.0f;
	mists::Advance(once, onceRemainder, 1000.0f);
	EXPECT_NEAR(counter, once, 1);
	EXPECT_EQ(once, 255);

	// It wraps only once it passes 900
	int full = 900;
	float none = 0.0f;
	mists::Advance(full, none, 0.0f);
	EXPECT_EQ(full, 900);
	mists::Advance(full, none, 4.0f);
	EXPECT_EQ(full, 1);
}

TEST(Mists, ShrinkEdgeOn)
{
	// Seen from straight above: the full size; seen level: edgeShrink times smaller
	EXPECT_FLOAT_EQ(mists::EdgeOnSize(10.0f, 3.0f, {0.0f, -100.0f, 0.0f}), 10.0f);
	EXPECT_FLOAT_EQ(mists::EdgeOnSize(10.0f, 3.0f, {100.0f, 0.0f, 0.0f}), 10.0f / 3.0f);
	EXPECT_FLOAT_EQ(mists::EdgeOnSize(10.0f, 3.0f, {0.0f, 0.0f, 0.0f}), 10.0f);
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Camera/NearClipping.h"

using namespace openblack;

TEST(NearClipping, FollowsTheHeightOverTheLand)
{
	EXPECT_FLOAT_EQ(near_clipping::NearPlane(-5.0f, false), 0.3f);
	EXPECT_FLOAT_EQ(near_clipping::NearPlane(0.0f, false), 0.3f);
	EXPECT_FLOAT_EQ(near_clipping::NearPlane(10.0f, false), 1.9f);
	EXPECT_FLOAT_EQ(near_clipping::NearPlane(20.0f, false), 3.5f);
	EXPECT_FLOAT_EQ(near_clipping::NearPlane(500.0f, false), 3.5f);
}

TEST(NearClipping, ScriptsCanClipClose)
{
	EXPECT_FLOAT_EQ(near_clipping::NearPlane(500.0f, true), 0.1f);
	EXPECT_FLOAT_EQ(near_clipping::NearPlane(0.0f, true), 0.1f);
}

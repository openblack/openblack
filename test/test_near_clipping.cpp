/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Camera/CameraPathControl.h"
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

TEST(CameraPathControl, AMovementKeyTakesTheCameraBackOnlyWhenItWouldMoveIt)
{
	// 400 units a second: 2 ms is 0.8 of a unit, 3 ms 1.2
	EXPECT_FALSE(camera_path::KeyStepMoves(0));
	EXPECT_FALSE(camera_path::KeyStepMoves(2));
	EXPECT_TRUE(camera_path::KeyStepMoves(3));
	EXPECT_TRUE(camera_path::KeyStepMoves(16));
	// A long frame counts as a tenth of a second
	EXPECT_FLOAT_EQ(camera_path::FrameSeconds(500), 0.1f);
	EXPECT_TRUE(camera_path::KeyStepMoves(500));
}

TEST(CameraPathControl, GrippingTheLandAlwaysTakesTheCameraBack)
{
	EXPECT_TRUE(camera_path::TakesCameraBack(false, 0, true));
	EXPECT_TRUE(camera_path::TakesCameraBack(true, 16, false));
	EXPECT_FALSE(camera_path::TakesCameraBack(true, 2, false));
	EXPECT_FALSE(camera_path::TakesCameraBack(false, 16, false));
}

TEST(NearClipping, ScriptsCanClipClose)
{
	EXPECT_FLOAT_EQ(near_clipping::NearPlane(500.0f, true), 0.1f);
	EXPECT_FLOAT_EQ(near_clipping::NearPlane(0.0f, true), 0.1f);
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Camera/CameraHelp.h"

using namespace openblack::camera_help;

TEST(CameraHelp, StartsWithEveryFeatureButSelfTilting)
{
	const CameraHelp help;
	EXPECT_EQ(help.features, 0x1BFu);
	EXPECT_EQ(help.features & feature::k_AutoPitch, 0u);
	EXPECT_FLOAT_EQ(help.autoPitch, 0.523599f);
	EXPECT_FLOAT_EQ(help.autoPitchHeight, 75.0f);
}

TEST(CameraHelp, EnablingSetsOnlyTheMaskedFeatures)
{
	CameraHelp help;
	help.Enable(0x08, ~0u);
	EXPECT_EQ(help.features, 0x08u);
	help.Enable(feature::k_AutoPitch, feature::k_AutoPitch);
	EXPECT_EQ(help.features, 0x48u);
	help.Enable(0, feature::k_AutoPitch);
	EXPECT_EQ(help.features, 0x08u);
}

TEST(CameraHelp, TheInterfaceLevelsSetTheFeatures)
{
	CameraHelp help;
	ASSERT_TRUE(help.SetInterfaceLevel(3));
	EXPECT_EQ(help.features, 0x02u);
	ASSERT_TRUE(help.SetInterfaceLevel(15));
	EXPECT_EQ(help.features, 0x3Fu);
	ASSERT_TRUE(help.SetInterfaceLevel(0));
	EXPECT_EQ(help.features, 0x1BFu);
	// The first tutorial levels strafe only, the camera tilting itself to about 26 degrees 15 above the land
	ASSERT_TRUE(help.SetInterfaceLevel(1));
	EXPECT_EQ(help.features, 0x48u);
	EXPECT_NEAR(help.autoPitch, 0.448799f, 1e-6f);
	EXPECT_FLOAT_EQ(help.autoPitchHeight, 15.0f);
	ASSERT_TRUE(help.SetInterfaceLevel(10));
	EXPECT_EQ(help.features, 0x4Au);
}

TEST(CameraHelp, UnknownLevelsChangeNothing)
{
	CameraHelp help;
	help.Enable(0x26, ~0u);
	EXPECT_FALSE(help.SetInterfaceLevel(9));
	EXPECT_FALSE(help.SetInterfaceLevel(16));
	EXPECT_FALSE(help.SetInterfaceLevel(-1));
	EXPECT_EQ(help.features, 0x26u);
}

TEST(CameraHelp, ANewLandGivesEveryFeatureBackAndTheFirstHeight)
{
	CameraHelp help;
	ASSERT_TRUE(help.SetInterfaceLevel(1));
	help.ResetForNewLand();
	EXPECT_EQ(help.features, 0x1BFu);
	EXPECT_FLOAT_EQ(help.autoPitchHeight, 75.0f);
	// The pitch stays as the script left it
	EXPECT_NEAR(help.autoPitch, 0.448799f, 1e-6f);
}

TEST(CameraHelp, WatchingAFightTheCameraCantBeSentToFights)
{
	EXPECT_EQ(DuringFight(0x1BF), 0x1AFu);
}

TEST(CameraHelp, TheSelfTiltingCameraTiltsATenthOfTheWayAtMostTheFramesSeconds)
{
	// Within a hundredth (after the tenth), nothing
	EXPECT_FALSE(AutoPitchInput(0.5f, 0.45f, 0.02f).has_value());
	// A tenth of 0.3 is 0.03, more than the frame's 0.02 seconds
	ASSERT_TRUE(AutoPitchInput(0.5f, 0.2f, 0.02f).has_value());
	EXPECT_NEAR(*AutoPitchInput(0.5f, 0.2f, 0.02f), 0.02f * -150.0f, 1e-4f);
	EXPECT_NEAR(*AutoPitchInput(0.2f, 0.5f, 0.02f), -0.02f * -150.0f, 1e-4f);
	// A slow frame leaves the tenth
	EXPECT_NEAR(*AutoPitchInput(0.5f, 0.2f, 0.1f), 0.03f * -150.0f, 1e-4f);
}

TEST(CameraHelp, TheInterfaceLevelsSetTheKeysAndTheHandsReach)
{
	using openblack::input::BindableActionMap;
	CameraHelp help;
	EXPECT_EQ(help.BlockedActions(), BindableActionMap::NONE);
	// The first tutorial level keeps the camera keys but not the places', and the hand reaches 75
	ASSERT_TRUE(help.SetInterfaceLevel(1));
	EXPECT_FLOAT_EQ(help.handReach, 75.0f);
	EXPECT_EQ(help.BlockedActions(), static_cast<BindableActionMap>(static_cast<uint64_t>(BindableActionMap::ZOOM_TO_TEMPLE) |
	                                                                static_cast<uint64_t>(BindableActionMap::ZOOM_TO_CREATURE) |
	                                                                static_cast<uint64_t>(BindableActionMap::ZOOM_TO_REALM)));
	// Level 2 leaves the reach as it was
	ASSERT_TRUE(help.SetInterfaceLevel(2));
	EXPECT_FLOAT_EQ(help.handReach, 75.0f);
	// Level 8 takes every camera key, but not talking
	ASSERT_TRUE(help.SetInterfaceLevel(8));
	const auto blocked = static_cast<uint64_t>(help.BlockedActions());
	EXPECT_NE(blocked & static_cast<uint64_t>(BindableActionMap::ROTATE_AROUND_MOUSE_ON), 0u);
	EXPECT_NE(blocked & static_cast<uint64_t>(BindableActionMap::ZOOM_OUT), 0u);
	EXPECT_EQ(blocked & static_cast<uint64_t>(BindableActionMap::TALK), 0u);
	EXPECT_EQ(blocked & static_cast<uint64_t>(BindableActionMap::MOVE), 0u);
	// Level 0 gives everything back and the full reach
	ASSERT_TRUE(help.SetInterfaceLevel(0));
	EXPECT_EQ(help.BlockedActions(), BindableActionMap::NONE);
	EXPECT_FLOAT_EQ(help.handReach, 1800.0f);
}

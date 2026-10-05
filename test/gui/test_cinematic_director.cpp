/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include <gtest/gtest.h>

#include "ECS/Systems/Implementations/CinematicDirectorSystem.h"

using openblack::ecs::systems::CinematicDirectorSystem;

TEST(CinematicDirector, AScriptsBarsPutTheInterfaceAway)
{
	CinematicDirectorSystem director;
	EXPECT_TRUE(director.IsInterfaceActive());
	EXPECT_FALSE(director.TakeHideDialogs());

	director.SetWideScreen(true, 7);
	EXPECT_TRUE(director.IsWideScreenOn());
	EXPECT_EQ(director.GetWideScreenOwner(), 7u);
	EXPECT_FALSE(director.IsInterfaceActive());
	// The dialogs are told to hide once
	EXPECT_TRUE(director.TakeHideDialogs());
	EXPECT_FALSE(director.TakeHideDialogs());

	director.SetWideScreen(false, 7);
	EXPECT_FALSE(director.IsWideScreenOn());
	EXPECT_EQ(director.GetWideScreenOwner(), 0u);
	EXPECT_TRUE(director.IsInterfaceActive());
	EXPECT_FALSE(director.TakeHideDialogs());
}

TEST(CinematicDirector, TheGamesOwnBarsLeaveTheInterface)
{
	CinematicDirectorSystem director;
	director.SetWideScreen(true, 0);
	EXPECT_TRUE(director.IsInterfaceActive());
	EXPECT_TRUE(director.TakeHideDialogs());
}

TEST(CinematicDirector, SettingTheSameAgainChangesNothing)
{
	CinematicDirectorSystem director;
	director.SetWideScreen(true, 7);
	director.SetWideScreen(true, 9);
	EXPECT_EQ(director.GetWideScreenOwner(), 7u);
	director.SetWideScreen(false, 9);
	director.SetWideScreen(false, 7);
	EXPECT_EQ(director.GetWideScreenOwner(), 0u);
	EXPECT_TRUE(director.IsInterfaceActive());
}

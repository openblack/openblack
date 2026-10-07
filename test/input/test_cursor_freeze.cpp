/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <tuple>

#include <gtest/gtest.h>

#include "Input/CursorFreeze.h"

using openblack::input::CursorFreeze;

TEST(CursorFreeze, FollowsThePointerWhenNotTurning)
{
	CursorFreeze freeze;
	const auto result = freeze.Update(false, {120, 340});
	EXPECT_EQ(result.cursor, glm::ivec2(120, 340));
	EXPECT_FALSE(result.started);
	EXPECT_FALSE(result.warpTo.has_value());
	EXPECT_FALSE(freeze.IsFrozen());
}

TEST(CursorFreeze, HoldsTheCursorWhereTheTurningStarted)
{
	CursorFreeze freeze;
	auto result = freeze.Update(true, {400, 300});
	EXPECT_TRUE(result.started);
	EXPECT_EQ(result.cursor, glm::ivec2(400, 300));

	// The pointer wanders while the mouse turns the camera; the cursor doesn't
	result = freeze.Update(true, {10, 20});
	EXPECT_FALSE(result.started);
	EXPECT_EQ(result.cursor, glm::ivec2(400, 300));
	result = freeze.Update(true, {900, 700});
	EXPECT_EQ(result.cursor, glm::ivec2(400, 300));
	EXPECT_FALSE(result.warpTo.has_value());
}

TEST(CursorFreeze, PutsThePointerBackWhenTheTurningEnds)
{
	CursorFreeze freeze;
	std::ignore = freeze.Update(true, {400, 300});
	std::ignore = freeze.Update(true, {900, 700});

	auto result = freeze.Update(false, {900, 700});
	ASSERT_TRUE(result.warpTo.has_value());
	EXPECT_EQ(*result.warpTo, glm::ivec2(400, 300));
	// The hand stays put on the frame the turning ends, rather than jumping to the pointer
	EXPECT_EQ(result.cursor, glm::ivec2(400, 300));
	EXPECT_FALSE(freeze.IsFrozen());

	// From then on it follows the pointer, which starts from where it was put back
	result = freeze.Update(false, {405, 302});
	EXPECT_EQ(result.cursor, glm::ivec2(405, 302));
	EXPECT_FALSE(result.warpTo.has_value());
}

TEST(CursorFreeze, EachTurningHoldsItsOwnSpot)
{
	CursorFreeze freeze;
	std::ignore = freeze.Update(true, {100, 100});
	std::ignore = freeze.Update(false, {500, 500});
	const auto result = freeze.Update(true, {250, 260});
	EXPECT_TRUE(result.started);
	EXPECT_EQ(result.cursor, glm::ivec2(250, 260));
}

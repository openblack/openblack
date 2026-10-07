/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <limits>

#include <gtest/gtest.h>

#include "Camera/KeyboardMoveSpeed.h"

using namespace openblack;

TEST(KeyboardMoveSpeed, DefaultLeavesTheGamesDistanceUntouched)
{
	// At the default the camera has to move exactly as far as the game moves it, to the bit
	for (const float distance : {0.0f, 0.4f, 6.666667f, -13.25f, 400.0f})
	{
		EXPECT_EQ(ScaleKeyboardMove(distance, k_KeyboardMoveSpeedDefault), distance);
	}
}

TEST(KeyboardMoveSpeed, ScalesTheDistance)
{
	EXPECT_FLOAT_EQ(ScaleKeyboardMove(6.0f, 2.0f), 12.0f);
	EXPECT_FLOAT_EQ(ScaleKeyboardMove(-6.0f, 0.5f), -3.0f);
}

TEST(KeyboardMoveSpeed, KeepsWithinTheRange)
{
	EXPECT_EQ(ClampKeyboardMoveSpeed(0.0f), k_KeyboardMoveSpeedMin);
	EXPECT_EQ(ClampKeyboardMoveSpeed(-3.0f), k_KeyboardMoveSpeedMin);
	EXPECT_EQ(ClampKeyboardMoveSpeed(100.0f), k_KeyboardMoveSpeedMax);
	EXPECT_EQ(ClampKeyboardMoveSpeed(std::numeric_limits<float>::infinity()), k_KeyboardMoveSpeedMax);
	EXPECT_EQ(ClampKeyboardMoveSpeed(std::numeric_limits<float>::quiet_NaN()), k_KeyboardMoveSpeedDefault);
	EXPECT_EQ(ClampKeyboardMoveSpeed(3.0f), 3.0f);
	// A speed out of range scales as the nearest one in range
	EXPECT_FLOAT_EQ(ScaleKeyboardMove(1.0f, 50.0f), k_KeyboardMoveSpeedMax);
}

TEST(KeyboardMoveSpeed, TwoStepsDoubleAndHalve)
{
	EXPECT_FLOAT_EQ(StepKeyboardMoveSpeed(1.0f, 2), 2.0f);
	EXPECT_FLOAT_EQ(StepKeyboardMoveSpeed(1.0f, -2), 0.5f);
	EXPECT_FLOAT_EQ(StepKeyboardMoveSpeed(1.0f, 1), 1.41421356f);
	EXPECT_FLOAT_EQ(StepKeyboardMoveSpeed(4.0f, 2), 8.0f);
}

TEST(KeyboardMoveSpeed, SteppingBackReturnsToExactlyTheDefault)
{
	auto speed = k_KeyboardMoveSpeedDefault;
	for (int i = 0; i < 3; ++i)
	{
		speed = StepKeyboardMoveSpeed(speed, 1);
	}
	for (int i = 0; i < 3; ++i)
	{
		speed = StepKeyboardMoveSpeed(speed, -1);
	}
	EXPECT_EQ(speed, k_KeyboardMoveSpeedDefault);
	// A speed set off the steps by the slider snaps onto the nearest step first
	EXPECT_FLOAT_EQ(StepKeyboardMoveSpeed(1.1f, 0), 1.0f);
	EXPECT_FLOAT_EQ(StepKeyboardMoveSpeed(1.1f, 2), 2.0f);
}

TEST(KeyboardMoveSpeed, SteppingStopsAtTheEnds)
{
	EXPECT_EQ(StepKeyboardMoveSpeed(k_KeyboardMoveSpeedMax, 1), k_KeyboardMoveSpeedMax);
	EXPECT_EQ(StepKeyboardMoveSpeed(k_KeyboardMoveSpeedMin, -1), k_KeyboardMoveSpeedMin);
	EXPECT_EQ(StepKeyboardMoveSpeed(k_KeyboardMoveSpeedDefault, 100), k_KeyboardMoveSpeedMax);
	EXPECT_EQ(StepKeyboardMoveSpeed(k_KeyboardMoveSpeedDefault, -100), k_KeyboardMoveSpeedMin);
}

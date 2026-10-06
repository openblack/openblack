/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "3D/DayNightClock.h"

using openblack::DayNightClock;

namespace
{
DayNightClock ResetClock()
{
	DayNightClock clock;
	clock.Reset();
	return clock;
}
} // namespace

TEST(DayNightClock, TheGameCycleHasShortNights)
{
	const auto clock = ResetClock();
	// Night is 8.3% of a half day, about an hour, and the sky turns a quarter of the change's 0.84 hours either side
	const auto& times = clock.GetVisualTimes();
	EXPECT_NEAR(times[0], 0.786f, 1e-4f);
	EXPECT_NEAR(times[1], 1.206f, 1e-4f);
	EXPECT_NEAR(times[2], 1.626f, 1e-4f);
	EXPECT_NEAR(times[3], 2.046f, 1e-4f);
}

TEST(DayNightClock, ScriptTimeFallsOnTheStandardHours)
{
	const auto clock = ResetClock();
	const auto& times = clock.GetVisualTimes();
	for (size_t i = 0; i < times.size(); ++i)
	{
		EXPECT_NEAR(clock.ScriptToVisual(DayNightClock::k_ScriptTimes.at(i)), times.at(i), 1e-5f);
		EXPECT_NEAR(clock.VisualToScript(times.at(i)), DayNightClock::k_ScriptTimes.at(i), 1e-5f);
	}
	// Noon and midnight stay, and the evening mirrors the morning
	EXPECT_FLOAT_EQ(clock.ScriptToVisual(12.0f), 12.0f);
	EXPECT_FLOAT_EQ(clock.ScriptToVisual(0.0f), 0.0f);
	EXPECT_NEAR(clock.ScriptToVisual(20.5f), 24.0f - clock.ScriptToVisual(3.5f), 1e-5f);
	EXPECT_FLOAT_EQ(clock.GetScriptTime(), 12.0f);
}

TEST(DayNightClock, TheSkyTurnsAtTheVisualHours)
{
	const auto clock = ResetClock();
	EXPECT_FLOAT_EQ(clock.SkyType(0.5f), 0.0f);
	EXPECT_FLOAT_EQ(clock.SkyType(1.4f), 1.0f);
	EXPECT_FLOAT_EQ(clock.SkyType(12.0f), 2.0f);
	EXPECT_FLOAT_EQ(clock.SkyType(23.5f), 0.0f);
}

TEST(DayNightClock, ADayLastsItsDuration)
{
	auto clock = ResetClock();
	// 1700 seconds is 708 tenths of a second an hour; a turn is a tenth of a second
	constexpr float k_HoursPerTurn = 10.0f / 708.0f * 0.1f;
	for (int turn = 0; turn < 100; ++turn)
	{
		clock.ProcessTurn();
	}
	EXPECT_NEAR(clock.GetVisualTime(), 12.0f + 100.0f * k_HoursPerTurn, 1e-4f);

	// Stopped, the time stays
	const float stopped = clock.GetVisualTime();
	clock.SetRunning(false);
	clock.ProcessTurn();
	EXPECT_FLOAT_EQ(clock.GetVisualTime(), stopped);
}

TEST(DayNightClock, MovesToAnHourInItsSeconds)
{
	auto clock = ResetClock();
	clock.SetRunning(false);
	// Noon back to midnight, twelve hours in ten seconds: 0.12 hours a turn
	clock.MoveScriptTime(0.0f, 10.0f);
	for (int turn = 0; turn < 50; ++turn)
	{
		clock.ProcessTurn();
	}
	EXPECT_NEAR(clock.GetVisualTime(), 6.0f, 1e-3f);
	for (int turn = 0; turn < 51; ++turn)
	{
		clock.ProcessTurn();
	}
	EXPECT_NEAR(clock.GetVisualTime(), 0.0f, 1e-3f);
}

TEST(DayNightClock, ALandsCycleIsClamped)
{
	DayNightClock clock;
	clock.SetCycleFromLand(1700.0f, 2.0f, 0.5f);
	// All night: the change is none
	const auto& times = clock.GetVisualTimes();
	EXPECT_FLOAT_EQ(times[0], 12.0f);
	EXPECT_FLOAT_EQ(times[3], 12.0f);
}

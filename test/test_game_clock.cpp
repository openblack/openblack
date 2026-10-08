/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstdint>

#include <gtest/gtest.h>

#define LOCATOR_IMPLEMENTATIONS
#include <ECS/Systems/Implementations/TimeSystem.h>

using openblack::ecs::systems::TimeSystem;

namespace
{
struct Clock
{
	uint32_t now = 1000;
	TimeSystem time {[this] { return now; }};

	/// Plays a frame: the turn if one is due, then the frame clock. Whether it played a turn
	bool Frame()
	{
		const auto played = time.IsTurnDue();
		if (played)
		{
			time.StartTurn();
		}
		time.UpdateFrame();
		return played;
	}
};
} // namespace

TEST(GameClock, TurnsFollowTheTimer)
{
	Clock clock;
	clock.time.StartGameClock(false);
	// turn 0 is due at once, and turn 1 after 100 ms
	EXPECT_TRUE(clock.Frame());
	EXPECT_EQ(clock.time.GetTurn(), 1);
	clock.now += 99;
	EXPECT_FALSE(clock.Frame());
	clock.now += 1;
	EXPECT_TRUE(clock.Frame());
	EXPECT_EQ(clock.time.GetTurn(), 2);
}

TEST(GameClock, NoTurnLosesItsLeftover)
{
	Clock clock;
	clock.time.StartGameClock(false);
	clock.Frame();
	// Frames 150 ms apart still play a turn every 100 ms on average
	int turns = 0;
	for (int i = 0; i < 20; ++i)
	{
		clock.now += 150;
		turns += clock.Frame() ? 1 : 0;
		clock.now += 1;
		turns += clock.Frame() ? 1 : 0;
	}
	EXPECT_EQ(clock.time.GetTurn(), 1 + turns);
	EXPECT_EQ(turns, 30);
}

TEST(GameClock, OneTurnAFrame)
{
	Clock clock;
	clock.time.StartGameClock(false);
	clock.Frame();
	clock.now += 500;
	EXPECT_TRUE(clock.Frame());
	EXPECT_EQ(clock.time.GetTurn(), 2);
}

TEST(GameClock, FarBehindGivesUpTheLostTime)
{
	Clock clock;
	clock.time.StartGameClock(false);
	clock.Frame();
	clock.now += 10000;
	EXPECT_TRUE(clock.Frame());
	// Back to the turn played: one more turn after another 100 ms, not a hundred to catch up
	clock.now += 50;
	EXPECT_FALSE(clock.Frame());
	clock.now += 50;
	EXPECT_TRUE(clock.Frame());
	clock.now += 1;
	EXPECT_FALSE(clock.Frame());
}

TEST(GameClock, PausingStopsTheClock)
{
	Clock clock;
	clock.time.StartGameClock(true);
	clock.now += 1000;
	EXPECT_FALSE(clock.Frame());
	EXPECT_EQ(clock.time.GetFrameGameTime().count(), 0);
	clock.time.SetPaused(false);
	EXPECT_TRUE(clock.Frame());
	clock.now += 50;
	clock.Frame();
	clock.time.SetPaused(true);
	clock.now += 5000;
	clock.time.SetPaused(false);
	// The paused time doesn't count: the next turn is 50 ms on
	clock.now += 49;
	EXPECT_FALSE(clock.Frame());
	clock.now += 1;
	EXPECT_TRUE(clock.Frame());
}

TEST(GameClock, SpeedScalesTheTimer)
{
	Clock clock;
	clock.time.StartGameClock(false);
	clock.Frame();
	clock.time.SetSpeed(2.0f);
	clock.now += 50;
	EXPECT_TRUE(clock.Frame());
	clock.time.SetSpeed(0.5f);
	clock.now += 199;
	EXPECT_FALSE(clock.Frame());
	clock.now += 1;
	EXPECT_TRUE(clock.Frame());
}

TEST(GameClock, TheFrameClockFollowsTheTurn)
{
	Clock clock;
	clock.time.StartGameClock(false);
	clock.Frame();
	clock.now += 30;
	clock.Frame();
	EXPECT_EQ(clock.time.GetFrameGameTime().count(), 30);
	EXPECT_FLOAT_EQ(clock.time.GetTurnFraction(), 0.3f);
	clock.now += 30;
	clock.Frame();
	EXPECT_EQ(clock.time.GetFrameGameTime().count(), 30);
	EXPECT_FLOAT_EQ(clock.time.GetTurnFraction(), 0.6f);
	// The next turn starts the fraction again
	clock.now += 40;
	EXPECT_TRUE(clock.Frame());
	EXPECT_EQ(clock.time.GetFrameGameTime().count(), 40);
	EXPECT_FLOAT_EQ(clock.time.GetTurnFraction(), 0.0f);
}

TEST(GameClock, TheFrameRealTimeIsWholeMillisecondsOfTheWallClockAndNeverNone)
{
	Clock clock;
	clock.time.Start();
	clock.now += 16;
	clock.time.Update();
	EXPECT_EQ(clock.time.GetFrameRealTime().count(), 16);
	// A frame within the same millisecond still takes one
	clock.time.Update();
	EXPECT_EQ(clock.time.GetFrameRealTime().count(), 1);
	// Pausing the game doesn't stop it
	clock.time.StartGameClock(true);
	clock.now += 33;
	clock.time.Update();
	EXPECT_EQ(clock.time.GetFrameRealTime().count(), 33);
}

TEST(GameClock, TheHandAndCameraStepByRealTimeExceptInAScriptsCutScene)
{
	using namespace std::chrono_literals;
	EXPECT_EQ(openblack::ecs::systems::CameraStep(17ms, 0ms, false), 17ms);
	EXPECT_EQ(openblack::ecs::systems::CameraStep(17ms, 20ms, true), 20ms);
}

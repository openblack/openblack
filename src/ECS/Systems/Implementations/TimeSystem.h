/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <chrono>
#include <functional>

#include "ECS/Systems/TimeSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::ecs::systems
{

/// A timer of milliseconds that runs at a speed, and can be stopped and started again from where it stopped
struct GameTimer
{
	uint32_t base {0};       ///< the tick count it last counted from
	int32_t elapsed {0};     ///< the scaled milliseconds it had counted by then
	float speed {1.0f};      ///< 0 while stopped
	float savedSpeed {1.0f}; ///< the speed to start again at

	[[nodiscard]] int32_t Milliseconds(uint32_t now) const;
	void Stop(uint32_t now);
	/// Running, the time so far keeps the old speed; stopped, the speed is kept for when it starts again
	void SetSpeed(float factor, uint32_t now);
	/// Starts again at the saved speed, the time it was stopped not counting
	void Start(uint32_t now);
};

class TimeSystem final: public TimeSystemInterface
{
public:
	/// The tick count in milliseconds, which may wrap; the wall clock by default
	using TickSource = std::function<uint32_t()>;

	TimeSystem();
	explicit TimeSystem(TickSource ticks);

	void Start() override;
	void Update() override;
	[[nodiscard]] std::chrono::milliseconds GetElapsedTime() const override;

	void StartGameClock(bool paused) override;
	[[nodiscard]] bool IsTurnDue() override;
	void StartTurn() override;
	void UpdateFrame() override;

	[[nodiscard]] uint32_t GetTurn() const override { return _turn; }
	[[nodiscard]] float GetTurnFraction() const override { return _turnFraction; }
	[[nodiscard]] std::chrono::milliseconds GetFrameGameTime() const override
	{
		return std::chrono::milliseconds(_frameGameMs);
	}

	[[nodiscard]] std::chrono::milliseconds GetFrameRealTime() const override
	{
		return std::chrono::milliseconds(_frameRealMs);
	}

	void SetPaused(bool paused) override;
	[[nodiscard]] bool IsPaused() const override { return _paused; }
	void SetSpeed(float speed) override;
	[[nodiscard]] float GetSpeed() const override { return _speed; }

private:
	/// Sets the timer back to the current turn, from now
	void ResetTimerToTurn();

	TickSource _ticks;
	/// The wall clock's whole milliseconds at the last frame, and the frame's real time
	uint32_t _lastFrameTicks {0};
	uint32_t _frameRealMs {1};
	std::chrono::time_point<std::chrono::steady_clock> _start;
	std::chrono::milliseconds _elapsedTime {0};

	GameTimer _timer;
	uint32_t _turn {0};
	bool _paused {true};
	float _speed {1.0f};
	uint32_t _turnsThisFrame {0};
	// The frame clock: the timer at the last frame, the turn then, and the time past that turn
	int32_t _lastFrameSample {0};
	uint32_t _lastFrameTurn {0};
	int32_t _turnRemainderMs {0};
	/// The turn's start plus the time past it, which can go back
	uint32_t _visualMs {0};
	uint32_t _frameGameMs {0};
	float _turnFraction {0.0f};
};
} // namespace openblack::ecs::systems

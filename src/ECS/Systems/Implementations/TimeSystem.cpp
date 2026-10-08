/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "TimeSystem.h"

#include <algorithm>
#include <utility>

#include "3D/MapCoords.h"

using namespace openblack::ecs::systems;

namespace
{
constexpr auto k_TurnMs = static_cast<uint32_t>(TimeSystemInterface::k_TurnDuration.count());
/// Further behind than this, the game gives up on the time it lost
constexpr int32_t k_MaxLagMs = 2000;
constexpr uint32_t k_MaxTurnsPerFrame = 1;
constexpr int32_t k_MaxRemainderMs = 99;
constexpr float k_FractionPerMs = 0.01f;
/// The timer is started at this speed so that taking its saved speed counts from now
constexpr float k_StartSpeed = 0.00001f;

uint32_t WallTicks()
{
	using namespace std::chrono;
	return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}
} // namespace

int32_t GameTimer::Milliseconds(uint32_t now) const
{
	// The ticks since the base, scaled, plus what was counted by then, as one float sum truncated
	const auto ticks = static_cast<float>(now - base);
	return openblack::map_coords::FtoL(ticks * speed + static_cast<float>(elapsed));
}

void GameTimer::Stop(uint32_t now)
{
	if (speed != 0.0f)
	{
		savedSpeed = speed;
		elapsed = Milliseconds(now);
		base = now;
		speed = 0.0f;
	}
}

void GameTimer::SetSpeed(float factor, uint32_t now)
{
	if (speed != 0.0f)
	{
		elapsed = Milliseconds(now);
		base = now;
		speed = factor;
	}
	else
	{
		savedSpeed = factor;
	}
}

void GameTimer::Start(uint32_t now)
{
	speed = k_StartSpeed;
	SetSpeed(savedSpeed, now);
}

TimeSystem::TimeSystem()
    : TimeSystem(&WallTicks)
{
}

TimeSystem::TimeSystem(TickSource ticks)
    : _ticks(std::move(ticks))
{
}

void TimeSystem::Start()
{
	_start = std::chrono::steady_clock::now();
	_lastFrameTicks = _ticks();
}

void TimeSystem::Update()
{
	auto now = std::chrono::steady_clock::now();
	_elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - _start);
	// The frame's real time in whole milliseconds, never none
	const auto ticks = _ticks();
	const auto step = static_cast<int32_t>(ticks - _lastFrameTicks);
	_frameRealMs = step > 0 ? static_cast<uint32_t>(step) : 1u;
	_lastFrameTicks = ticks;
}

std::chrono::milliseconds TimeSystem::GetElapsedTime() const
{
	return _elapsedTime;
}

void TimeSystem::StartGameClock(bool paused)
{
	const auto now = _ticks();
	_turn = 0;
	_timer.base = now;
	_timer.elapsed = 0;
	_timer.Stop(now);
	_timer.Start(now);
	_paused = paused;
	if (_paused)
	{
		_timer.Stop(now);
	}
	_turnsThisFrame = 0;
	_lastFrameSample = 0;
	_lastFrameTurn = 0;
	_turnRemainderMs = 0;
	_visualMs = 0;
	_frameGameMs = 0;
	_turnFraction = 0.0f;
	ResetTimerToTurn();
}

bool TimeSystem::IsTurnDue()
{
	if (_paused || _turnsThisFrame >= k_MaxTurnsPerFrame)
	{
		return false;
	}
	const auto timer = _timer.Milliseconds(_ticks());
	const auto due = static_cast<int32_t>(_turn * k_TurnMs);
	if (timer - due > k_MaxLagMs)
	{
		ResetTimerToTurn();
	}
	// From the time taken before any reset
	return timer >= due;
}

void TimeSystem::StartTurn()
{
	++_turnsThisFrame;
	if (!_paused)
	{
		++_turn;
	}
}

void TimeSystem::ResetTimerToTurn()
{
	const auto now = _ticks();
	const auto running = _timer.speed != 0.0f;
	_timer.Stop(now);
	_timer.base = now;
	_timer.elapsed = static_cast<int32_t>(_turn * k_TurnMs);
	if (running)
	{
		_timer.Start(now);
	}
}

void TimeSystem::UpdateFrame()
{
	_turnsThisFrame = 0;
	if (_paused)
	{
		// The fraction stays as it was
		_frameGameMs = 0;
		return;
	}
	const auto sample = _timer.Milliseconds(_ticks());
	const auto delta = sample - _lastFrameSample;
	_lastFrameSample = sample;
	// The time past the last turn played: a new turn takes a turn's length off it
	if (_lastFrameTurn == _turn)
	{
		_turnRemainderMs += delta;
	}
	else
	{
		_turnRemainderMs += delta - static_cast<int32_t>(k_TurnMs);
		_lastFrameTurn = _turn;
	}
	_turnRemainderMs = std::clamp(_turnRemainderMs, 0, k_MaxRemainderMs);
	// The visual clock can go back, and then the frame takes no time
	const auto visual = _turn * k_TurnMs + static_cast<uint32_t>(_turnRemainderMs);
	if (visual < _visualMs)
	{
		_visualMs = visual;
		_turnRemainderMs = 0;
	}
	_frameGameMs = visual - _visualMs;
	_visualMs = visual;
	_turnFraction = static_cast<float>(_turnRemainderMs) * k_FractionPerMs;
}

void TimeSystem::SetPaused(bool paused)
{
	if (paused == _paused)
	{
		return;
	}
	_paused = paused;
	const auto now = _ticks();
	if (paused)
	{
		_timer.Stop(now);
	}
	else
	{
		_timer.Start(now);
	}
}

void TimeSystem::SetSpeed(float speed)
{
	_speed = speed;
	_timer.SetSpeed(speed, _ticks());
}

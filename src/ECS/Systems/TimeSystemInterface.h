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

#include <entt/fwd.hpp>

namespace openblack::ecs::systems
{

/// The real time since the game started, and the game clock: a timer that runs at the game's speed and stops while the
/// game is paused, which the game's turns follow, ten a second.
///
/// A turn is due once the timer reaches the turn number times the turn's length, so no turn loses what is left of the
/// one before. When the game falls more than two seconds behind, it gives up on the lost time rather than racing to
/// catch up, and it never plays more than one turn a frame.
class TimeSystemInterface
{
public:
	static constexpr auto k_TurnDuration = std::chrono::milliseconds(100);

	virtual ~TimeSystemInterface() = default;

	virtual void Start() = 0;
	virtual void Update() = 0;
	[[nodiscard]] virtual std::chrono::milliseconds GetElapsedTime() const = 0;

	/// Starts the game clock from turn 0, as a map starts
	virtual void StartGameClock(bool paused) = 0;
	/// Whether the game is to play a turn now
	[[nodiscard]] virtual bool IsTurnDue() = 0;
	/// The turn number goes up as a turn starts, unless the game is paused
	virtual void StartTurn() = 0;
	/// Works out the frame's game time and how far through its turn the game is, after the frame's turns
	virtual void UpdateFrame() = 0;

	[[nodiscard]] virtual uint32_t GetTurn() const = 0;
	/// How far the game is through the turn after the last one played, from 0 to 0.99
	[[nodiscard]] virtual float GetTurnFraction() const = 0;
	/// The game time of the frame, in whole milliseconds: none while paused, and quicker or slower with the game's speed
	[[nodiscard]] virtual std::chrono::milliseconds GetFrameGameTime() const = 0;
	/// The real time of the frame in whole milliseconds, the wall clock's whole milliseconds now less those at the last
	/// frame, at least 1: what the hand and the camera step by
	[[nodiscard]] virtual std::chrono::milliseconds GetFrameRealTime() const = 0;

	/// Pausing stops the game clock; unpausing starts it again from where it stopped
	virtual void SetPaused(bool paused) = 0;
	[[nodiscard]] virtual bool IsPaused() const = 0;
	/// The game's speed: 1 is normal and 2 twice as fast. The time already gone keeps the speed it went at
	virtual void SetSpeed(float speed) = 0;
	[[nodiscard]] virtual float GetSpeed() const = 0;
};

/// What the hand and the camera step by in a frame: the frame's real time, except while a script holds the widescreen
/// for a cut scene, when they keep to the game's time
[[nodiscard]] constexpr std::chrono::milliseconds CameraStep(std::chrono::milliseconds realTime,
                                                             std::chrono::milliseconds gameTime, bool scriptWidescreen)
{
	return scriptWidescreen ? gameTime : realTime;
}

} // namespace openblack::ecs::systems

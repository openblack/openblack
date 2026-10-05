/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>

namespace openblack
{

/// Black & White's clock of day and night, in hours from 0 to 24.
///
/// The visual time runs evenly, a whole day in a number of seconds of game time, moving on once a turn. The sky turns
/// at four hours of each half day: full night, the start and end of dusk, and full day, which the cycle's length of
/// night and of change decide. Scripts see the script time instead: the visual time stretched so those four hours fall
/// at 3.5, 7.5, 8 and 8.5.
class DayNightClock
{
public:
	/// The hours of the script time the sky turns at: full night, dusk's start and end, full day
	static constexpr std::array<float, 4> k_ScriptTimes = {3.5f, 7.5f, 8.0f, 8.5f};
	/// The game's cycle: a day of 1700 seconds, 8.3% of it night and 7% of it changing
	static constexpr float k_DefaultDuration = 1700.0f;
	static constexpr float k_DefaultNight = 0.083f;
	static constexpr float k_DefaultChange = 0.07f;

	/// As a land opens: the game's cycle, running, at noon
	void Reset();
	/// One turn of game time
	void ProcessTurn();

	/// A day of `duration` seconds, with `night` and `change` the fractions of it at night and changing
	void SetCycle(float duration, float night, float change);
	/// As a land's script sets it: the night at most the whole day, and the change at most what is left
	void SetCycleFromLand(float duration, float night, float change);
	/// Whether the clock runs
	void SetRunning(bool running) { _scale = running ? 1.0f : 0.0f; }
	/// Jumps to an hour of script time
	void SetScriptTime(float hour);
	/// Moves to an hour of script time over `seconds` of game time, the short way round
	void MoveScriptTime(float hour, float seconds);

	[[nodiscard]] float GetVisualTime() const { return _visualTime; }
	[[nodiscard]] float GetScriptTime() const { return VisualToScript(_visualTime); }
	/// The hours of the visual time the sky turns at, as k_ScriptTimes
	[[nodiscard]] const std::array<float, 4>& GetVisualTimes() const { return _times; }
	/// The sky at an hour of visual time: 0 at night, 1 at dusk and 2 by day, between them as it turns
	[[nodiscard]] float SkyType(float visualHour) const;

	[[nodiscard]] float ScriptToVisual(float hour) const;
	[[nodiscard]] float VisualToScript(float hour) const;

private:
	void SetTarget(float hour, float seconds);

	float _visualTime {12.0f};
	/// The hour the visual time moves to, and how many hours a second it moves at most
	float _target {12.0f};
	float _step {2.5f};
	/// The seconds of a move to an hour, while one is under way
	float _moveSeconds {0.0f};
	/// Hours a second by day and by night
	float _dayRate {0.0f};
	float _nightRate {0.0f};
	float _scale {1.0f};
	std::array<float, 4> _times {k_ScriptTimes};
};

} // namespace openblack

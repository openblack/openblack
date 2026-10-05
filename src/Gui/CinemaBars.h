/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

namespace openblack::gui
{

/// Black bars across the top and bottom of the screen that the scripts slide in for their cut scenes, turning the picture
/// to 16:9. They slide in a set time of game time, from wherever they are. A new game starts with them sliding out.
/// The time they take is the game's, passed in each time.
class CinemaBars
{
public:
	/// Slides the bars in or out
	void Set(bool on, float transitionSeconds);
	/// Slides them on by the game time of the frame, none while the game is paused
	void Update(float gameMilliseconds, float transitionSeconds);
	/// How far in the bars are, 0 for none and 1 for a 16:9 picture
	[[nodiscard]] float GetFraction() const { return _fraction; }
	/// Whether the bars are in or coming in
	[[nodiscard]] bool IsOn() const { return _on; }
	/// Whether the bars have finished sliding
	[[nodiscard]] bool IsTransitionFinished() const { return _on ? _fraction >= 1.0f : _fraction <= 0.0f; }
	/// The height of each bar in pixels on a screen, none on screens 16:9 or wider
	[[nodiscard]] static int BarHeight(int width, int height, float fraction);

private:
	[[nodiscard]] float Fraction() const;

	bool _on {false};
	/// Game time since they started sliding, in milliseconds
	float _timer {0.0f};
	float _transitionSeconds {0.0f};
	float _fraction {0.0f};
};

} // namespace openblack::gui

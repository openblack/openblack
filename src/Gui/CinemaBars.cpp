/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CinemaBars.h"

#include <cmath>

#include <algorithm>

using namespace openblack::gui;

void CinemaBars::Set(bool on, float transitionSeconds)
{
	if (on == _on)
	{
		return;
	}
	// Turning round partway goes back from where the bars are
	_transitionSeconds = transitionSeconds;
	const float fraction = Fraction();
	_on = on;
	_timer = (on ? fraction : 1.0f - fraction) * transitionSeconds * 1000.0f;
}

void CinemaBars::Update(float gameMilliseconds, float transitionSeconds)
{
	_transitionSeconds = transitionSeconds;
	_timer += gameMilliseconds;
	_fraction = Fraction();
}

float CinemaBars::Fraction() const
{
	const float slid = _transitionSeconds > 0.0f ? std::abs(_timer * 0.001f / _transitionSeconds) : 1.0f;
	return std::clamp(_on ? slid : 1.0f - slid, 0.0f, 1.0f);
}

int CinemaBars::BarHeight(int width, int height, float fraction)
{
	// Half what the picture loses to 16:9, rounded towards zero
	const auto lost = static_cast<int>((static_cast<float>(height) - static_cast<float>(width) * 0.5625f) * fraction);
	return std::max(0, lost / 2);
}

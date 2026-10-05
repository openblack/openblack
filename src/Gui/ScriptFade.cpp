/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ScriptFade.h"

#include <cstdint>

using namespace openblack::gui;

namespace
{
/// Ten game turns a second
constexpr double k_TurnsPerSecond = 10.0;

uint32_t WithAlpha(uint32_t colour, uint32_t alpha)
{
	return (colour & 0xFFFFFFu) | ((alpha & 0xFFu) << 24u);
}
} // namespace

void ScriptFade::FadeTo(uint8_t red, uint8_t green, uint8_t blue, int8_t seconds)
{
	_colour = (_colour & 0xFF000000u) | (static_cast<uint32_t>(red) << 16u) | (static_cast<uint32_t>(green) << 8u) | blue;
	if (seconds <= 0)
	{
		_colour = WithAlpha(_colour, 0xFF);
		_rate = 0.0f;
		return;
	}
	_alpha = 0.0f;
	_rate = static_cast<float>(255.0 / (seconds * k_TurnsPerSecond));
}

void ScriptFade::FadeBackToNormal(int8_t seconds)
{
	if (seconds <= 0)
	{
		_colour = WithAlpha(_colour, 0);
		_rate = 0.0f;
		return;
	}
	_alpha = 255.0f;
	_rate = -static_cast<float>(255.0 / (seconds * k_TurnsPerSecond));
}

void ScriptFade::ProcessTurn()
{
	if (_rate == 0.0f)
	{
		return;
	}
	const float alpha = _alpha + _rate;
	// At either end it stops, its alpha left where it was going
	if (alpha > 255.0f)
	{
		_colour = WithAlpha(_colour, 0xFF);
		_rate = 0.0f;
	}
	else if (alpha < 0.0f)
	{
		_colour = WithAlpha(_colour, 0);
		_rate = 0.0f;
	}
	else
	{
		_alpha = alpha;
		_colour = WithAlpha(_colour, static_cast<uint32_t>(static_cast<int32_t>(alpha)));
	}
}

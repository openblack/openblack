/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ScreenFade.h"

#include <algorithm>

using namespace openblack::gui;

void ScreenFade::FadeFrom(float amount)
{
	_amount = amount;
	_target = 0.0f;
}

void ScreenFade::FadeFrom(float amount, glm::vec3 colour)
{
	FadeFrom(amount);
	_colour = colour;
	_turns = 0;
}

void ScreenFade::FadeThrough(glm::vec3 colour)
{
	_amount = 0.0f;
	_target = 1.0f;
	_colour = colour;
	_turns = 0;
}

void ScreenFade::Update(float seconds)
{
	if (_amount < _target)
	{
		_amount += seconds;
		if (_amount >= _target)
		{
			_amount = _target;
			_target = 0.0f;
			++_turns;
		}
	}
	else if (_amount > _target)
	{
		_amount -= seconds;
		if (_amount <= _target)
		{
			_amount = _target;
			++_turns;
			_colour = glm::vec3(0.0f);
		}
	}
}

glm::vec4 ScreenFade::GetColour() const
{
	return {_colour, std::min(_amount, 1.0f)};
}

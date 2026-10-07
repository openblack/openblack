/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CursorFreeze.h"

using namespace openblack::input;

CursorFreeze::Result CursorFreeze::Update(bool freeze, glm::ivec2 pointer)
{
	if (freeze)
	{
		const bool started = !_frozenAt.has_value();
		if (started)
		{
			_frozenAt = pointer;
		}
		return {.cursor = *_frozenAt, .started = started};
	}
	if (_frozenAt.has_value())
	{
		// The turning ended: the pointer goes back to where the cursor was held, and the cursor stays there this frame
		const auto heldAt = *_frozenAt;
		_frozenAt.reset();
		return {.cursor = heldAt, .warpTo = heldAt};
	}
	return {.cursor = pointer};
}

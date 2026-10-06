/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Camera/ZoomToPlaces.h"

namespace openblack::input
{

/// Acts on the options screen's one-press actions that no other part of the game takes: the temple and realm keys,
/// the villagers' names and details, and a log line for those whose feature isn't built yet. The camera and hand keys
/// are read where they are used; the temple's room keys by the game; the leash and creature keys by their own work.
class ShortcutKeys
{
public:
	/// Acts on the presses of the frame
	void Update();

private:
	zoom_to::ZoomToPlaces _zoomTo;
};

} // namespace openblack::input

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Magic/MiracleVisuals.h"

namespace openblack::ecs::components
{

/// How a hand holding a miracle shows it: the bands flying on and off and the bracelets it wears, one for a plain
/// miracle and one more for each power-up, and the glow flowing over it in its player's colour
struct HandMiracleFx
{
	magic::visuals::HandBands bands;
	/// The glow shows while the hand holds a miracle; its frame runs on all the while
	bool glowing {false};
	float glowFrame {0.0f};
};

} // namespace openblack::ecs::components

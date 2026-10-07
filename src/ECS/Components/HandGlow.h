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

namespace openblack::ecs::components
{

/// A colour added to the light the hand is drawn in, after its texture: a recognised gesture's flash makes the hand glow
/// in its player's colour
struct HandGlow
{
	/// 0xRRGGBB, black for none
	uint32_t rgb {0};
};

} // namespace openblack::ecs::components

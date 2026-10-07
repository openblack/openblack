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

#include <optional>

namespace openblack::ecs::components
{

/// A heap of rubble a blast left on the land: it lies there for a while of the game's time and fades away in its last
/// second
struct GroundMark
{
	float millisecondsLeft {0.0f};
	/// Its alpha, 0 to 255, once it is fading; drawn without blending until then
	std::optional<uint8_t> alpha;
};

} // namespace openblack::ecs::components

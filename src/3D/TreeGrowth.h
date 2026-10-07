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

/// A tree smaller than it may grow grows a little every so many turns, faster in the rain or snow and on good land,
/// until it reaches its largest size
namespace openblack::tree_growth
{

/// What a kind of tree does as it grows
struct Type
{
	/// Turns between growths
	uint32_t turnsBetween;
	/// How much it grows each time in plain weather on neutral land
	float amount;
	/// How much faster rain makes it grow, for each point of rain, in hundredths
	float rainAccelerator;
};

/// How much it grows: its amount, more by the share of the heavier of the rain and snow falling on it times its
/// accelerator, and by half the land's alignment
[[nodiscard]] float Growth(const Type& type, int rainOrSnow, float landAlignment);
/// Its size after growing by an amount, no larger than its largest
[[nodiscard]] float Grown(float size, float amount, float largest);

} // namespace openblack::tree_growth

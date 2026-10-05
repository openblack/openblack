/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

namespace openblack::ecs::components
{

/// A villager inside its abode: it isn't drawn while it is
struct AtHome
{
	/// It has been to bed since it came in
	bool beenToBed {false};
};

} // namespace openblack::ecs::components

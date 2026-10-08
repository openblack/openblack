/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Enums.h"

namespace openblack::ecs::components
{

/// A tree that has been uprooted: it no longer grows or sways, and lies where it was left
struct DeadTree
{
	TreeInfo type;
	/// What its wood is worth against an ordinary tree's, as the tree it was when it died
	float woodMultiplier {1.0f};
	/// A forester felled it: it falls as a tree whatever its model, and calls nobody to its wood when it comes down
	bool felled {false};
};

} // namespace openblack::ecs::components

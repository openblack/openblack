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

/// A tree that fell dead: the first time it is drawn, its roots drop from it
struct DropsRoots
{
};

/// The roots of a tree that fell dead, dropping to the land under it and fading away
struct FallingRoots
{
	/// The game's seconds since they began to fall
	float seconds {0.0f};
	/// The height they fell from, and the height they come to rest at
	float startHeight {0.0f};
	float restHeight {0.0f};
};

} // namespace openblack::ecs::components

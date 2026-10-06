/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>

namespace openblack::ecs::systems
{

/// The sky's clouds (components::Cloud)
class CloudSystemInterface
{
public:
	virtual ~CloudSystemInterface() = default;
	/// Lays out the clouds of a new land in place of the last land's
	virtual void Reset() = 0;
	/// Moves the clouds with the wind by the game time that has passed, which stops while the game is paused
	virtual void Update(std::chrono::duration<float, std::milli> gameTime) = 0;
};

} // namespace openblack::ecs::systems

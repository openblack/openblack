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

/// Moves the creatures' hair: each strand hangs from its root on the posed body, swings and sags as the body moves, and
/// takes the look of the creature's alignment (see components::CreatureHair)
class CreatureHairSystemInterface
{
public:
	virtual ~CreatureHairSystemInterface() = default;

	/// Once a frame, by the game time, after the bodies are posed
	virtual void Update(std::chrono::duration<float, std::milli> gameTime) = 0;
	/// Whether the hair is moved and drawn at all; shown again, it starts afresh from the roots
	[[nodiscard]] virtual bool IsShown() const = 0;
	virtual void SetShown(bool shown) = 0;
};

} // namespace openblack::ecs::systems

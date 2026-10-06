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

/// The lights villages keep at night (components::VillageLight)
class VillageLightSystemInterface
{
public:
	virtual ~VillageLightSystemInterface() = default;
	/// Flickers the lights and plays their flames by the game time that has passed, which stops while the game is
	/// paused. They only flicker and show while the land is dark enough for them.
	virtual void Update(std::chrono::duration<float, std::milli> gameTime) = 0;
	/// Whether the land was dark enough for the lights at the last update; the street lanterns crackle while it is
	[[nodiscard]] virtual bool IsDark() const = 0;
};

} // namespace openblack::ecs::systems

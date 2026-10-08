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

#include <array>

#include <glm/vec3.hpp>

#include "3D/MapCoords.h"

namespace openblack::ecs::components
{

/// A firefly's own speeds and where its loops start, drawn as it is made
struct FireflyDrift
{
	/// For its flights and its slow loop
	float flightSpeed {1.0f};
	/// For its quick loop
	float quickSpeed {1.0f};
	/// Where the slow loop's two angles start, two drawn but never used, and the quick loop's two
	std::array<float, 6> phases {};
};

/// A firefly: a small glowing light that hides in a tree or rock by day and hovers by a building at night. Hidden, it is
/// not drawn; caught in its hiding place by a hand that lifts the tree or rock, it leaves a one-shot miracle
struct Firefly
{
	/// Where it is in its night: hiding, hovering by a building, or flying between the two
	enum class State : uint8_t
	{
		Resting,
		Hovering,
		FlyingHome,
		FlyingOut,
	};

	State state {State::Resting};
	/// Its place this turn and last turn, where it hides and where it hovers
	map_coords::MapCoords at;
	map_coords::MapCoords previous;
	map_coords::MapCoords home;
	map_coords::MapCoords hover;
	/// Where it was last drawn, in the world
	glm::vec3 drawn {0.0f};
	/// Seconds of drawn time its loops have turned through
	float clock {0.0f};
	/// How much of its drift it shows, 0 to 1
	float amplitude {0.0f};
	/// How far through its flight it is, 0 to 1, and how many seconds the flight takes
	float progress {0.0f};
	float flightSeconds {0.5f};
	FireflyDrift drift;
	/// Sent home (or never sent out) rather than out
	bool hidden {true};
};

} // namespace openblack::ecs::components

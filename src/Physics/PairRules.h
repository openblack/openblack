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

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace openblack::physics
{

/// What decides whether one body's points are tested against another's faces in a step
struct PairMember
{
	glm::vec3 centre {0.0f};
	float radius {0.0f};
	bool resting {false};
	bool justSetUp {false};
	/// Buildings and a few other fixed things never test their own points against others
	bool checksPoints {true};
	/// Set on what must not hit other objects (a villager's dropped load, building pieces)
	bool noObjectCollision {false};
	bool isVillager {false};
	/// Pushed by a villager, kicked or felled: villagers' bodies pass through it
	bool pushedByLiving {false};
	/// Identifies the object, and the one that threw it (zero for none)
	uint32_t object {0};
	uint32_t thrower {0};
};

/// Whether the first body's points are tested against the second's faces this step
[[nodiscard]] inline bool TestsPoints(const PairMember& a, const PairMember& b)
{
	if (!a.checksPoints || a.noObjectCollision)
	{
		return false;
	}
	// Two resting bodies leave each other alone, unless one was just placed
	if (a.resting && b.resting && !a.justSetUp && !b.justSetUp)
	{
		return false;
	}
	if ((a.isVillager && b.pushedByLiving) || (b.isVillager && a.pushedByLiving))
	{
		return false;
	}
	// A thrown thing passes through whoever threw it
	if ((a.thrower != 0 && a.thrower == b.object) || (b.thrower != 0 && b.thrower == a.object))
	{
		return false;
	}
	const auto offset = a.centre - b.centre;
	const float reach = a.radius + b.radius;
	return glm::dot(offset, offset) < reach * reach;
}

} // namespace openblack::physics

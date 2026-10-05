/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "WaterRingSystem.h"

#include <algorithm>

using namespace openblack::ecs::systems;

bool WaterRingSystem::Add(const water_rings::Ring& ring)
{
	if (_rings.size() >= water_rings::k_MostRings)
	{
		return false;
	}
	_rings.push_back(ring);
	return true;
}

void WaterRingSystem::Update(std::chrono::duration<float, std::milli> gameTime)
{
	std::erase_if(_rings, [gameTime](water_rings::Ring& ring) { return !water_rings::Advance(ring, gameTime.count()); });
}

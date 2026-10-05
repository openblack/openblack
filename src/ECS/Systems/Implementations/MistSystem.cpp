/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "MistSystem.h"

#include "3D/Mists.h"
#include "ECS/Components/Mist.h"
#include "ECS/Registry.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

void MistSystem::Update(std::chrono::duration<float, std::milli> gameTime)
{
	// The game moves a mist on only while it is in view; every mist moves on here, which looks the same
	Locator::entitiesRegistry::value().Each<Mist>(
	    [gameTime](Mist& mist) { mists::Advance(mist.counter, mist.counterRemainder, gameTime.count()); });
}

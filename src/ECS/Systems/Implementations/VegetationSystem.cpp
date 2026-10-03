
/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "VegetationSystem.h"

#include "ECS/Components/Swayable.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "Game.h"
#include "Locator.h"

using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

void VegetationSystem::Sway()
{
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<const Transform, const Tree, Swayable>([](const Transform& transform, const Tree& tree, Swayable& swayable) {
		// Calculate sway parameters
		float swayStrength = swayable.swayStrength;
		float swayOffset = swayable.swayOffset * 100.0f;

		if (!swayable.swayedByHand)
		{
			auto elapsedTime = Locator::time::value().GetElapsedTime();
			auto gameSpeed = Game::Instance()->GetGameSpeed();
			auto naturalSwayTime = std::sin((elapsedTime.count() / 1000.0f * gameSpeed) + (swayable.swayOffset * 100.0f));
			swayable.swayTime = naturalSwayTime;
			swayable.swayStrength = std::min(swayable.swayStrength, swayable.swayStrengthBase);
		}
	});
}

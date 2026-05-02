/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CreatureMindSystem.h"

#include "Creature/CreatureMind.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureState.h"
#include "ECS/Registry.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::creature;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

void CreatureMindSystem::Update()
{
	auto& registry = Locator::entitiesRegistry::value();
	auto& mindManager = Locator::resources::value().GetCreatureMinds();

	registry.Each<const Creature, CreatureState>([&mindManager](const Creature& creature, CreatureState& state) {
		if (state.initialized && state.mind == creature.mind)
		{
			return;
		}

		if (!mindManager.Contains(creature.mind))
		{
			state.desireWeights.fill(0.0f);
			state.initialized = false;
			return;
		}

		const CreatureMind& mind = *mindManager.Handle(creature.mind);
		state.desireWeights = mind.baseDesireWeights;
		state.mind = creature.mind;
		state.initialized = true;
	});
}

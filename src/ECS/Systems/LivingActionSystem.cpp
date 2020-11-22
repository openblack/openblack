/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "LivingActionSystem.h"

#include "ECS/Components/LivingAction.h"
#include "ECS/Registry.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

LivingActionSystem& LivingActionSystem::instance()
{
	static LivingActionSystem* instance = nullptr;
	if (instance == nullptr)
	{
		instance = new LivingActionSystem();
	}
	return *instance;
}

LivingActionSystem::LivingActionSystem() = default;

void LivingActionSystem::Update()
{
	auto& registry = Game::instance()->GetEntityRegistry();

	registry.Each<LivingAction>([](LivingAction& action) { ++action.turnsSinceStateChange; });
}

VillagerStates LivingActionSystem::VillagerGetState(const LivingAction& action, LivingAction::Index index) const
{
	assert(index < LivingAction::Index::_Count);
	return static_cast<VillagerStates>(action.states[static_cast<size_t>(index)]);
}

void LivingActionSystem::VillagerSetState(LivingAction& action, LivingAction::Index index, VillagerStates state) const
{
	assert(index < LivingAction::Index::_Count);
	if (static_cast<VillagerStates>(action.states[static_cast<size_t>(index)]) != state)
	{
		action.states[static_cast<size_t>(index)] = static_cast<uint8_t>(state);
		if (index == LivingAction::Index::Top)
		{
			action.turnsSinceStateChange = 0;
		}
	}
}

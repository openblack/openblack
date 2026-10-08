/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "VillagerMemory.h"

#include "ECS/Components/LivingAction.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;

void villager_memory::StorePreviousState(LivingAction& action)
{
	auto& living = Locator::livingActionSystem::value();
	// The state it is in, or the one it is walking to
	auto state = living.VillagerGetState(action, LivingAction::Index::Final);
	if (state == VillagerStates::InvalidState)
	{
		state = living.VillagerGetState(action, LivingAction::Index::Top);
	}
	const auto& info = Locator::infoConstants::value().villagerStateTable.at(static_cast<size_t>(state));
	if (info.keepsPreviousState != 0 || info.isReactionState != 0)
	{
		state = living.VillagerGetState(action, LivingAction::Index::Previous);
	}
	living.VillagerSetState(action, LivingAction::Index::Previous, state, true);
}

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
	// The state it has settled in: the one it is in when that state is an end in itself, else the one it is heading for
	const auto& table = Locator::infoConstants::value().villagerStateTable;
	const auto top = living.VillagerGetState(action, LivingAction::Index::Top);
	auto state = table.at(static_cast<size_t>(top)).isFinalState != 0
	                 ? top
	                 : living.VillagerGetState(action, LivingAction::Index::Final);
	const auto& info = table.at(static_cast<size_t>(state));
	if (info.keepsPreviousState != 0 || info.isReactionState != 0)
	{
		state = living.VillagerGetState(action, LivingAction::Index::Previous);
	}
	living.VillagerSetState(action, LivingAction::Index::Previous, state, true);
}

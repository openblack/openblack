/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "ECS/Components/LivingAction.h"

namespace openblack::ecs::systems
{
class LivingActionSystem
{
public:
	static LivingActionSystem& instance();
	void Update();

	[[nodiscard]] VillagerStates VillagerGetState(const components::LivingAction& action,
	                                              components::LivingAction::Index index) const;
	void VillagerSetState(components::LivingAction& action, components::LivingAction::Index index, VillagerStates state,
	                      bool skipTransition) const;
	uint32_t VillagerCallState(components::LivingAction& action, components::LivingAction::Index index) const;
	bool VillagerCallEntryState(components::LivingAction& action, components::LivingAction::Index index, VillagerStates src,
	                            VillagerStates dst) const;
	bool VillagerCallExitState(components::LivingAction& action, components::LivingAction::Index index) const;
	int VillagerCallOutOfAnimation(components::LivingAction& action, components::LivingAction::Index index) const;
	bool VillagerCallValidate(components::LivingAction& action, components::LivingAction::Index index) const;

private:
	LivingActionSystem();
};
} // namespace openblack::ecs::systems

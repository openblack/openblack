/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Components/LivingAction.h"
#include "ECS/Systems/LivingActionSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack::ecs::systems
{

class LivingActionSystem final: public LivingActionSystemInterface
{
public:
	void Update() override;

	[[nodiscard]] VillagerStates VillagerGetState(const components::LivingAction& action,
	                                              components::LivingAction::Index index) const override;
	void VillagerSetState(components::LivingAction& action, components::LivingAction::Index index, VillagerStates state,
	                      bool skipTransition) const override;
	uint32_t VillagerCallState(components::LivingAction& action, components::LivingAction::Index index) const override;
	bool VillagerCallEntryState(components::LivingAction& action, components::LivingAction::Index index, VillagerStates src,
	                            VillagerStates dst) const override;
	bool VillagerCallExitState(components::LivingAction& action, components::LivingAction::Index index) const override;
	int VillagerCallOutOfAnimation(components::LivingAction& action, components::LivingAction::Index index) const override;
	bool VillagerCallValidate(components::LivingAction& action, components::LivingAction::Index index) const override;
};
} // namespace openblack::ecs::systems

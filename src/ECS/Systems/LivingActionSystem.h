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

	VillagerStates VillagerGetState(const components::LivingAction& action, components::LivingAction::Index index) const;
	void VillagerSetState(components::LivingAction& action, components::LivingAction::Index index, VillagerStates state) const;

private:
	LivingActionSystem();
};
} // namespace openblack::ecs::systems

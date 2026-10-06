/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/CreatureSkinSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class CreatureSkinSystem final: public CreatureSkinSystemInterface
{
public:
	void Update() override;
	void ProcessTurn() override;
	void SetTattoo(entt::entity creature, size_t slot, const creature_tattoo::Slot& tattoo) override;
	void AddWound(entt::entity creature, const creature_marks::Mark& wound) override;
	void AddBlood(entt::entity creature, const creature_marks::Mark& blood) override;
	void Heal(entt::entity creature, uint32_t counts) override;
};

} // namespace openblack::ecs::systems

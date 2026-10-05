/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/TownDesireSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class TownDesireSystem final: public TownDesireSystemInterface
{
public:
	void ProcessTurn() override;
	uint32_t OfferVillager(entt::entity town, bool child, float trigger,
	                       const std::function<uint32_t(TownDesireInfo)>& satisfy) override;
	[[nodiscard]] float GetDesire(entt::entity town, TownDesireInfo desire) const override;
	[[nodiscard]] float GetRawDesire(entt::entity town, TownDesireInfo desire) const override;
	[[nodiscard]] TownDesireInfo GetMostWanted(entt::entity town) const override;
	void SetBoost(entt::entity town, TownDesireInfo desire, float boost, bool resort) override;
};

} // namespace openblack::ecs::systems

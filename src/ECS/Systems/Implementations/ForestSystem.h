/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/ForestSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class ForestSystem final: public ForestSystemInterface
{
public:
	uint32_t Plant(entt::entity spell, components::Spell& miracle, float tribalPower) override;
	[[nodiscard]] bool CanGrowAt(glm::vec3 point) const override;
	[[nodiscard]] bool HasTrees(entt::entity spell) const override;
	std::optional<entt::entity> AddTreeNear(entt::entity tree) override;
	void ProcessTurn() override;
	void Reset() override;

private:
	/// The forests' turn: growing while their miracles last, withering once they have gone
	void ProcessForests();

	/// The turn a forest last gained a tree planted near another, 0 on a new land
	uint32_t _lastTreeAddedTurn {0};
	/// The number the next forest miracle's forest takes
	uint32_t _nextMiracleForestId {k_FirstMiracleForestId};
	/// The forest miracles' forests are numbered from here, beyond any a land's script gives its forests
	static constexpr uint32_t k_FirstMiracleForestId = 0x80000000u;
};

} // namespace openblack::ecs::systems

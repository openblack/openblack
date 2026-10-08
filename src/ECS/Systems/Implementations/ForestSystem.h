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
	uint32_t MakeLandForest(std::optional<uint32_t> id, glm::vec3 position, entt::entity bigForest, bool scenic) override;
	[[nodiscard]] std::optional<entt::entity> LandForestOf(uint32_t id) const override;
	[[nodiscard]] std::vector<entt::entity> LandForests() const override;
	[[nodiscard]] std::vector<entt::entity> TreesOf(entt::entity forest, bool growing) const override;
	[[nodiscard]] float WoodOf(entt::entity forest) const override;
	[[nodiscard]] glm::vec3 NearestPointOf(entt::entity forest, glm::vec3 to) const override;
	void AssignForestsToTowns() override;
	[[nodiscard]] std::optional<glm::vec3> ForestLair(AnimalInfo kind, glm::vec3 from) const override;
	void MakeScenicForests() override;
	void ProcessTurn() override;
	void Reset() override;

private:
	/// The forests' turn: growing while their miracles last, withering once they have gone
	void ProcessForests();

	/// The turn a forest last gained a tree planted near another, 0 on a new land
	uint32_t _lastTreeAddedTurn {0};
	/// The number the next forest miracle's forest takes
	uint32_t _nextMiracleForestId {k_FirstMiracleForestId};
	/// The number the next forest of the land takes when none is given, and how many have been made
	uint32_t _nextLandForestId {1};
	uint32_t _landForestsMade {0};
	/// The forest miracles' forests are numbered from here, beyond any a land's script gives its forests
	static constexpr uint32_t k_FirstMiracleForestId = 0x80000000u;
};

} // namespace openblack::ecs::systems

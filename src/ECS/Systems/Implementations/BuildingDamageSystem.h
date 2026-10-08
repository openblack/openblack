/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/BuildingDamageSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class BuildingDamageSystem final: public BuildingDamageSystemInterface
{
public:
	void ReactToImpact(DynamicsSystemInterface& dynamics, PhysicsEntry& entry, const ImpactInfo& impact) override;
	void Smash(entt::entity building, entt::entity creature, float creatureSize) override;
	entt::entity PieceAtRest(DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity piece, bool insert) override;
	void ForgetHitter(entt::entity rock) override;
	void ProcessTurn() override;
	[[nodiscard]] entt::id_type DrawnMesh(entt::entity object, entt::id_type own) const override;
	void Reset() override;
};

} // namespace openblack::ecs::systems

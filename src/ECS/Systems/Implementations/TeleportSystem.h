/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include "ECS/Systems/TeleportSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class TeleportSystem final: public TeleportSystemInterface
{
public:
	entt::entity CreateStone(glm::vec3 point, PlayerNames player, entt::entity spell) override;
	void RemoveStone(entt::entity stone) override;
	[[nodiscard]] bool CanPlaceStone(glm::vec3 point) const override;
	[[nodiscard]] bool BlocksNewBuilding(glm::vec3 point) const override;
	[[nodiscard]] std::vector<entt::entity> GetStones(PlayerNames player) const override;
	[[nodiscard]] std::optional<entt::entity> StoneOf(entt::entity spell) const override;
	[[nodiscard]] bool ShouldReact(entt::entity stone, entt::entity living) const override;
	void SetupReact(entt::entity stone, entt::entity living) override;
	bool DoTeleport(entt::entity stone, entt::entity living, bool forced) override;
	bool DropOnStone(entt::entity villager, entt::entity stone, PlayerNames dropper) override;
	[[nodiscard]] std::optional<entt::entity> StoneAlong(glm::vec3 origin, glm::vec3 direction) const override;
	[[nodiscard]] std::optional<entt::entity> RouteStoneFor(PlayerNames player, glm::vec3 worshipper, glm::vec3 site,
	                                                        float maxDistance) const override;
	bool RouteWorshipper(entt::entity villager, glm::vec3 site) override;
	void ProcessTurn() override;
	void Reset() override;

private:
	/// A turn of a creature the stone carries: fading out, coming out of the far stone, fading back in, walking on
	void Carry(entt::entity creature);
	/// A villager or creature going somewhere turns aside into a stone
	void ReactTo(entt::entity stone, entt::entity living, glm::vec3 destination);
	/// The stones of miracles that have closed down or gone go
	void RemoveOrphans();
	/// Travellers that went or stopped heading for the stone are dropped from it
	void PruneTravellers();
	/// The creatures heading for a stone that reached it jump, and those that jumped walk on
	void MoveCreatures();

	uint32_t _nextSerial {0};
};

} // namespace openblack::ecs::systems

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/LeashSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class LeashSystem final: public LeashSystemInterface
{
public:
	void ProcessTurn() override;
	void Update(float seconds) override;
	void HandleInput(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) override;

	[[nodiscard]] bool Knows(entt::entity creature, LeashType type) const override;
	void SetKnown(entt::entity creature, LeashType type, bool known) override;
	bool PutOn(entt::entity creature, LeashType type) override;
	void TakeOff(entt::entity creature) override;
	bool Toggle(entt::entity creature) override;
	bool ChangeType(entt::entity creature, LeashType type) override;
	bool TieTo(entt::entity creature, entt::entity object) override;
	void UntieToHand(entt::entity creature) override;
	void SetWorks(entt::entity creature, bool works) override;
	void SetDrawn(bool drawn) override;
	void ConfineToHome(entt::entity creature, float radius) override;
	void ClearConfinement(entt::entity creature) override;
	[[nodiscard]] bool FreeOfHome(entt::entity creature) const override;
	[[nodiscard]] bool IsLeashed(entt::entity creature) const override;
	[[nodiscard]] std::optional<entt::entity> TiedTo(entt::entity creature) const override;
	[[nodiscard]] LeashType TypeOf(entt::entity creature) const override;
	[[nodiscard]] std::optional<entt::entity> PlayersCreature(PlayerNames player) const override;
	void PlacePosts(PlayerNames owner, const std::array<glm::vec3, 3>& points) override;
	bool TapPost(entt::entity post) override;

private:
	/// The posts of each temple whose heart mesh has the three points to hang them at, once the temples are there
	void PlacePostsAtTemples();
	/// A taut rope in the hand pulls the creature to the hand
	void Pull(entt::entity creature);
	/// Whether the posts have been looked for at the temples yet
	bool _postsPlaced {false};
	/// The two leash-tying sounds play in turn
	bool _secondAttachSound {false};
};

} // namespace openblack::ecs::systems

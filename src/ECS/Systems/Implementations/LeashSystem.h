/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>
#include <vector>

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
	void HandleInput(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, bool actionTaken) override;

	[[nodiscard]] bool Knows(entt::entity creature, LeashType type) const override;
	void SetKnown(entt::entity creature, LeashType type, bool known) override;
	[[nodiscard]] bool IsLeashable(entt::entity creature) const override;
	bool SetLeashable(entt::entity creature, bool leashable) override;
	void SetOwner(entt::entity creature, PlayerNames owner) override;
	void ClaimOnArrival(entt::entity creature) override;
	[[nodiscard]] creature_leash::Refusal WhyNot(PlayerNames player, entt::entity creature, LeashType type) const override;
	[[nodiscard]] std::optional<Refused> LastRefusal() const override;
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
	bool PressKey(PlayerNames player, creature_leash::LeashKey key) override;
	bool TapCreature(PlayerNames player, entt::entity creature) override;
	bool TrackHand(PlayerNames player, glm::vec2 cursor, float seconds, bool handFree) override;
	bool Shake(PlayerNames player) override;

private:
	/// Puts the leash on for the player, unless the rules refuse it, which is logged and remembered
	bool PutOnFor(PlayerNames player, entt::entity creature, LeashType type);
	/// Carries out what a shortcut does to the player's creature
	bool Carry(PlayerNames player, entt::entity creature, const creature_leash::KeyCommand& command);
	void Refuse(PlayerNames player, entt::entity creature, creature_leash::Refusal why);
	/// The creatures as the one-each assignment sees them
	[[nodiscard]] std::vector<creature_leash::Claim> Claims() const;
	/// The posts of each temple whose heart mesh has the three points to hang them at, once the temples are there
	void PlacePostsAtTemples();
	/// A taut rope in the hand pulls the creature to the hand
	void Pull(entt::entity creature);
	/// Whether the posts have been looked for at the temples yet
	bool _postsPlaced {false};
	/// The two leash-tying sounds play in turn
	bool _secondAttachSound {false};
	std::optional<Refused> _lastRefusal;
	/// Following the local player's cursor to tell when the hand is shaken
	creature_leash::ShakeTracker _shake;
};

} // namespace openblack::ecs::systems

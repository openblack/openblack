/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <optional>

#include <entt/entity/fwd.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::systems
{

/// The leashes the player leads creatures with (see components::CreatureLeash). Once a frame each worn leash's rope
/// swings between the hand, or what the leash is tied to, and the creature's collar. Once a game turn a rope pulled taut
/// in the hand makes the creature stop and walk to the hand, a tied leash keeps the creature near what it is tied to,
/// and the leash's feelings and lessons are passed to the creature's mind. The player picks a leash at the citadel's
/// leash posts or with the hotkeys, puts it on and takes it off with the leash key, and taps things with the Action
/// button to tie the leash to them and untie it again.
class LeashSystemInterface
{
public:
	virtual ~LeashSystemInterface() = default;

	/// Once a game turn
	virtual void ProcessTurn() = 0;
	/// Once a frame, some seconds of game time on: the ropes swing
	virtual void Update(float seconds) = 0;
	/// The leash hotkeys, and the Action button tapping what is along the cursor's ray
	virtual void HandleInput(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) = 0;

	/// Whether the creature knows a leash, which it must before it can wear it; the learning leash before any
	[[nodiscard]] virtual bool Knows(entt::entity creature, LeashType type) const = 0;
	virtual void SetKnown(entt::entity creature, LeashType type, bool known) = 0;

	/// Puts a leash on the creature, held in its player's hand. Returns whether it could: it must know the leash.
	virtual bool PutOn(entt::entity creature, LeashType type) = 0;
	virtual void TakeOff(entt::entity creature) = 0;
	/// Unties a tied leash back to the hand, or else puts the picked leash on or takes it off
	virtual bool Toggle(entt::entity creature) = 0;
	/// Swaps the leash worn for another the creature knows, or picks the one to put on next
	virtual bool ChangeType(entt::entity creature, LeashType type) = 0;
	/// Ties the worn leash to something, or puts the picked leash on tied to it
	virtual bool TieTo(entt::entity creature, entt::entity object) = 0;
	virtual void UntieToHand(entt::entity creature) = 0;
	/// Whether the leash makes the creature do anything, as scripts set
	virtual void SetWorks(entt::entity creature, bool works) = 0;
	/// Whether ropes are drawn
	virtual void SetDrawn(bool drawn) = 0;

	/// Keeps the creature within a radius of its home, as it is while it starts to grow up
	virtual void ConfineToHome(entt::entity creature, float radius) = 0;
	virtual void ClearConfinement(entt::entity creature) = 0;
	/// Whether the creature is near enough its home, its player having a temple, to roam
	[[nodiscard]] virtual bool FreeOfHome(entt::entity creature) const = 0;

	[[nodiscard]] virtual bool IsLeashed(entt::entity creature) const = 0;
	[[nodiscard]] virtual std::optional<entt::entity> TiedTo(entt::entity creature) const = 0;
	[[nodiscard]] virtual LeashType TypeOf(entt::entity creature) const = 0;
	/// The player's creature: the one the player leads if any, else the first the player owns
	[[nodiscard]] virtual std::optional<entt::entity> PlayersCreature(PlayerNames player) const = 0;

	/// Hangs a player's three leash posts at three points, the aggression, learning and compassion leashes in turn
	virtual void PlacePosts(PlayerNames owner, const std::array<glm::vec3, 3>& points) = 0;
	/// A player taps one of their leash posts: it picks that leash, or unpicks it when tapped again, and swaps the worn
	/// leash for it
	virtual bool TapPost(entt::entity post) = 0;
};

} // namespace openblack::ecs::systems

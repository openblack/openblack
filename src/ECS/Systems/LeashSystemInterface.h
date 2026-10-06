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
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Creature/LeashKeys.h"
#include "Creature/LeashOwnership.h"
#include "Enums.h"

namespace openblack::ecs::systems
{

/// The leashes the player leads creatures with (see components::CreatureLeash). Once a frame each worn leash's rope
/// swings between the hand, or what the leash is tied to, and the creature's collar. Once a game turn a rope pulled taut
/// in the hand makes the creature stop and walk to the hand, a tied leash keeps the creature near what it is tied to,
/// and the leash's feelings and lessons are passed to the creature's mind. The player picks a leash at the citadel's
/// leash posts or with the hotkeys, puts it on and takes it off with the leash key, and taps things with the Action
/// button to tie the leash to them and untie it again.
///
/// Who may lead which creature is decided here and nowhere else (see creature_leash::WhyNot): each player leads only
/// their one leashable creature, so the hand, the shortcuts, the scripts, the debug windows and the scenarios all go
/// through these calls. A refusal is logged and kept as the last refusal for the debug windows to show.
class LeashSystemInterface
{
public:
	/// A leash that was refused: who wanted it on which creature, and why not
	struct Refused
	{
		PlayerNames player;
		entt::entity creature;
		creature_leash::Refusal why;
	};

	virtual ~LeashSystemInterface() = default;

	/// Once a game turn
	virtual void ProcessTurn() = 0;
	/// Once a frame, some seconds of game time on: the ropes swing
	virtual void Update(float seconds) = 0;
	/// The leash hotkeys, and the Action button tapping what is along the cursor's ray, unless the hand used the press
	/// for something else, such as letting go of a miracle
	virtual void HandleInput(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, bool actionTaken) = 0;

	/// Whether the creature knows a leash, which it must before it can wear it; the learning leash before any
	[[nodiscard]] virtual bool Knows(entt::entity creature, LeashType type) const = 0;
	virtual void SetKnown(entt::entity creature, LeashType type, bool known) = 0;

	/// Whether the creature is the one its owner can lead
	[[nodiscard]] virtual bool IsLeashable(entt::entity creature) const = 0;
	/// Makes the creature the one its owner can lead, which stops their other creature being it and takes that one's
	/// leash off; or stops it being it, taking its leash off. A creature that belongs to nobody can't be made leashable.
	virtual bool SetLeashable(entt::entity creature, bool leashable) = 0;
	/// Gives the creature to another player, taking its leash off. It stays leashable only if the new owner has no
	/// leashable creature of their own.
	virtual void SetOwner(entt::entity creature, PlayerNames owner) = 0;
	/// A new creature becomes its owner's leashable one when they have none yet
	virtual void ClaimOnArrival(entt::entity creature) = 0;
	/// Why the player may not put the leash on the creature, or creature_leash::Refusal::None when they may
	[[nodiscard]] virtual creature_leash::Refusal WhyNot(PlayerNames player, entt::entity creature, LeashType type) const = 0;
	/// The last leash refused, for the debug windows
	[[nodiscard]] virtual std::optional<Refused> LastRefusal() const = 0;

	/// Puts a leash on the creature, held in its owner's hand. Returns whether it could: it must be its owner's leashable
	/// creature and know the leash.
	virtual bool PutOn(entt::entity creature, LeashType type) = 0;
	virtual void TakeOff(entt::entity creature) = 0;
	/// Unties a tied leash back to the hand, or else puts the picked leash on or takes it off. This is all the game's leash
	/// key and its toggle-leash script command do, however many leashes the creature knows.
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
	/// The player's creature: the one creature they can lead, if they have one
	[[nodiscard]] virtual std::optional<entt::entity> PlayersCreature(PlayerNames player) const = 0;

	/// A player presses a leash shortcut, which acts on their creature. Returns whether it did anything.
	virtual bool PressKey(PlayerNames player, creature_leash::LeashKey key) = 0;
	/// A player clicks a creature with the right button: their own creature gets the picked leash put on; any other
	/// is refused. Returns whether the leash went on.
	virtual bool TapCreature(PlayerNames player, entt::entity creature) = 0;
	/// Once a frame, the player's cursor in screen heights from the top left and the frame's seconds, and whether the
	/// hand is free to shake, holding nothing and gripping nothing. Shaken, it takes off a leash held in the hand.
	/// Returns whether that happened.
	virtual bool TrackHand(PlayerNames player, glm::vec2 cursor, float seconds, bool handFree) = 0;
	/// The player shakes the hand: a leash held in it comes off; one tied to something stays. Returns whether it came
	/// off.
	virtual bool Shake(PlayerNames player) = 0;

	/// Hangs a player's three leash posts at three points, the aggression, learning and compassion leashes in turn
	virtual void PlacePosts(PlayerNames owner, const std::array<glm::vec3, 3>& points) = 0;
	/// A player taps one of their leash posts: it picks that leash, or unpicks it when tapped again, and swaps the worn
	/// leash for it
	virtual bool TapPost(entt::entity post) = 0;
};

} // namespace openblack::ecs::systems

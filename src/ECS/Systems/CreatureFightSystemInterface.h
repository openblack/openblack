/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <optional>

#include <entt/entity/fwd.hpp>
#include <glm/vec3.hpp>

#include "Creature/CreatureFight.h"
#include "Creature/CreatureFightHud.h"

namespace openblack::ecs::systems
{

/// Creature fights, and creatures knocked out (see components::CreatureFighting, CreatureFightRecord and
/// CreatureKnockedOut). A fight starts when an angry creature picks one, when the player leashes their creature to
/// another, or when told to by a script or the debug tools. The two walk to their places in an arena, taunt each other
/// and duel until one is knocked out; the player directs their own creature with the hand, or the creature fights by
/// itself. The loser lies out cold, is taken home, rests and gets up again; only a script kills a creature for good.
class CreatureFightSystemInterface
{
public:
	enum class StartResult : uint8_t
	{
		Started,
		/// Not two different creatures
		NoOpponent,
		/// Either is already fighting or out cold
		Busy,
		/// The creature starting it isn't healthy enough
		TooWeak,
	};

	virtual ~CreatureFightSystemInterface() = default;

	/// Once a game turn, after the creatures have moved: fights start and end, moves are chosen and made, stamina comes
	/// back, and creatures knocked out come round
	virtual void ProcessTurn() = 0;
	/// Once a frame, by the game time, before the creatures are placed: the fight animations play, blows land and the
	/// fighters move as their animations carry them
	virtual void Update(std::chrono::duration<float, std::milli> gameTime) = 0;

	/// A fight between two creatures, the first making the arena, as scripts and the leash start them
	virtual StartResult StartFight(entt::entity creature, entt::entity opponent) = 0;
	/// Ends a creature's fight with no winner: both finish and stand
	virtual void AbortFight(entt::entity creature) = 0;
	[[nodiscard]] virtual bool IsFighting(entt::entity creature) const = 0;
	[[nodiscard]] virtual std::optional<entt::entity> OpponentOf(entt::entity creature) const = 0;

	/// A move added to a fighter's queue as the player makes it, in place of the rest or after them; a blow waits for its
	/// charge until ReleaseCharge
	virtual bool QueueMove(entt::entity creature, const creature_fight::Move& move, bool replace) = 0;
	/// The charge, in milliseconds held, of the blow waiting in the fighter's queue
	virtual void ReleaseCharge(entt::entity creature, float heldMs) = 0;
	/// Whether the creature fights by itself whatever the player does
	virtual void SetAutoFighting(entt::entity creature, bool autoFight) = 0;
	[[nodiscard]] virtual bool IsAutoFighting(entt::entity creature) const = 0;

	/// The hand's button was pressed along a line of sight while the player's creature fights: whether the fight took
	/// the press, which then charges a blow until it is let go
	virtual bool Press(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) = 0;
	/// The button was let go after a press the fight took
	virtual void Release() = 0;
	/// Whether a press the fight took is held
	[[nodiscard]] virtual bool IsPressed() const = 0;

	/// Knocks a creature out, as a fight's loser is
	virtual void KnockOut(entt::entity creature) = 0;
	/// Kills a creature for good: it faints and never gets up. Only scripts do this.
	virtual void KillPermanently(entt::entity creature) = 0;
	/// Brings a creature knocked out round at once, at 0.4 of its life
	virtual void Resurrect(entt::entity creature) = 0;
	[[nodiscard]] virtual bool IsKnockedOut(entt::entity creature) const = 0;

	/// The panel of the fight the player's creature is in, or else of any fight, if there is one
	[[nodiscard]] virtual std::optional<creature_fight_hud::Values> GetPanel() const = 0;

	/// Whether angry creatures pick fights by themselves, and whether the camera goes to watch the player's creature's
	/// fights
	virtual void SetAngerStartsFights(bool enabled) = 0;
	[[nodiscard]] virtual bool GetAngerStartsFights() const = 0;
	virtual void SetCameraWatches(bool enabled) = 0;
	[[nodiscard]] virtual bool GetCameraWatches() const = 0;
};

} // namespace openblack::ecs::systems

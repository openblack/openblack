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

#include <optional>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "Magic/SpellRules.h"
#include "Particles/ParticleSpellLink.h"

namespace openblack::ecs::systems
{

/// The running miracles, the seeds the player holds and the miracle dispensers.
///
/// A miracle is an entity with a components::Spell: cast at a point or on an object by a caster who pays for it, it runs
/// its particle effect, acts on the events the effect sends (a fireball's flight, a bolt striking, food landing) and
/// pays its upkeep each turn until its time or its prayer power runs out. A dispenser floats a one-shot bubble above
/// itself; tapping the bubble with the hand puts its miracle in the hand, fully charged. The hand casts it as its seed
/// says: held (lightning, water, food, wood), thrown on letting go (fireball, shield, storm) or placed on pressing
/// (heal). Gestures and powering up are not part of it yet: a miracle that needs a gesture for its size takes its usual
/// size.
class MagicSystemInterface
{
public:
	/// What a frame tells the miracles about the player's hand
	struct HandFrame
	{
		/// Where the hand is
		glm::vec3 handPosition {0.0f};
		/// The land or object under the cursor, none over the sky
		std::optional<glm::vec3> point;
		/// The line of sight through the cursor
		glm::vec3 rayOrigin {0.0f};
		glm::vec3 rayDirection {0.0f, 0.0f, 1.0f};
		/// Which way the camera looks
		glm::vec3 cameraForward {0.0f, 0.0f, 1.0f};
		/// Whether the hand is over the world, not over a window or in the temple
		bool overWorld {true};
	};

	/// What the debug window shows of a running miracle
	struct SpellInfo
	{
		entt::entity entity;
		MagicType magicType;
		PlayerNames player;
		float age;
		float duration;
		float chants;
		float initialChants;
		float strength;
		float upkeep;
		bool closing;
		bool fromHand;
		uint32_t effect;
		glm::vec3 position;
	};

	/// What the debug window shows of a dispenser
	struct DispenserInfo
	{
		entt::entity entity;
		MagicType magicType;
		glm::vec3 position;
		bool hasOrb;
		uint32_t tick;
		uint32_t period;
		bool active;
	};

	/// What became of the last thing the hand tried, for the debug window
	enum class HandResult : uint8_t
	{
		None,
		TookMiracle,
		Readied,
		Cast,
		CastHeld,
		NotReady,
		CantCastThere,
		Released,
		Discarded,
	};

	virtual ~MagicSystemInterface() = default;

	// Casting

	/// A miracle cast by a player at a point, as the hand, a script or a debug tool casts it: the entity, or none when
	/// it could not start. The process info is what its effect starts with: the hand's place, the camera, the throw.
	virtual entt::entity CastAtPoint(MagicType type, PlayerNames player, glm::vec3 point, const magic::SpellCastData& cast,
	                                 const particles::ProcessInfo& info) = 0;
	/// A miracle cast on an object: at its feet, its effect given the object to act on
	virtual entt::entity CastOnObject(MagicType type, PlayerNames player, entt::entity target, const magic::SpellCastData& cast,
	                                  const particles::ProcessInfo& info) = 0;
	/// The miracle stops: its effect dies away and it goes once that has gone
	virtual void CloseDown(entt::entity spell) = 0;
	/// Whether a player may cast a magic type at a point
	[[nodiscard]] virtual bool CanCastAt(MagicType type, PlayerNames player, glm::vec3 point) = 0;

	// Dispensers and one-shot miracles

	/// A dispenser of a magic type standing at a point, turned about the vertical; it floats its first bubble at once
	virtual entt::entity CreateDispenser(glm::vec3 position, MagicType type, float yAngleRadians) = 0;
	/// The time from a dispenser's bubble being taken to the next, in seconds
	virtual void SetDispenserPeriod(entt::entity dispenser, float seconds) = 0;
	/// A one-shot bubble of a seed at a point, floating there
	virtual entt::entity CreateOneOffSeed(glm::vec3 position, SpellSeedType seed, int powerUp, float multiplier) = 0;
	/// A one-shot bubble of whatever seed casts a magic type, at the power-up level that casts it
	virtual entt::entity CreateOneOffSeedFor(glm::vec3 position, MagicType type) = 0;
	/// Takes a dispenser away with its bubble and its swirl, or a bubble with the seed spinning in it; false for anything
	/// else, which is left alone
	virtual bool Remove(entt::entity entity) = 0;
	/// A seed straight into the player's hand, fully charged and ready, if the hand is free: the seed, or none
	virtual entt::entity GiveSeedToHand(PlayerNames player, SpellSeedType seed, int powerUp, float multiplier) = 0;
	/// The hand lets go of the miracle it holds, which goes
	virtual void DiscardHeldSeed() = 0;

	// The hand

	/// Once a frame, before any press or release
	virtual void UpdateHand(const HandFrame& frame, float seconds) = 0;
	/// The action button went down: a bubble under the cursor is taken, or the held miracle readied or cast. Whether
	/// the miracles took the press, so the hand doesn't grip the land with it.
	virtual bool PressAction() = 0;
	/// The action button came up: a readied miracle is thrown, a held one stops
	virtual void ReleaseAction() = 0;
	/// Whether the hand holds a miracle, so it neither grips the land nor takes hold of creatures
	[[nodiscard]] virtual bool IsHandBusy() const = 0;
	[[nodiscard]] virtual std::optional<entt::entity> GetHeldSeed() const = 0;
	/// The one-shot bubble nearest along a line of sight, if any
	[[nodiscard]] virtual std::optional<entt::entity> OrbAlong(glm::vec3 origin, glm::vec3 direction) const = 0;
	[[nodiscard]] virtual HandResult GetLastHandResult() const = 0;

	// Turns and frames

	/// Once a game turn, before the particles: the dispensers, then every miracle's upkeep, effect and events
	virtual void ProcessTurn() = 0;
	/// Once a frame: the held miracle follows the hand, the bubbles' seeds spin
	virtual void Update(float seconds) = 0;
	/// A new land: every miracle goes
	virtual void Reset() = 0;

	// For the debug window and the testbed

	/// The testbed has no temple, so its player has no influence to cast in: its miracles may be cast anywhere
	virtual void SetIgnoreInfluence(bool ignore) = 0;
	[[nodiscard]] virtual bool IsIgnoringInfluence() const = 0;
	[[nodiscard]] virtual std::vector<SpellInfo> GetSpells() const = 0;
	[[nodiscard]] virtual std::vector<DispenserInfo> GetDispensers() const = 0;
};

} // namespace openblack::ecs::systems

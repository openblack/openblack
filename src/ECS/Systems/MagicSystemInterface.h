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
#include "Magic/CastInput.h"
#include "Magic/HandMotion.h"
#include "Magic/PrayerRules.h"
#include "Magic/SpellRules.h"
#include "Particles/ParticleSpellLink.h"

namespace openblack::magic
{
struct EffectSource;
} // namespace openblack::magic

namespace openblack::ecs::systems
{

/// The running miracles, the seeds the player holds and the miracle dispensers.
///
/// A miracle is an entity with a components::Spell: cast at a point or on an object by a caster who pays for it, it runs
/// its particle effect, acts on the events the effect sends (a fireball's flight, a bolt striking, food landing) and
/// pays its upkeep each turn until its time or its prayer power runs out; it goes once its effect, and whatever its kind
/// made, has gone. A dispenser floats a one-shot bubble above itself; tapping the bubble with the hand (the left button)
/// puts its miracle in the hand, fully charged and ready. A seed summoned from the player's worship is charged from
/// their prayer power and is ready after a moment. The action button (the right) casts the seed as it says: held
/// (lightning, water, food, wood: applied every turn while the button is down), thrown on letting go (the fireball and
/// the flocks; the storms and shields at the circle drawn) or placed on pressing (heal, forest, teleport, the beam
/// explosion, the creature spells on a creature). The hand must be in the player's influence. The gestures drawn reach
/// it through the gesture events: a circle, a power-up, a scribble or a shake, which drops the seed.
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
		/// A storm or shield let go of with no circle drawn
		NoCircle,
		PoweredUp,
	};

	/// What the debug window shows of the hand's casting
	struct HandCastState
	{
		magic::CastInput::State state {magic::CastInput::State::Idle};
		bool holding {false};
		bool ready {false};
		/// Seconds until the seed is ready
		float readyIn {0.0f};
		magic::SeedOrigin origin {magic::SeedOrigin::Bubble};
		int powerUp {-1};
		float chantStore {0.0f};
		float storedChants {-1.0f};
		/// The hand's smoothed movement, and the speed a fireball would be thrown at
		glm::vec3 velocity {0.0f};
		float throwSpeed {0.0f};
		/// The circle remembered for a storm or shield, and the seconds it has left
		std::optional<glm::vec3> circleCentre;
		float circleRadius {0.0f};
		float circleSecondsLeft {0.0f};
		magic::PourPose pour;
		/// Whether the hand's point is somewhere the held seed may be cast
		bool pointValid {false};
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
	/// A creature casts a miracle at an object, as its casting pose's loop begins: as big as the object, its effect
	/// flowing from the creature's hands, paid for with the creature's body, cast at the object's feet or on the object
	/// as the miracle's seed is. Whether it could: not when the creature is too tired to pay for it, nor when the
	/// miracle can't be cast at the object. Past those it counts as cast even if no miracle comes of it.
	virtual bool CastByCreature(entt::entity /*creature*/, MagicType /*type*/, entt::entity /*target*/) { return false; }
	/// The creature lets go of the miracle it cast: one that lasts only while it is held stops
	virtual void ReleaseCreatureCast(entt::entity /*creature*/) {}
	/// An effect at a point that no miracle is behind, such as a lightning strike a script calls down: everything it
	/// reaches takes it, as from the player
	virtual void ApplyEffectAt(glm::vec3 /*point*/, const magic::EffectValues& /*values*/, PlayerNames /*player*/) {}
	/// An effect on one object that no miracle is behind, such as the crush of a blow, from a player: what its defence
	/// lets through, as any effect does. Whether the object has life to take it.
	virtual bool ApplyEffectToObject(entt::entity /*object*/, const magic::EffectValues& /*values*/, PlayerNames /*player*/)
	{
		return false;
	}
	/// The same from a source that says what applied the effect, and whether any player is behind it
	virtual bool ApplyEffectToObject(entt::entity /*object*/, const magic::EffectValues& /*values*/,
	                                 const magic::EffectSource& /*source*/)
	{
		return false;
	}
	/// Whether a player may cast a magic type at a point
	[[nodiscard]] virtual bool CanCastAt(MagicType type, PlayerNames player, glm::vec3 point) = 0;
	/// Something the miracle made acts for it, as its particles would: whether the miracle acted
	virtual bool SpellEvent(entt::entity spell, const particles::SpellEventInfo& event) = 0;
	/// The miracle pays prayer power, or is given it back for a negative cost
	virtual void PayForSpell(entt::entity spell, float cost) = 0;

	// Dispensers and one-shot miracles

	/// A dispenser of a magic type standing at a point, turned about the vertical; it floats its first bubble once its
	/// period has passed
	virtual entt::entity CreateDispenser(glm::vec3 position, MagicType type, float yAngleRadians) = 0;
	/// For the testbed: a dispenser that has counted its period, floating its bubble now if it has none
	virtual void ChargeDispenser(entt::entity /*dispenser*/) {}
	/// The time from a dispenser's bubble being taken to the next, in seconds
	virtual void SetDispenserPeriod(entt::entity dispenser, float seconds) = 0;
	/// A one-shot bubble of a seed at a point, floating there
	virtual entt::entity CreateOneOffSeed(glm::vec3 position, SpellSeedType seed, int powerUp, float multiplier) = 0;
	/// A one-shot bubble of whatever seed casts a magic type, at the power-up level that casts it
	virtual entt::entity CreateOneOffSeedFor(glm::vec3 position, MagicType type) = 0;
	/// Takes a dispenser away with its bubble and its swirl, or a bubble with the seed spinning in it; false for anything
	/// else, which is left alone
	virtual bool Remove(entt::entity entity) = 0;
	/// A seed straight into the player's hand, fully charged and ready, as from a bubble, if the hand is free: the seed,
	/// or none
	virtual entt::entity GiveSeedToHand(PlayerNames player, SpellSeedType seed, int powerUp, float multiplier) = 0;
	/// A seed summoned from the player's worship into their hand, if it is free: its cost to create is charged from the
	/// player's prayer power, as much as they have, and it is ready after the usual delay. The seed, or none.
	virtual entt::entity SummonSeed(PlayerNames player, SpellSeedType seed, int powerUp) = 0;
	/// The hand drops the miracle it holds, as a scribble or a shake does: what was started is called off and what the
	/// seed still holds goes back to the player's worship
	virtual void DiscardHeldSeed() = 0;

	// The hand

	/// Once a frame, before any press or release
	virtual void UpdateHand(const HandFrame& frame, float seconds) = 0;
	/// The left button tapped: a bubble under the cursor goes into the hand. Whether one did, so the hand doesn't grip
	/// the land with the press.
	virtual bool TapAction() = 0;
	/// The action button went down with a miracle in the hand: it is armed, locked on or cast. Whether the miracles took
	/// the press, so the creatures don't.
	virtual bool PressAction() = 0;
	/// The action button came up: an armed miracle is cast, a locked one let go
	virtual void ReleaseAction() = 0;
	/// Whether the hand holds a miracle, so it doesn't take hold of creatures or things
	[[nodiscard]] virtual bool IsHandBusy() const = 0;
	/// How a pour of food or wood lifts and tips the hand now, a fraction of the way from the last turn to the next
	[[nodiscard]] virtual magic::PourPose GetHandPour(float fraction) const = 0;
	[[nodiscard]] virtual HandCastState GetHandCastState() const = 0;
	[[nodiscard]] virtual std::optional<entt::entity> GetHeldSeed() const = 0;
	/// The one-shot bubble nearest along a line of sight, if any
	[[nodiscard]] virtual std::optional<entt::entity> OrbAlong(glm::vec3 origin, glm::vec3 direction) const = 0;
	[[nodiscard]] virtual HandResult GetLastHandResult() const = 0;
	/// Once a frame, once the hand is posed: where the held miracle's in-hand effect sits, and how large the hand is
	/// drawn, which is how large the effect is
	virtual void PlaceHandEffect(glm::vec3 /*point*/, float /*handScale*/) {}
	/// Whether the point under the hand is in the influence of the player whose seed it holds (or influence is ignored)
	[[nodiscard]] virtual bool IsHandInInfluence() const { return true; }

	// What a miracle's objects in the world do for it

	/// An event a miracle's object sends it, as its particles do, such as a blow on the physical shield's dome; whether it
	/// acted
	virtual bool SendSpellEvent(entt::entity spell, const particles::SpellEventInfo& event) = 0;
	/// A miracle is made to pay prayer power, its caster asked for the whole shortfall, as a blow on a shield is; its
	/// strength after
	virtual float ForcePayForSpell(entt::entity spell, float cost) = 0;
	/// A miracle's strength now, 0 once it has gone
	[[nodiscard]] virtual float SpellStrength(entt::entity spell) = 0;

	// Turns and frames

	/// Once a game turn, before the particles: the dispensers, then every miracle's upkeep, effect and events
	virtual void ProcessTurn() = 0;
	/// Once a frame: the held miracle follows the hand, the bubbles' seeds spin
	virtual void Update(float seconds) = 0;
	/// A new land: every miracle goes
	virtual void Reset() = 0;

	// For the debug window and the testbed

	/// For the testbed and the debug window, until worship sets them: a player's power multiplier for a tribe
	virtual void SetTribalPower(PlayerNames /*player*/, Tribe /*tribe*/, float /*power*/) {}
	/// A cheat for the debug window: the miracles may be cast outside the player's influence
	virtual void SetIgnoreInfluence(bool ignore) = 0;
	/// For the testbed's scenarios: the hand is where the scenario puts it, not where the mouse is, until none is given,
	/// so that a scenario can cast through the hand as a player does
	virtual void DriveHand(std::optional<HandFrame> frame) = 0;
	/// Where a scenario puts the hand, if it does: the hand is drawn there
	[[nodiscard]] virtual std::optional<HandFrame> GetDrivenHand() const = 0;
	[[nodiscard]] virtual bool IsIgnoringInfluence() const = 0;
	[[nodiscard]] virtual std::vector<SpellInfo> GetSpells() const = 0;
	/// The newest miracle of a kind, closing down or not, whose last event was strictly within a radius of a point
	[[nodiscard]] virtual std::optional<entt::entity> SpellAt(MagicType /*type*/, glm::vec3 /*point*/, float /*radius*/) const
	{
		return std::nullopt;
	}
	/// The rain puts out a fire at a point: the first storm miracle whose size covers it, without such a reaction going,
	/// has the people come to watch
	virtual void RainOnFire(const glm::vec3& point) = 0;
	[[nodiscard]] virtual std::vector<DispenserInfo> GetDispensers() const = 0;
};

} // namespace openblack::ecs::systems

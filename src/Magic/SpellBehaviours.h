/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <functional>
#include <optional>
#include <utility>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "FlockMiracleInterface.h"
#include "InfoConstants.h"
#include "MagicWorldInterface.h"
#include "Particles/ParticleSpellLink.h"
#include "PrayerRules.h"
#include "SpellChants.h"

namespace openblack::ecs::components
{
struct Spell;
}

// What each kind of miracle does once cast: how it starts, what it does with the events its particles send (a
// fireball's flight, a bolt striking, a grain of food landing, a chakra reaching a person), what more it does each turn
// (the water's rain) and what it costs to keep. Written against the services the miracle system gives, so they are
// tested on fakes.

namespace openblack::magic
{

/// What the miracles' rules need from the miracle system
class SpellServicesInterface
{
public:
	SpellServicesInterface() = default;
	SpellServicesInterface(const SpellServicesInterface&) = delete;
	SpellServicesInterface& operator=(const SpellServicesInterface&) = delete;
	SpellServicesInterface(SpellServicesInterface&&) = delete;
	SpellServicesInterface& operator=(SpellServicesInterface&&) = delete;
	virtual ~SpellServicesInterface() = default;

	[[nodiscard]] virtual const InfoConstants& Info() const = 0;
	[[nodiscard]] virtual MagicWorldInterface& World() = 0;
	/// The caster who pays for the miracle, none once it has gone
	[[nodiscard]] virtual SpellCasterInterface* CasterOf(const ecs::components::Spell& spell) = 0;
	/// The tribal power of the miracle's caster for its magic type, 1 without one
	[[nodiscard]] virtual float TribalPower(const ecs::components::Spell& spell) const = 0;
	/// The power of the seed that cast it, 1 without one
	[[nodiscard]] virtual float SeedPower(const ecs::components::Spell& spell) const = 0;
	/// An object for the miracle's particle effect to act on, such as a person for the heal's chakra
	virtual void AddEffectTarget(const ecs::components::Spell& spell, entt::entity target) = 0;
	/// The miracle of the shield holding a point, its sphere grown by a margin, if any
	[[nodiscard]] virtual std::optional<entt::entity> ShieldAt(glm::vec3 point, float margin) = 0;
	/// The shield of a miracle was struck at a point, which it shows with a spark
	virtual void StrikeShield(entt::entity shieldSpell, glm::vec3 point) = 0;
	/// Another running miracle by its entity
	[[nodiscard]] virtual ecs::components::Spell* FindSpell(entt::entity spell) = 0;
	/// A random number up to a maximum on the stream every machine of the game shares
	[[nodiscard]] virtual float GameRandom(float max) = 0;
	/// The living near the miracle react to it as its table says, as it is cast or as it acts, unless it already has a
	/// reaction going
	virtual void ReactToSpell(ecs::components::Spell& spell, bool onCast) = 0;
	/// A further effect of the miracle's own at the point it is cast, such as the storm's swirl, which plays out by itself
	/// and goes with the miracle
	virtual void StartCastEffect(ecs::components::Spell& /*spell*/, ParticleType /*type*/) {}
	/// The flock miracles' animals, none where there are none to make
	[[nodiscard]] virtual FlockMiracleInterface* Flocks() { return nullptr; }
	/// The teleport's stone for the miracle at where it was cast, none if one may not go there
	virtual entt::entity CreateTeleportStone(const ecs::components::Spell& /*spell*/) { return entt::null; }
	/// Whether a teleport stone may go at a point: nothing fixed on the land within its reach
	[[nodiscard]] virtual bool CanPlaceTeleportStone(glm::vec3 /*point*/) const { return true; }
	/// A shield was struck by another miracle and stood, or was destroyed: the people about it see it
	virtual void ShieldStruck(const ecs::components::Spell& /*shield*/, bool /*destroyed*/) {}
	/// Whether what a miracle made in the world still stands: a shield's dome, a forest's trees
	[[nodiscard]] virtual bool HasWorldObjects(const ecs::components::Spell& /*spell*/) const { return false; }
	/// The forest plants its trees round where it was cast; whether it planted any
	virtual bool PlantForest(ecs::components::Spell& /*spell*/) { return false; }
	/// Whether a forest may be cast at a point: there is room for a tree, and no building's fire there
	[[nodiscard]] virtual bool ForestCanGrowAt(glm::vec3 /*point*/) const { return true; }
	/// The miracle's own entity, none where there are no entities
	[[nodiscard]] virtual entt::entity SpellEntity(const ecs::components::Spell& /*spell*/) const { return entt::null; }
};

/// A player as a caster: the neutral player, whose miracles scripts cast, gives all the prayer power they ask for; any
/// other player pays from their prayer power, which the store function finds (none without a store)
class PlayerSpellCaster final: public SpellCasterInterface
{
public:
	using StoreOf = std::function<ecs::components::PrayerPower*(PlayerNames)>;
	explicit PlayerSpellCaster(PlayerNames player, StoreOf store = {})
	    : _player(player)
	    , _store(std::move(store))
	{
	}
	float MaintainSpell(float amount) override
	{
		return PlayerMaintainSpell(_player, _store ? _store(_player) : nullptr, amount);
	}

private:
	PlayerNames _player;
	StoreOf _store;
};

/// A player as the caster of a miracle whose seed came from a globe or a dispenser. The player made that seed, not a
/// worship site, and a player tops up no miracle; only the neutral player gives all its miracles ask for.
class GlobeSpellCaster final: public SpellCasterInterface
{
public:
	void Bind(PlayerNames player) { _player = player; }
	float MaintainSpell(float amount) override { return PlayerMaintainSpell(_player, nullptr, amount); }

private:
	PlayerNames _player {PlayerNames::NEUTRAL};
};

/// Any other object as a caster gives all the prayer power its miracle asks for
class ObjectSpellCaster final: public SpellCasterInterface
{
public:
	float MaintainSpell(float amount) override { return amount; }
};

namespace spells
{

/// The chant rules of a running miracle now: its magic type's, its upkeep (a shield's by its size), its caster's
/// tribal power and its seed's power
[[nodiscard]] SpellChantRules RulesOf(SpellServicesInterface& services, const ecs::components::Spell& spell);
/// Its strength now, 0 to 1 times the multipliers
[[nodiscard]] float StrengthOf(SpellServicesInterface& services, const ecs::components::Spell& spell);
/// What it costs to keep each turn
[[nodiscard]] float CostToMaintain(const InfoConstants& info, const ecs::components::Spell& spell);

/// Whether a miracle held in the hand may stay where the hand points: on the map, and its table's rule alone, land being
/// a cell without water (influence counts unless ignored)
[[nodiscard]] bool MeetsCastRule(SpellServicesInterface& services, MagicType type, PlayerNames player, glm::vec3 point,
                                 bool ignoreInfluence);
/// Whether a player may cast a magic type at a point: its tables' rule (on land, a cell without water, and in the player's
/// influence, unless influence is ignored), then its kind's own (the heal needs someone to heal, food, wood and the
/// forest land)
[[nodiscard]] bool CanCastAt(SpellServicesInterface& services, MagicType type, PlayerNames player, glm::vec3 point,
                             bool ignoreInfluence);
/// Whether a miracle can be cast at a point by itself, as a creature casts it, with no influence to hold it back: a heal
/// only where it finds someone to heal
[[nodiscard]] bool CanCastAtPointItself(SpellServicesInterface& services, MagicType type, glm::vec3 point);

/// What its kind does before its effect starts: a shield takes its size within its limits, and what a kind keeps
/// starts afresh
void Prepare(SpellServicesInterface& services, ecs::components::Spell& spell);

/// What its kind does as it starts, after it is set up and its effect started: the heal gives its effect the people to
/// heal. False when it can't start.
bool Start(SpellServicesInterface& services, ecs::components::Spell& spell);

/// An event its particles send; whether it acted
bool OnEvent(SpellServicesInterface& services, ecs::components::Spell& spell, const particles::SpellEventInfo& event);

/// What every miracle does at an event while it runs: it moves there and pays for the event, and its effect, scaled by
/// its strength, its tribal power and the event's strength, goes to the object the event names, to the miracle it
/// struck, or round the point. A shield in the way may stop it. Whether it acted.
bool ApplyDefaultEffect(SpellServicesInterface& services, ecs::components::Spell& spell,
                        const particles::SpellEventInfo& event);

/// A miracle strikes another, such as a shield: the other pays this one's strength times its cost per shield impact, this
/// one pays for the event. Whether it got through: the other had nothing left, or this one destroyed it.
bool StrikeSpell(SpellServicesInterface& services, ecs::components::Spell& spell, ecs::components::Spell& other);

/// What its kind does each turn beyond its particles: the water rains a drop round where it is cast
void ProcessTurn(SpellServicesInterface& services, ecs::components::Spell& spell);

/// Whether what its kind made keeps it alive after its effect has gone or it has closed down: a shield's dome fading out,
/// a forest's trees withering, a flock still flying
[[nodiscard]] bool KeptByKind(SpellServicesInterface& services, const ecs::components::Spell& spell);

/// The particle effect its kind starts: its magic type's, but the flocks' by kind (and the flying flock's by its caster's
/// alignment)
[[nodiscard]] ParticleType ParticleTypeOf(SpellServicesInterface& services, const ecs::components::Spell& spell);

/// Whether its kind keeps following its caster's hand after the cast: a flock while it still makes its animals
[[nodiscard]] bool FollowsHand(SpellServicesInterface& services, const ecs::components::Spell& spell);

/// Who its effects come from, whose alignment they move
[[nodiscard]] EffectSource SourceOf(const ecs::components::Spell& spell);

/// Whether what it has left is enough to cast it again from its seed: food and wood need enough for their first grain,
/// a forest trees still to make
[[nodiscard]] bool HasEnoughForRecast(const InfoConstants& info, const ecs::components::Spell& spell);

/// The heat a fireball of a magic type gives off at full strength
[[nodiscard]] float FireballTemperature(const InfoConstants& info, MagicType type);

} // namespace spells

} // namespace openblack::magic

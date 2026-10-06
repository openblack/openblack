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

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "InfoConstants.h"
#include "Particles/ParticleSpellLink.h"
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
class MagicWorldInterface;

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
};

/// A player as a caster: the neutral player, whose miracles scripts cast, gives all the prayer power they ask for; a
/// human player gives none, so a miracle from the hand lives on what it was cast with
class PlayerSpellCaster final: public SpellCasterInterface
{
public:
	explicit PlayerSpellCaster(PlayerNames player)
	    : _player(player)
	{
	}
	float MaintainSpell(float amount) override { return _player == PlayerNames::NEUTRAL ? amount : 0.0f; }

private:
	PlayerNames _player;
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

/// Whether a player may cast a magic type at a point: its tables' rule (on land, in the player's influence, unless
/// influence is ignored), then its kind's own (the heal needs someone to heal, food and wood dry land)
[[nodiscard]] bool CanCastAt(SpellServicesInterface& services, MagicType type, PlayerNames player, glm::vec3 point,
                             bool ignoreInfluence);

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

/// Whether what it has left is enough to cast it again from its seed: food and wood need enough for their first grain
[[nodiscard]] bool HasEnoughForRecast(const InfoConstants& info, const ecs::components::Spell& spell);

/// The heat a fireball of a magic type gives off at full strength
[[nodiscard]] float FireballTemperature(const InfoConstants& info, MagicType type);

} // namespace spells

} // namespace openblack::magic

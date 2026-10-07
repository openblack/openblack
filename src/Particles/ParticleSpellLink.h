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

/// What a miracle and its particle effect tell each other: what the miracle gives the effect each step, the events the
/// effect's rules send back (a particle landing, striking a target), and how far the miracle was powered up. The
/// miracles implement SpellSink; an effect started without one simply has none.
namespace openblack::particles
{

/// What a miracle hands its effect for each step
struct ProcessInfo
{
	/// Where the caster's hand is, or where a script casts from
	glm::vec3 handPosition {0.0f};
	/// Which way the camera looks, or from the script's point to its target
	glm::vec3 cameraForward {0.0f};
	/// The hand's movement, or the way the miracle was cast
	glm::vec3 direction {0.0f};
	/// The miracle's strength, which the strength float providers read
	float power {1.0f};
	/// While false, the conditions that ask whether the miracle is still being cast fail
	bool enabled {true};
	/// How the caster spun the miracle as it let go, in radians a second, which curves a thrown fireball
	float spin {0.0f};
};

/// An event a rule sends to the miracle
struct SpellEventInfo
{
	enum class Type : uint8_t
	{
		/// The effect has started
		Started = 1,
		/// Something happens at a point
		Point = 2,
		/// A particle reached the land
		Landed = 3,
		/// A particle struck a shield or another miracle
		HitSpell = 4,
		/// A particle reached its target object
		Object = 5,
		/// The effect asks to pick up the target object, which the miracle allows if it can destroy it
		Capture = 7,
		/// A miracle whose magic type has no particle effect has started without one, at its point
		InitWithoutEffect = 11,
	};
	Type type {Type::Point};
	glm::vec3 position {0.0f};
	/// The particle's movement over the step
	glm::vec3 velocity {0.0f};
	/// How strongly the miracle acts on it
	float strength {1.0f};
	bool checkShields {false};
	entt::entity target {entt::null};
};

/// The miracle behind an effect
class SpellSink
{
public:
	SpellSink() = default;
	SpellSink(const SpellSink&) = default;
	SpellSink(SpellSink&&) = default;
	SpellSink& operator=(const SpellSink&) = default;
	SpellSink& operator=(SpellSink&&) = default;
	virtual ~SpellSink() = default;

	/// Whether the miracle acted on the event
	virtual bool SpellEvent(const SpellEventInfo& event) = 0;
	/// How far it was powered up: -1 for not at all, then 0 and 1
	[[nodiscard]] virtual int PowerUpLevel() const = 0;
	/// Whether this computer's player is casting it
	[[nodiscard]] virtual bool IsMyInterfaceCasting() const { return false; }
	/// Whether a human player, rather than a creature or a script, is casting it
	[[nodiscard]] virtual bool IsHumanPlayerCasting() const { return false; }
	/// Whether a creature casts it, or the neutral player by script
	[[nodiscard]] virtual bool IsCreatureCasting() const { return false; }
	[[nodiscard]] virtual bool IsScriptCasting() const { return false; }
	/// What a creature spell does to the creature it is cast on, by the game's numbering (CreatureReceiveSpellType), -1
	/// for a miracle that isn't a creature spell
	[[nodiscard]] virtual int CreatureSpellKind() const { return -1; }
	/// The miracle's own entity, for the shields it raises
	[[nodiscard]] virtual entt::entity Spell() const { return entt::null; }
	/// The tribal power of the miracle's caster for its magic type, 1 without one
	[[nodiscard]] virtual float TribalPower() const { return 1.0f; }
	/// How much a storm miracle rains, none for a miracle that isn't a storm
	[[nodiscard]] virtual std::optional<float> RainAmount() const { return std::nullopt; }
	/// The miracle now acts where its effect has moved to, as a storm drifts
	virtual void MoveTo(glm::vec3 /*position*/) {}
	/// A creature spell cast by a creature turns against it once the way from its hands to its target creature has
	/// swung round from where it began by more than this (the game takes the number as radians); none for no limit
	[[nodiscard]] virtual std::optional<float> MaxDirectionChange() const { return std::nullopt; }
	/// The miracle closes down, as a creature's spell does once it swings too far
	virtual void CloseDown() {}
	/// The radius its effect reaches, from its tables
	[[nodiscard]] virtual float EffectRadius() const { return 5.0f; }
};

} // namespace openblack::particles

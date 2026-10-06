/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

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
	/// The miracle's own entity, for the shields it raises
	[[nodiscard]] virtual entt::entity Spell() const { return entt::null; }
};

} // namespace openblack::particles

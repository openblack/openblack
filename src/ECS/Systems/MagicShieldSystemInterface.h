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
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::systems
{

/// The shield miracles' objects in the world. Each running shield miracle raises one where it was cast: the spiritual
/// shield's marks where it is (its sphere is its particle effect's), the physical shield's is a solid dome that grows,
/// spins and bobs, and stops what is thrown at it. While a shield stands no other player has influence under it, the
/// people about it shelter under it, and other players' creatures walk round it. Once its miracle lets go the spiritual
/// shield's goes at once, the dome fades away.
class MagicShieldSystemInterface
{
public:
	virtual ~MagicShieldSystemInterface() = default;

	/// Once a game turn, after the miracles: new shields are raised, the domes grow, spin, bob and fade, the people react
	virtual void ProcessTurn() = 0;
	/// Once a frame: what is thrown at a dome bounces off it, each blow costing the shield
	virtual void Update(float seconds) = 0;
	/// A new land: no shields
	virtual void Reset() = 0;

	/// A shield miracle was struck by another and stood, or was destroyed by it
	virtual void ShieldStruck(entt::entity spell, bool destroyed) = 0;
	/// Whether a shield miracle's object still stands
	[[nodiscard]] virtual bool HasObject(entt::entity spell) const = 0;

	/// A circle across the land a creature has to walk round
	struct Avoid
	{
		glm::vec2 centre;
		float radius;
	};
	/// The shields a creature of a player walks round: those of any other player, unless a script moves it
	[[nodiscard]] virtual std::vector<Avoid> CreatureAvoids(PlayerNames creaturePlayer, bool scripted) const = 0;

	/// Whether a shield keeps a reaction from someone standing under it: they stand within a shield's reach on the
	/// ground and what made the reaction is not inside that shield
	[[nodiscard]] virtual bool KeepsReactionOff(glm::vec3 /*watcher*/, glm::vec3 /*initiator*/) const { return false; }
	/// The first shield whose reach on the ground a point is strictly within, of either kind, as a script asks for a
	/// shield miracle at a point
	[[nodiscard]] virtual std::optional<entt::entity> ShieldAt(glm::vec3 /*point*/) const { return std::nullopt; }

	/// A dome as it is drawn this frame
	struct DomeDraw
	{
		entt::id_type mesh;
		glm::vec3 position;
		float angle;
		float scale;
	};
	/// The domes to draw, between their last two turns by the turn's fraction
	[[nodiscard]] virtual std::vector<DomeDraw> GetDomes(float fraction) const = 0;
};

} // namespace openblack::ecs::systems

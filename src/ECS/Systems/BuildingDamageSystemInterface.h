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

#include <entt/entity/fwd.hpp>

namespace openblack::ecs
{
struct PhysicsEntry;
struct ImpactInfo;
} // namespace openblack::ecs

namespace openblack::ecs::systems
{
class DynamicsSystemInterface;

/// Buildings broken by rocks: the pieces knocked out of their models fly off and lie about until their time runs out,
/// and what is left of a building is drawn as it stands
class BuildingDamageSystemInterface
{
public:
	virtual ~BuildingDamageSystemInterface() = default;

	/// A building's reaction to its last turn's knock: a rock struck it, which only sounds when it is slow, and breaks
	/// pieces off it when it is fast and heavy enough
	virtual void ReactToImpact(DynamicsSystemInterface& dynamics, PhysicsEntry& entry, const ImpactInfo& impact) = 0;
	/// A creature's blow on a building breaks it about where it stands, as far round as the creature is big
	virtual void Smash(entt::entity building, entt::entity creature, float creatureSize) = 0;
	/// A piece comes to rest: a big one goes back into its building as rubble, the rest lie where they are. The object that
	/// stays, none when it went into its building.
	virtual entt::entity PieceAtRest(DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity piece,
	                                 bool insert) = 0;
	/// A rock stops being one that breaks buildings (it came to rest or was taken): the buildings it broke forget it
	virtual void ForgetHitter(entt::entity rock) = 0;
	/// Every game turn: pieces whose building has gone forget it, and pieces whose time has run out go
	virtual void ProcessTurn() = 0;
	/// The model an object is drawn with in place of its own: a broken building's broken model; its own otherwise
	[[nodiscard]] virtual entt::id_type DrawnMesh(entt::entity object, entt::id_type own) const = 0;
	/// How much of a broken or unfinished building is drawn standing over what is left of it, none for one drawn whole
	/// or not at all
	[[nodiscard]] virtual std::optional<float> PartialShare(entt::entity building) const = 0;
	/// A new land: every building whole again
	virtual void Reset() = 0;
};

} // namespace openblack::ecs::systems

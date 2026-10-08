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

#include <entt/entity/entity.hpp>

namespace openblack::ecs::script_objects
{

/// The kinds of object a script lets go of differently
enum class Kind : uint8_t
{
	Other,
	/// A villager or a villager's child
	Villager,
	Animal,
	Creature,
};

/// What the scripts' object table needs of the world: the objects, their script flags, and how each kind goes back into
/// the game. The game's own world works on the entities; tests give a fake.
class World
{
public:
	virtual ~World() = default;

	/// The object is still in the world
	[[nodiscard]] virtual bool Exists(entt::entity object) const = 0;
	/// The object can still be dealt with: in the world, and for a villager not on its way to dying
	[[nodiscard]] virtual bool IsAvailable(entt::entity object) const = 0;
	[[nodiscard]] virtual Kind KindOf(entt::entity object) const = 0;
	/// A script holds a reference to the object
	[[nodiscard]] virtual bool IsInScript(entt::entity object) const = 0;
	virtual void SetInScript(entt::entity object, bool inScript) = 0;
	/// A script controls the object
	[[nodiscard]] virtual bool IsControlled(entt::entity object) const = 0;
	virtual void SetControlled(entt::entity object, bool controlled) = 0;
	/// Flying or lying in the physics
	[[nodiscard]] virtual bool IsInPhysics(entt::entity object) const = 0;
	/// Standing in the map's cells: not held, flying or carried
	[[nodiscard]] virtual bool IsInMap(entt::entity object) const = 0;
	/// A villager is set to decide what to do next as a script sets it: the state it was in is kept as its previous
	/// one, that state is left, and its clip and its wait start afresh
	virtual void SetVillagerDecideWhatToDo(entt::entity villager) = 0;
	/// A villager in the physics goes back to deciding what to do when it is out: its previous state is set to that
	virtual void SetVillagerPreviousDecideWhatToDo(entt::entity villager) = 0;
	/// A creature gives up what it is doing as a failure
	virtual void AbandonCreatureAction(entt::entity creature) = 0;
	/// The object goes from the world at once
	virtual void Delete(entt::entity object) = 0;
};

} // namespace openblack::ecs::script_objects

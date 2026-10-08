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

#include <entt/entity/fwd.hpp>

namespace openblack::ecs::systems
{

/// The objects the scripts hold, the references they keep to them, and which of them a script controls
class ScriptObjectsSystemInterface
{
public:
	virtual ~ScriptObjectsSystemInterface() = default;

	/// An object a native made (or only found) for a script takes its place in the table; false when the table is full
	virtual bool Register(entt::entity object, bool createdByScript) = 0;
	/// Before each native runs: whether it takes control of the objects it is given
	virtual void EnterNative(uint32_t native) = 0;
	/// An object a native is given: a native that takes control takes control of it
	virtual entt::entity Fetch(entt::entity object) = 0;
	/// A script variable takes or lets go of an object
	virtual void AddReference(entt::entity object) = 0;
	virtual void RemoveReference(entt::entity object) = 0;
	/// A script lets go of its control of an object, which goes back into the game
	virtual void ReleaseFromScript(entt::entity object) = 0;
	/// What a script held as one object it now holds as another
	virtual void Replace(entt::entity from, entt::entity to) = 0;
	[[nodiscard]] virtual bool IsCreatedByScript(entt::entity object) const = 0;
	/// The land's scripts are gone: every place is free again
	virtual void Reset() = 0;
};

} // namespace openblack::ecs::systems

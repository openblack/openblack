/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "ScriptObjectsSystem.h"

#include <spdlog/spdlog.h>

#include "ECS/Components/LivingAction.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/ScriptControl.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
uint32_t Key(entt::entity object)
{
	return static_cast<uint32_t>(object);
}

void SetControlled(entt::entity object, bool controlled)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (controlled)
	{
		registry.AssignOrReplace<ScriptControlled>(object);
	}
	else if (registry.AllOf<ScriptControlled>(object))
	{
		registry.Remove<ScriptControlled>(object);
	}
}
} // namespace

bool ScriptObjectsSystem::Register(entt::entity object, bool createdByScript)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "Adding null script thing");
		return false;
	}
	if (!_table.Register(Key(object), createdByScript, registry.AllOf<InScript>(object)))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "Script offsets exceeded");
		return false;
	}
	return true;
}

void ScriptObjectsSystem::EnterNative(uint32_t native)
{
	_takesControl = script_objects::TakesControl(native);
}

entt::entity ScriptObjectsSystem::Fetch(entt::entity object)
{
	auto& registry = Locator::entitiesRegistry::value();
	// A native that takes control takes it of an object not yet controlled; any other native leaves it as it is
	if (_takesControl && registry.Valid(object) && !registry.AllOf<ScriptControlled>(object))
	{
		SetControlled(object, true);
	}
	return object;
}

void ScriptObjectsSystem::AddReference(entt::entity object)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object))
	{
		return;
	}
	// openblack's natives hand objects to the scripts without placing them, as the game's finding natives do, so the
	// place is taken at the first reference
	auto place = _table.Find(Key(object));
	if (!place && Register(object, false))
	{
		place = _table.Find(Key(object));
	}
	if (!place)
	{
		return;
	}
	const auto referenced = _table.AddReference(*place);
	if (referenced == script_objects::Referenced::First)
	{
		registry.AssignOrReplace<InScript>(object);
		SetControlled(object, registry.AllOf<ScriptControlled>(object) || _table.At(*place).createdByScript);
	}
	else if (referenced == script_objects::Referenced::Again)
	{
		registry.AssignOrReplace<InScript>(object);
	}
}

void ScriptObjectsSystem::RemoveReference(entt::entity object)
{
	if (const auto place = _table.Find(Key(object)))
	{
		_table.RemoveReference(*place);
	}
}

void ScriptObjectsSystem::ReleaseFromScript(entt::entity object)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "Thing not valid");
		return;
	}
	if (!registry.AllOf<ScriptControlled>(object))
	{
		return;
	}
	SetControlled(object, false);
	// A villager goes back to deciding what to do, unless it is flying or lying in the physics
	if (registry.AllOf<Villager>(object) && !registry.AllOf<InPhysics>(object))
	{
		if (auto* action = registry.TryGet<LivingAction>(object))
		{
			Locator::livingActionSystem::value().VillagerSetState(*action, LivingAction::Index::Top,
			                                                      VillagerStates::DecideWhatToDo, false);
		}
	}
}

void ScriptObjectsSystem::Replace(entt::entity from, entt::entity to)
{
	if (!_table.Find(Key(from)))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "Script thing without slot");
		return;
	}
	_table.Replace(Key(from), Key(to));
}

bool ScriptObjectsSystem::IsCreatedByScript(entt::entity object) const
{
	const auto place = _table.Find(Key(object));
	return place.has_value() && _table.At(*place).createdByScript;
}

void ScriptObjectsSystem::Reset()
{
	_table.Clear();
	_takesControl = false;
}

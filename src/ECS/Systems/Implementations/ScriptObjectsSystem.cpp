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

#include "ECS/Components/Animal.h"
#include "ECS/Components/CarriedByTornado.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/MapCellResident.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/ScriptControl.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/VillagerMemory.h"
#include "ECS/WorldObjects.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
using openblack::ecs::script_objects::Kind;

namespace
{
uint32_t Key(entt::entity object)
{
	return static_cast<uint32_t>(object);
}

template <typename Tag>
void SetTag(openblack::ecs::Registry& registry, entt::entity object, bool on)
{
	if (on)
	{
		registry.AssignOrReplace<Tag>(object);
	}
	else if (registry.AllOf<Tag>(object))
	{
		registry.Remove<Tag>(object);
	}
}

/// The game's own entities, as the scripts' object table sees them
class GameWorld final: public ecs::script_objects::World
{
public:
	[[nodiscard]] bool Exists(entt::entity object) const override { return Registry().Valid(object); }

	[[nodiscard]] bool IsAvailable(entt::entity object) const override
	{
		auto& registry = Registry();
		if (!registry.Valid(object))
		{
			return false;
		}
		// A villager on its way to dying is no longer to be dealt with
		const auto* action = registry.AllOf<Villager>(object) ? registry.TryGet<const LivingAction>(object) : nullptr;
		return action == nullptr || !Locator::livingActionSystem::has_value() ||
		       Locator::livingActionSystem::value().VillagerGetState(*action, LivingAction::Index::Final) !=
		           VillagerStates::Dying;
	}

	[[nodiscard]] Kind KindOf(entt::entity object) const override
	{
		const auto& registry = Registry();
		if (registry.AllOf<Villager>(object))
		{
			return Kind::Villager;
		}
		if (registry.AllOf<Animal>(object))
		{
			return Kind::Animal;
		}
		if (registry.AllOf<Creature>(object))
		{
			return Kind::Creature;
		}
		return Kind::Other;
	}

	[[nodiscard]] bool IsInScript(entt::entity object) const override { return Registry().AllOf<InScript>(object); }
	void SetInScript(entt::entity object, bool inScript) override { SetTag<InScript>(Registry(), object, inScript); }
	[[nodiscard]] bool IsControlled(entt::entity object) const override { return Registry().AllOf<ScriptControlled>(object); }
	void SetControlled(entt::entity object, bool controlled) override
	{
		SetTag<ScriptControlled>(Registry(), object, controlled);
	}
	[[nodiscard]] bool IsInPhysics(entt::entity object) const override { return Registry().AllOf<InPhysics>(object); }
	[[nodiscard]] bool IsInMap(entt::entity object) const override
	{
		const auto& registry = Registry();
		return registry.AllOf<MapCellResident>(object) && !registry.AnyOf<InHand, InPhysics, CarriedByTornado>(object);
	}

	void SetVillagerDecideWhatToDo(entt::entity villager) override
	{
		auto* action = Registry().TryGet<LivingAction>(villager);
		if (action == nullptr || !Locator::livingActionSystem::has_value())
		{
			return;
		}
		auto& living = Locator::livingActionSystem::value();
		ecs::villager_memory::StorePreviousState(*action);
		// What it was doing, and where it was heading if that differs, is left whatever they say; deciding what to do has
		// nothing to do on entry, so it is simply taken up
		constexpr auto k_Next = VillagerStates::DecideWhatToDo;
		living.VillagerCallExitState(*action, LivingAction::Index::Top, k_Next);
		if (living.VillagerGetState(*action, LivingAction::Index::Final) !=
		    living.VillagerGetState(*action, LivingAction::Index::Top))
		{
			living.VillagerCallExitState(*action, LivingAction::Index::Final, k_Next);
		}
		living.VillagerSetState(*action, LivingAction::Index::Top, k_Next, true);
		// The new state's clip is chosen afresh by its state, and it waits no turns before acting
		action->turnsUntilStateChange = 0;
	}

	void SetVillagerPreviousDecideWhatToDo(entt::entity villager) override
	{
		auto* action = Registry().TryGet<LivingAction>(villager);
		if (action == nullptr || !Locator::livingActionSystem::has_value())
		{
			return;
		}
		Locator::livingActionSystem::value().VillagerSetState(*action, LivingAction::Index::Previous,
		                                                      VillagerStates::DecideWhatToDo, true);
	}

	void AbandonCreatureAction(entt::entity creature) override
	{
		if (Locator::creatureMindSystem::has_value())
		{
			Locator::creatureMindSystem::value().AbandonAction(creature);
		}
	}

	void Delete(entt::entity object) override { ecs::world_objects::Remove(object); }

private:
	static ecs::Registry& Registry() { return Locator::entitiesRegistry::value(); }
};
} // namespace

ScriptObjectsSystem::ScriptObjectsSystem()
    : ScriptObjectsSystem(std::make_unique<GameWorld>())
{
}

ScriptObjectsSystem::ScriptObjectsSystem(std::unique_ptr<script_objects::World> world)
    : _world(std::move(world))
{
}

bool ScriptObjectsSystem::Register(entt::entity object, bool createdByScript)
{
	if (!_world->Exists(object))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "Adding null script thing");
		return false;
	}
	if (!_table.Register(Key(object), createdByScript, _world->IsInScript(object)))
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
	// A native that takes control takes it of an object not yet controlled; any other native leaves it as it is
	if (_takesControl && _world->Exists(object) && !_world->IsControlled(object))
	{
		_world->SetControlled(object, true);
	}
	return object;
}

void ScriptObjectsSystem::AddReference(entt::entity object)
{
	if (!_world->Exists(object))
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
		_world->SetInScript(object, true);
		_world->SetControlled(object, _world->IsControlled(object) || _table.At(*place).createdByScript);
	}
	else if (referenced == script_objects::Referenced::Again)
	{
		_world->SetInScript(object, true);
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
	if (!_world->Exists(object))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "Thing not valid");
		return;
	}
	if (_world->IsControlled(object))
	{
		ReleaseIntoGame(object);
	}
}

void ScriptObjectsSystem::ReleaseIntoGame(entt::entity object)
{
	_world->SetControlled(object, false);
	switch (_world->KindOf(object))
	{
	case Kind::Villager:
		if (!_world->IsInMap(object) && !_world->IsInPhysics(object))
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "Releasing object not in map");
		}
		if (_world->IsInPhysics(object))
		{
			// It decides what to do once it is out of the physics
			_world->SetVillagerPreviousDecideWhatToDo(object);
		}
		else if (_world->IsAvailable(object) && _world->IsInMap(object))
		{
			// Only a villager standing in the map is set to decide what to do at once
			_world->SetVillagerDecideWhatToDo(object);
		}
		// TODO(physics): the villager then leaves the flock it was in and finds its town again, or wanders homeless;
		// openblack keeps no villager flocks or homeless list yet
		break;
	case Kind::Creature:
		// It gives up whatever the script had it doing
		_world->AbandonCreatureAction(object);
		break;
	case Kind::Animal:
		// TODO(physics): an animal is set to wander, keeping that as its previous state while in the physics, and finds
		// itself a flock; openblack's animals keep no previous state yet
		break;
	case Kind::Other:
		break;
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

void ScriptObjectsSystem::Reset()
{
	for (uint16_t index = 1; index < script_objects::k_Places; ++index)
	{
		const auto& place = _table.At(index);
		const auto object = static_cast<entt::entity>(place.object);
		if ((place.object == 0 && place.count == 0) || !_world->IsAvailable(object))
		{
			continue;
		}
		if (!_world->IsInScript(object))
		{
			SPDLOG_LOGGER_WARN(spdlog::get("scripting"), "Releasing reference to script thing not in script");
		}
		if (place.createdByScript)
		{
			// What a script made goes with its scripts
			_world->Delete(object);
			continue;
		}
		// Anything else a script held goes back into the game, no longer in a script
		if (_world->IsControlled(object))
		{
			ReleaseIntoGame(object);
		}
		_world->SetInScript(object, false);
	}
	_table.Clear();
	_takesControl = false;
}

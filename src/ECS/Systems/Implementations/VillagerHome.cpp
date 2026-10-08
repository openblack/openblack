/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "VillagerHome.h"

#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <glm/trigonometric.hpp>

#include "3D/L3DMesh.h"
#include "Common/GameRandom.h"
#include "Common/RandomNumberManager.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/AtHome.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/WallHug.h"
#include "ECS/Registry.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/TownDesireSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
namespace components = openblack::ecs::components;
namespace villager_home = openblack::ecs::villager_home;

namespace
{
/// How far an idle villager may roam from its town when it has nothing to do
constexpr float k_WanderRadius = 40.0f;
/// Turns an idle villager pauses before deciding again
constexpr uint16_t k_DecideCooldownTurns = 20;
/// Keep goals clear of the map's edges, where the pathfinder runs out of cells
constexpr float k_WanderWorldBoundMin = 30.0f;
constexpr float k_WanderWorldBoundMax = 5090.0f;
/// A child's trigger when nothing else presses it
constexpr float k_ChildTrigger = 0.11f;
/// An idle villager goes home one time in this many
constexpr uint32_t k_NothingToDoChoices = 9;
/// A villager at home with nothing to do goes to bed one time in this many
constexpr uint32_t k_GoToBedChance = 4;

entt::entity EntityOf(LivingAction& action)
{
	return Locator::entitiesRegistry::value().ToEntity(action);
}

void SetTopState(LivingAction& action, VillagerStates state)
{
	Locator::livingActionSystem::value().VillagerSetState(action, LivingAction::Index::Top, state, false);
}

VillagerStates State(const LivingAction& action, LivingAction::Index index)
{
	return Locator::livingActionSystem::value().VillagerGetState(action, index);
}

const GVillagerInfo& InfoOf(const Villager& villager)
{
	const auto& infos = Locator::infoConstants::value().villager;
	return infos.at(static_cast<size_t>(GVillagerInfo::Find(villager.tribe, villager.number)));
}

entt::entity AbodeOf(entt::entity villager)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto abode = registry.Get<const Villager>(villager).abode;
	return abode != entt::null && registry.Valid(abode) && registry.AllOf<Abode>(abode) ? abode : entt::null;
}

entt::entity TownOf(entt::entity villager)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto town = registry.Get<const Villager>(villager).town;
	return town != entt::null && registry.Valid(town) ? town : entt::null;
}

bool IsChild(entt::entity villager)
{
	return Locator::entitiesRegistry::value().Get<const Villager>(villager).lifeStage == Villager::LifeStage::Child;
}

bool IsAtHome(entt::entity villager)
{
	return Locator::entitiesRegistry::value().AnyOf<AtHome>(villager);
}

/// Where a villager goes into its abode: the mesh's door, or the abode itself without one
glm::vec2 ArrivePosition(entt::entity abode)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto& transform = registry.Get<const Transform>(abode);
	glm::vec3 point = transform.position;
	if (const auto* mesh = registry.TryGet<const Mesh>(abode); mesh != nullptr)
	{
		const auto& meshes = Locator::resources::value().GetMeshes();
		if (meshes.Contains(mesh->id))
		{
			if (const auto& door = meshes.Handle(mesh->id)->GetDoorPos(); door.has_value())
			{
				point = transform.position + transform.rotation * (transform.scale * *door);
			}
		}
	}
	return glm::xz(point);
}

/// How strongly the villager's own needs press it: none while its hunger and life don't change, a little for a child
float OwnDesiresTrigger(entt::entity villager)
{
	// TODO: the game weighs the villager's hunger and life here once they are kept
	return IsChild(villager) ? k_ChildTrigger : 0.0f;
}

/// Offers the villager to what its town wants most
uint32_t CheckNeededForTownDesire(LivingAction& action)
{
	const auto villager = EntityOf(action);
	const auto town = TownOf(villager);
	if (town == entt::null)
	{
		return 0;
	}
	return Locator::townDesireSystem::value().OfferVillager(
	    town, IsChild(villager), OwnDesiresTrigger(villager), [&action](TownDesireInfo desire) -> uint32_t {
		    // TODO: villagers only take up the town's sleep yet; the game has them fetch food and wood, build, repair,
		    // play and relax for it too
		    return desire == TownDesireInfo::ForSleep ? villager_home::CheckSatisfySleep(action) : 0;
	    });
}

/// Whether the villager is wanted for something: by its town, and by its own needs
uint32_t CheckNeededForSomething(LivingAction& action)
{
	// TODO: the homeless move into an abode with room, and villagers worship and see to their own hunger first
	return CheckNeededForTownDesire(action);
}

/// Walks the villager somewhere near its town for want of anything better
void Wander(LivingAction& action)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto villager = EntityOf(action);
	const auto& transform = registry.Get<const Transform>(villager);
	auto& rng = Locator::rng::value();
	const float angle = rng.NextValue(0.0f, glm::two_pi<float>());
	const float distance = rng.NextValue(0.0f, k_WanderRadius);
	auto origin = glm::xz(transform.position);
	if (const auto town = TownOf(villager); town != entt::null && registry.AllOf<Transform>(town))
	{
		origin = glm::xz(registry.Get<const Transform>(town).position);
	}
	auto goal = origin + glm::vec2(glm::cos(angle), glm::sin(angle)) * distance;
	goal = glm::clamp(goal, glm::vec2(k_WanderWorldBoundMin), glm::vec2(k_WanderWorldBoundMax));
	villager_home::SetupMoveTo(action, goal, VillagerStates::DecideWhatToDo);
}

/// What an idle villager does: now and then go home, otherwise wander
void SetupNothingToDo(LivingAction& action)
{
	const auto villager = EntityOf(action);
	if (Locator::gameRandom::value().GameRand(k_NothingToDoChoices) == 0 && AbodeOf(villager) != entt::null)
	{
		SetTopState(action, VillagerStates::GoHome);
		return;
	}
	// TODO: the game has them sit and chill out outside their home or about the town instead
	Wander(action);
}

/// Going to bed once a stay
bool CheckWhenGoingToBed(entt::entity villager)
{
	auto& atHome = Locator::entitiesRegistry::value().Get<AtHome>(villager);
	// TODO: dying of old age and having children at bed time
	atHome.beenToBed = true;
	return true;
}

/// Whether the villager sleeps on: while its town wants sleep most, or it is hurt
bool DoSleeping(entt::entity villager)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto& villagerComponent = registry.Get<const Villager>(villager);
	const auto& info = InfoOf(villagerComponent);
	const auto town = TownOf(villager);
	const bool sleepWanted =
	    town != entt::null && Locator::townDesireSystem::value().GetMostWanted(town) == TownDesireInfo::ForSleep;
	if (!sleepWanted && static_cast<float>(villagerComponent.health) >= info.damageThresholdToSleepUntil)
	{
		return false;
	}
	registry.Get<LivingAction>(villager).turnsUntilStateChange = static_cast<uint16_t>(info.restAtHomeTime);
	return true;
}
} // namespace

void villager_home::SetupMoveTo(LivingAction& action, glm::vec2 goal, VillagerStates final)
{
	SetupMobileMoveTo(action, goal, final);
	SetTopState(action, VillagerStates::MoveToPos);
}

void villager_home::SetupMobileMoveTo(LivingAction& action, glm::vec2 goal, VillagerStates final)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto villager = EntityOf(action);
	auto& wallHug = registry.Get<WallHug>(villager);
	wallHug.goal = goal;
	// A fresh step is worked out on the next pathfinding turn
	wallHug.step = glm::vec2(0.0f);
	registry.Remove<MoveStateLinearTag, MoveStateOrbitTag, MoveStateExitCircleTag, MoveStateStepThroughTag,
	                MoveStateFinalStepTag, MoveStateArrivedTag>(villager);
	registry.Remove<WallHugObjectReference>(villager);
	registry.Assign<MoveStateLinearTag>(villager);
	Locator::livingActionSystem::value().VillagerSetState(action, LivingAction::Index::Final, final, true);
}

void villager_home::ArriveHome(entt::entity villager)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto abode = AbodeOf(villager);
	if (abode == entt::null)
	{
		return;
	}
	// As in the game, arriving twice counts twice
	registry.AssignOrReplace<components::AtHome>(villager);
	++registry.Get<Abode>(abode).presentAtHome;
	registry.SetDirty();
}

void villager_home::LeaveHome(entt::entity villager)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!IsAtHome(villager))
	{
		return;
	}
	registry.Remove<components::AtHome>(villager);
	if (const auto abode = AbodeOf(villager); abode != entt::null)
	{
		auto& present = registry.Get<Abode>(abode).presentAtHome;
		present = present > 0 ? present - 1 : 0;
	}
	registry.SetDirty();
}

uint32_t villager_home::CheckSatisfySleep(LivingAction& action)
{
	const auto villager = EntityOf(action);
	if (IsAtHome(villager))
	{
		if (CheckWhenGoingToBed(villager))
		{
			SetTopState(action, VillagerStates::GotoBedAtHome);
		}
		return 1;
	}
	if (AbodeOf(villager) != entt::null)
	{
		SetTopState(action, VillagerStates::GoHome);
		return 1;
	}
	return State(action, LivingAction::Index::Top) == VillagerStates::SleepInTent ? 1 : 0;
}

uint32_t villager_home::DecideWhatToDo(LivingAction& action)
{
	// TODO(#863): villagers pause between decisions until they have animations to fill the time
	if (action.turnsSinceStateChange < k_DecideCooldownTurns)
	{
		return 0;
	}
	// TODO: children's own activities
	if (CheckNeededForSomething(action) == 1)
	{
		return 1;
	}
	SetupNothingToDo(action);
	return 1;
}

uint32_t villager_home::MoveToPos(LivingAction& action)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto villager = EntityOf(action);
	// The pathfinding system takes the moving tags off once the goal is reached
	const bool stillMoving =
	    registry.AnyOf<MoveStateLinearTag, MoveStateOrbitTag, MoveStateExitCircleTag, MoveStateStepThroughTag>(villager);
	if (stillMoving)
	{
		return 0;
	}
	registry.Remove<MoveStateFinalStepTag, MoveStateArrivedTag>(villager);
	auto final = State(action, LivingAction::Index::Final);
	if (final == VillagerStates::InvalidState)
	{
		final = VillagerStates::DecideWhatToDo;
	}
	Locator::livingActionSystem::value().VillagerSetState(action, LivingAction::Index::Final, VillagerStates::InvalidState,
	                                                      true);
	SetTopState(action, final);
	return 0;
}

uint32_t villager_home::GoHome(LivingAction& action)
{
	const auto villager = EntityOf(action);
	const auto abode = AbodeOf(villager);
	if (abode == entt::null)
	{
		// TODO: the homeless pitch a tent near their town, and those without a town wander off
		SetTopState(action, VillagerStates::DecideWhatToDo);
		return 1;
	}
	if (IsAtHome(villager))
	{
		SetTopState(action, VillagerStates::AtHome);
		return 1;
	}
	if (State(action, LivingAction::Index::Final) != VillagerStates::ArrivesHome)
	{
		SetupMoveTo(action, ArrivePosition(abode), VillagerStates::ArrivesHome);
	}
	return 1;
}

uint32_t villager_home::ArrivesHome(LivingAction& action)
{
	const auto villager = EntityOf(action);
	if (AbodeOf(villager) == entt::null)
	{
		// TODO: the game makes them homeless
		SetTopState(action, VillagerStates::DecideWhatToDo);
		return 0;
	}
	// TODO: an unbuilt or damaged home has them build or repair it, or pitch a tent when hurt
	ArriveHome(villager);
	SetTopState(action, VillagerStates::AtHome);
	return 1;
}

uint32_t villager_home::AtHome(LivingAction& action)
{
	// TODO: a town in an emergency sends everyone to bed, and women have their own business at home
	if (CheckNeededForSomething(action) == 1)
	{
		return 1;
	}
	// Nothing to do: now and then to bed, otherwise as anywhere else
	const auto villager = EntityOf(action);
	if (IsAtHome(villager) && Locator::gameRandom::value().GameRand(k_GoToBedChance) == 0)
	{
		action.turnsUntilStateChange = 0;
		SetTopState(action, VillagerStates::GotoBedAtHome);
		return 1;
	}
	SetupNothingToDo(action);
	return 1;
}

uint32_t villager_home::GotoBedAtHome(LivingAction& action)
{
	SetTopState(action, VillagerStates::SleepingAtHome);
	const auto& villager = Locator::entitiesRegistry::value().Get<const Villager>(EntityOf(action));
	action.turnsUntilStateChange = static_cast<uint16_t>(InfoOf(villager).restAtHomeTime);
	return 1;
}

uint32_t villager_home::SleepingAtHome(LivingAction& action)
{
	const auto villager = EntityOf(action);
	if (TownOf(villager) == entt::null)
	{
		return 1;
	}
	--action.turnsUntilStateChange;
	if (action.turnsUntilStateChange == 0 && !DoSleeping(villager))
	{
		SetTopState(action, VillagerStates::AtHome);
	}
	return 1;
}

bool villager_home::ExitAtHome(LivingAction& action, VillagerStates next)
{
	const auto& table = Locator::infoConstants::value().villagerStateTable;
	if (table.at(static_cast<size_t>(next)).staysAtHomeOnExit == 0)
	{
		LeaveHome(EntityOf(action));
	}
	// Leaving is never held up
	return false;
}

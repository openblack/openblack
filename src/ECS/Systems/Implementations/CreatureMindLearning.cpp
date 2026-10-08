/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include <cmath>

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <LHVM.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "Camera/Camera.h"
#include "Creature/CreatureDecisionTree.h"
#include "Creature/CreatureDesires.h"
#include "Creature/CreatureLayers.h"
#include "Creature/CreatureLearning.h"
#include "Creature/CreatureMind.h"
#include "Creature/CreatureMindModel.h"
#include "Creature/CreatureMindTables.h"
#include "Creature/CreaturePlanActions.h"
#include "Creature/CreaturePlanner.h"
#include "Creature/CreatureWatching.h"
#include "CreatureMindSystem.h"
#include "CreatureMindSystemDetail.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/Ball.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureObjectAction.h"
#include "ECS/Components/CreatureSpells.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/Spell.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AnimalSystemInterface.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/CreatureObjectActionSystemInterface.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "MagicLiving.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems::mind_detail;
using creature_desires::Desire;
using creature_mind_model::TreeKind;
using creature_plan_actions::Target;

namespace
{
constexpr float k_TurnSeconds = std::chrono::duration<float>(TimeSystemInterface::k_TurnDuration).count();
constexpr float k_TurnsPerSecond = 1.0f / k_TurnSeconds;

/// The weakest plan worth carrying out instead of what the creature does with nothing better to do: a desire of 0.3
/// acted on with nothing learnt. The game weighs its idle activities through its planner too; this stands in for that.
constexpr float k_MinPlanPriority = 15.0f;
/// Feedback teaches lessons only once the creature has grown up this far; before, it only stirs its feelings
constexpr uint32_t k_MinLessonPhase = 2;
/// A stroke holds back sadness this long
constexpr float k_StrokeSadnessSeconds = 90.0f;
/// The source of the desire to copy the player, and how much a stroke while copying adds to it
constexpr uint32_t k_FollowPlayerSource = 54;
constexpr float k_MimicStrokeBoost = 0.3f;
/// Forgiven for running away, the threshold of its wish to run raises this much
constexpr float k_RunAwayThresholdStep = 0.1f;
/// A thing to use is learnt about at most this strongly for poo
constexpr float k_PooUseFeedback = 0.5f;
/// A creature watches skills, miracles and the player's deeds this far away at most
constexpr float k_ViewDistance = 150.0f;
/// Copying the player moves on a step once a second
constexpr uint32_t k_MimicStepTurns = 10;
/// Things nobody owns, and things of no player
constexpr uint32_t k_Neutral = 1;
constexpr uint32_t k_NoPlayer = 7;

/// The land's script that curses the player's creature, and how tall a creature of size 1 is
constexpr std::string_view k_CurseScript = "CreatureCurse";
constexpr float k_HeightOfSizeOne = 15.0f;
/// A villager hurt to this share of its life or less can be healed by a creature
constexpr float k_HurtEnoughToHeal = 0.7f;

/// The actions that cast a power-up of a miracle, which only a creature grown up enough tries
bool IsPowerUpCast(std::string_view action)
{
	return action.ends_with("PU1") || action.ends_with("PU2");
}

/// While doing these, which it chose with nothing better to do, the creature changes to any plan good enough
bool Interruptible(creature_mind::Activity activity)
{
	using enum creature_mind::Activity;
	return activity == None || activity == BeIdle || activity == Sit || activity == HangAround;
}

/// The action of the game's table an idle activity counts as, for feedback to be credited to
std::string_view ActionForActivity(creature_mind::Activity activity)
{
	using enum creature_mind::Activity;
	switch (activity)
	{
	case Eat:
		return "EatAfterExamining";
	case Examine:
		return "ExamineByPickingUp";
	case PlayWithObject:
		return "PracticeThrow";
	case Hurl:
		return "Hurl";
	case Sleep:
		return "SleepOnTheSpot";
	case Poo:
		return "Poo";
	case Puke:
		return "Puke";
	case Drink:
		return "DrinkFromTheSea";
	case Sit:
		return "SitDown";
	case BeIdle:
		return "BeIdle";
	case HangAround:
		return "HangAroundAtHome";
	case ShowDesire:
		return "CommunicateState";
	default:
		return {};
	}
}

std::optional<glm::vec2> PointOf(const ecs::Registry& registry, entt::entity entity)
{
	if (!registry.Valid(entity))
	{
		return std::nullopt;
	}
	const auto* transform = registry.TryGet<const Transform>(entity);
	if (transform == nullptr)
	{
		return std::nullopt;
	}
	return glm::vec2(transform->position.x, transform->position.z);
}

uint32_t PlayerNumberOf(PlayerNames owner)
{
	return static_cast<uint32_t>(owner);
}

/// Whether a thing is the kind an action is done to
bool Accepts(const ecs::Registry& registry, Target target, entt::entity entity, entt::entity self)
{
	if (entity == self || !registry.Valid(entity))
	{
		return false;
	}
	const auto* hands =
	    Locator::creatureObjectActionSystem::has_value() ? &Locator::creatureObjectActionSystem::value() : nullptr;
	const bool heldByOther = [&] {
		const auto* held = registry.TryGet<const HeldByCreature>(entity);
		return held != nullptr && held->creature != self;
	}();
	switch (target)
	{
	case Target::None:
		return false;
	case Target::Food:
		return !heldByOther && FoodValueOf(entity).has_value();
	case Target::LiveFood:
		return !heldByOther && registry.AllOf<Villager>(entity) && FoodValueOf(entity).has_value();
	case Target::Pickable:
		return !heldByOther && registry.AnyOf<MobileObject, Ball>(entity) && hands != nullptr && hands->CanPickUp(entity);
	case Target::Destroyable:
		return hands != nullptr && hands->CanDestroy(entity);
	case Target::Tree:
		return registry.AllOf<Tree>(entity) && hands != nullptr && hands->CanDestroy(entity);
	case Target::Villager:
		return registry.AllOf<Villager>(entity);
	case Target::HurtVillager:
	{
		// Hurt to seven tenths of its life or less
		const auto life = registry.AllOf<Villager>(entity) ? magic_living::LifeOf(entity) : std::nullopt;
		return life.has_value() && *life <= k_HurtEnoughToHeal;
	}
	case Target::Creature:
		return registry.AllOf<Creature>(entity);
	case Target::Living:
		return registry.AllOf<Villager>(entity) || registry.AllOf<Creature>(entity);
	case Target::Frightening:
		return registry.AllOf<Creature>(entity) || registry.AllOf<ecs::components::Spell>(entity) ||
		       (registry.AllOf<Animal>(entity) && Locator::animalSystem::has_value() &&
		        Locator::animalSystem::value().IsFrighteningToCreature(entity));
	case Target::Anything:
		return true;
	}
	return false;
}

struct Found
{
	entt::entity entity;
	glm::vec2 point;
	float distance;
};

/// The things of a kind about a point, nearest first
std::vector<Found> Gather(ecs::Registry& registry, Target target, entt::entity self, glm::vec2 from)
{
	std::vector<Found> found;
	if (target == Target::None)
	{
		return found;
	}
	const auto consider = [&](entt::entity entity, const Transform& at) {
		const glm::vec2 point {at.position.x, at.position.z};
		const auto distance = glm::distance(point, from);
		if (distance <= creature_planner::k_MaxGoalDistance && Accepts(registry, target, entity, self))
		{
			found.push_back({entity, point, distance});
		}
	};
	registry.Each<const Villager, const Transform>(
	    [&](entt::entity entity, const Villager&, const Transform& at) { consider(entity, at); });
	registry.Each<const Creature, const Transform>(
	    [&](entt::entity entity, const Creature&, const Transform& at) { consider(entity, at); });
	registry.Each<const MobileObject, const Transform>(
	    [&](entt::entity entity, const MobileObject&, const Transform& at) { consider(entity, at); });
	// The animals and miracles that frighten it are only ever run from
	if (target == Target::Frightening)
	{
		registry.Each<const Animal, const Transform>(
		    [&](entt::entity entity, const Animal&, const Transform& at) { consider(entity, at); });
		registry.Each<const ecs::components::Spell>([&](entt::entity entity, const ecs::components::Spell& spell) {
			consider(entity, Transform {.position = spell.position, .rotation = glm::mat3(1.0f), .scale = glm::vec3(1.0f)});
		});
	}
	// Pots and piles of food are only ever eaten
	if (target == Target::Food)
	{
		registry.Each<const Pot, const Transform>(
		    [&](entt::entity entity, const Pot&, const Transform& at) { consider(entity, at); });
	}
	if (target == Target::Destroyable || target == Target::Tree || target == Target::Anything)
	{
		registry.Each<const Tree, const Transform>(
		    [&](entt::entity entity, const Tree&, const Transform& at) { consider(entity, at); });
	}
	if (target == Target::Destroyable || target == Target::Anything)
	{
		registry.Each<const Abode, const Transform>(
		    [&](entt::entity entity, const Abode&, const Transform& at) { consider(entity, at); });
	}
	std::ranges::sort(found, {}, &Found::distance);
	return found;
}

float Usefulness(const creature_mind_model::Learnt& learnt, Desire desire, const std::optional<creature_tree::Belief>& belief)
{
	if (!belief.has_value())
	{
		return creature_planner::k_DefaultUsefulness;
	}
	const auto& tree = learnt.trees.at(static_cast<size_t>(TreeKind::ActOn)).at(static_cast<size_t>(desire));
	return creature_tree::Usefulness(creature_tree::Evaluate(tree, *belief));
}

std::string ObjectName(const std::optional<creature_tree::Belief>& belief)
{
	return belief.has_value() ? fmt::format("{}s", creature_tree::BeliefName(belief->type)) : "that";
}
} // namespace

/// What a creature knows of a thing: its kind and attributes, as far as the game's world here tells them
std::optional<creature_tree::Belief> mind_detail::BeliefOf(const ecs::Registry& registry, entt::entity entity,
                                                           entt::entity self)
{
	using creature_tree::Attribute;
	namespace types = creature_tree::belief_types;
	if (!registry.Valid(entity))
	{
		return std::nullopt;
	}
	const auto* me = registry.TryGet<const Creature>(self);
	creature_tree::Belief belief;
	const auto common = [&belief](uint32_t type, uint32_t allegiance, uint32_t origin, uint32_t animate, uint32_t player) {
		belief.type = type;
		belief.Set(Attribute::Allegiance, allegiance);
		belief.Set(Attribute::Origin, origin);
		belief.Set(Attribute::Animate, animate);
		belief.Set(Attribute::PlayerNumber, player);
		belief.Set(Attribute::HarderThanMe, 0);
		belief.Set(Attribute::CreatureType, 0);
		belief.Set(Attribute::Type, type);
	};
	if (const auto* villager = registry.TryGet<const Villager>(entity))
	{
		common(types::k_Villager, k_Neutral, 0, 1, k_NoPlayer);
		belief.Set(Attribute::Sex, static_cast<uint32_t>(villager->sex));
		belief.Set(Attribute::VillagerJob, static_cast<uint32_t>(villager->number));
		belief.Set(Attribute::Life, villager->health > 0 ? 1 : 0);
		belief.Set(Attribute::OnFire,
		           Locator::fireSystem::has_value() && Locator::fireSystem::value().IsOnFire(entity) ? 1 : 0);
		belief.Set(Attribute::Tribe, static_cast<uint32_t>(villager->tribe));
		return belief;
	}
	if (const auto* creature = registry.TryGet<const Creature>(entity))
	{
		const bool mine = me != nullptr && me->owner == creature->owner;
		common(types::k_Creature, mine ? 0 : 2, 0, 1, PlayerNumberOf(creature->owner));
		belief.Set(Attribute::HarderThanMe,
		           me != nullptr && creature->strength * creature->size > me->strength * me->size ? 1 : 0);
		belief.Set(Attribute::CreatureType, static_cast<uint32_t>(creature::InfoRow(creature->species)));
		belief.Set(Attribute::Height, static_cast<uint32_t>(std::lround(creature->size * 3.0f)));
		belief.Set(Attribute::SpellKnowledge, 0);
		belief.Set(Attribute::Carrying, registry.AllOf<CreatureHeldObject>(entity) ? 1 : 0);
		uint32_t dominant = 0;
		if (const auto* mind = registry.TryGet<const CreatureMindState>(entity); mind != nullptr && mind->desires.has_value())
		{
			float strongest = 0.0f;
			for (size_t d = 0; d < creature_desires::k_DesireCount; ++d)
			{
				const auto& state = mind->desires->desires.at(d);
				if (state.activated && state.value > strongest)
				{
					strongest = state.value;
					dominant = static_cast<uint32_t>(d);
				}
			}
		}
		belief.Set(Attribute::DominantDesire, dominant);
		return belief;
	}
	if (const auto* abode = registry.TryGet<const Abode>(entity))
	{
		common(types::k_Abode, k_Neutral, 1, 0, k_NoPlayer);
		belief.Set(Attribute::AbodeType, static_cast<uint32_t>(abode->type));
		belief.Set(Attribute::Life, 1);
		belief.Set(Attribute::OnFire,
		           Locator::fireSystem::has_value() && Locator::fireSystem::value().IsOnFire(entity) ? 1 : 0);
		belief.Set(Attribute::AbodeBeingBuilt, 0);
		return belief;
	}
	if (registry.AllOf<Tree>(entity))
	{
		common(types::k_Tree, k_Neutral, 0, 0, k_NoPlayer);
		return belief;
	}
	if (registry.AllOf<Temple>(entity))
	{
		common(types::k_Citadel, 0, 1, 0, me != nullptr ? PlayerNumberOf(me->owner) : 0);
		return belief;
	}
	if (registry.AllOf<Feature>(entity))
	{
		common(types::k_Feature, k_Neutral, 0, 0, k_NoPlayer);
		return belief;
	}
	if (registry.AnyOf<MobileObject, Ball, Pot>(entity))
	{
		common(types::k_Other, k_Neutral, 1, 0, k_NoPlayer);
		return belief;
	}
	return std::nullopt;
}

float mind_detail::FoodUsefulness(const ecs::Registry& registry, const CreatureMindState& mind, entt::entity self,
                                  entt::entity food)
{
	if (!mind.learnt.has_value())
	{
		return creature_planner::k_DefaultUsefulness;
	}
	const auto belief = BeliefOf(registry, food, self);
	if (!belief.has_value())
	{
		return creature_planner::k_DefaultUsefulness;
	}
	const auto& tree = mind.learnt->trees.at(static_cast<size_t>(TreeKind::ActOn)).at(static_cast<size_t>(Desire::Hunger));
	return creature_tree::Usefulness(creature_tree::Evaluate(tree, *belief));
}

uint32_t CreatureMindSystem::Random(uint32_t range)
{
	return range == 0 ? 0u : std::uniform_int_distribution<uint32_t>(0, range - 1)(_random);
}

float CreatureMindSystem::Chance()
{
	return std::uniform_real_distribution<float>(0.0f, 1.0f)(_random);
}

const creature_mind_tables::Tables* CreatureMindSystem::GetTables()
{
	if (!_tables.has_value() && Locator::infoConstants::has_value())
	{
		_tables = creature_mind_tables::Build(Locator::infoConstants::value());
	}
	return _tables.has_value() ? &*_tables : nullptr;
}

void CreatureMindSystem::SetUpLearning(entt::entity creature, CreatureMindState& mind)
{
	const auto* tables = GetTables();
	const auto actionCount = tables != nullptr ? tables->actions.size() : 0;
	auto knowledge = tables != nullptr ? creature_watching::StartKnowledge(tables->skills, tables->miracles)
	                                   : creature_watching::Knowledge {};
	mind.learnt = creature_mind_model::Fresh(*mind.desires, actionCount, std::move(knowledge));
	if (mind.resourceChecked)
	{
		return;
	}
	mind.resourceChecked = true;
	// A creature made from a mind file takes up what the file holds
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* body = registry.TryGet<const Creature>(creature);
	if (body == nullptr || body->mind == 0 || !Locator::resources::has_value())
	{
		return;
	}
	const auto& minds = Locator::resources::value().GetCreatureMinds();
	if (minds.Contains(body->mind))
	{
		const auto handle = minds.Handle(body->mind);
		if (handle && handle->Loaded())
		{
			mind.pendingFile = std::make_shared<const creaturemind::MindFileData>(handle->data);
		}
	}
}

void CreatureMindSystem::TakeUpFile(entt::entity creature, CreatureMindState& mind)
{
	const auto file = std::move(mind.pendingFile);
	mind.pendingFile.reset();
	const auto* body = Locator::entitiesRegistry::value().TryGet<const Creature>(creature);
	if (body == nullptr || !mind.desires.has_value() || !mind.learnt.has_value())
	{
		return;
	}
	const auto fresh =
	    creature_desires::Create(SetupFor(body->species), [](float low, float high) { return 0.5f * (low + high); });
	const auto* tables = GetTables();
	auto knowledge = tables != nullptr ? creature_watching::StartKnowledge(tables->skills, tables->miracles)
	                                   : creature_watching::Knowledge {};
	const auto freshLearnt = creature_mind_model::Fresh(fresh, mind.learnt->opinions.size(), std::move(knowledge));
	auto loaded = creature_mind_model::FromFile(*file, fresh, freshLearnt);
	if (tables != nullptr)
	{
		creature_mind_model::RebuildTrees(loaded.learnt, tables->attributes);
	}
	Abandon(mind);
	mind.planner = {};
	mind.desires = std::move(loaded.desires);
	mind.learnt = std::move(loaded.learnt);
	mind.developmentPhase = std::min(loaded.developmentPhase, CreatureMindState::k_FullyGrownUp);
	// The file's desires are active as it says, for the stage it gives
	mind.desiresPhase = mind.developmentPhase;
	mind.attitudeToPlayer = loaded.attitudeToPlayer;
	SPDLOG_LOGGER_INFO(spdlog::get("game"), "Creature {} took up a mind file: version {}, stage {}",
	                   entt::to_integral(creature), file->version, mind.developmentPhase);
}

void CreatureMindSystem::Abandon(CreatureMindState& mind)
{
	if (mind.planActive && mind.learnt.has_value() && !mind.learnt->contexts.empty())
	{
		mind.learnt->contexts.back().running = false;
	}
	mind.planActive = false;
	mind.planner.current.reset();
}

void CreatureMindSystem::FollowAgenda(entt::entity creature, CreatureMindState& mind)
{
	if (!mind.learnt.has_value() || !mind.desires.has_value())
	{
		return;
	}
	const auto* tables = GetTables();
	auto& idle = mind.idle;
	if (mind.planActive && (idle.serial != mind.planSerial || idle.step >= idle.agenda.size()))
	{
		// The plan is over. Carried out to its end, the desire it served is less, unless one of its steps saw to that
		// already; cut short by something else (fainting, a fight, a more pressing plan) or given up, it is still wanted.
		const auto plan = *mind.planner.current;
		const bool carriedOut = idle.serial == mind.planSerial && !idle.gaveUp;
		if (carriedOut && !mind.satisfiedByEffect && tables != nullptr && plan.action < tables->actions.size())
		{
			Satisfied(creature, *mind.desires, tables->actions[plan.action].name);
		}
		Abandon(mind);
		mind.planner.best.at(static_cast<size_t>(plan.desire)).reset();
	}
	// What the idle mind started is over once its agenda runs out
	if (!mind.planActive && idle.serial == mind.agendaSeen && idle.step >= idle.agenda.size() && !mind.learnt->contexts.empty())
	{
		mind.learnt->contexts.back().running = false;
	}
	// The trainer judges what it did as soon as it is over
	if (mind.trainer.has_value() && !mind.learnt->contexts.empty())
	{
		const auto& last = mind.learnt->contexts.back();
		if (!last.running && !last.credited.has_value() && last.belief.has_value())
		{
			const bool reward = last.belief->type == *mind.trainer;
			mind.learnt->contexts.back().credited = reward ? 1.0f : -1.0f;
			ReceiveFeedback(creature, reward ? 1.0f : -1.0f);
			SPDLOG_LOGGER_INFO(spdlog::get("game"), "Creature {} {} for a {}", entt::to_integral(creature),
			                   reward ? "stroked" : "slapped", creature_tree::BeliefName(last.belief->type));
		}
	}
	if (idle.serial == mind.agendaSeen)
	{
		return;
	}
	mind.agendaSeen = idle.serial;
	mind.satisfiedByEffect = false;
	if (mind.planActive || tables == nullptr)
	{
		return;
	}
	// Something the idle mind started counts as its action, for feedback
	const auto action = creature_mind_tables::FindAction(*tables, ActionForActivity(idle.activity));
	if (!action.has_value() || tables->actions[*action].desire >= creature_desires::k_DesireCount)
	{
		return;
	}
	std::optional<uint32_t> object;
	for (const auto& step : idle.agenda)
	{
		if (step.order.object.has_value())
		{
			object = step.order.object;
			break;
		}
		if (step.object.has_value())
		{
			object = step.object;
			break;
		}
	}
	const auto& registry = Locator::entitiesRegistry::value();
	const auto& info = tables->actions[*action];
	creature_learning::Remember(
	    mind.learnt->contexts,
	    {
	        .action = *action,
	        .desire = static_cast<Desire>(info.desire),
	        .object = object,
	        .belief = object.has_value() ? BeliefOf(registry, static_cast<entt::entity>(*object), creature) : std::nullopt,
	        .learnable = info.learnable,
	        .windowSeconds = info.learningWindowSeconds,
	    });
	mind.learnt->turnsSinceDone.at(*action) = 0;
}

bool CreatureMindSystem::Adopt(entt::entity creature, CreatureMindState& mind, const creature_planner::Plan& plan,
                               const creature_plan_actions::Situation& situation)
{
	const auto* tables = GetTables();
	if (tables == nullptr || plan.action >= tables->actions.size() || !mind.learnt.has_value() || !mind.desires.has_value())
	{
		return false;
	}
	const auto& info = tables->actions[plan.action];
	const auto* executor = creature_plan_actions::For(info.name);
	if (executor == nullptr)
	{
		return false;
	}
	auto& registry = Locator::entitiesRegistry::value();
	glm::vec2 point {0.0f};
	if (plan.object.has_value())
	{
		const auto at = PointOf(registry, static_cast<entt::entity>(*plan.object));
		if (!at.has_value())
		{
			return false;
		}
		point = *at;
	}
	const auto cast = creature_plan_actions::IsCast(*executor) ? CastInfoFor(creature, plan.action) : std::nullopt;
	auto agenda = creature_plan_actions::Agenda(
	    *executor, plan.object, point, situation, [this](uint32_t range) { return Random(range); }, cast);
	if (!agenda.has_value())
	{
		return false;
	}
	// Whatever its hands were doing is given up for the plan
	if (Locator::creatureObjectActionSystem::has_value() &&
	    Locator::creatureObjectActionSystem::value().GetState(creature) == CreatureObjectActionSystemInterface::State::Busy)
	{
		Locator::creatureObjectActionSystem::value().Cancel(creature);
	}
	if (!Replan(creature, executor->activity, std::move(*agenda)))
	{
		return false;
	}
	auto& learnt = *mind.learnt;
	mind.planner.current = plan;
	mind.planActive = true;
	mind.planSerial = mind.idle.serial;
	mind.agendaSeen = mind.idle.serial;
	mind.satisfiedByEffect = false;
	creature_learning::ResetDrives((*mind.desires)[plan.desire]);
	creature_learning::SuppressOpposed(*mind.desires, plan.desire, tables->dependencies, k_TurnsPerSecond);
	learnt.turnsSinceDone.at(plan.action) = 0;
	creature_learning::Remember(learnt.contexts,
	                            {
	                                .action = plan.action,
	                                .desire = plan.desire,
	                                .object = plan.object,
	                                .belief = plan.object.has_value()
	                                              ? BeliefOf(registry, static_cast<entt::entity>(*plan.object), creature)
	                                              : std::nullopt,
	                                .learnable = info.learnable,
	                                .windowSeconds = info.learningWindowSeconds,
	                            });
	if (executor->build == creature_plan_actions::Build::ShowDesire)
	{
		mind.idle.showDesireSeconds = creature_mind::k_ShowDesireSeconds;
	}
	return true;
}

void CreatureMindSystem::PlanCreature(entt::entity creature, CreatureMindState& mind, bool everyDesire)
{
	const auto* tables = GetTables();
	auto& registry = Locator::entitiesRegistry::value();
	const auto* transform = registry.TryGet<const Transform>(creature);
	if (tables == nullptr || transform == nullptr || !mind.desires.has_value() || !mind.learnt.has_value() || mind.paused ||
	    mind.leash.obeying || creature_mind::IsUnconscious(mind.idle))
	{
		return;
	}
	if (mind.plannedTurn == mind.turn)
	{
		return;
	}
	mind.plannedTurn = mind.turn;
	const glm::vec2 position {transform->position.x, transform->position.z};
	const auto& desires = *mind.desires;
	const auto& learnt = *mind.learnt;

	creature_plan_actions::Situation situation;
	if (Locator::camera::has_value())
	{
		const auto eye = Locator::camera::value().GetOrigin();
		situation.camera = glm::vec2(eye.x, eye.z);
	}
	if (mind.idle.showDesireSeconds <= 0.0f)
	{
		if (const auto strongest = creature_desires::StrongestShowable(desires, creature_mind::k_MinDesireShown))
		{
			situation.showDesireAnimation = creature_desires::EmoteFor(*strongest);
		}
	}
	bool waterLooked = false;
	bool hurlLooked = false;
	const auto prepare = [&](const creature_plan_actions::Executor& executor) {
		if (executor.build == creature_plan_actions::Build::Drink && !waterLooked)
		{
			waterLooked = true;
			situation.water = NearestWater(position);
		}
		if (executor.build == creature_plan_actions::Build::Hurl && !hurlLooked)
		{
			hurlLooked = true;
			situation.hurlTarget = NearestHurlTarget(registry, position);
		}
	};

	std::array<float, creature_desires::k_DesireCount> strengths {};
	std::array<bool, creature_desires::k_DesireCount> eligible {};
	for (size_t d = 0; d < creature_desires::k_DesireCount; ++d)
	{
		const auto& state = desires.desires.at(d);
		auto strength = state.activated && state.suppressedTurns == 0 ? state.value : 0.0f;
		if (mind.leash.forcedDesire == static_cast<Desire>(d))
		{
			strength = std::max(strength, mind.leash.forcedValue);
		}
		strengths.at(d) = strength;
		eligible.at(d) = strength > 0.0f && std::ranges::any_of(tables->desireActions.at(d), [tables](uint32_t action) {
			                 return creature_plan_actions::For(tables->actions.at(action).name) != nullptr;
		                 });
	}

	// Something shown on the leash is the only goal there is this turn, for every desire
	std::optional<entt::entity> leashTarget;
	if (!mind.leash.actOn.empty())
	{
		leashTarget = static_cast<entt::entity>(mind.leash.actOn.back());
		mind.leash.actOn.clear();
	}
	auto goals =
	    leashTarget.has_value() || everyDesire ? std::vector<Desire> {} : creature_planner::NextGoals(mind.planner, eligible);
	if (leashTarget.has_value() || everyDesire)
	{
		for (size_t d = 0; d < eligible.size(); ++d)
		{
			if (eligible.at(d))
			{
				goals.push_back(static_cast<Desire>(d));
			}
			else
			{
				mind.planner.best.at(d).reset();
			}
		}
	}

	for (const auto desire : goals)
	{
		const auto d = static_cast<size_t>(desire);
		std::optional<creature_planner::Plan> best;
		// Actions are weighed in groups by the kind of thing they are done to, so each has a goal it can be done to
		std::array<std::vector<creature_planner::ActionCandidate>, static_cast<size_t>(Target::Anything) + 1> groups {};
		for (const auto action : tables->desireActions.at(d))
		{
			const auto* executor = creature_plan_actions::For(tables->actions.at(action).name);
			if (executor == nullptr)
			{
				continue;
			}
			prepare(*executor);
			groups.at(static_cast<size_t>(executor->target))
			    .push_back({
			        .action = action,
			        .needsObject = executor->target != Target::None,
			        .opinion = learnt.opinions.at(action),
			        .turnsSinceDone = learnt.turnsSinceDone.at(action),
			        .servesFirstDesire = tables->actions.at(action).desire == 0,
			        .disabled = !creature_plan_actions::Possible(*executor, situation) ||
			                    (creature_plan_actions::IsCast(*executor) &&
			                     !MayCast(creature, mind, action, IsPowerUpCast(tables->actions.at(action).name))),
			    });
		}
		for (size_t g = 0; g < groups.size(); ++g)
		{
			if (groups.at(g).empty())
			{
				continue;
			}
			const auto target = static_cast<Target>(g);
			std::vector<creature_planner::ObjectCandidate> objects;
			if (target != Target::None)
			{
				if (leashTarget.has_value())
				{
					if (Accepts(registry, target, *leashTarget, creature))
					{
						const auto at = PointOf(registry, *leashTarget).value_or(position);
						objects.push_back(
						    {.id = entt::to_integral(*leashTarget),
						     .distance = glm::distance(at, position),
						     .usefulness = Usefulness(learnt, desire, BeliefOf(registry, *leashTarget, creature))});
					}
				}
				else
				{
					for (const auto& found : Gather(registry, target, creature, position))
					{
						const auto* held = registry.TryGet<const HeldByCreature>(found.entity);
						objects.push_back({
						    .id = entt::to_integral(found.entity),
						    .distance = found.distance,
						    .held = held != nullptr && held->creature == creature,
						    .usefulness = Usefulness(learnt, desire, BeliefOf(registry, found.entity, creature)),
						});
					}
				}
			}
			const auto plan = creature_planner::PlanDesire(desire, strengths.at(d), objects, tables->rules.at(d).distanceWeight,
			                                               groups.at(g));
			if (plan.has_value() && (!best.has_value() || plan->priority > best->priority))
			{
				best = plan;
			}
		}
		mind.planner.best.at(d) = best;
	}

	// Doing something it chose for itself that isn't idling, it carries on
	if (!mind.planActive && !Interruptible(mind.idle.activity) && mind.idle.step < mind.idle.agenda.size())
	{
		return;
	}
	const auto choice = creature_planner::Choose(mind.planner, k_MinPlanPriority);
	if (choice.has_value() && !Adopt(creature, mind, *choice, situation))
	{
		mind.planner.best.at(static_cast<size_t>(choice->desire)).reset();
	}
}

void CreatureMindSystem::PlanTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<entt::entity> creatures;
	registry.Each<const Creature, CreatureMindState>(
	    [&creatures](entt::entity entity, const Creature&, CreatureMindState&) { creatures.push_back(entity); });
	for (const auto entity : creatures)
	{
		if (auto* mind = registry.TryGet<CreatureMindState>(entity))
		{
			PlanCreature(entity, *mind);
		}
	}
}

void CreatureMindSystem::LearnFromFeedback(entt::entity creature, CreatureMindState& mind, float feedback)
{
	namespace sources = creature_desires::sources;
	if (!mind.desires.has_value() || !mind.learnt.has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	const auto* tables = GetTables();
	auto& desires = *mind.desires;
	auto& learnt = *mind.learnt;
	const auto contextIndex = creature_learning::BestContext(learnt.contexts);
	auto* context = contextIndex.has_value() ? &learnt.contexts.at(*contextIndex) : nullptr;
	const auto* info = context != nullptr && tables != nullptr && context->action < tables->actions.size()
	                       ? &tables->actions[context->action]
	                       : nullptr;

	// Stroked for holding something while doing an action that allows it, it learns to eat from the hand
	if (feedback > 0.0f && mind.developmentPhase > 0 && info != nullptr && info->eatWhenStroked &&
	    Locator::creatureObjectActionSystem::has_value())
	{
		if (const auto held = Locator::creatureObjectActionSystem::value().GetHeld(creature);
		    held.has_value() && FoodValueOf(*held).has_value())
		{
			creature_learning::MakeFullyDominant(desires, Desire::Hunger);
			if (const auto belief = BeliefOf(registry, *held, creature); belief.has_value() && tables != nullptr)
			{
				creature_mind_model::Learn(learnt, TreeKind::Use, Desire::Hunger, {.belief = *belief, .feedback = 1.0f},
				                           tables->attributes.at(static_cast<size_t>(Desire::Hunger)));
			}
			creature_mind_model::Think(learnt, "I've learnt to eat what I'm given");
			Replan(creature, creature_mind::Activity::Eat, creature_mind::EatHeld());
			return;
		}
	}
	// Stroked for running away from the player, it is forgiven and wants to run away less
	if (feedback > 0.0f && context != nullptr && context->desire == Desire::RunAwayFromPlayer)
	{
		auto& runAway = desires[Desire::RunAwayFromPlayer];
		creature_learning::MakeLeastDominant(desires, Desire::RunAwayFromPlayer, creature_learning::k_LeastDominantFactor);
		for (auto& source : runAway.sources)
		{
			if (source.type == sources::k_RunAwayFromPlayer)
			{
				source.value = 0.0f;
				source.threshold = std::min(source.threshold + k_RunAwayThresholdStep, creature_learning::k_MaxThreshold);
			}
		}
		Abandon(mind);
		return;
	}

	const auto strength = creature_learning::Strengthened(feedback);
	if (feedback > 0.0f)
	{
		if (learnt.mimicry.has_value() && tables != nullptr)
		{
			creature_watching::StrokedWhileMimicking(learnt.mimicry, tables->mimics);
			creature_desires::ChangeSource(desires, k_FollowPlayerSource, k_MimicStrokeBoost);
		}
		creature_desires::Suppress(desires, Desire::Sadness, k_StrokeSadnessSeconds, k_TurnsPerSecond);
	}

	if (context == nullptr || info == nullptr || mind.developmentPhase < k_MinLessonPhase)
	{
		// Nothing to learn from yet: stroking makes it playful, showy and kind; slapping angry and fearful
		const auto amount = 0.5f * std::abs(feedback);
		constexpr std::array k_Stroked {sources::k_PlayFromWatchingPlayer, sources::k_ManifestState,
		                                sources::k_CompassionFromWatchingPlayer};
		constexpr std::array k_Slapped {sources::k_AngerFromDamage, sources::k_FearFromDamage};
		const auto pushed = feedback > 0.0f ? std::span<const uint32_t>(k_Stroked) : std::span<const uint32_t>(k_Slapped);
		for (const auto type : pushed)
		{
			creature_desires::ChangeSource(desires, type, amount);
		}
		return;
	}

	const auto desire = context->desire;
	const auto d = static_cast<size_t>(desire);
	const auto& texts = tables->texts.at(d);
	context->credited = strength;
	if (tables->rules.at(d).learnable)
	{
		creature_learning::LearnDesireLesson(desires, desire, creature_learning::DominantSource(desires[desire]), strength,
		                                     tables->rules, tables->dependencies);
		creature_mind_model::Think(learnt, creature_learning::DesireLessonText(strength, texts.desire));
		if (context->belief.has_value())
		{
			for (const auto& [other, share] : creature_learning::Spread(desire, strength, tables->dependencies))
			{
				creature_mind_model::Learn(learnt, TreeKind::ActOn, other,
				                           {.belief = *context->belief, .feedback = std::clamp(share, -1.0f, 1.0f)},
				                           tables->attributes.at(static_cast<size_t>(other)));
			}
			creature_mind_model::Think(
			    learnt, creature_learning::ObjectLessonText(strength, ObjectName(context->belief), texts.trying));
		}
		if (context->usedBelief.has_value() && context->used != context->object)
		{
			const auto used = desire == Desire::Poo ? std::clamp(strength, -k_PooUseFeedback, k_PooUseFeedback) : strength;
			for (const auto& [other, share] : creature_learning::Spread(desire, used, tables->dependencies))
			{
				creature_mind_model::Learn(learnt, TreeKind::Use, other,
				                           {.belief = *context->usedBelief, .feedback = std::clamp(share, -1.0f, 1.0f)},
				                           tables->attributes.at(static_cast<size_t>(other)));
			}
		}
		auto& opinion = learnt.opinions.at(context->action);
		opinion = creature_learning::OpinionAfter(opinion, strength);
		creature_mind_model::Think(
		    learnt, creature_learning::ActionLessonText(strength, info->name, texts.trying, ObjectName(context->belief)));
	}

	if (feedback <= 0.0f)
	{
		// Slapped for it, it gives it up and wants it least of all
		if (context->running)
		{
			creature_learning::MakeLeastDominant(desires, desire, creature_learning::k_LeastDominantFactor);
			for (auto& state : desires.desires)
			{
				state.suppressedTurns = 0;
			}
			Abandon(mind);
		}
		return;
	}
	// Stroked for something it did to a thing, it does it again to something like it
	const auto* executor = creature_plan_actions::For(info->name);
	if (context->running || !context->belief.has_value() || executor == nullptr || executor->target == Target::None)
	{
		return;
	}
	const auto* transform = registry.TryGet<const Transform>(creature);
	if (transform == nullptr)
	{
		return;
	}
	const glm::vec2 position {transform->position.x, transform->position.z};
	for (const auto& found : Gather(registry, executor->target, creature, position))
	{
		const auto belief = BeliefOf(registry, found.entity, creature);
		if (belief.has_value() && belief->type == context->belief->type)
		{
			const creature_planner::Plan again {
			    .desire = desire,
			    .action = context->action,
			    .object = entt::to_integral(found.entity),
			    // As pressing as the planner would find it, so that something more pressing can still take over
			    .priority = creature_planner::Priority(desires[desire].value, Usefulness(learnt, desire, belief),
			                                           creature_planner::ActionPriority(learnt.opinions.at(context->action)),
			                                           creature_planner::k_DefaultUsefulness, std::nullopt),
			};
			creature_plan_actions::Situation situation;
			if (Locator::camera::has_value())
			{
				const auto eye = Locator::camera::value().GetOrigin();
				situation.camera = glm::vec2(eye.x, eye.z);
			}
			situation.hurlTarget = NearestHurlTarget(registry, position);
			Adopt(creature, mind, again, situation);
			return;
		}
	}
}

void CreatureMindSystem::LearnTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* tables = GetTables();
	if (tables == nullptr)
	{
		return;
	}
	std::vector<entt::entity> creatures;
	registry.Each<const Creature, CreatureMindState>(
	    [&creatures](entt::entity entity, const Creature&, CreatureMindState&) { creatures.push_back(entity); });
	for (const auto creature : creatures)
	{
		auto* mind = registry.TryGet<CreatureMindState>(creature);
		if (mind == nullptr || !mind->learnt.has_value() || !mind->desires.has_value())
		{
			continue;
		}
		auto& learnt = *mind->learnt;
		// What the player showed it on a leash: anger or compassion towards such things
		for (const auto& shown : mind->leash.shown)
		{
			const auto belief = BeliefOf(registry, static_cast<entt::entity>(shown.object), creature);
			if (!belief.has_value())
			{
				continue;
			}
			for (const auto& lesson : shown.lessons)
			{
				creature_mind_model::Learn(learnt, TreeKind::ActOn, lesson.desire,
				                           {.belief = *belief, .feedback = lesson.change},
				                           tables->attributes.at(static_cast<size_t>(lesson.desire)));
				creature_mind_model::Think(
				    learnt, creature_learning::ObjectLessonText(lesson.change, ObjectName(belief),
				                                                tables->texts.at(static_cast<size_t>(lesson.desire)).trying));
			}
		}
		mind->leash.shown.clear();
		// Leashed to another creature, it comes to find it nicer or nastier, and learns about creatures like it
		for (const auto& change : mind->leash.attitudes)
		{
			auto& attitude = creature_learning::AttitudeTo(learnt.creatures, change.creature);
			const auto lessons = creature_learning::ChangeHowNice(attitude, change.change);
			if (const auto belief = BeliefOf(registry, static_cast<entt::entity>(change.creature), creature))
			{
				for (const auto& [desire, amount] : lessons)
				{
					creature_mind_model::Learn(learnt, TreeKind::ActOn, desire, {.belief = *belief, .feedback = amount},
					                           tables->attributes.at(static_cast<size_t>(desire)));
				}
			}
		}
		mind->leash.attitudes.clear();

		// Copying the player moves on a stage at a time
		if (!learnt.mimicry.has_value() || mind->turn % k_MimicStepTurns != 0)
		{
			continue;
		}
		const auto before = learnt.mimicry->stage;
		creature_watching::StepMimicry(learnt.mimicry, tables->mimics, [this](uint32_t range) { return Random(range); });
		if (!learnt.mimicry.has_value() || learnt.mimicry->stage == before)
		{
			continue;
		}
		const auto& rule = tables->mimics.at(learnt.mimicry->rule);
		if (learnt.mimicry->stage == creature_watching::MimicStage::CopyAction)
		{
			// It does what the player did, to the same thing or one like it
			const auto* transform = registry.TryGet<const Transform>(creature);
			for (const auto action : rule.copyActions)
			{
				const auto* executor = creature_plan_actions::For(tables->actions.at(action).name);
				if (executor == nullptr || transform == nullptr)
				{
					continue;
				}
				std::optional<uint32_t> object;
				if (executor->target != Target::None)
				{
					if (learnt.mimicry->object.has_value() &&
					    Accepts(registry, executor->target, static_cast<entt::entity>(*learnt.mimicry->object), creature))
					{
						object = learnt.mimicry->object;
					}
					else if (const auto found = Gather(registry, executor->target, creature,
					                                   glm::vec2(transform->position.x, transform->position.z));
					         !found.empty())
					{
						object = entt::to_integral(found.front().entity);
					}
					if (!object.has_value())
					{
						continue;
					}
				}
				const auto desire = static_cast<Desire>(
				    std::min<uint32_t>(tables->actions.at(action).desire, creature_desires::k_DesireCount - 1));
				creature_plan_actions::Situation situation;
				situation.hurlTarget = NearestHurlTarget(registry, glm::vec2(transform->position.x, transform->position.z));
				if (Adopt(creature, *mind,
				          {.desire = desire,
				           .action = action,
				           .object = object,
				           // Copying counts as pressing as a desire at its strongest, with nothing learnt
				           .priority = creature_planner::Priority(1.0f, std::nullopt,
				                                                  creature_planner::ActionPriority(learnt.opinions.at(action)),
				                                                  creature_planner::k_DefaultUsefulness, std::nullopt)},
				          situation))
				{
					creature_mind_model::Think(learnt, fmt::format("I'm copying you: {}", tables->actions.at(action).name));
					break;
				}
			}
		}
		else if (learnt.mimicry->stage == creature_watching::MimicStage::CopyDesire &&
		         rule.desire < creature_desires::k_DesireCount)
		{
			// It comes to want what the player seemed to want
			creature_learning::MakeFullyDominant(*mind->desires, static_cast<Desire>(rule.desire));
			creature_mind_model::Think(learnt, fmt::format("I want what you wanted: {}", tables->texts.at(rule.desire).desire));
		}
	}
}

void CreatureMindSystem::LoadMind(entt::entity creature, std::shared_ptr<const creaturemind::MindFileData> mind)
{
	if (auto* state = Locator::entitiesRegistry::value().TryGet<CreatureMindState>(creature))
	{
		state->pendingFile = std::move(mind);
	}
}

std::optional<creaturemind::MindFileData> CreatureMindSystem::SaveMind(entt::entity creature) const
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* mind = registry.TryGet<const CreatureMindState>(creature);
	const auto* body = registry.TryGet<const Creature>(creature);
	if (mind == nullptr || body == nullptr || !mind->desires.has_value() || !mind->learnt.has_value())
	{
		return std::nullopt;
	}
	auto file = creature_mind_model::ToFile(*mind->desires, *mind->learnt, mind->developmentPhase, mind->attitudeToPlayer,
	                                        static_cast<uint32_t>(creature::InfoRow(body->species)));
	// It is saved as itself: as it was before any spell on it changed it
	const creature_spells::SavedBody now {.size = body->size, .strength = body->strength, .alignment = body->alignment};
	const auto* spells = registry.TryGet<const CreatureSpells>(creature);
	auto saved = spells != nullptr ? creature_spells::ValuesToSave(spells->spells, now) : now;
	// While the land's curse on the player's creature runs, it is saved as the curse found it
	if (body->owner == PlayerNames::PLAYER_ONE && Locator::vm::has_value())
	{
		const auto& vm = Locator::vm::value();
		const bool cursed =
		    std::ranges::any_of(vm.GetTasks(), [](const auto& entry) { return entry.second.name == k_CurseScript; });
		if (cursed)
		{
			const auto global = [&vm](std::string_view name) {
				const auto& variables = vm.GetVariables();
				const auto found = std::ranges::find(variables, name, &lhvm::VMVar::name);
				return found != variables.end() ? found->value.floatVal : 0.0f;
			};
			saved.size = global("OriginalSizeOfMyCreature") / k_HeightOfSizeOne;
			saved.strength = global("OriginalStrengthOfMyCreature");
			saved.alignment = global("OriginalAlignmentOfMyCreature");
		}
	}
	file.physique.size = saved.size;
	file.physique.strength = saved.strength;
	file.alignment = saved.alignment;
	return file;
}

void CreatureMindSystem::ClearLearning(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* mind = registry.TryGet<CreatureMindState>(creature);
	const auto* body = registry.TryGet<const Creature>(creature);
	if (mind == nullptr || body == nullptr)
	{
		return;
	}
	const auto name = mind->learnt.has_value() ? mind->learnt->name : std::u16string {};
	Abandon(*mind);
	mind->desires = creature_desires::Create(SetupFor(body->species), [this](float low, float high) {
		return high > low ? std::uniform_real_distribution<float>(low, high)(_random) : low;
	});
	mind->desiresPhase.reset();
	mind->planner = {};
	mind->pendingFile.reset();
	mind->resourceChecked = true;
	SetUpLearning(creature, *mind);
	mind->learnt->name = name;
}

void CreatureMindSystem::SeeSkill(const glm::vec3& point, size_t skill)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* tables = GetTables();
	if (tables == nullptr)
	{
		return;
	}
	registry.Each<const Creature, CreatureMindState, const Transform>(
	    [&](entt::entity, const Creature&, CreatureMindState& mind, const Transform& at) {
		    if (!mind.learnt.has_value() || glm::distance(at.position, point) > k_ViewDistance)
		    {
			    return;
		    }
		    const auto progress = creature_watching::SeeSkill(mind.learnt->knowledge, skill, tables->skills,
		                                                      mind.developmentPhase, mind.turn, k_TurnsPerSecond);
		    if (progress.learnt && !progress.ignored)
		    {
			    creature_mind_model::Think(*mind.learnt, fmt::format("I've learnt to {}", tables->skills.at(skill).name));
		    }
	    });
}

void CreatureMindSystem::SeeMiracle(const glm::vec3& point, size_t miracle)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* tables = GetTables();
	if (tables == nullptr)
	{
		return;
	}
	registry.Each<const Creature, CreatureMindState, const Transform>(
	    [&](entt::entity, const Creature& creature, CreatureMindState& mind, const Transform& at) {
		    // A mind stilled, as by the freeze spell, learns nothing
		    if (!mind.learnt.has_value() || mind.paused || glm::distance(at.position, point) > k_ViewDistance)
		    {
			    return;
		    }
		    const auto progress = creature_watching::SeeMiracle(
		        mind.learnt->knowledge, miracle, tables->miracles, mind.developmentPhase, mind.turn,
		        mind.leash.miracleSightingWeight, creature_mind_tables::MiracleMultiplier(creature::InfoRow(creature.species)));
		    if (progress.learnt && !progress.ignored)
		    {
			    creature_mind_model::Think(*mind.learnt,
			                               fmt::format("I've learnt the miracle {}", tables->miracles.at(miracle).name));
		    }
	    });
}

void CreatureMindSystem::PlayerDid(size_t deed, const glm::vec3& point, std::optional<entt::entity> object,
                                   std::optional<PlayerNames> player)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* tables = GetTables();
	if (tables == nullptr || deed >= tables->mimics.size())
	{
		return;
	}
	std::vector<entt::entity> noticed;
	registry.Each<const Creature, CreatureMindState, const Transform>(
	    [&](entt::entity entity, const Creature& creature, CreatureMindState& mind, const Transform& at) {
		    if (!mind.learnt.has_value() || (player.has_value() && (creature.owner != *player || !creature.leashable)))
		    {
			    return;
		    }
		    const creature_watching::MimicConditions conditions {
		        .phase = mind.developmentPhase,
		        .learningLeashInHand = mind.leash.learningInHand,
		        .reactionPriority = 0.0f,
		        .canSee = glm::distance(at.position, point) <= k_ViewDistance,
		    };
		    const auto target = object.has_value() ? std::optional(entt::to_integral(*object)) : std::nullopt;
		    if (creature_watching::StartMimicry(mind.learnt->mimicry, deed, tables->mimics, conditions, target,
		                                        [this] { return Chance(); }))
		    {
			    noticed.push_back(entity);
		    }
	    });
	// It notices: it turns to where the player did it and looks at it
	for (const auto entity : noticed)
	{
		if (auto* mind = registry.TryGet<CreatureMindState>(entity))
		{
			creature_mind_model::Think(*mind->learnt, fmt::format("I saw you: {}", tables->mimics.at(deed).name));
			Replan(entity, creature_mind::Activity::Planned, creature_mind::LookAt(glm::vec2(point.x, point.z), 2.0f));
		}
	}
}

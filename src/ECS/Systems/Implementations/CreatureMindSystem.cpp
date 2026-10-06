/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CreatureMindSystem.h"

#include <cmath>
#include <cstring>

#include <algorithm>
#include <array>
#include <chrono>
#include <iterator>
#include <limits>
#include <random>
#include <ranges>
#include <span>
#include <vector>

#include "3D/CreatureBody.h"
#include "3D/DayNightClock.h"
#include "3D/LandIslandInterface.h"
#include "3D/SkyInterface.h"
#include "Camera/Camera.h"
#include "Creature/CreatureDesires.h"
#include "Creature/CreatureFeedback.h"
#include "Creature/CreatureIdleMind.h"
#include "Creature/CreatureLayers.h"
#include "Creature/CreatureLook.h"
#include "Creature/CreaturePhysiology.h"
#include "Creature/CreatureRoute.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/CreatureObjectAction.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureAnimationSystemInterface.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/CreatureObjectActionSystemInterface.h"
#include "ECS/Systems/CreaturePhysiologySystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;
namespace animations = openblack::creature_layers::animations;
using creature_desires::Desire;

namespace
{
constexpr float k_TurnSeconds = std::chrono::duration<float>(TimeSystemInterface::k_TurnDuration).count();
constexpr float k_TurnsPerSecond = 1.0f / k_TurnSeconds;

/// A creature looks for food and water this far away at most
constexpr float k_FoodSearchDistance = 200.0f;
constexpr int32_t k_WaterSearchCells = 30;
/// Where it stands to drink is found this far at most from the water
constexpr float k_ShoreSearchDistance = 60.0f;
/// Food further than this from its mouth, for its size, has got away by the time it eats
constexpr float k_EatReach = 15.0f;
/// Having drunk it doesn't want water for a while, and having had a poo it doesn't want another for longer
constexpr float k_WaterSuppressSeconds = 20.0f;
constexpr float k_PooSuppressSeconds = 60.0f;
/// The rows of the game's creature action table that seeing to each need counts as
constexpr std::string_view k_EatAction = "EatAfterExamining";
constexpr std::string_view k_DrinkAction = "DrinkFromTheSea";
constexpr std::string_view k_PooAction = "Poo";
constexpr std::string_view k_PukeAction = "Puke";
constexpr std::string_view k_SleepAction = "SleepOnTheSpot";
constexpr std::string_view k_ExamineAction = "ExamineByPickingUp";
constexpr std::string_view k_ThrowAboutAction = "PracticeThrow";
constexpr std::string_view k_HurlAction = "Hurl";
/// A creature looks for things to pick up this far away at most, and for homes and trees to hurl them at
constexpr float k_ObjectSearchDistance = 80.0f;
constexpr float k_HurlSearchDistance = 120.0f;
/// The creature wants the player's attention more the longer it is alone, fully after a minute, and from lack of
/// anything to do with the player, fully after two
constexpr float k_LonelySeconds = 60.0f;
constexpr float k_UninterestedSeconds = 120.0f;
/// The desire to show how it is grows with all its desires added up, fully at this much
constexpr float k_ManifestSum = 3.0f;

/// Just woken up, the eyelids droop and blink slowly; otherwise they are as they start
constexpr float k_SleepyOpenness = -0.7f;
constexpr int32_t k_SleepyBlinkIntervalMs = 2500;

/// How high above their feet creatures look at other things
constexpr float k_VillagerHeadHeight = 1.8f;
constexpr float k_AbodeLookHeight = 4.0f;
constexpr float k_TreeLookHeight = 6.0f;
constexpr float k_CitadelLookHeight = 30.0f;

float Clamp01(float value)
{
	return std::clamp(value, 0.0f, 1.0f);
}

/// The 17 per species values of a creature table row
template <typename Row>
float PerSpecies(const Row& row, size_t species)
{
	static_assert(sizeof(Row) == 17 * sizeof(float));
	std::array<float, 17> values {};
	std::memcpy(values.data(), &row, sizeof(values));
	return values.at(std::min(species, values.size() - 1));
}

/// How a species' desires start, from the game's creature tables
std::array<creature_desires::DesireSetup, creature_desires::k_DesireCount> SetupFor(CreatureType species)
{
	std::array<creature_desires::DesireSetup, creature_desires::k_DesireCount> setup {};
	if (!Locator::infoConstants::has_value())
	{
		return setup;
	}
	const auto& info = Locator::infoConstants::value();
	const auto row = creature::InfoRow(species);
	for (size_t d = 0; d < setup.size(); ++d)
	{
		const auto& initial = info.creatureInitialDesire.at(d);
		auto& desire = setup.at(d);
		desire.max = initial.field0x3c;
		desire.decayMin = initial.field0x40;
		desire.decayMax = initial.field0x44;
		desire.increaseSeconds = PerSpecies(info.creatureDesireForType.at(d), row);
		const std::array<uint32_t, creature_desires::k_MaxSources> sourceTypes {
		    initial.field0x0,  initial.field0x4,  initial.field0x8,  initial.field0xc,
		    initial.field0x10, initial.field0x14, initial.field0x18, initial.field0x1c,
		};
		for (const auto type : sourceTypes)
		{
			if (type >= creature_desires::k_NoSource)
			{
				continue;
			}
			desire.sources.push_back({
			    .type = type,
			    .value = PerSpecies(info.creatureInitialSource1.at(type), row),
			    .threshold = PerSpecies(info.creatureInitialSource2.at(type), row),
			    .multiplier = info.desireSourceTable.at(type).field0x8,
			});
		}
	}
	return setup;
}

/// The desires each stage of growing up brings and takes away, from the game's tables
std::vector<creature_desires::PhaseDesires> Phases()
{
	std::vector<creature_desires::PhaseDesires> phases;
	if (!Locator::infoConstants::has_value())
	{
		return phases;
	}
	const auto valid = [](uint32_t desire) { return desire < creature_desires::k_DesireCount; };
	for (const auto& entry : Locator::infoConstants::value().creatureDevelopmentPhaseEntry)
	{
		auto& phase = phases.emplace_back();
		const std::array<uint32_t, 10> add {entry.field0x3c, entry.field0x40, entry.field0x44, entry.field0x48,
		                                    entry.field0x4c, entry.field0x50, entry.field0x54, entry.field0x58,
		                                    entry.field0x5c, entry.field0x60};
		const std::array<uint32_t, 4> remove {entry.field0x64, entry.field0x68, entry.field0x6c, entry.field0x70};
		for (const auto desire : add | std::views::filter(valid))
		{
			phase.add.push_back(static_cast<Desire>(desire));
		}
		for (const auto desire : remove | std::views::filter(valid))
		{
			phase.remove.push_back(static_cast<Desire>(desire));
		}
	}
	return phases;
}

/// The sources that follow the creature's state: its body's, and its mind's own
std::optional<float> ReadSource(uint32_t type, const creature_desires::Desires& desires, const CreatureMindState& mind,
                                const creature_physiology::Needs& needs, bool night)
{
	namespace sources = creature_desires::sources;
	if (const auto value = creature_physiology::SourceValue(type, needs, night))
	{
		return value;
	}
	switch (type)
	{
	case sources::k_AttentionFromLoneliness:
		return Clamp01(mind.secondsAlone / k_LonelySeconds);
	case sources::k_AttentionFromLackOfInteraction:
		return Clamp01(mind.secondsAlone / k_UninterestedSeconds);
	case sources::k_ManifestState:
		return Clamp01(desires.sum / k_ManifestSum);
	case sources::k_RunAwayFromPlayer:
		return Clamp01(-mind.attitudeToPlayer);
	case sources::k_AngerFromSadness:
	case sources::k_PlayFromSadness:
	case sources::k_TirednessFromSadness:
		return creature_desires::SourceValue(desires[Desire::Sadness], sources::k_Sadness).value_or(0.0f);
	default:
		return std::nullopt;
	}
}

/// Everything on the land a creature might look at
std::vector<creature_look::Candidate> GatherCandidates(ecs::Registry& registry)
{
	std::vector<creature_look::Candidate> candidates;
	const auto add = [&candidates](entt::entity entity, creature_look::Interest kind, const glm::vec3& point) {
		candidates.push_back({.id = entt::to_integral(entity), .kind = kind, .point = point});
	};
	registry.Each<const Creature, const Transform>([&](entt::entity entity, const Creature& creature, const Transform& at) {
		add(entity, creature_look::Interest::Creature,
		    at.position + glm::vec3(0.0f, creature_look::k_HeadHeight * creature.size, 0.0f));
	});
	registry.Each<const Villager, const Transform>([&](entt::entity entity, const Villager&, const Transform& at) {
		add(entity, creature_look::Interest::Villager, at.position + glm::vec3(0.0f, k_VillagerHeadHeight, 0.0f));
	});
	registry.Each<const Abode, const Transform>([&](entt::entity entity, const Abode&, const Transform& at) {
		add(entity, creature_look::Interest::Abode, at.position + glm::vec3(0.0f, k_AbodeLookHeight, 0.0f));
	});
	registry.Each<const Tree, const Transform>([&](entt::entity entity, const Tree&, const Transform& at) {
		add(entity, creature_look::Interest::Tree, at.position + glm::vec3(0.0f, k_TreeLookHeight, 0.0f));
	});
	registry.Each<const Temple, const Transform>([&](entt::entity entity, const Temple&, const Transform& at) {
		add(entity, creature_look::Interest::Citadel, at.position + glm::vec3(0.0f, k_CitadelLookHeight, 0.0f));
	});
	return candidates;
}

void ApplyEyes(CreatureEyes* eyes, creature_mind::Eyes look)
{
	if (eyes == nullptr || look == creature_mind::Eyes::Unchanged)
	{
		return;
	}
	const bool sleepy = look == creature_mind::Eyes::Sleepy;
	eyes->mode = look == creature_mind::Eyes::Closed ? creature_eyes::Mode::Closed : creature_eyes::Mode::Calm;
	eyes->openness = sleepy ? k_SleepyOpenness : 0.0f;
	eyes->blink.intervalMs = sleepy ? k_SleepyBlinkIntervalMs : creature_eyes::k_BlinkIntervalMs;
}

/// Sends the creature where its mind wants it to go
void Move(entt::entity creature, const creature_mind::Commands& commands)
{
	if (!Locator::creatureLocomotionSystem::has_value())
	{
		return;
	}
	auto& locomotion = Locator::creatureLocomotionSystem::value();
	if (commands.stopMoving)
	{
		locomotion.Stop(creature);
	}
	if (!commands.move.has_value())
	{
		return;
	}
	using Kind = creature_mind::Movement::Kind;
	using Pace = CreatureLocomotionSystemInterface::Pace;
	const auto& move = *commands.move;
	const auto pace = move.run ? Pace::Run : Pace::Walk;
	const auto object = move.object.has_value() ? std::optional(static_cast<entt::entity>(*move.object)) : std::nullopt;
	switch (move.kind)
	{
	case Kind::ToPoint:
	case Kind::Nearby:
		locomotion.MoveTo(creature, move.point, pace, move.minDistance, move.maxDistance);
		break;
	case Kind::ToObject:
		if (object.has_value())
		{
			locomotion.MoveToObject(creature, *object, pace, move.maxDistance);
		}
		break;
	case Kind::Follow:
		if (object.has_value())
		{
			locomotion.Follow(creature, *object, move.maxDistance, pace);
		}
		break;
	case Kind::FleeFrom:
		locomotion.FleeFrom(creature, move.point);
		break;
	case Kind::TurnToFace:
		locomotion.TurnToFace(creature, move.point);
		break;
	}
}

void Apply(const creature_mind::Commands& commands, CreatureAnimation& animation, CreatureEyes* eyes)
{
	if (commands.endSit)
	{
		animation.body = creature_layers::EndLoop(animation.body);
	}
	if (commands.playOnce.has_value())
	{
		if (auto body = creature_layers::PlayOnce(animation.body, *commands.playOnce, commands.mirrored))
		{
			animation.body = *body;
		}
	}
	if (commands.startSequence.has_value())
	{
		const auto& sequence = *commands.startSequence;
		if (auto body = creature_layers::PlaySequence(animation.body, sequence[0], sequence[1], sequence[2], commands.holdLoop))
		{
			animation.body = *body;
		}
	}
	if (commands.face.has_value())
	{
		animation.face.wanted = *commands.face;
	}
	ApplyEyes(eyes, commands.eyes);
}

bool IsNight()
{
	return Locator::skySystem::has_value() && Locator::skySystem::value().GetClock().IsVisualNight();
}

float DesireValue(const creature_desires::Desires& desires, Desire desire)
{
	const auto& state = desires[desire];
	return state.activated ? state.value : 0.0f;
}

/// What something is worth to eat, if anything
std::optional<float> FoodValueOf(entt::entity entity)
{
	return Locator::creatureObjectActionSystem::has_value() ? Locator::creatureObjectActionSystem::value().FoodValueOf(entity)
	                                                        : std::nullopt;
}

/// The nearest food within reach of a point, and where it is
std::optional<std::pair<entt::entity, glm::vec2>> NearestFood(ecs::Registry& registry, glm::vec2 from)
{
	std::optional<std::pair<entt::entity, glm::vec2>> nearest;
	float best = k_FoodSearchDistance;
	const auto consider = [&](entt::entity entity, const Transform& at) {
		const glm::vec2 point {at.position.x, at.position.z};
		const auto distance = glm::distance(point, from);
		if (distance <= best && !registry.AllOf<HeldByCreature>(entity) && FoodValueOf(entity).has_value())
		{
			best = distance;
			nearest = {entity, point};
		}
	};
	registry.Each<const Villager, const Transform>(
	    [&](entt::entity entity, const Villager&, const Transform& at) { consider(entity, at); });
	registry.Each<const MobileObject, const Transform>(
	    [&](entt::entity entity, const MobileObject&, const Transform& at) { consider(entity, at); });
	return nearest;
}

/// The nearest water to a point, searching the cells round it: shallow water it can wade into or open sea, and where it
/// can stand at its edge
std::optional<creature_mind::Wants::WaterSpot> NearestWater(glm::vec2 from)
{
	if (!Locator::creatureLocomotionSystem::has_value() || !Locator::terrainSystem::has_value())
	{
		return std::nullopt;
	}
	const auto& land = Locator::creatureLocomotionSystem::value().GetWalkableLand();
	const auto& terrain = Locator::terrainSystem::value();
	const auto cellX = static_cast<int32_t>(std::floor(from.x / creature_route::k_CellSize));
	const auto cellZ = static_cast<int32_t>(std::floor(from.y / creature_route::k_CellSize));
	std::optional<glm::vec2> nearest;
	float best = std::numeric_limits<float>::max();
	for (int32_t x = cellX - k_WaterSearchCells; x <= cellX + k_WaterSearchCells; ++x)
	{
		for (int32_t z = cellZ - k_WaterSearchCells; z <= cellZ + k_WaterSearchCells; ++z)
		{
			if (x < 0 || z < 0 || x >= creature_route::k_CellsPerSide || z >= creature_route::k_CellsPerSide)
			{
				continue;
			}
			const auto centre = (glm::vec2(static_cast<float>(x), static_cast<float>(z)) + 0.5f) * creature_route::k_CellSize;
			const auto ground = land.At(x, z);
			const bool sea = ground == creature_route::Ground::Blocked && terrain.GetHeightAt(centre) <= 0.0f;
			if (ground != creature_route::Ground::Water && !sea)
			{
				continue;
			}
			const auto distance = glm::distance(centre, from);
			if (distance < best)
			{
				best = distance;
				nearest = centre;
			}
		}
	}
	if (!nearest.has_value())
	{
		return std::nullopt;
	}
	// It stands at the edge of the water on its own side, or in the shallows
	const auto shore = land.NearestValid(*nearest, creature_route::k_DestinationClearance, k_ShoreSearchDistance);
	if (!shore.has_value())
	{
		return std::nullopt;
	}
	return creature_mind::Wants::WaterSpot {.shore = *shore, .water = *nearest};
}

/// The nearest thing a creature could pick up, and where it is
std::optional<std::pair<entt::entity, glm::vec2>> NearestObject(ecs::Registry& registry, glm::vec2 from)
{
	if (!Locator::creatureObjectActionSystem::has_value())
	{
		return std::nullopt;
	}
	const auto& hands = Locator::creatureObjectActionSystem::value();
	std::optional<std::pair<entt::entity, glm::vec2>> nearest;
	float best = k_ObjectSearchDistance;
	registry.Each<const MobileObject, const Transform>([&](entt::entity entity, const MobileObject&, const Transform& at) {
		const glm::vec2 point {at.position.x, at.position.z};
		const auto distance = glm::distance(point, from);
		if (distance <= best && !registry.AllOf<HeldByCreature>(entity) && hands.CanPickUp(entity))
		{
			best = distance;
			nearest = {entity, point};
		}
	});
	return nearest;
}

/// The nearest home or tree, to hurl something at
std::optional<glm::vec2> NearestHurlTarget(ecs::Registry& registry, glm::vec2 from)
{
	std::optional<glm::vec2> nearest;
	float best = k_HurlSearchDistance;
	const auto consider = [&](const Transform& at) {
		const glm::vec2 point {at.position.x, at.position.z};
		const auto distance = glm::distance(point, from);
		if (distance <= best)
		{
			best = distance;
			nearest = point;
		}
	};
	registry.Each<const Abode, const Transform>([&](const Abode&, const Transform& at) { consider(at); });
	registry.Each<const Tree, const Transform>([&](const Tree&, const Transform& at) { consider(at); });
	return nearest;
}

/// The needs the mind might see to now, and the food and water at hand for them
creature_mind::Wants WantsOf(ecs::Registry& registry, entt::entity creature, const creature_desires::Desires& desires,
                             glm::vec2 position)
{
	creature_mind::Wants wants {
	    .hunger = DesireValue(desires, Desire::Hunger),
	    .tiredness = DesireValue(desires, Desire::Tiredness),
	    .poo = DesireValue(desires, Desire::Poo),
	    .water = DesireValue(desires, Desire::Water),
	    .curiosity = DesireValue(desires, Desire::Curiosity),
	    .play = DesireValue(desires, Desire::Play),
	    .anger = DesireValue(desires, Desire::Anger),
	    .holding = registry.AllOf<CreatureHeldObject>(creature),
	};
	if (!wants.holding && std::max({wants.curiosity, wants.play, wants.anger}) >= creature_mind::k_ActOnDesire)
	{
		if (const auto object = NearestObject(registry, position))
		{
			wants.object = entt::to_integral(object->first);
			wants.objectPoint = object->second;
		}
		if (wants.anger >= creature_mind::k_ActOnDesire)
		{
			wants.hurlTarget = NearestHurlTarget(registry, position);
		}
	}
	if (wants.hunger >= creature_mind::k_ActOnNeed)
	{
		if (const auto food = NearestFood(registry, position))
		{
			wants.food = entt::to_integral(food->first);
			wants.foodPoint = food->second;
		}
	}
	if (wants.water >= creature_mind::k_ActOnNeed)
	{
		wants.waterSpot = NearestWater(position);
	}
	return wants;
}

/// Having done an action, the desire it satisfies is less, by the game's action table, and its body pays for it
void Satisfied(entt::entity creature, creature_desires::Desires& desires, std::string_view action)
{
	if (Locator::infoConstants::has_value())
	{
		const auto& actions = Locator::infoConstants::value().creatureAction;
		const auto found = std::ranges::find_if(actions, [action](const auto& row) {
			return std::string_view(row.name.data(), strnlen(row.name.data(), row.name.size())) == action;
		});
		if (found != actions.end() && found->desire < creature_desires::k_DesireCount)
		{
			auto& state = desires[static_cast<Desire>(found->desire)];
			if (state.activated)
			{
				state.value = std::clamp(state.value * found->desireMultiplier, 0.0f, std::max(state.max, 0.0f));
			}
		}
	}
	if (Locator::creaturePhysiologySystem::has_value())
	{
		Locator::creaturePhysiologySystem::value().FinishAction(creature, action);
	}
}

/// What a step did to the body
void TakeEffect(entt::entity creature, const creature_mind::Commands& commands, creature_desires::Desires& desires)
{
	using creature_mind::Effect;
	if (commands.effect == Effect::None || !Locator::creaturePhysiologySystem::has_value())
	{
		return;
	}
	auto& physiology = Locator::creaturePhysiologySystem::value();
	switch (commands.effect)
	{
	case Effect::Eat:
		// It took its nourishment as it ate what it held
		Satisfied(creature, desires, k_EatAction);
		break;
	case Effect::Examined:
		Satisfied(creature, desires, k_ExamineAction);
		break;
	case Effect::ThrewAbout:
		Satisfied(creature, desires, k_ThrowAboutAction);
		break;
	case Effect::Hurled:
		Satisfied(creature, desires, k_HurlAction);
		break;
	case Effect::Drink:
		physiology.Drink(creature);
		Satisfied(creature, desires, k_DrinkAction);
		creature_desires::Suppress(desires, Desire::Water, k_WaterSuppressSeconds, k_TurnsPerSecond);
		break;
	case Effect::Poo:
		physiology.Poo(creature);
		Satisfied(creature, desires, k_PooAction);
		desires[Desire::Poo].value = 0.0f;
		creature_desires::Suppress(desires, Desire::Poo, k_PooSuppressSeconds, k_TurnsPerSecond);
		break;
	case Effect::Puke:
		physiology.Puke(creature);
		Satisfied(creature, desires, k_PukeAction);
		break;
	case Effect::Slept:
		Satisfied(creature, desires, k_SleepAction);
		break;
	case Effect::CameRound:
		physiology.WakeFromFaint(creature);
		break;
	case Effect::None:
		break;
	}
}

/// Tells the creature's hands what to do with a thing
void Order(ecs::Registry& registry, entt::entity creature, const creature_mind::Commands& commands)
{
	if (!commands.object.has_value() || !Locator::creatureObjectActionSystem::has_value())
	{
		return;
	}
	auto& hands = Locator::creatureObjectActionSystem::value();
	const auto& order = *commands.object;
	const auto object = order.object.has_value() ? std::optional(static_cast<entt::entity>(*order.object)) : std::nullopt;
	const auto ground = Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(order.point) : 0.0f;
	const glm::vec3 point {order.point.x, ground, order.point.y};
	using Kind = creature_mind::ObjectOrder::Kind;
	switch (order.kind)
	{
	case Kind::PickUp:
		if (object.has_value() && registry.Valid(*object))
		{
			hands.PickUp(creature, *object);
		}
		else
		{
			hands.Cancel(creature);
		}
		break;
	case Kind::PutDown:
		hands.PutDown(creature);
		break;
	case Kind::Discard:
		hands.Discard(creature);
		break;
	case Kind::Eat:
		hands.EatHeld(creature);
		break;
	case Kind::Keep:
		hands.Keep(creature, order.animation);
		break;
	case Kind::Throw:
	case Kind::ThrowNearby:
		hands.Throw(creature, point);
		break;
	case Kind::Destroy:
		if (object.has_value() && registry.Valid(*object))
		{
			hands.Destroy(creature, *object);
		}
		else
		{
			hands.Cancel(creature);
		}
		break;
	}
}

creature_mind::HandsState HandsOf(entt::entity creature)
{
	if (!Locator::creatureObjectActionSystem::has_value())
	{
		return creature_mind::HandsState::Idle;
	}
	using State = CreatureObjectActionSystemInterface::State;
	switch (Locator::creatureObjectActionSystem::value().GetState(creature))
	{
	case State::Busy:
		return creature_mind::HandsState::Busy;
	case State::Done:
		return creature_mind::HandsState::Done;
	case State::Failed:
		return creature_mind::HandsState::Failed;
	case State::Idle:
		break;
	}
	return creature_mind::HandsState::Idle;
}
} // namespace

void CreatureMindSystem::ProcessTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto random = [this](uint32_t range) {
		return range == 0 ? 0u : std::uniform_int_distribution<uint32_t>(0, range - 1)(_random);
	};
	const auto uniform = [this](float low, float high) {
		return high > low ? std::uniform_real_distribution<float>(low, high)(_random) : low;
	};
	std::optional<std::vector<creature_look::Candidate>> candidates;
	const bool night = IsNight();

	registry.Each<const Creature, CreatureMindState, CreatureAnimation, const Transform>(
	    [&](entt::entity entity, const Creature& creature, CreatureMindState& mind, CreatureAnimation& animation,
	        const Transform& transform) {
		    if (!mind.desires.has_value())
		    {
			    mind.desires = creature_desires::Create(SetupFor(creature.species), uniform);
		    }
		    if (mind.desiresPhase != mind.developmentPhase)
		    {
			    creature_desires::ActivateForPhase(*mind.desires, Phases(), mind.developmentPhase);
			    mind.desiresPhase = mind.developmentPhase;
		    }
		    mind.secondsAlone += k_TurnSeconds;
		    if (mind.feedbackSeconds.has_value())
		    {
			    *mind.feedbackSeconds += k_TurnSeconds;
		    }
		    auto* needs = registry.TryGet<CreatureNeeds>(entity);
		    const auto body = needs != nullptr ? needs->needs : creature_physiology::Needs {};
		    creature_desires::UpdateSources(*mind.desires,
		                                    [&mind, &body, night](uint32_t type, const creature_desires::Desires& desires) {
			                                    return ReadSource(type, desires, mind, body, night);
		                                    });
		    creature_desires::UpdateDesires(*mind.desires, k_TurnsPerSecond);

		    auto* eyes = registry.TryGet<CreatureEyes>(entity);
		    if (mind.paused)
		    {
			    return;
		    }
		    // Exhausted, starved or out of life, it drops where it stands
		    if (needs != nullptr && needs->faint.has_value() && !creature_mind::IsUnconscious(mind.idle))
		    {
			    if (Locator::creatureLocomotionSystem::has_value())
			    {
				    Locator::creatureLocomotionSystem::value().Stop(entity);
			    }
			    animation.body = {};
			    creature_mind::Plan(mind.idle, creature_mind::Activity::Faint, creature_mind::Faint());
			    needs->faint.reset();
		    }
		    const bool moving =
		        Locator::creatureLocomotionSystem::has_value() && Locator::creatureLocomotionSystem::value().IsMoving(entity);
		    const creature_mind::Senses senses {
		        .seconds = k_TurnSeconds,
		        .bodyBusy = creature_layers::IsPlaying(animation.body) || moving,
		        .bodyLooping = creature_layers::IsLooping(animation.body),
		        .moving = moving,
		        .position = glm::vec2(transform.position.x, transform.position.z),
		        .strongest = creature_desires::StrongestShowable(*mind.desires, creature_mind::k_MinDesireShown),
		        .feedbackSeconds = mind.feedbackSeconds,
		        .feedbackWasStroke = mind.feedbackWasStroke,
		        // Food and water are only looked for when it is free to choose what to do next
		        .wants = mind.idle.step >= mind.idle.agenda.size()
		                     ? WantsOf(registry, entity, *mind.desires, glm::vec2(transform.position.x, transform.position.z))
		                     : creature_mind::Wants {},
		        .rested = needs != nullptr && needs->rested,
		        .hands = HandsOf(entity),
		    };
		    const auto commands = creature_mind::Think(mind.idle, senses, random);
		    Apply(commands, animation, eyes);
		    Move(entity, commands);
		    Order(registry, entity, commands);
		    TakeEffect(entity, commands, *mind.desires);
		    if (needs != nullptr)
		    {
			    needs->rest = creature_mind::IsUnconscious(mind.idle) ? CreatureNeeds::Rest::Unconscious
			                  : creature_mind::IsAsleep(mind.idle)    ? CreatureNeeds::Rest::Asleep
			                                                          : CreatureNeeds::Rest::Awake;
		    }

		    // Looking about, the head turns to the most interesting thing in sight, or ahead
		    mind.lookingAbout = commands.lookAbout;
		    if (commands.lookAbout)
		    {
			    if (!candidates.has_value())
			    {
				    candidates = GatherCandidates(registry);
			    }
			    const auto self = entt::to_integral(entity);
			    std::vector<creature_look::Candidate> others;
			    others.reserve(candidates->size());
			    std::ranges::copy_if(*candidates, std::back_inserter(others),
			                         [self](const creature_look::Candidate& candidate) { return candidate.id != self; });
			    const creature_look::Viewer viewer {
			        .position = transform.position,
			        .ahead = -(transform.rotation * glm::vec3(0.0f, 0.0f, 1.0f)),
			        .size = creature.size,
			    };
			    mind.look = creature_look::LookAbout(mind.look, others, viewer, k_TurnsPerSecond);
			    animation.lookAt = mind.look.id.has_value() ? mind.look.point : creature_look::PointAhead(viewer);
		    }
		    else
		    {
			    animation.lookAt.reset();
		    }
		    if (eyes != nullptr)
		    {
			    eyes->lookAt = animation.lookAt;
		    }
	    });
}

bool CreatureMindSystem::PlayAction(entt::entity creature, size_t animation)
{
	auto* body = Locator::entitiesRegistry::value().TryGet<CreatureAnimation>(creature);
	if (body == nullptr)
	{
		return false;
	}
	const auto played = creature_layers::PlayOnce(body->body, animation, std::bernoulli_distribution(0.5)(_random));
	if (played.has_value())
	{
		body->body = *played;
	}
	return played.has_value();
}

bool CreatureMindSystem::PlayGesture(entt::entity creature, size_t animation)
{
	auto* body = Locator::entitiesRegistry::value().TryGet<CreatureAnimation>(creature);
	if (body == nullptr)
	{
		return false;
	}
	const auto played = creature_layers::PlayGesture(body->gesture, animation);
	if (played.has_value())
	{
		body->gesture = *played;
	}
	return played.has_value();
}

void CreatureMindSystem::PullFace(entt::entity creature, size_t animation)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (auto* body = registry.TryGet<CreatureAnimation>(creature))
	{
		body->face.wanted = animation;
	}
	if (auto* mind = registry.TryGet<CreatureMindState>(creature))
	{
		mind->idle.faceSeconds = creature_mind::k_FaceSeconds;
	}
}

bool CreatureMindSystem::SitDown(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* body = registry.TryGet<CreatureAnimation>(creature);
	auto* mind = registry.TryGet<CreatureMindState>(creature);
	if (body == nullptr || mind == nullptr || creature_layers::IsPlaying(body->body))
	{
		return false;
	}
	// A paused mind won't get it up again: it sits until told to stand
	if (mind->paused)
	{
		body->body =
		    *creature_layers::PlaySequence(body->body, animations::k_StartSit, animations::k_Sit, animations::k_EndSit);
		return true;
	}
	const auto random = [this](uint32_t range) {
		return range == 0 ? 0u : std::uniform_int_distribution<uint32_t>(0, range - 1)(_random);
	};
	creature_mind::Plan(mind->idle, creature_mind::Activity::Told, {creature_mind::SitDown(random)});
	return true;
}

void CreatureMindSystem::ReceiveFeedback(entt::entity creature, float feedback)
{
	namespace sources = creature_desires::sources;
	auto& registry = Locator::entitiesRegistry::value();
	auto* mind = registry.TryGet<CreatureMindState>(creature);
	if (mind == nullptr)
	{
		return;
	}
	mind->secondsAlone = 0.0f;
	mind->lastFeedback = CreatureMindState::Feedback {.value = feedback, .activity = mind->idle.activity};
	// Too slight to count, it only looks at the player
	if (std::abs(feedback) <= creature_feedback::k_SlightFeedback)
	{
		if (Locator::creatureLocomotionSystem::has_value() && Locator::camera::has_value())
		{
			const auto eye = Locator::camera::value().GetOrigin();
			Locator::creatureLocomotionSystem::value().TurnToFace(creature, glm::vec2(eye.x, eye.z));
		}
		return;
	}
	mind->attitudeToPlayer = creature_feedback::AttitudeAfter(mind->attitudeToPlayer, feedback);
	mind->averageFeedback = creature_feedback::AverageAfter(mind->averageFeedback, feedback);
	mind->feedbackSeconds = 0.0f;
	mind->feedbackWasStroke = feedback > 0.0f;
	// Slapped, it stops whatever it was doing
	if (feedback < 0.0f)
	{
		if (Locator::creatureObjectActionSystem::has_value())
		{
			Locator::creatureObjectActionSystem::value().Cancel(creature);
		}
		if (Locator::creatureLocomotionSystem::has_value())
		{
			Locator::creatureLocomotionSystem::value().Stop(creature);
		}
		mind->idle.agenda.clear();
		mind->idle.step = 0;
		mind->idle.stepStarted = false;
	}
	// Stroking makes it playful, showy and kind; slapping angry and fearful
	if (mind->desires.has_value())
	{
		const auto amount = 0.5f * std::abs(feedback);
		constexpr std::array k_Stroked {sources::k_PlayFromWatchingPlayer, sources::k_ManifestState,
		                                sources::k_CompassionFromWatchingPlayer};
		constexpr std::array k_Slapped {sources::k_AngerFromDamage, sources::k_FearFromDamage};
		const auto pushed = feedback > 0.0f ? std::span<const uint32_t>(k_Stroked) : std::span<const uint32_t>(k_Slapped);
		for (const auto type : pushed)
		{
			creature_desires::ChangeSource(*mind->desires, type, amount);
		}
	}
	// The game reacts through its planner, which isn't here yet: the creature shows how it feels as soon as it is free
	mind->idle.showDesireSeconds = 0.0f;
	if (mind->idle.activity != creature_mind::Activity::ShowDesire)
	{
		mind->idle.agenda.resize(std::min(mind->idle.agenda.size(), mind->idle.step + (mind->idle.stepStarted ? 1 : 0)));
	}
}

bool CreatureMindSystem::ForceAction(entt::entity creature, size_t animation, bool mirrored, std::optional<size_t> face,
                                     float faceSeconds, float interruptsAfter)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* body = registry.TryGet<CreatureAnimation>(creature);
	auto* mind = registry.TryGet<CreatureMindState>(creature);
	if (body == nullptr || mind == nullptr || creature_mind::IsUnconscious(mind->idle))
	{
		return false;
	}
	// Not while it is only just into something
	if (Locator::creatureObjectActionSystem::has_value())
	{
		if (const auto progress = Locator::creatureObjectActionSystem::value().GetProgress(creature);
		    progress.has_value() && *progress < interruptsAfter)
		{
			return false;
		}
	}
	if (creature_layers::IsPlaying(body->body) && Locator::creatureAnimationSystem::has_value())
	{
		const auto duration = Locator::creatureAnimationSystem::value().AnimationDuration(
		    creature, creature_layers::CurrentAnimation(body->body));
		if (duration.has_value() && *duration > 0.0f && body->body.timeMs / *duration < interruptsAfter)
		{
			return false;
		}
	}
	const auto played = creature_layers::PlayOnce(creature_layers::BodyAction {}, animation, mirrored);
	if (!played.has_value())
	{
		return false;
	}
	if (Locator::creatureObjectActionSystem::has_value())
	{
		Locator::creatureObjectActionSystem::value().Cancel(creature);
	}
	if (Locator::creatureLocomotionSystem::has_value())
	{
		Locator::creatureLocomotionSystem::value().Stop(creature);
	}
	body->body = *played;
	if (face.has_value())
	{
		body->face.wanted = *face;
		mind->idle.faceSeconds = faceSeconds;
	}
	// What it was doing is over; it decides afresh once the action has played
	mind->idle.agenda.resize(std::min(mind->idle.agenda.size(), mind->idle.step));
	mind->idle.stepStarted = false;
	return true;
}

bool CreatureMindSystem::Replan(entt::entity creature, creature_mind::Activity activity,
                                std::vector<creature_mind::Step> agenda)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* mind = registry.TryGet<CreatureMindState>(creature);
	auto* body = registry.TryGet<CreatureAnimation>(creature);
	if (mind == nullptr || body == nullptr || agenda.empty())
	{
		return false;
	}
	if (Locator::creatureLocomotionSystem::has_value())
	{
		Locator::creatureLocomotionSystem::value().Stop(creature);
	}
	// Whatever it plays ends at once, as the game ends a creature's animations when it changes what it does
	body->body = {};
	if (auto* eyes = registry.TryGet<CreatureEyes>(creature))
	{
		ApplyEyes(eyes, creature_mind::Eyes::Normal);
	}
	creature_mind::Plan(mind->idle, activity, std::move(agenda));
	return true;
}

bool CreatureMindSystem::Sleep(entt::entity creature)
{
	return Replan(creature, creature_mind::Activity::Sleep, creature_mind::Sleep([this](uint32_t range) {
		              return range == 0 ? 0u : std::uniform_int_distribution<uint32_t>(0, range - 1)(_random);
	              }));
}

bool CreatureMindSystem::Eat(entt::entity creature, std::optional<entt::entity> food)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* transform = registry.TryGet<const Transform>(creature);
	if (!food.has_value() && transform != nullptr)
	{
		if (const auto nearest = NearestFood(registry, glm::vec2(transform->position.x, transform->position.z)))
		{
			food = nearest->first;
		}
	}
	if (!food.has_value() || !FoodValueOf(*food).has_value())
	{
		return false;
	}
	return Replan(creature, creature_mind::Activity::Eat, creature_mind::Eat(entt::to_integral(*food)));
}

bool CreatureMindSystem::Drink(entt::entity creature)
{
	const auto* transform = Locator::entitiesRegistry::value().TryGet<const Transform>(creature);
	if (transform == nullptr)
	{
		return false;
	}
	const auto water = NearestWater(glm::vec2(transform->position.x, transform->position.z));
	return water.has_value() &&
	       Replan(creature, creature_mind::Activity::Drink, creature_mind::Drink(water->shore, water->water));
}

bool CreatureMindSystem::Poo(entt::entity creature)
{
	return Replan(creature, creature_mind::Activity::Poo, creature_mind::Poo([this](uint32_t range) {
		              return range == 0 ? 0u : std::uniform_int_distribution<uint32_t>(0, range - 1)(_random);
	              }));
}

bool CreatureMindSystem::Puke(entt::entity creature)
{
	return Replan(creature, creature_mind::Activity::Puke, creature_mind::Puke());
}

bool CreatureMindSystem::Faint(entt::entity creature)
{
	return Replan(creature, creature_mind::Activity::Faint, creature_mind::Faint());
}

void CreatureMindSystem::Wake(entt::entity creature)
{
	if (auto* mind = Locator::entitiesRegistry::value().TryGet<CreatureMindState>(creature))
	{
		mind->idle.wakeWanted = true;
	}
}

void CreatureMindSystem::StandUp(entt::entity creature)
{
	if (auto* body = Locator::entitiesRegistry::value().TryGet<CreatureAnimation>(creature))
	{
		body->body = creature_layers::EndLoop(body->body);
	}
}

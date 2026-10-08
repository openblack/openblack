/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "ReactionSystem.h"

#include <cstring>

#include <algorithm>
#include <array>
#include <optional>

#include <glm/geometric.hpp>

#include "3D/CreatureBody.h"
#include "3D/MapCoords.h"
#include "Common/GUtilsDistance.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/LivingReaction.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/MiracleImpression.h"
#include "ECS/Components/Spell.h"
#include "ECS/Components/SpellSeed.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/TownDesire.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Map.h"
#include "ECS/PhysicsEntry.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/AnimalSystemInterface.h"
#include "ECS/Systems/CreatureFightSystemInterface.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/MagicShieldSystemInterface.h"
#include "ECS/Systems/TeleportSystemInterface.h"
#include "ECS/TownDesire.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/AreaEffect.h"
#include "Magic/Impressiveness.h"
#include "Magic/ReactionRules.h"
#include "Magic/VillagerReactionRules.h"
#include "Physics/LivingRules.h"
#include "VillagerFire.h"
#include "VillagerPhysics.h"
#include "VillagerReactions.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace villager_reaction = openblack::magic::villager_reaction;

namespace
{
/// Whole seconds of game turns, at the game's ten turns a second
constexpr uint32_t k_TurnsPerSecond = 10;

/// The seed in the hand that cast a miracle, if the miracle has one
SpellSeed* SeedOf(ecs::Registry& registry, entt::entity spell)
{
	const auto* miracle = registry.Valid(spell) ? registry.TryGet<const ecs::components::Spell>(spell) : nullptr;
	return miracle != nullptr && registry.Valid(miracle->seed) ? registry.TryGet<SpellSeed>(miracle->seed) : nullptr;
}

/// Where a town's centre stands, if it has one
std::optional<glm::vec3> TownCentreOf(ecs::Registry& registry, uint32_t townId)
{
	std::optional<glm::vec3> found;
	registry.Each<const Abode, const Transform>([&](entt::entity, const Abode& abode, const Transform& transform) {
		if (!found.has_value() && abode.type == AbodeNumber::TownCentre && abode.townId == townId)
		{
			found = transform.position;
		}
	});
	return found;
}

const ReactionInfo* InfoOf(Reaction type)
{
	if (!Locator::infoConstants::has_value() || type == Reaction::None)
	{
		return nullptr;
	}
	const auto& table = Locator::infoConstants::value().reaction;
	const auto index = static_cast<size_t>(type);
	return index < table.size() ? &table.at(index) : nullptr;
}

/// The living table of a villager or creature
const GLivingInfo* LivingInfoOf(const ecs::Registry& registry, entt::entity entity)
{
	const auto& info = Locator::infoConstants::value();
	if (const auto* creature = registry.TryGet<const Creature>(entity))
	{
		const auto row = creature::InfoRow(creature->species);
		return row < info.creature.size() ? &info.creature.at(row) : nullptr;
	}
	if (const auto* villager = registry.TryGet<const Villager>(entity))
	{
		const auto kind = static_cast<size_t>(GVillagerInfo::Find(villager->tribe, villager->number));
		return kind < info.villager.size() ? &info.villager.at(kind) : nullptr;
	}
	if (const auto* animal = registry.TryGet<const Animal>(entity))
	{
		const auto kind = static_cast<size_t>(animal->type);
		return kind < info.animal.size() ? &info.animal.at(kind) : nullptr;
	}
	return nullptr;
}

/// Whether a reaction reaches a kind of living thing: villagers and creatures every kind; animals, so far, only things
/// flying at them
// TODO(animals): the reactions animals take up beside flying things (fire, predators and the others whose priority the
// animal doesn't zero) belong to the animals' own behaviour, which openblack doesn't run yet
bool Reaches(const ecs::Registry& registry, entt::entity entity, Reaction type)
{
	if (registry.AnyOf<Villager, Creature>(entity))
	{
		return true;
	}
	return type == Reaction::ReactToFlyingObject && registry.AllOf<Animal>(entity);
}

/// Whether a kind of living thing reacts to a kind of reaction at all, by its table's flags in the reactions' order
bool Reacts(const GLivingInfo& info, Reaction type)
{
	constexpr size_t k_Flags = sizeof(IsReacting) / sizeof(uint32_t);
	std::array<uint32_t, k_Flags> flags {};
	std::memcpy(flags.data(), &info.isReacting, sizeof(IsReacting));
	const auto index = static_cast<size_t>(type);
	return index < flags.size() && flags.at(index) != 0;
}

/// The shields' own reactions, which their villagers take up through the shields
bool ShieldKind(Reaction type)
{
	return type == Reaction::ReactToMagicShield || type == Reaction::ReactToMagicShieldStruck ||
	       type == Reaction::ReactToMagicShieldDestroyed;
}

villager_reaction::Distance DistanceOf(const ReactionInfo& info)
{
	return {.maxDistance = info.maxReactionDistance, .importance = info.howImportantIsDistance};
}

float SpreadDistance(const glm::vec3& at, const glm::vec3& source)
{
	return villager_reaction::SpreadDistance({at.x, at.z}, {source.x, source.z});
}

/// The kinds of reaction a living thing took up lately: a villager's shared with the shields
villager_reaction::Memory& MemoryOf(ecs::Registry& registry, entt::entity living, LivingReaction& state)
{
	if (!registry.AllOf<Villager>(living))
	{
		return state.creatureMemory;
	}
	auto* memory = registry.TryGet<VillagerReactionMemory>(living);
	if (memory == nullptr)
	{
		memory = &registry.Assign<VillagerReactionMemory>(living);
	}
	return memory->memory;
}

/// A creature copying the player takes up only reactions more urgent than this
constexpr uint32_t k_LeastPriorityWhileMimicking = 150;

/// Whether a living thing may take up a kind of reaction now: a villager able and alive and in a state that allows it, a
/// creature not fighting or out cold, and while copying the player only for the most urgent kinds
bool Available(const ecs::Registry& registry, entt::entity entity, Reaction type)
{
	if (registry.AllOf<Villager>(entity))
	{
		return villager_reactions::Available(entity, type);
	}
	if (registry.AllOf<Animal>(entity))
	{
		return Locator::animalSystem::has_value() && Locator::animalSystem::value().IsAvailableForReaction(entity);
	}
	if (!registry.AllOf<Creature>(entity))
	{
		return false;
	}
	if (Locator::creatureFightSystem::has_value())
	{
		const auto& fights = Locator::creatureFightSystem::value();
		if (fights.IsFighting(entity) || fights.IsKnockedOut(entity))
		{
			return false;
		}
	}
	// TODO(raffclar): the game also keeps a creature reactions are turned off for by script, one in the dance editor, one
	// forced into an action by fainting or a teleport, and a few more states, from reacting; none of them is modelled
	if (const auto* mind = registry.TryGet<const CreatureMindState>(entity);
	    mind != nullptr && mind->learnt.has_value() && mind->learnt->mimicry.has_value())
	{
		const auto* info = InfoOf(type);
		return info != nullptr && info->priority > k_LeastPriorityWhileMimicking;
	}
	return true;
}

/// A villager hiding in a building, or one of the kinds of reaction only believed in, is impressed by what it sees
/// without taking the reaction up
bool BelievesWithoutReacting(const ecs::Registry& registry, entt::entity entity, Reaction type)
{
	if (!registry.AllOf<Villager>(entity))
	{
		return false;
	}
	if (type == Reaction::ReactToMagicWaterPuttingOutFire || type == Reaction::ReactToMissionary ||
	    type == Reaction::ReactToFightWon)
	{
		return true;
	}
	// TODO(raffclar): the game also counts a villager out of reach by two flags whose meaning is unknown
	const auto* action = registry.TryGet<const LivingAction>(entity);
	return action != nullptr && Locator::livingActionSystem::has_value() &&
	       Locator::livingActionSystem::value().VillagerGetState(*action, LivingAction::Index::Top) ==
	           VillagerStates::GoAndHideInNearbyBuilding;
}

/// The kinds of reaction a living thing switching to another reaction of the same kind may take up: fire, a predator, a
/// shield struck
bool SameKindRetakes(Reaction type)
{
	return type == Reaction::ReactToFire || type == Reaction::FleeFromPredator || type == Reaction::ReactToMagicShieldStruck;
}
} // namespace

uint32_t ReactionSystem::Create(const Source& source)
{
	const auto* info = InfoOf(source.type);
	const uint32_t id = _nextId++;
	_reactions.push_back(
	    {.id = id,
	     .source = source,
	     .age = 0,
	     .reach = source.reach.value_or(
	         info != nullptr ? magic::StartingReach(info->whetherReactionGrows != 0, info->maxReactionDistance) : 0.0f),
	     .firstTaken = source.onCast ? std::optional<uint32_t>(0) : std::nullopt});
	// It is spread at once
	Spread(_reactions.back());
	return id;
}

void ReactionSystem::Move(uint32_t id, const glm::vec3& position, const glm::vec3& velocity, float strength)
{
	const auto found = std::ranges::find(_reactions, id, &Active::id);
	if (found != _reactions.end())
	{
		found->source.position = position;
		found->source.strength = strength;
		found->velocity = velocity;
	}
}

void ReactionSystem::SetInitiator(uint32_t id, entt::entity initiator)
{
	const auto found = std::ranges::find(_reactions, id, &Active::id);
	if (found != _reactions.end())
	{
		found->source.initiator = initiator;
	}
}

std::optional<ReactionSystemInterface::Active> ReactionSystem::Find(uint32_t id) const
{
	const auto found = std::ranges::find(_reactions, id, &Active::id);
	return found != _reactions.end() ? std::optional(*found) : std::nullopt;
}

void ReactionSystem::RemoveFrom(entt::entity initiator, Reaction type)
{
	std::vector<uint32_t> gone;
	for (const auto& reaction : _reactions)
	{
		if (reaction.source.initiator == initiator && reaction.source.type == type)
		{
			gone.push_back(reaction.id);
		}
	}
	for (const auto id : gone)
	{
		ShutDown(id);
	}
}

bool ReactionSystem::IsActive(uint32_t id) const
{
	return std::ranges::any_of(_reactions, [id](const Active& reaction) { return reaction.id == id; });
}

void ReactionSystem::RemoveFrom(entt::entity initiator)
{
	std::vector<uint32_t> gone;
	for (const auto& reaction : _reactions)
	{
		if (reaction.source.initiator == initiator)
		{
			gone.push_back(reaction.id);
		}
	}
	for (const auto id : gone)
	{
		ShutDown(id);
	}
}

void ReactionSystem::ShutDown(uint32_t id)
{
	if (Locator::entitiesRegistry::has_value())
	{
		auto& registry = Locator::entitiesRegistry::value();
		std::vector<entt::entity> reacting;
		registry.Each<LivingReaction>([&](entt::entity entity, const LivingReaction& state) {
			if (state.reaction == id)
			{
				reacting.push_back(entity);
			}
		});
		for (const auto entity : reacting)
		{
			Stop(entity, registry.Get<LivingReaction>(entity), true);
		}
	}
	std::erase_if(_reactions, [id](const Active& reaction) { return reaction.id == id; });
}

void ReactionSystem::Remove(uint32_t id)
{
	ShutDown(id);
}

bool ReactionSystem::HasReaction(entt::entity initiator) const
{
	return std::ranges::any_of(_reactions,
	                           [initiator](const Active& reaction) { return reaction.source.initiator == initiator; });
}

uint32_t ReactionSystem::PriorityTo(const Active& reaction, entt::entity living, const glm::vec3& at) const
{
	return PriorityTo(reaction, living, at, SpreadDistance(at, reaction.source.position));
}

uint32_t ReactionSystem::PriorityTo(const Active& reaction, entt::entity living, const glm::vec3& at, float distance) const
{
	const auto* info = InfoOf(reaction.source.type);
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* livingInfo = LivingInfoOf(registry, living);
	if (info == nullptr || livingInfo == nullptr)
	{
		return 0;
	}
	// A teleport stone matters only to one whose walk it would shorten enough
	if (reaction.source.type == Reaction::ReactToTeleport &&
	    (!Locator::teleportSystem::has_value() ||
	     !Locator::teleportSystem::value().ShouldReact(reaction.source.initiator, living)))
	{
		return 0;
	}
	// A villager flying or coming down takes no notice of what else flies
	if (reaction.source.type == Reaction::ReactToFlyingObject && registry.AllOf<Villager>(living) &&
	    !villager_physics::TakesFlyingObjectReaction(living))
	{
		return 0;
	}
	// TODO(physics): a dancing villager (one in a dance group) notices a flying thing only when it comes at it
	// (living::FlyingAt); openblack's villagers don't dance in groups yet
	// A villager sheltering under a magic shield takes no notice of what flies outside the shield
	if (reaction.source.type == Reaction::ReactToFlyingObject && registry.AllOf<Villager>(living))
	{
		if (const auto* shelter = registry.TryGet<const VillagerShieldReaction>(living);
		    shelter != nullptr && shelter->type == Reaction::ReactToMagicShield)
		{
			const auto* shield =
			    registry.Valid(shelter->shield) ? registry.TryGet<const MagicShield>(shelter->shield) : nullptr;
			const auto* where = registry.Valid(shelter->shield) ? registry.TryGet<const Transform>(shelter->shield) : nullptr;
			if (shield == nullptr || where == nullptr ||
			    !(physics::living::MapDistance(where->position, reaction.source.position) < shield->radius))
			{
				return 0;
			}
		}
	}
	// A villager weighs a fire by its own rule: how far the fire reaches of its fiercest, and whether it fights it already
	if (reaction.source.type == Reaction::ReactToFire && registry.AllOf<Villager>(living))
	{
		return villager_fire::ReactToFirePriority(living, reaction.source.initiator);
	}
	const auto kind =
	    magic::KindPriority(reaction.source.type, info->priority, magic::FastMapDistance(at, reaction.source.position),
	                        living == reaction.source.casterCreature);
	return villager_reaction::Priority(static_cast<uint8_t>(std::min<uint32_t>(kind, 255)),
	                                   Reacts(*livingInfo, reaction.source.type), DistanceOf(*info), distance);
}

void ReactionSystem::Spread(Active& reaction)
{
	const auto* info = InfoOf(reaction.source.type);
	if (info == nullptr || !Locator::entitiesRegistry::has_value() || !Locator::entitiesMap::has_value())
	{
		return;
	}
	// The reaction spreads over the map's cells in a spiral out to its reach, as many cells as its initiator's strength
	// makes of the spiral, to the living each cell keeps, the last to come into it first
	const auto& map = Locator::entitiesMap::value();
	const glm::vec2 centre(reaction.source.position.x, reaction.source.position.z);
	const auto start = map_coords::FromMetres(centre);
	auto cell = start;
	map_coords::Spiral spiral;
	const auto steps =
	    static_cast<int32_t>(static_cast<float>(map_coords::CellSpiralSize(reaction.reach)) * reaction.source.strength);
	for (int32_t i = 0; i < steps; ++i)
	{
		if (i > 0)
		{
			map_coords::AddCells(cell, spiral.Next());
		}
		if (!map_coords::InBounds(cell) || gutils::GetDistanceInMetres(start, cell) > reaction.reach)
		{
			continue;
		}
		const auto id = ecs::MapInterface::CellId(map_coords::Cell(cell));
		const auto& mobiles = map.GetMobileInGridCell(id);
		SpreadInCell(reaction, std::vector<entt::entity>(mobiles.begin(), mobiles.end()));
	}
}

void ReactionSystem::SpreadInCell(Active& reaction, const std::vector<entt::entity>& mobiles)
{
	const auto* info = InfoOf(reaction.source.type);
	auto& registry = Locator::entitiesRegistry::value();
	const auto type = static_cast<uint32_t>(reaction.source.type);
	for (const auto entity : mobiles)
	{
		// The shields' villagers react to the shields' own reactions through the shields, and while they do, to nothing
		// else
		// TODO(raffclar): the game weighs another reaction against a villager's shield reaction as against any other
		if (!registry.Valid(entity) || !registry.AllOf<Transform>(entity) || entity == reaction.source.initiator ||
		    !Reaches(registry, entity, reaction.source.type) ||
		    (registry.AllOf<Villager>(entity) &&
		     (ShieldKind(reaction.source.type) || registry.AllOf<VillagerShieldReaction>(entity))) ||
		    !Available(registry, entity, reaction.source.type))
		{
			continue;
		}
		const auto at = registry.Get<const Transform>(entity).position;
		// Under a shield that what made the reaction isn't inside, where it is now, it is kept off
		const auto* initiatorAt =
		    registry.Valid(reaction.source.initiator) ? registry.TryGet<const Transform>(reaction.source.initiator) : nullptr;
		if (Locator::magicShieldSystem::has_value() &&
		    Locator::magicShieldSystem::value().KeepsReactionOff(at, initiatorAt != nullptr ? initiatorAt->position
		                                                                                    : reaction.source.position))
		{
			continue;
		}
		auto* state = registry.TryGet<LivingReaction>(entity);
		if (state == nullptr)
		{
			state = &registry.Assign<LivingReaction>(entity);
		}
		auto& memory = MemoryOf(registry, entity, *state);
		const bool creature = registry.AllOf<Creature>(entity);
		const auto again =
		    creature ? info->numGameTurnsForCreatureBeforeReactingAgain : info->numGameTurnsForNormalThingsBeforeReactingAgain;
		// Impressed without reacting, it is the last in the cell the reaction reaches
		if (BelievesWithoutReacting(registry, entity, reaction.source.type))
		{
			if (memory.MayReactAgain(type, _turn, again))
			{
				Impress(reaction, entity, at);
			}
			return;
		}
		if (state->reaction == 0)
		{
			if (PriorityTo(reaction, entity, at) != 0 && memory.MayReactAgain(type, _turn, again))
			{
				if (!reaction.firstTaken.has_value())
				{
					reaction.firstTaken = reaction.age;
				}
				Start(reaction, entity, *state);
			}
			continue;
		}
		const auto current = Find(state->reaction);
		if (!current.has_value())
		{
			continue;
		}
		// Weighed against what it reacts to now, by how far it is from the middle of that reaction's cell
		const auto cellOfCurrent =
		    map_coords::Cell(map_coords::FromMetres({current->source.position.x, current->source.position.z}));
		const auto currentPriority = PriorityTo(
		    *current, entity, at,
		    gutils::GetDistanceInMetresToCell(map_coords::FromMetres({at.x, at.z}),
		                                      {static_cast<int16_t>(cellOfCurrent.x), static_cast<int16_t>(cellOfCurrent.y)}));
		const auto priority = PriorityTo(reaction, entity, at);
		const bool differs =
		    state->type != reaction.source.type || (SameKindRetakes(reaction.source.type) && state->reaction != reaction.id);
		const auto seconds = (_turn - memory.LastReacted(static_cast<uint32_t>(state->type))) / k_TurnsPerSecond;
		if (differs && magic::ChangesReaction(state->type, currentPriority, priority, seconds))
		{
			memory.Record(type, _turn);
			Stop(entity, *state, false);
			Start(reaction, entity, *state);
		}
	}
}

void ReactionSystem::Start(Active& reaction, entt::entity living, LivingReaction& state)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto& source = reaction.source;
	const bool nice = source.type == Reaction::LookAtNiceSpell || source.type == Reaction::ReactToImpressiveSpell;
	const bool shield = source.type == Reaction::ReactToMagicShield;
	if (const auto* creature = registry.TryGet<const Creature>(living))
	{
		// A creature takes no notice of another player's nice miracle or shield, nor of a shield struck or gone; it
		// takes up a magic tree's reaction without doing anything about it, so not at all
		const bool othersPlayer = source.player != creature->owner;
		if (((nice || shield) && othersPlayer) || source.type == Reaction::ReactToMagicShieldStruck ||
		    source.type == Reaction::ReactToMagicShieldDestroyed || source.type == Reaction::ReactToMagicTree)
		{
			return;
		}
	}
	// An animal takes up a flying thing's reaction only when the thing is near enough to flee
	if (registry.AllOf<Animal>(living))
	{
		const auto* entry =
		    Locator::dynamicsSystem::has_value() ? Locator::dynamicsSystem::value().Find(source.initiator) : nullptr;
		if (source.type != Reaction::ReactToFlyingObject || entry == nullptr || entry->body == nullptr ||
		    !Locator::animalSystem::has_value() ||
		    !Locator::animalSystem::value().SetupReactToFlyingObject(living, source.initiator, entry->body->Speed()))
		{
			return;
		}
		state.reaction = reaction.id;
		state.type = reaction.source.type;
		state.startTurn = _turn;
		Impress(reaction, living, registry.Get<const Transform>(living).position);
		return;
	}
	const bool wasReacting = state.reaction != 0;
	state.reaction = reaction.id;
	state.type = reaction.source.type;
	state.startTurn = _turn;
	Impress(reaction, living, registry.Get<const Transform>(living).position);
	if (registry.AllOf<Villager>(living))
	{
		villager_reactions::Start(living, reaction.source.type, state, wasReacting);
		return;
	}
	// A creature turns aside into a teleport stone
	if (source.type == Reaction::ReactToTeleport && registry.AllOf<Creature>(living) && Locator::teleportSystem::has_value())
	{
		Locator::teleportSystem::value().SetupReact(source.initiator, living);
		return;
	}
	if (!registry.AllOf<Creature>(living) || !Locator::creatureMindSystem::has_value())
	{
		return;
	}
	// A creature learns a miracle it reacts to: one another creature cast, or a player's, but not a computer player's, nor
	// for a nasty one the neutral player's
	auto& minds = Locator::creatureMindSystem::value();
	const bool byCreature = source.casterCreature != entt::null;
	const bool computer = source.player != PlayerNames::PLAYER_ONE && source.player != PlayerNames::NEUTRAL;
	// A shield, either kind, teaches the spiritual shield
	const auto magic = shield                                ? std::optional(static_cast<size_t>(MagicType::Shield))
	                   : source.magicType != MagicType::None ? std::optional(static_cast<size_t>(source.magicType))
	                                                         : std::nullopt;
	// A miracle from a seed teaches only the first creature that learns from it
	auto* seed = SeedOf(registry, source.initiator);
	const bool seedSpent = seed != nullptr && seed->learnedFrom;
	if (source.type == Reaction::FleeFromSpell)
	{
		const bool learnable = (byCreature || (!computer && source.player != PlayerNames::NEUTRAL)) && !seedSpent;
		minds.ReactToNastyMagic(living, source.position, learnable ? magic : std::nullopt);
		if (learnable && magic.has_value() && seed != nullptr)
		{
			seed->learnedFrom = true;
		}
	}
	else if ((nice || shield) && magic.has_value() && (byCreature || !computer) && !seedSpent)
	{
		// A seed's miracle a creature has already learnt from isn't looked at again
		minds.ReactToNiceMagic(living, source.position, magic);
		if (seed != nullptr)
		{
			seed->learnedFrom = true;
		}
	}
}

void ReactionSystem::Stop(entt::entity living, LivingReaction& state, bool resetState)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (state.reaction == 0)
	{
		return;
	}
	if (registry.AllOf<Villager>(living))
	{
		villager_reactions::Stop(living, state, resetState);
	}
	else if (registry.AllOf<Animal>(living) && Locator::animalSystem::has_value())
	{
		Locator::animalSystem::value().StopReaction(living);
	}
	state.reaction = 0;
	state.type = Reaction::None;
}

void ReactionSystem::ProcessLiving()
{
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<entt::entity> reacting;
	registry.Each<const LivingReaction>([&](entt::entity entity, const LivingReaction& state) {
		if (state.reaction != 0)
		{
			reacting.push_back(entity);
		}
	});
	for (const auto entity : reacting)
	{
		auto& state = registry.Get<LivingReaction>(entity);
		const auto reaction = Find(state.reaction);
		if (!reaction.has_value())
		{
			Stop(entity, state, false);
			continue;
		}
		const auto* info = InfoOf(state.type);
		// What it reacts to gone, it stops at once
		if (info == nullptr || !registry.AllOf<Transform>(entity) ||
		    (reaction->source.initiator != entt::null && !registry.Valid(reaction->source.initiator)))
		{
			Stop(entity, state, true);
			continue;
		}
		// Its time counts from when it last took up a reaction of the kind, the same wherever it stands
		const bool creature = registry.AllOf<Creature>(entity);
		const auto turns =
		    static_cast<int32_t>(creature ? info->numGameTurnsForCreatureToReact : info->numGameTurnsForNormalThingsToReact);
		const auto since =
		    static_cast<int32_t>(_turn - MemoryOf(registry, entity, state).LastReacted(static_cast<uint32_t>(state.type)));
		if (since > turns)
		{
			Stop(entity, state, true);
		}
	}
}

void ReactionSystem::ProcessTurn()
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	++_turn;
	for (auto& reaction : _reactions)
	{
		++reaction.age;
	}
	// A reaction goes once what made it has
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<uint32_t> gone;
	for (const auto& reaction : _reactions)
	{
		if (reaction.source.initiator != entt::null && !registry.Valid(reaction.source.initiator))
		{
			gone.push_back(reaction.id);
		}
	}
	for (const auto id : gone)
	{
		ShutDown(id);
	}
	// A reaction to a flying thing is where the thing is now
	for (auto& reaction : _reactions)
	{
		if (reaction.source.type != Reaction::ReactToFlyingObject || !registry.Valid(reaction.source.initiator))
		{
			continue;
		}
		if (const auto* transform = registry.TryGet<const Transform>(reaction.source.initiator))
		{
			reaction.source.position = transform->position;
		}
	}
	// One reaction a turn, in turn, grows, ends once its time is up, or is spread again
	if (!_reactions.empty())
	{
		_cursor %= _reactions.size();
		auto& reaction = _reactions.at(_cursor);
		++_cursor;
		const auto* info = InfoOf(reaction.source.type);
		if (info != nullptr && info->whetherReactionGrows != 0 && !reaction.source.reach.has_value())
		{
			reaction.reach += info->reactionGrowthPerGameTurn;
		}
		if (info != nullptr &&
		    magic::TimedOut(reaction.source.type, reaction.firstTaken, reaction.age, info->numGameTurnsForNormalThingsToReact))
		{
			ShutDown(reaction.id);
		}
		else
		{
			Spread(reaction);
		}
	}
	ProcessLiving();
	BelieveInTowns();
}

void ReactionSystem::BelieveInTowns()
{
	auto& registry = Locator::entitiesRegistry::value();
	const float decay = Locator::infoConstants::value().player.computerPlayerBeliefChangeDecay;
	const float lostTownScale = registry.Context().mapScriptGlobals.lostTownScale;
	registry.Each<TownImpression>([&](entt::entity entity, TownImpression& impression) {
		// Its boredom with each kind of reaction wears off, as long as it is still bored
		for (auto& [type, boredom] : impression.boredom)
		{
			if (const auto* info = InfoOf(type))
			{
				boredom = magic::BoredomAtTownTurn(boredom, info->additionToTownBoredomMultipliers, lostTownScale);
			}
		}
		const auto gained = magic::town_belief::Turn(impression.belief, decay);
		const auto* town = registry.TryGet<const Town>(entity);
		if (gained.empty() || town == nullptr || !Locator::particleSystem::has_value())
		{
			return;
		}
		// The belief gained rises as a symbol from the town centre's foot, in the believer's colour
		const auto centre = TownCentreOf(registry, town->id);
		if (!centre.has_value())
		{
			return;
		}
		for (const auto& [player, amount] : gained)
		{
			villager_reactions::ShowTownBelief(*centre, player, amount);
		}
	});
}

void ReactionSystem::Impress(const Active& reaction, entt::entity living, const glm::vec3& at)
{
	const auto* info = InfoOf(reaction.source.type);
	const auto& source = reaction.source;
	// What belongs to no player impresses no one
	if (info == nullptr || source.playerless)
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	// Measured across the land between map positions, as the game measures it
	const float distance = gutils::GetDistanceInMetres(at, source.position);
	// A reaction to food or wood impresses a villager by how much its town wants what it shows; one without a town by 1
	const auto multiplier = [&](entt::entity town) {
		std::optional<float> desire;
		if (info->correspondingTownDesire != TownDesireInfo::None && town != entt::null && registry.Valid(town))
		{
			if (const auto* wants = registry.TryGet<const TownDesire>(town))
			{
				desire = town_desire::GetDesire(*wants, static_cast<size_t>(info->correspondingTownDesire));
			}
		}
		return magic::ReactionMultiplier(source.type, info->defaultReactionImpressiveMultiplier, desire);
	};
	const auto value = [&](float boredom, entt::entity town) {
		return magic::ImpressiveValue({.landBalance = _landBalance,
		                               .impressiveValue = source.impressiveValue,
		                               .reactionMultiplier = multiplier(town),
		                               .distance = distance,
		                               .maxDistance = info->maxReactionDistance,
		                               .power = source.power,
		                               .boredom = boredom});
	};
	if (const auto* villager = registry.TryGet<const Villager>(living))
	{
		// A villager believes in the player, through its town, which tires of the same kind of thing
		if (villager->town == entt::null || !registry.Valid(villager->town))
		{
			return;
		}
		auto* impression = &villager_reactions::ImpressionOf(villager->town);
		auto& boredom = impression->boredom.try_emplace(source.type, 1.0f).first->second;
		const float share = villager_reactions::TownShare(villager->town);
		const float belief = value(boredom, villager->town) * share;
		magic::town_belief::Add(impression->belief, source.player, belief);
		impression->lastImpression = belief;
		villager_reactions::ShowBelief(living, source.player, belief, info->alignmentForSFX, _voice, _turn);
		// Whatever a villager is impressed by moves the player's alignment a little, by how much its town wants what it
		// shows for those kinds that look to a desire
		std::optional<float> desire;
		if (info->correspondingTownDesireForAlignment != TownDesireInfo::None)
		{
			if (const auto* wants = registry.TryGet<const TownDesire>(villager->town))
			{
				desire = town_desire::GetRawDesire(*wants, static_cast<size_t>(info->correspondingTownDesireForAlignment));
			}
		}
		if (Locator::alignmentSystem::has_value())
		{
			auto& alignment = Locator::alignmentSystem::value();
			alignment.AddPendingAlignment(
			    source.player, magic::DampAlignmentChange(magic::ImpressionAlignment(info->alignmentModifier, desire),
			                                              alignment.GetPlayerAlignment(source.player)));
		}
		// The town tires of the kind of thing, by its share of the town
		if (Locator::infoConstants::has_value())
		{
			boredom = magic::BoredomAfterImpression(boredom, Locator::infoConstants::value().belief.defaultBoredomOfMe * share);
		}
		return;
	}
	if (const auto* creature = registry.TryGet<const Creature>(living))
	{
		// A creature is impressed by what its own player does, by a creature's hand or the player's, and by what another
		// creature itself does; a miracle another player made impresses it not at all. Its own doings don't impress it.
		if (living == source.initiator)
		{
			return;
		}
		// A creature has no town
		const float base = value(1.0f, entt::null);
		auto* impression = registry.TryGet<CreatureImpression>(living);
		if (impression == nullptr)
		{
			impression = &registry.Assign<CreatureImpression>(living);
		}
		if (creature->owner == source.player)
		{
			impression->byOwnPlayer += base * magic::k_CreatureImpressedByOwnPlayer;
		}
		else if (registry.Valid(source.initiator) && registry.AllOf<Creature>(source.initiator))
		{
			impression->byOtherCreatures += base * magic::k_CreatureImpressedByOtherCreature;
		}
	}
}

void ReactionSystem::Reset()
{
	_reactions.clear();
	_cursor = 0;
	_turn = 0;
	_voice = {};
	_landBalance = 1.0f;
}

std::vector<ReactionSystemInterface::Active> ReactionSystem::ReactionsAt(const glm::vec3& point) const
{
	std::vector<Active> found;
	std::ranges::copy_if(_reactions, std::back_inserter(found), [&point](const Active& reaction) {
		return glm::distance(point, reaction.source.position) <= reaction.reach;
	});
	return found;
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "TeleportSystem.h"

#include <algorithm>
#include <chrono>
#include <limits>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <spdlog/spdlog.h>

#include "3D/LandIslandInterface.h"
#include "Audio/Sound.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/AnimatedStatic.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureLocomotion.h"
#include "ECS/Components/CreatureSpells.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Flowers.h"
#include "ECS/Components/Forest.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/LivingReaction.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Spell.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/TeleportStone.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/WallHug.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "ECS/Systems/SoundTagSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/TeleportRules.h"
#include "VillagerTeleport.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace teleport = openblack::magic::teleport;

namespace
{
/// A creature walks to its stone and jumps once this close
constexpr float k_CreatureArriveDistance = teleport::k_CreatureArriveDistance;
/// A walk's destination this close to a stone was the walk to it
constexpr float k_SameStone = 0.01f;
/// A game turn's milliseconds
constexpr float k_TurnMilliseconds =
    std::chrono::duration<float, std::milli>(ecs::systems::TimeSystemInterface::k_TurnDuration).count();

ecs::Registry& EntityRegistry()
{
	return Locator::entitiesRegistry::value();
}

float LandHeight(glm::vec2 xz)
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(xz) : 0.0f;
}

glm::vec3 OnLand(glm::vec2 xz)
{
	return {xz.x, LandHeight(xz), xz.y};
}

float Across(glm::vec3 a, glm::vec3 b)
{
	return glm::distance(glm::vec2(a.x, a.z), glm::vec2(b.x, b.z));
}

/// The player a villager or creature belongs to
std::optional<PlayerNames> PlayerOf(entt::entity living)
{
	const auto& registry = EntityRegistry();
	if (const auto* creature = registry.TryGet<const Creature>(living))
	{
		return creature->owner;
	}
	if (const auto* villager = registry.TryGet<const Villager>(living))
	{
		if (villager->town != entt::null && registry.Valid(villager->town))
		{
			if (const auto* town = registry.TryGet<const Town>(villager->town))
			{
				return town->owner;
			}
		}
	}
	return std::nullopt;
}

/// How far a stone's reaction reaches, from the reaction table
float ReactionReach()
{
	if (!Locator::infoConstants::has_value())
	{
		return teleport::k_StoneRadius;
	}
	const auto& table = Locator::infoConstants::value().reaction;
	const auto index = static_cast<size_t>(Reaction::ReactToTeleport);
	return index < table.size() ? table.at(index).maxReactionDistance : teleport::k_StoneRadius;
}

/// A villager walking somewhere, and where
std::optional<glm::vec3> VillagerWalkingTo(entt::entity villager)
{
	const auto& registry = EntityRegistry();
	const auto* action = registry.TryGet<const LivingAction>(villager);
	const auto* wallHug = registry.TryGet<const WallHug>(villager);
	if (action == nullptr || wallHug == nullptr)
	{
		return std::nullopt;
	}
	const auto state = Locator::livingActionSystem::value().VillagerGetState(*action, LivingAction::Index::Top);
	const bool moving =
	    registry.AnyOf<MoveStateLinearTag, MoveStateOrbitTag, MoveStateExitCircleTag, MoveStateStepThroughTag>(villager);
	if (state != VillagerStates::MoveToPos || !moving)
	{
		return std::nullopt;
	}
	return OnLand(wallHug->goal);
}

/// A creature walking somewhere, and where
std::optional<glm::vec3> CreatureWalkingTo(entt::entity creature)
{
	const auto* locomotion = EntityRegistry().TryGet<const CreatureLocomotion>(creature);
	if (locomotion == nullptr || !locomotion->destination.has_value() || !Locator::creatureLocomotionSystem::has_value() ||
	    !Locator::creatureLocomotionSystem::value().IsMoving(creature))
	{
		return std::nullopt;
	}
	return OnLand(*locomotion->destination);
}

/// A villager whose stone went stops reacting to it, taking up again what it was doing
void ResumeVillager(entt::entity villager, const TeleportTraveller& traveller)
{
	auto& registry = EntityRegistry();
	if (auto* action = registry.TryGet<LivingAction>(villager))
	{
		registry.AssignOrReplace<TeleportTraveller>(villager, traveller);
		ecs::villager_teleport::StopReacting(*action, villager);
	}
}
} // namespace

entt::entity TeleportSystem::CreateStone(glm::vec3 point, PlayerNames player, entt::entity spell)
{
	if (!CanPlaceStone(point))
	{
		return entt::null;
	}
	auto& registry = EntityRegistry();
	const auto position = OnLand({point.x, point.z});
	const auto stone = registry.Create();
	registry.Assign<Transform>(stone, position, glm::mat3(1.0f), glm::vec3(1.0f));
	auto& component = registry.Assign<TeleportStone>(stone);
	component.player = player;
	component.spell = spell;
	component.serial = _nextSerial++;
	// The passers-by react to it
	if (Locator::reactionSystem::has_value())
	{
		component.reaction = Locator::reactionSystem::value().Create(
		    {.initiator = stone, .type = Reaction::ReactToTeleport, .player = player, .position = position});
	}
	// Its swirling pool, with its hum
	if (Locator::particleSystem::has_value())
	{
		auto& particles = Locator::particleSystem::value();
		component.pool = particles.Start(ParticleType::TeleportVortex, position, 1.0f, true);
		if (component.pool != ParticleSystemInterface::k_NoEffect)
		{
			particles.SetPlayer(component.pool, static_cast<int>(player));
		}
	}
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Teleport: stone of player {} at ({}, {})", static_cast<int>(player), position.x,
	                    position.z);
	return stone;
}

void TeleportSystem::RemoveStone(entt::entity stone)
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(stone))
	{
		return;
	}
	if (const auto* component = registry.TryGet<const TeleportStone>(stone))
	{
		if (Locator::reactionSystem::has_value())
		{
			Locator::reactionSystem::value().RemoveFrom(stone);
		}
		// The pool goes and its hum fades away
		if (component->pool != ParticleSystemInterface::k_NoEffect && Locator::particleSystem::has_value())
		{
			Locator::particleSystem::value().Delete(component->pool);
		}
	}
	// Whoever was on the way to it carries on where they were going
	std::vector<std::pair<entt::entity, TeleportTraveller>> stranded;
	registry.Each<const TeleportTraveller>([&](entt::entity living, const TeleportTraveller& traveller) {
		if (traveller.stone == stone)
		{
			stranded.emplace_back(living, traveller);
		}
	});
	for (const auto& [living, traveller] : stranded)
	{
		registry.Remove<TeleportTraveller>(living);
		if (registry.AllOf<Villager>(living) && !traveller.jumped)
		{
			ResumeVillager(living, traveller);
		}
	}
	registry.Destroy(stone);
}

bool TeleportSystem::CanPlaceStone(glm::vec3 point) const
{
	// Anything that stands fixed over the land's cells: buildings and fields, features, the things that stand still and
	// dead trees, big forests, the temple, the miracle dispensers and other stones. (The game counts worship sites,
	// totems and football pitches too; openblack has none yet.)
	std::vector<glm::vec3> fixed;
	const auto& registry = EntityRegistry();
	const auto add = [&fixed](entt::entity, const Transform& transform) { fixed.push_back(transform.position); };
	registry.Each<const Abode, const Transform>([&](entt::entity e, const Abode&, const Transform& t) { add(e, t); });
	registry.Each<const Field, const Transform>([&](entt::entity e, const Field&, const Transform& t) { add(e, t); });
	registry.Each<const Feature, const Transform>([&](entt::entity e, const Feature&, const Transform& t) { add(e, t); });
	registry.Each<const AnimatedStatic, const Transform>(
	    [&](entt::entity e, const AnimatedStatic&, const Transform& t) { add(e, t); });
	registry.Each<const Flowers, const Transform>([&](entt::entity e, const Flowers&, const Transform& t) { add(e, t); });
	registry.Each<const MobileStatic, const Transform>(
	    [&](entt::entity e, const MobileStatic&, const Transform& t) { add(e, t); });
	registry.Each<const DeadTree, const Transform>([&](entt::entity e, const DeadTree&, const Transform& t) { add(e, t); });
	registry.Each<const BigForest, const Transform>([&](entt::entity e, const BigForest&, const Transform& t) { add(e, t); });
	registry.Each<const Temple, const Transform>([&](entt::entity e, const Temple&, const Transform& t) { add(e, t); });
	registry.Each<const SpellDispenser, const Transform>(
	    [&](entt::entity e, const SpellDispenser&, const Transform& t) { add(e, t); });
	registry.Each<const TeleportStone, const Transform>(
	    [&](entt::entity e, const TeleportStone&, const Transform& t) { add(e, t); });
	return teleport::CanPlaceStone(point, fixed);
}

bool TeleportSystem::BlocksNewBuilding(glm::vec3 point) const
{
	bool blocked = false;
	EntityRegistry().Each<const TeleportStone, const Transform>([&](const TeleportStone&, const Transform& transform) {
		blocked = blocked || Across(point, transform.position) < teleport::k_StoneRadius;
	});
	return blocked;
}

std::vector<entt::entity> TeleportSystem::GetStones(PlayerNames player) const
{
	std::vector<std::pair<uint32_t, entt::entity>> found;
	EntityRegistry().Each<const TeleportStone>([&](entt::entity entity, const TeleportStone& stone) {
		if (stone.player == player)
		{
			found.emplace_back(stone.serial, entity);
		}
	});
	std::ranges::sort(found, std::greater<> {}, &std::pair<uint32_t, entt::entity>::first);
	std::vector<entt::entity> stones;
	stones.reserve(found.size());
	std::ranges::transform(found, std::back_inserter(stones), &std::pair<uint32_t, entt::entity>::second);
	return stones;
}

std::optional<entt::entity> TeleportSystem::StoneOf(entt::entity spell) const
{
	std::optional<entt::entity> found;
	EntityRegistry().Each<const TeleportStone>([&](entt::entity entity, const TeleportStone& stone) {
		if (stone.spell == spell)
		{
			found = entity;
		}
	});
	return found;
}

void TeleportSystem::SetupReact(entt::entity stone, entt::entity living)
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(stone) || !registry.Valid(living) || !registry.AllOf<TeleportStone>(stone))
	{
		return;
	}
	const bool isVillager = registry.AllOf<Villager>(living);
	const auto destination = isVillager ? VillagerWalkingTo(living) : CreatureWalkingTo(living);
	if (!destination.has_value())
	{
		return;
	}
	ReactTo(stone, living, *destination);
}

void TeleportSystem::ReactTo(entt::entity stone, entt::entity living, glm::vec3 destination)
{
	auto& registry = EntityRegistry();
	auto& component = registry.Get<TeleportStone>(stone);
	const bool isVillager = registry.AllOf<Villager>(living);
	TeleportTraveller traveller {.stone = stone, .destination = destination};
	if (isVillager)
	{
		const auto& action = registry.Get<const LivingAction>(living);
		const auto final = Locator::livingActionSystem::value().VillagerGetState(action, LivingAction::Index::Final);
		traveller.finalState = final == VillagerStates::InvalidState ? VillagerStates::DecideWhatToDo : final;
		// What it comes back to once it is through
		traveller.previousState = ecs::villager_teleport::PreviousToKeep(final);
	}
	// Listed afresh at the head of the stone's travellers
	std::erase_if(component.travellers, [living](const auto& entry) { return entry.living == living; });
	component.travellers.insert(component.travellers.begin(),
	                            {.living = living, .destination = destination, .finalState = traveller.finalState});
	registry.AssignOrReplace<TeleportTraveller>(living, traveller);
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Teleport: #{} turns aside into the stone at ({}, {})",
	                    static_cast<uint32_t>(living), destination.x, destination.z);
	// It holds the stone's reaction while it is on its way, however it came to turn aside
	if (!registry.AllOf<LivingReaction>(living))
	{
		registry.Assign<LivingReaction>(living);
	}
	auto& reacting = registry.Get<LivingReaction>(living);
	if (reacting.reaction != component.reaction)
	{
		reacting.reaction = component.reaction;
		reacting.type = Reaction::ReactToTeleport;
	}
	const auto stonePosition = registry.Get<const Transform>(stone).position;
	if (isVillager)
	{
		// It walks to the stone, or runs when it was already going faster than its walk
		auto& action = registry.Get<LivingAction>(living);
		const auto& villager = registry.Get<const Villager>(living);
		bool quickly = false;
		if (Locator::infoConstants::has_value())
		{
			const auto& info = Locator::infoConstants::value().villager;
			const auto kind = static_cast<size_t>(GVillagerInfo::Find(villager.tribe, villager.number));
			if (kind < info.size())
			{
				// Against the third of its kind's speeds
				quickly = registry.Get<const WallHug>(living).speed > GetSpeedStateSpeed(info.at(kind).speedGroup.speed2);
			}
		}
		Locator::livingActionSystem::value().VillagerSetState(
		    action, LivingAction::Index::Top,
		    quickly ? VillagerStates::GoTowardsTeleportReactionQuickly : VillagerStates::GoTowardsTeleportReaction, false);
	}
	else if (Locator::creatureLocomotionSystem::has_value())
	{
		Locator::creatureLocomotionSystem::value().MoveTo(living, {stonePosition.x, stonePosition.z},
		                                                  CreatureLocomotionSystemInterface::Pace::Walk, 0.0f,
		                                                  k_CreatureArriveDistance);
	}
}

bool TeleportSystem::DoTeleport(entt::entity stone, entt::entity living, bool forced)
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(stone) || !registry.Valid(living) || !registry.AllOf<TeleportStone, Transform>(stone) ||
	    !registry.AllOf<Transform>(living))
	{
		return false;
	}
	auto& component = registry.Get<TeleportStone>(stone);
	const auto node = std::ranges::find(component.travellers, living, &TeleportStone::Traveller::living);
	if (node == component.travellers.end())
	{
		return false;
	}
	const auto destination = node->destination;
	// The player's other stones, the newest first
	const auto stones = GetStones(component.player);
	std::vector<glm::vec3> positions;
	positions.reserve(stones.size());
	for (const auto entity : stones)
	{
		positions.push_back(registry.Get<const Transform>(entity).position);
	}
	const auto from = static_cast<size_t>(std::ranges::find(stones, stone) - stones.begin());
	const auto departure = registry.Get<const Transform>(living).position;
	const auto jump = teleport::ChooseTarget(departure, destination, positions, from, forced);
	component.travellers.erase(node);
	if (!jump.has_value())
	{
		return false;
	}
	const auto arrival = positions.at(jump->stone);
	const auto stonePosition = registry.Get<const Transform>(stone).position;
	// The miracle acts at the stone, then is paid back for the way saved, or pays for a backwards jump
	if (Locator::magicSystem::has_value() && component.spell != entt::null && registry.Valid(component.spell))
	{
		auto& magic = Locator::magicSystem::value();
		magic.SpellEvent(component.spell, {.type = particles::SpellEventInfo::Type::Point,
		                                   .position = stonePosition,
		                                   .velocity = glm::vec3(0.0f),
		                                   .strength = 1.0f,
		                                   .checkShields = false,
		                                   .target = entt::null});
		const float costPerKilometre =
		    Locator::infoConstants::has_value() ? Locator::infoConstants::value().magicTeleport.at(0).costPerKilometer : 0.0f;
		// Paid as the game forces it: a recharging caster is asked for the whole shortfall, so a refund tops it up
		magic.ForcePayForSpell(component.spell, teleport::JumpCost(jump->saving, costPerKilometre));
	}
	// A flash and a sound where it leaves and where it comes out
	if (Locator::particleSystem::has_value())
	{
		auto& particles = Locator::particleSystem::value();
		particles.StartSpotVisual(SpotVisualType::VillagerTeleport, departure, std::nullopt, entt::null);
		particles.StartSpotVisual(SpotVisualType::VillagerTeleport, arrival, std::nullopt, entt::null);
	}
	if (registry.AllOf<Creature>(living))
	{
		// A creature is carried: it fades out where it stands, comes out with a sound and fades back in
		if (auto* traveller = registry.TryGet<TeleportTraveller>(living))
		{
			traveller->arrival = arrival;
			traveller->transported = true;
			traveller->fadingIn = false;
			traveller->transportTurns = teleport::CreatureFadeTurns(k_TurnMilliseconds);
		}
		if (Locator::creatureLocomotionSystem::has_value())
		{
			Locator::creatureLocomotionSystem::value().Stop(living);
		}
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Teleport: carrying creature {} m closer from ({}, {}) to ({}, {})",
		                    jump->saving, departure.x, departure.z, arrival.x, arrival.z);
		return true;
	}
	if (Locator::soundTagSystem::has_value())
	{
		auto& sounds = Locator::soundTagSystem::value();
		sounds.CreatePointSound(static_cast<entt::id_type>(audio::SoundId::G_SpellTeleportEnergiseGo), departure, false);
		sounds.CreatePointSound(static_cast<entt::id_type>(audio::SoundId::G_SpellTeleportEnergiseArrive), arrival, false);
	}
	// At once, without a step between
	registry.Get<Transform>(living).position = arrival;
	registry.SetDirty();
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Teleport: jumped {} m closer from ({}, {}) to ({}, {})", jump->saving,
	                    departure.x, departure.z, arrival.x, arrival.z);
	return true;
}

bool TeleportSystem::DropOnStone(entt::entity villager, entt::entity stone, PlayerNames dropper)
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(villager) || !registry.Valid(stone) || !registry.AllOf<Villager, LivingAction, WallHug>(villager) ||
	    !registry.AllOf<TeleportStone>(stone))
	{
		return false;
	}
	const auto& component = registry.Get<const TeleportStone>(stone);
	// The hand that dropped it must be the stone's player's, whoever the villager belongs to
	if (!teleport::CanDropOnStone(dropper == component.player, GetStones(component.player).size()))
	{
		return false;
	}
	// Put down on the stone, it decides there and then what to do, then jumps towards where that takes it, whatever the
	// way, and decides again once through. (The game first has it fly and land from the hand; openblack's hand doesn't
	// hold villagers.)
	auto& action = registry.Get<LivingAction>(villager);
	registry.Get<Transform>(villager).position = registry.Get<const Transform>(stone).position;
	auto& living = Locator::livingActionSystem::value();
	const auto decide = [&] {
		living.VillagerSetState(action, LivingAction::Index::Top, VillagerStates::DecideWhatToDo, true);
		living.VillagerCallState(action, LivingAction::Index::Top);
	};
	decide();
	const auto& wallHug = registry.Get<const WallHug>(villager);
	auto& listed = registry.Get<TeleportStone>(stone);
	std::erase_if(listed.travellers, [villager](const auto& entry) { return entry.living == villager; });
	listed.travellers.insert(listed.travellers.begin(), {.living = villager, .destination = OnLand(wallHug.goal)});
	if (!DoTeleport(stone, villager, true))
	{
		return false;
	}
	decide();
	return true;
}

std::optional<entt::entity> TeleportSystem::StoneAlong(glm::vec3 origin, glm::vec3 direction) const
{
	std::optional<entt::entity> nearest;
	float best = std::numeric_limits<float>::max();
	EntityRegistry().Each<const TeleportStone, const Transform>(
	    [&](entt::entity entity, const TeleportStone&, const Transform& transform) {
		    const float along = glm::dot(transform.position - origin, direction);
		    if (along <= 0.0f)
		    {
			    return;
		    }
		    const auto closest = origin + direction * along;
		    if (glm::distance(closest, transform.position) < teleport::k_HandTouchRadius && along < best)
		    {
			    best = along;
			    nearest = entity;
		    }
	    });
	return nearest;
}

bool TeleportSystem::RouteWorshipper(entt::entity villager, glm::vec3 site)
{
	auto& registry = EntityRegistry();
	const auto player = PlayerOf(villager);
	auto* action = registry.TryGet<LivingAction>(villager);
	auto* wallHug = registry.TryGet<WallHug>(villager);
	if (!player.has_value() || action == nullptr || wallHug == nullptr || !Locator::infoConstants::has_value())
	{
		return false;
	}
	// A site within the distance villagers go to worship is walked to; a further one through the stones, if that is
	// shorter than the distance
	const float maxDistance = Locator::infoConstants::value().town.maxDistanceThatVillagersWillGoToWorship;
	const auto position = registry.Get<const Transform>(villager).position;
	if (Across(position, site) <= maxDistance)
	{
		return false;
	}
	const auto stone = RouteStoneFor(*player, position, site, maxDistance);
	if (!stone.has_value())
	{
		return false;
	}
	// On its way to worship, arriving at the site at the end, it turns aside into the stone
	wallHug->goal = {site.x, site.z};
	auto& living = Locator::livingActionSystem::value();
	living.VillagerSetState(*action, LivingAction::Index::Final, VillagerStates::ArrivesAtWorshipSiteForWorship, false);
	living.VillagerSetState(*action, LivingAction::Index::Top, VillagerStates::GotoWorshipSiteForWorship, false);
	ReactTo(*stone, villager, site);
	return true;
}

std::optional<entt::entity> TeleportSystem::RouteStoneFor(PlayerNames player, glm::vec3 worshipper, glm::vec3 site,
                                                          float maxDistance) const
{
	const auto stones = GetStones(player);
	std::vector<glm::vec3> positions;
	positions.reserve(stones.size());
	for (const auto entity : stones)
	{
		positions.push_back(EntityRegistry().Get<const Transform>(entity).position);
	}
	if (const auto found = teleport::FindRouteStone(worshipper, site, positions, maxDistance))
	{
		return stones.at(*found);
	}
	return std::nullopt;
}

void TeleportSystem::ProcessTurn()
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	RemoveOrphans();
	PruneTravellers();
	MoveCreatures();
}

void TeleportSystem::RemoveOrphans()
{
	auto& registry = EntityRegistry();
	std::vector<entt::entity> orphans;
	registry.Each<const TeleportStone>([&](entt::entity entity, const TeleportStone& stone) {
		const auto* spell =
		    stone.spell != entt::null && registry.Valid(stone.spell) ? registry.TryGet<const Spell>(stone.spell) : nullptr;
		if (spell == nullptr || spell->closedDown)
		{
			orphans.push_back(entity);
		}
	});
	for (const auto stone : orphans)
	{
		RemoveStone(stone);
	}
}

void TeleportSystem::PruneTravellers()
{
	auto& registry = EntityRegistry();
	std::vector<entt::entity> dropped;
	registry.Each<TeleportStone>([&](entt::entity entity, TeleportStone& stone) {
		std::erase_if(stone.travellers, [&](const TeleportStone::Traveller& traveller) {
			if (!registry.Valid(traveller.living))
			{
				return true;
			}
			const auto* heading = registry.TryGet<const TeleportTraveller>(traveller.living);
			if (heading == nullptr || heading->stone != entity || heading->jumped)
			{
				return true;
			}
			// One that took up another reaction since is no longer on its way to the stone
			const auto* reacting = registry.TryGet<const LivingReaction>(traveller.living);
			if (reacting != nullptr && reacting->reaction != stone.reaction && !heading->transported)
			{
				dropped.push_back(traveller.living);
				return true;
			}
			return false;
		});
	});
	for (const auto living : dropped)
	{
		registry.Remove<TeleportTraveller>(living);
	}
}

bool TeleportSystem::ShouldReact(entt::entity stone, entt::entity living) const
{
	const auto& registry = EntityRegistry();
	if (!registry.Valid(stone) || !registry.Valid(living) || !registry.AllOf<TeleportStone, Transform>(stone) ||
	    !registry.AllOf<Transform>(living))
	{
		return false;
	}
	// Only on its way somewhere
	const auto destination = registry.AllOf<Villager>(living) ? VillagerWalkingTo(living) : CreatureWalkingTo(living);
	if (!destination.has_value())
	{
		return false;
	}
	// Through the stone and any other of its player's stones
	const auto& component = registry.Get<const TeleportStone>(stone);
	std::vector<glm::vec3> network {registry.Get<const Transform>(stone).position};
	registry.Each<const TeleportStone, const Transform>(
	    [&](entt::entity other, const TeleportStone& otherStone, const Transform& transform) {
		    if (other != stone && otherStone.player == component.player)
		    {
			    network.push_back(transform.position);
		    }
	    });
	return teleport::ShouldReact(registry.Get<const Transform>(living).position, *destination, network, 0);
}

void TeleportSystem::MoveCreatures()
{
	if (!Locator::creatureLocomotionSystem::has_value())
	{
		return;
	}
	auto& registry = EntityRegistry();
	auto& locomotion = Locator::creatureLocomotionSystem::value();
	std::vector<entt::entity> done;
	std::vector<entt::entity> arrived;
	std::vector<entt::entity> carried;
	registry.Each<const Creature, const TeleportTraveller, const Transform>(
	    [&](entt::entity creature, const Creature&, const TeleportTraveller& traveller, const Transform& transform) {
		    if (traveller.transported)
		    {
			    carried.push_back(creature);
			    return;
		    }
		    if (traveller.jumped)
		    {
			    // On its way after the jump, it doesn't turn aside again until it gets there
			    if (!locomotion.IsMoving(creature))
			    {
				    done.push_back(creature);
			    }
			    return;
		    }
		    if (!registry.Valid(traveller.stone) || !registry.AllOf<Transform>(traveller.stone))
		    {
			    done.push_back(creature);
			    return;
		    }
		    const auto stone = registry.Get<const Transform>(traveller.stone).position;
		    if (Across(transform.position, stone) <= k_CreatureArriveDistance)
		    {
			    arrived.push_back(creature);
		    }
		    else if (!locomotion.IsMoving(creature))
		    {
			    // Its walk to the stone ended there, or something else took it off its way
			    const auto* moving = registry.TryGet<const CreatureLocomotion>(creature);
			    const bool reached = moving != nullptr && !moving->failed &&
			                         (!moving->destination.has_value() ||
			                          glm::distance(*moving->destination, glm::vec2(stone.x, stone.z)) < k_SameStone);
			    (reached ? arrived : done).push_back(creature);
		    }
	    });
	for (const auto creature : done)
	{
		registry.Remove<TeleportTraveller>(creature);
		// Through, or taken off its way, it no longer reacts to the stone
		if (auto* reacting = registry.TryGet<LivingReaction>(creature))
		{
			reacting->reaction = 0;
			reacting->type = Reaction::None;
		}
	}
	for (const auto creature : arrived)
	{
		auto& traveller = registry.Get<TeleportTraveller>(creature);
		if (!DoTeleport(traveller.stone, creature, false))
		{
			registry.Remove<TeleportTraveller>(creature);
		}
	}
	for (const auto creature : carried)
	{
		Carry(creature);
	}
}

void TeleportSystem::Carry(entt::entity creature)
{
	auto& registry = EntityRegistry();
	auto& traveller = registry.Get<TeleportTraveller>(creature);
	auto* spells = registry.TryGet<CreatureSpells>(creature);
	const auto turns = teleport::CreatureFadeTurns(k_TurnMilliseconds);
	Locator::creatureLocomotionSystem::value().Stop(creature);
	--traveller.transportTurns;
	if (!traveller.fadingIn)
	{
		if (traveller.transportTurns > 0)
		{
			if (spells != nullptr)
			{
				spells->fizz = teleport::CreatureFadeOut(traveller.transportTurns, turns);
			}
			return;
		}
		// Faded out, it comes out of the far stone with the sound of arriving, still faded
		if (Locator::soundTagSystem::has_value())
		{
			Locator::soundTagSystem::value().CreatePointSound(
			    static_cast<entt::id_type>(audio::SoundId::G_SpellTeleportEnergiseArrive), traveller.arrival, false);
		}
		const auto at = OnLand({traveller.arrival.x, traveller.arrival.z});
		registry.Get<Transform>(creature).position = at;
		if (auto* moving = registry.TryGet<CreatureLocomotion>(creature))
		{
			moving->fromPosition = at;
			moving->toPosition = at;
		}
		registry.SetDirty();
		traveller.fadingIn = true;
		traveller.transportTurns = turns;
		return;
	}
	if (traveller.transportTurns > 0)
	{
		if (spells != nullptr)
		{
			spells->fizz = teleport::CreatureFadeIn(traveller.transportTurns, turns);
		}
		return;
	}
	// Back in sight, on to where it was going
	if (spells != nullptr)
	{
		spells->fizz = 0.0f;
	}
	traveller.transported = false;
	traveller.jumped = true;
	Locator::creatureLocomotionSystem::value().MoveTo(creature, {traveller.destination.x, traveller.destination.z},
	                                                  CreatureLocomotionSystemInterface::Pace::Walk, 0.0f,
	                                                  teleport::k_CreatureArriveDistance);
}

void TeleportSystem::Reset()
{
	_nextSerial = 0;
}

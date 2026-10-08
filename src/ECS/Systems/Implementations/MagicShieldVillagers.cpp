/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The villagers' reactions to the shields: a shield standing, being struck and being destroyed. Each reaction is spread
// to the villagers in its reach as it is made and then whenever its turn comes round among all the land's reactions;
// a villager takes one up by its priority and cooldown, and keeps it for as long as its kind says.

#define LOCATOR_IMPLEMENTATIONS

#include <cmath>

#include <algorithm>
#include <bit>
#include <numbers>

#include <glm/geometric.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <spdlog/spdlog.h>

#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Common/GameRandom.h"
#include "ECS/Components/AtHome.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/LivingReaction.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/Spell.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/TownAggression.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/WallHug.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/MagicTables.h"
#include "Magic/ShieldRules.h"
#include "Magic/VillagerReactionRules.h"
#include "MagicShieldSystem.h"
#include "VillagerHome.h"
#include "VillagerShieldShelter.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace shield = openblack::magic::shield;
namespace villager_reaction = openblack::magic::villager_reaction;

namespace
{
/// A villager switches to a more urgent reaction once it has been at its last kind this many seconds
constexpr uint32_t k_SwitchSeconds = 10;
constexpr uint32_t k_TurnsPerSecond = 10;
/// A shield's villager runs from its fall in a direction away from it, turned by up to an eighth of a half turn either
/// way
constexpr float k_FleeTurnRange = std::numbers::pi_v<float> / 4.0f;

ecs::Registry& EntityRegistry()
{
	return Locator::entitiesRegistry::value();
}

const InfoConstants& Info()
{
	return Locator::infoConstants::value();
}

uint32_t GameTurn()
{
	return Locator::time::has_value() ? static_cast<uint32_t>(Locator::time::value().GetTurn()) : 0;
}

const ReactionInfo& RowOf(Reaction type)
{
	return Info().reaction.at(static_cast<size_t>(type));
}

villager_reaction::Distance DistanceOf(Reaction type)
{
	const auto& row = RowOf(type);
	return {.maxDistance = row.maxReactionDistance, .importance = row.howImportantIsDistance};
}

const GVillagerInfo& VillagerInfoOf(const Villager& villager)
{
	return Info().villager.at(static_cast<size_t>(GVillagerInfo::Find(villager.tribe, villager.number)));
}

/// Whether a villager's kind takes up a kind of reaction at all
bool KindReacts(const Villager& villager, Reaction type)
{
	constexpr size_t k_Kinds = sizeof(IsReacting) / sizeof(uint32_t);
	const auto flags = std::bit_cast<std::array<uint32_t, k_Kinds>>(VillagerInfoOf(villager).isReacting);
	const auto index = static_cast<size_t>(type);
	return index < flags.size() && flags.at(index) != 0;
}

/// A villager's town, its significance of protection and the turns since it was last attacked
struct TownState
{
	bool hasTown {false};
	float protection {0.0f};
	uint32_t sinceAttacked {0};
};
TownState TownStateOf(const Villager& villager)
{
	auto& registry = EntityRegistry();
	TownState state;
	if (villager.town == entt::null || !registry.Valid(villager.town))
	{
		return state;
	}
	state.hasTown = true;
	state.protection = ecs::villager_shield::ProtectionSignificance(villager.town);
	const auto* aggression = registry.TryGet<const TownAggression>(villager.town);
	state.sinceAttacked = GameTurn() - (aggression != nullptr ? aggression->record.lastTurn : 0u);
	return state;
}

/// The shield a reaction is to, while it stands with its miracle
const MagicShield* StandingShield(entt::entity object)
{
	auto& registry = EntityRegistry();
	if (object == entt::null || !registry.Valid(object))
	{
		return nullptr;
	}
	const auto* found = registry.TryGet<const MagicShield>(object);
	return found != nullptr && found->spell != entt::null ? found : nullptr;
}

glm::vec2 PositionOf(entt::entity entity)
{
	return glm::xz(EntityRegistry().Get<const Transform>(entity).position);
}
} // namespace

uint32_t MagicShieldSystem::CreateReaction(entt::entity object, Reaction type, glm::vec3 position, float multiplier)
{
	auto& registry = EntityRegistry();
	const auto& shieldObject = registry.Get<const MagicShield>(object);
	const auto magicType = shieldObject.kind == MagicShield::Kind::Physical ? MagicType::PhysicalShield : MagicType::Shield;
	uint32_t id = 0;
	if (Locator::reactionSystem::has_value())
	{
		id = Locator::reactionSystem::value().Create(
		    {.initiator = object,
		     .type = type,
		     .player = shieldObject.player,
		     .position = position,
		     .impressiveValue = magic::GetMagicEffectInfo(Info(), magicType).impressiveValue * multiplier,
		     .power = 1.0f,
		     .magicType = MagicType::None,
		     .casterCreature = entt::null,
		     .reach = std::nullopt});
	}
	_reactions.insert_or_assign(
	    id, ShieldReaction {
	            .object = object, .type = type, .radius = RowOf(type).maxReactionDistance, .firstReacted = 0, .reacting = {}});
	// Spread once at once
	SpreadReaction(id);
	return id;
}

void MagicShieldSystem::RemoveReaction(uint32_t id, bool setState)
{
	const auto found = _reactions.find(id);
	if (found == _reactions.end())
	{
		return;
	}
	const auto reacting = found->second.reacting;
	for (const auto villager : reacting)
	{
		StopReacting(villager, setState);
	}
	_reactions.erase(id);
	if (Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().Remove(id);
	}
}

void MagicShieldSystem::RemoveReactions(entt::entity object, Reaction type)
{
	std::vector<uint32_t> ids;
	for (const auto& [id, reaction] : _reactions)
	{
		if (reaction.object == object && reaction.type == type)
		{
			ids.push_back(id);
		}
	}
	for (const auto id : ids)
	{
		RemoveReaction(id, true);
	}
}

void MagicShieldSystem::ProcessReactions()
{
	if (!Locator::reactionSystem::has_value())
	{
		return;
	}
	// One of the land's reactions a turn, in turn
	const auto all = Locator::reactionSystem::value().GetReactions();
	if (all.empty())
	{
		return;
	}
	_cursor %= all.size();
	const auto id = all[_cursor].id;
	++_cursor;
	const auto found = _reactions.find(id);
	if (found == _reactions.end())
	{
		return;
	}
	auto& reaction = found->second;
	// Being struck runs out once its people have had their turns since the first of them took it up
	if (reaction.type == Reaction::ReactToMagicShieldStruck && reaction.firstReacted != 0 &&
	    GameTurn() - reaction.firstReacted > RowOf(reaction.type).numGameTurnsForNormalThingsToReact)
	{
		RemoveReaction(id, true);
		return;
	}
	SpreadReaction(id);
}

void MagicShieldSystem::SpreadReaction(uint32_t id)
{
	if (!Locator::entitiesMap::has_value())
	{
		return;
	}
	auto& registry = EntityRegistry();
	const auto& reaction = _reactions.at(id);
	if (!registry.Valid(reaction.object))
	{
		return;
	}
	const auto centre = PositionOf(reaction.object);
	const auto& map = Locator::entitiesMap::value();
	const auto cells = map_coords::CellSpiralSize(reaction.radius);
	map_coords::Spiral spiral;
	glm::ivec2 offset(0);
	std::vector<entt::entity> reached;
	for (int32_t i = 0; i < cells; ++i)
	{
		if (i > 0)
		{
			const auto& step = spiral.Next();
			offset += glm::ivec2(step.x, step.z);
		}
		const auto point = centre + glm::vec2(offset) * map_coords::k_CellSize;
		const float extent = static_cast<float>(map_coords::k_MapCells) * map_coords::k_CellSize;
		if (point.x < 0.0f || point.y < 0.0f || point.x >= extent || point.y >= extent ||
		    glm::length(glm::vec2(offset) * map_coords::k_CellSize) > reaction.radius)
		{
			continue;
		}
		for (const auto mobile : map.GetMobileInGridCell(ecs::MapInterface::GetGridCell(point)))
		{
			if (registry.AllOf<Villager>(mobile))
			{
				reached.push_back(mobile);
			}
		}
	}
	for (const auto villager : reached)
	{
		if (_reactions.contains(id))
		{
			ApplyToVillager(villager, id);
		}
	}
}

bool MagicShieldSystem::Blocked(glm::vec3 living, glm::vec3 source) const
{
	auto& registry = EntityRegistry();
	bool blocked = false;
	registry.Each<const ShieldDome, const Transform>([&](const ShieldDome& dome, const Transform& transform) {
		const auto volume = shield::VolumeOf(dome.halfExtent, dome.solidScale);
		const auto centre = glm::xz(transform.position);
		const float land =
		    Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(glm::xz(source)) : 0.0f;
		const bool livingInside = glm::distance(glm::xz(living), centre) < volume.radius;
		const bool sourceInside = shield::InsideDome(volume, glm::distance(glm::xz(source), centre), source.y - land);
		blocked = blocked || (livingInside && !sourceInside);
	});
	return blocked;
}

uint8_t MagicShieldSystem::PriorityFor(entt::entity villager, const ShieldReaction& reaction, float distance) const
{
	auto& registry = EntityRegistry();
	const auto& person = registry.Get<const Villager>(villager);
	auto priority = static_cast<uint8_t>(RowOf(reaction.type).priority);
	switch (reaction.type)
	{
	case Reaction::ReactToMagicShield:
	{
		if (StandingShield(reaction.object) == nullptr)
		{
			return 0;
		}
		const auto town = TownStateOf(person);
		priority = shield::VillagerPriority(priority, town.hasTown, town.protection, town.sinceAttacked,
		                                    VillagerInfoOf(person).numGameTurnsAfterAggressionInterestedInShield);
		break;
	}
	case Reaction::ReactToMagicShieldStruck:
	{
		// Only those sheltering under it, and not yet looking at where it was struck
		const auto* current = registry.TryGet<const VillagerShieldReaction>(villager);
		const auto* action = registry.TryGet<const LivingAction>(villager);
		const bool amazed =
		    action != nullptr && Locator::livingActionSystem::value().VillagerGetState(*action, LivingAction::Index::Top) ==
		                             VillagerStates::AmazedByMagicShieldReaction;
		if (current == nullptr || !amazed || current->lookAt == PositionOf(reaction.object))
		{
			return 0;
		}
		break;
	}
	default:
		break;
	}
	return villager_reaction::Priority(priority, KindReacts(person, reaction.type), DistanceOf(reaction.type), distance);
}

uint32_t MagicShieldSystem::AgainTurnsFor(entt::entity villager, const ShieldReaction& reaction) const
{
	auto& registry = EntityRegistry();
	const auto standard = RowOf(reaction.type).numGameTurnsForNormalThingsBeforeReactingAgain;
	if (reaction.type != Reaction::ReactToMagicShield)
	{
		return standard;
	}
	const auto& person = registry.Get<const Villager>(villager);
	const auto town = TownStateOf(person);
	bool goalUnder = false;
	if (const auto* found = StandingShield(reaction.object); found != nullptr)
	{
		if (const auto* hug = registry.TryGet<const WallHug>(villager))
		{
			goalUnder = shield::IsUnder(hug->goal, PositionOf(reaction.object), found->radius, 0.0f);
		}
	}
	return shield::VillagerAgainTurns(town.hasTown, town.sinceAttacked,
	                                  VillagerInfoOf(person).numGameTurnsAfterAggressionInterestedInShield, goalUnder,
	                                  standard);
}

uint32_t MagicShieldSystem::ReactTurnsFor(entt::entity villager, const ShieldReaction& reaction) const
{
	auto& registry = EntityRegistry();
	const auto standard = RowOf(reaction.type).numGameTurnsForNormalThingsToReact;
	if (reaction.type != Reaction::ReactToMagicShield)
	{
		return standard;
	}
	const auto& person = registry.Get<const Villager>(villager);
	const auto town = TownStateOf(person);
	auto& random = Locator::gameRandom::value();
	uint32_t roll4 = 0;
	uint32_t roll50 = 0;
	if (town.hasTown)
	{
		roll4 = random.GameRand(4);
		if (roll4 != 0)
		{
			roll50 = random.GameRand(50);
		}
	}
	return shield::VillagerReactTurns(town.hasTown, roll4, roll50, town.sinceAttacked,
	                                  VillagerInfoOf(person).numGameTurnsAfterAggressionInterestedInShield, standard);
}

void MagicShieldSystem::ApplyToVillager(entt::entity villager, uint32_t id)
{
	auto& registry = EntityRegistry();
	if (!registry.AllOf<LivingAction, Transform>(villager) || registry.AnyOf<AtHome>(villager))
	{
		return;
	}
	// One reacting to a miracle through the reaction system is busy with that
	if (const auto* other = registry.TryGet<const LivingReaction>(villager); other != nullptr && other->reaction != 0)
	{
		return;
	}
	auto& reaction = _reactions.at(id);
	const auto at = registry.Get<const Transform>(villager).position;
	if (Blocked(at, registry.Get<const Transform>(reaction.object).position))
	{
		return;
	}
	const float distance = villager_reaction::SpreadDistance(glm::xz(at), PositionOf(reaction.object));
	const auto again = AgainTurnsFor(villager, reaction);
	auto* memory = registry.TryGet<VillagerReactionMemory>(villager);
	if (memory == nullptr)
	{
		memory = &registry.Assign<VillagerReactionMemory>(villager);
	}
	const auto turn = GameTurn();
	const auto* current = registry.TryGet<const VillagerShieldReaction>(villager);
	if (current == nullptr)
	{
		if (PriorityFor(villager, reaction, distance) > 0 &&
		    memory->memory.MayReactAgain(static_cast<uint32_t>(reaction.type), turn, again))
		{
			if (reaction.firstReacted == 0)
			{
				reaction.firstReacted = turn;
			}
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Magic: a villager takes up reaction {} to a shield",
			                    static_cast<int>(reaction.type));
			StartReacting(villager, id);
		}
		return;
	}
	const auto currentId = current->reaction;
	const auto found = _reactions.find(currentId);
	if (found == _reactions.end())
	{
		return;
	}
	const auto& was = found->second;
	const float a = PriorityFor(villager, was, glm::distance(glm::xz(at), PositionOf(was.object)));
	const float b = PriorityFor(villager, reaction, distance);
	const bool reconsiderSame = reaction.type == Reaction::ReactToMagicShieldStruck;
	if ((reaction.type != was.type || (reconsiderSame && currentId != id)) && a < b)
	{
		const auto seconds = (turn - memory->memory.LastReacted(static_cast<uint32_t>(was.type))) / k_TurnsPerSecond;
		if (seconds >= k_SwitchSeconds)
		{
			memory->memory.Record(static_cast<uint32_t>(reaction.type), turn);
			std::erase(found->second.reacting, villager);
			StartReacting(villager, id);
		}
	}
}

void MagicShieldSystem::StartReacting(entt::entity villager, uint32_t id)
{
	auto& registry = EntityRegistry();
	auto& random = Locator::gameRandom::value();
	auto& action = registry.Get<LivingAction>(villager);
	auto& living = Locator::livingActionSystem::value();
	auto& reaction = _reactions.at(id);
	const auto at = PositionOf(villager);
	const auto centre = PositionOf(reaction.object);
	auto* current = registry.TryGet<VillagerShieldReaction>(villager);
	const auto previous = current != nullptr ? current->previous : living.VillagerGetState(action, LivingAction::Index::Top);

	if (reaction.type == Reaction::ReactToMagicShieldStruck)
	{
		// Struck: one already sheltering turns to look at the shield and keeps to its standing reaction
		const auto& shieldObject = registry.Get<const MagicShield>(reaction.object);
		if (current == nullptr || !_reactions.contains(shieldObject.reaction))
		{
			return;
		}
		current->reaction = shieldObject.reaction;
		current->type = Reaction::ReactToMagicShield;
		current->lookAt = centre;
		_reactions.at(shieldObject.reaction).reacting.push_back(villager);
		return;
	}

	auto& reacting = registry.AssignOrReplace<VillagerShieldReaction>(
	    villager,
	    VillagerShieldReaction {.previous = previous,
	                            .reaction = id,
	                            .type = reaction.type,
	                            .shield = reaction.type == Reaction::ReactToMagicShield ? reaction.object : entt::null,
	                            .lookAt = at,
	                            .animation = 0});
	reaction.reacting.push_back(villager);

	if (reaction.type == Reaction::ReactToMagicShieldDestroyed)
	{
		// Its fall: it runs from where the shield stood
		const auto& row = RowOf(reaction.type);
		const auto away = at - centre;
		const float angle = std::atan2(away.y, away.x) + random.GameFloatRand(k_FleeTurnRange) - (k_FleeTurnRange * 0.5f);
		const float distance = row.minDistanceToRunAwayFromObject +
		                       random.GameFloatRand(row.maxDistanceToRunAwayFromObject - row.minDistanceToRunAwayFromObject);
		ecs::villager_home::SetupMoveTo(action, at + glm::vec2(std::cos(angle), std::sin(angle)) * distance,
		                                VillagerStates::DecideWhatToDo);
		return;
	}

	// Standing: it shelters under the shield, on its own side, unless it is well inside already
	living.VillagerSetState(action, LivingAction::Index::Top, VillagerStates::AmazedByMagicShieldReaction, false);
	const auto& shieldObject = registry.Get<const MagicShield>(reaction.object);
	if (!shield::IsUnder(at, centre, shieldObject.radius, shieldObject.radius * (1.0f - shield::k_ShelterInside)))
	{
		const float turn = random.GameFloatRand(shield::k_ShelterTurnRange);
		const float u = random.GameFloatRand(1.0f);
		const auto place = shield::ShelterAt(centre, at, shieldObject.radius, turn, u, 0.0f);
		ecs::villager_home::SetupMoveTo(action, place.point, VillagerStates::AmazedByMagicShieldReaction);
	}
	reacting.lookAt = shield::LookOut(centre, at, random.GameFloatRand(shield::k_ShelterTurnRange));
}

void MagicShieldSystem::StopReacting(entt::entity villager, bool setState)
{
	auto& registry = EntityRegistry();
	auto* current = registry.TryGet<VillagerShieldReaction>(villager);
	if (current == nullptr)
	{
		return;
	}
	if (const auto found = _reactions.find(current->reaction); found != _reactions.end())
	{
		std::erase(found->second.reacting, villager);
	}
	const auto previous = current->previous;
	registry.Remove<VillagerShieldReaction>(villager);
	if (setState)
	{
		if (auto* action = registry.TryGet<LivingAction>(villager))
		{
			// Back to what it was doing
			const auto state = previous != VillagerStates::InvalidState ? previous : VillagerStates::DecideWhatToDo;
			Locator::livingActionSystem::value().VillagerSetState(*action, LivingAction::Index::Top, state, false);
		}
	}
}

void MagicShieldSystem::ProcessVillagers()
{
	auto& registry = EntityRegistry();
	if (!Locator::livingActionSystem::has_value())
	{
		return;
	}
	const auto& living = Locator::livingActionSystem::value();
	const auto turn = GameTurn();
	std::vector<std::pair<entt::entity, bool>> stops;
	registry.Each<const VillagerShieldReaction, const LivingAction>(
	    [&](entt::entity villager, const VillagerShieldReaction& current, const LivingAction& action) {
		    const auto top = living.VillagerGetState(action, LivingAction::Index::Top);
		    const auto final = living.VillagerGetState(action, LivingAction::Index::Final);
		    // Leaving the reaction's state for another ends the reaction
		    const bool reactionState =
		        top == VillagerStates::AmazedByMagicShieldReaction ||
		        (top == VillagerStates::MoveToPos && (final == VillagerStates::AmazedByMagicShieldReaction ||
		                                              current.type == Reaction::ReactToMagicShieldDestroyed));
		    const auto found = _reactions.find(current.reaction);
		    if (!reactionState || found == _reactions.end())
		    {
			    stops.emplace_back(villager, false);
			    return;
		    }
		    // It reacts to the shield while it stands, for as long as its reaction lasts
		    if (StandingShield(current.shield) != nullptr)
		    {
			    const auto* memory = registry.TryGet<const VillagerReactionMemory>(villager);
			    const auto elapsed =
			        turn - (memory != nullptr ? memory->memory.LastReacted(static_cast<uint32_t>(current.type)) : 0);
			    if (static_cast<int32_t>(elapsed) <= static_cast<int32_t>(ReactTurnsFor(villager, found->second)))
			    {
				    return;
			    }
		    }
		    stops.emplace_back(villager, true);
	    });
	for (const auto& [villager, setState] : stops)
	{
		StopReacting(villager, setState);
	}
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "TornadoSystem.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Common/GUtilsDistance.h"
#include "ECS/Archetypes/PotArchetype.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/AtHome.h"
#include "ECS/Components/CarriedByTornado.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AnimalSystemInterface.h"
#include "ECS/Systems/CreatureFightSystemInterface.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/LeashSystemInterface.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Particles/ParticleEffect.h"
#include "Resources/ResourcesInterface.h"
#include "VillagerFire.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
using particles::TornadoCandidate;

namespace
{
/// The land's cells the tornado looks through are this wide
constexpr float k_CellSize = 10.0f;
/// The search covers the cells the reach spans and two more each way, as a square
constexpr int32_t k_ExtraCells = 2;

/// The radius across the ground of an object's model as it stands: the larger of its box's half width and half depth,
/// scaled; none without a model
std::optional<float> RadiusOf(const ecs::Registry& registry, entt::entity entity, const Transform& transform)
{
	const auto* mesh = registry.TryGet<const Mesh>(entity);
	if (mesh == nullptr || !Locator::resources::has_value())
	{
		return std::nullopt;
	}
	auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(mesh->id))
	{
		return std::nullopt;
	}
	const auto size = meshes.Handle(mesh->id)->GetBoundingBox().Size() * transform.scale;
	return std::max(size.x, size.z) * 0.5f;
}

const GPotInfo* PotInfoOf(const Pot& pot)
{
	const auto& pots = Locator::infoConstants::value().pot;
	const auto index = static_cast<size_t>(pot.type);
	return index < pots.size() ? &pots.at(index) : nullptr;
}

/// A villager's state of a kind
VillagerStates StateOf(const LivingAction& action, LivingAction::Index index)
{
	return static_cast<VillagerStates>(action.states.at(static_cast<size_t>(index)));
}

/// Whether a villager is there to be acted on at all: not on its way to dying
bool IsAvailableVillager(const ecs::Registry& registry, entt::entity villager)
{
	const auto* action = registry.TryGet<const LivingAction>(villager);
	return action == nullptr || StateOf(*action, LivingAction::Index::Final) != VillagerStates::Dying;
}

/// Whether a villager can be reached: there, not at home indoors and not going to hide in a building. (The game also
/// leaves a villager in the hand alone; openblack's hand doesn't hold villagers.)
bool IsReachableVillager(const ecs::Registry& registry, entt::entity villager)
{
	if (!IsAvailableVillager(registry, villager) || registry.AllOf<AtHome>(villager))
	{
		return false;
	}
	const auto* action = registry.TryGet<const LivingAction>(villager);
	return action == nullptr || StateOf(*action, LivingAction::Index::Top) != VillagerStates::GoAndHideInNearbyBuilding;
}

/// What the tornado makes of an object in its reach, none for what it passes over
std::optional<TornadoCandidate> CandidateOf(const ecs::Registry& registry, entt::entity entity, const Transform& transform,
                                            float radius)
{
	TornadoCandidate candidate {.object = entity, .position = transform.position, .radius = radius};
	if (registry.AnyOf<Creature>(entity))
	{
		// Too big for it: caught where it stands
		candidate.kind = TornadoCandidate::Kind::Creature;
		return candidate;
	}
	if (const auto* pot = registry.TryGet<const Pot>(entity))
	{
		const auto* info = PotInfoOf(*pot);
		if (info == nullptr)
		{
			return std::nullopt;
		}
		const bool pile = info->potType == PotType::PileFood || info->potType == PotType::PileWood;
		// Whether a pot or pile is lifted whole is its kind's to say, as a pile the hand dropped is; any other pile gives
		// up a pot
		if (info->canBecomeAPhysicsObject != 0)
		{
			candidate.kind = TornadoCandidate::Kind::Liftable;
			candidate.pile = pile;
			return candidate;
		}
		if (pile)
		{
			candidate.kind = TornadoCandidate::Kind::Pile;
			return candidate;
		}
		return std::nullopt;
	}
	if (registry.AnyOf<Villager>(entity))
	{
		// The dead too, but not those it can't reach
		if (!IsReachableVillager(registry, entity))
		{
			return std::nullopt;
		}
		candidate.kind = TornadoCandidate::Kind::Liftable;
		return candidate;
	}
	// Trees, animals, things lying about and rocks and the like; never buildings, features or the teleport stones
	if (registry.AnyOf<Tree, Animal, MobileObject, MobileStatic>(entity))
	{
		candidate.kind = TornadoCandidate::Kind::Liftable;
		return candidate;
	}
	return std::nullopt;
}
} // namespace

std::vector<TornadoCandidate> TornadoSystem::Candidates(glm::vec3 foot, float reach) const
{
	std::vector<TornadoCandidate> candidates;
	if (!Locator::entitiesMap::has_value())
	{
		return candidates;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	const auto& map = Locator::entitiesMap::value();
	const map_coords::MapCoords base {.x = map_coords::ToFixed(foot.x), .z = map_coords::ToFixed(foot.z)};
	// So many cells about the foot's, in the game's spiral; in each cell what stands fixed first and then what moves, in
	// the order the cell keeps them, each counted only in its own cell
	const auto span = static_cast<int32_t>(std::ceil(reach / k_CellSize)) + k_ExtraCells;
	auto cell = map_coords::Cell(base);
	map_coords::Spiral spiral;
	for (int32_t left = span * span; left > 0; --left)
	{
		if (map_coords::InBounds(cell))
		{
			const ecs::MapInterface::CellId id(static_cast<uint16_t>(cell.x), static_cast<uint16_t>(cell.y));
			for (const auto& list : {map.GetFixedInGridCell(id), map.GetMobileInGridCell(id)})
			{
				for (const auto entity : list)
				{
					if (!registry.Valid(entity) || registry.AnyOf<CarriedByTornado>(entity))
					{
						continue;
					}
					const auto* transform = registry.TryGet<const Transform>(entity);
					if (transform == nullptr || map_coords::CellOf(transform->position) != cell)
					{
						continue;
					}
					if (registry.AnyOf<Villager>(entity) && !IsAvailableVillager(registry, entity))
					{
						continue;
					}
					const auto radius = RadiusOf(registry, entity, *transform);
					if (!radius.has_value())
					{
						continue;
					}
					// Within its own radius and the reach, as the game measures across the land
					const map_coords::MapCoords at {.x = map_coords::ToFixed(transform->position.x),
					                                .z = map_coords::ToFixed(transform->position.z)};
					if (!(gutils::GetDistanceInMetres(base, at) < *radius + reach))
					{
						continue;
					}
					if (const auto candidate = CandidateOf(registry, entity, *transform, *radius))
					{
						candidates.push_back(*candidate);
					}
				}
			}
		}
		const auto& step = spiral.Next();
		cell += glm::ivec2(step.x, step.z);
	}
	return candidates;
}

std::optional<glm::mat3> TornadoSystem::Carry(const std::shared_ptr<particles::CarriedObject>& carried)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (carried == nullptr || !registry.Valid(carried->object) || registry.AnyOf<CarriedByTornado>(carried->object))
	{
		return std::nullopt;
	}
	const auto* transform = registry.TryGet<const Transform>(carried->object);
	if (transform == nullptr)
	{
		return std::nullopt;
	}
	registry.Assign<CarriedByTornado>(carried->object, carried);
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Tornado: picked up #{}", static_cast<uint32_t>(carried->object));
	return transform->rotation;
}

void TornadoSystem::CatchCreature(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	// A creature fainted already isn't caught again until it has come round
	if (!registry.Valid(creature) ||
	    (Locator::creatureFightSystem::has_value() && Locator::creatureFightSystem::value().IsKnockedOut(creature)))
	{
		return;
	}
	registry.AssignOrReplace<CaughtByTornado>(creature);
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Tornado: caught creature #{}", static_cast<uint32_t>(creature));
	// It faints where it is, lying helpless for four seconds a size and eight more, the leash letting go of it, and comes
	// round as from a knock-out
	if (Locator::creatureFightSystem::has_value())
	{
		Locator::creatureFightSystem::value().ForceFaint(creature);
	}
	else
	{
		if (Locator::creatureLocomotionSystem::has_value())
		{
			Locator::creatureLocomotionSystem::value().Stop(creature);
		}
		if (Locator::leashSystem::has_value() && Locator::leashSystem::value().IsLeashed(creature))
		{
			Locator::leashSystem::value().TakeOff(creature);
		}
	}
}

entt::entity TornadoSystem::TakeFromPile(entt::entity pile, uint32_t amount, float sizeShare)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(pile))
	{
		return entt::null;
	}
	auto* pot = registry.TryGet<Pot>(pile);
	const auto* transform = registry.TryGet<const Transform>(pile);
	if (pot == nullptr || transform == nullptr)
	{
		return entt::null;
	}
	const auto* info = PotInfoOf(*pot);
	const auto taken = std::min<uint32_t>(amount, pot->amount);
	if (info == nullptr || taken == 0)
	{
		return entt::null;
	}
	const auto position = transform->position;
	// A pot such as the hand drops, of food or of wood
	const auto type = info->resourceType == ResourceType::Wood ? PotInfo::HandWood : PotInfo::HandFood;
	pot->amount -= taken;
	if (pot->amount == 0)
	{
		registry.Destroy(pile);
	}
	const auto made = archetypes::PotArchetype::Create(position, 0.0f, type, static_cast<int32_t>(taken));
	if (made != entt::null)
	{
		registry.Get<Transform>(made).scale *= sizeShare;
	}
	return made;
}

void TornadoSystem::ProcessTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<std::pair<entt::entity, particles::CarriedObject>> released;
	registry.Each<CarriedByTornado>([&](entt::entity entity, const CarriedByTornado& carried) {
		if (carried.carried == nullptr || carried.carried->atom == nullptr)
		{
			released.emplace_back(entity, carried.carried != nullptr ? *carried.carried : particles::CarriedObject {});
		}
	});
	for (const auto& [entity, carried] : released)
	{
		LetGo(entity, carried);
	}
	Update(1.0f);
}

void TornadoSystem::LetGo(entt::entity object, const particles::CarriedObject& carried)
{
	auto& registry = Locator::entitiesRegistry::value();
	registry.Remove<CarriedByTornado>(object);
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Tornado: let go of #{} at ({}, {}, {})", static_cast<uint32_t>(object),
	                    carried.position.x, carried.position.y, carried.position.z);
	// Something no longer there to be acted on, as a villager already dying, is only let go of
	if (registry.AnyOf<Villager>(object) && !IsAvailableVillager(registry, object))
	{
		return;
	}
	if (!registry.AnyOf<Villager, Animal>(object))
	{
		// Trees, pots and whatever else it carried are gone
		world_objects::Remove(object);
		return;
	}
	// The living are put down on the land where they were flung, and the miracle kills them there
	auto& transform = registry.Get<Transform>(object);
	transform.position = carried.position;
	if (Locator::terrainSystem::has_value())
	{
		transform.position.y = Locator::terrainSystem::value().GetHeightAt({carried.position.x, carried.position.z});
	}
	transform.rotation = carried.rotation;
	if (registry.AnyOf<Villager>(object))
	{
		villager_fire::DieByEffect(object);
		return;
	}
	if (Locator::animalSystem::has_value())
	{
		Locator::animalSystem::value().KillByEffect(object, transform.position);
	}
}

void TornadoSystem::Update(float turnFraction)
{
	const float t = std::clamp(turnFraction, 0.0f, 1.0f);
	Locator::entitiesRegistry::value().Each<const CarriedByTornado, Transform>(
	    [t](const CarriedByTornado& carried, Transform& transform) {
		    const auto* atom = carried.carried != nullptr ? carried.carried->atom : nullptr;
		    if (atom == nullptr || !atom->drawn)
		    {
			    return;
		    }
		    // Where its particle is drawn between its last two steps
		    const auto& a = atom->previous;
		    const auto& b = atom->current;
		    transform.position = a.position + (b.position - a.position) * t;
		    transform.rotation = a.rotation + (b.rotation - a.rotation) * t;
	    });
}

size_t TornadoSystem::CarriedCount() const
{
	size_t count = 0;
	Locator::entitiesRegistry::value().Each<const CarriedByTornado>([&count](const CarriedByTornado&) { ++count; });
	return count;
}

void TornadoSystem::Reset()
{
	// The registry is cleared with the land; nothing is kept here
}

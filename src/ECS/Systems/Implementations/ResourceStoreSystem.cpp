/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The stores of food and wood. A storage pit stores anything, in its one pile of food and its five of wood; a pile
// stores only through the pit it is part of. Giving a town's store a resource counts for the giver: their alignment
// moves by how much less the town now wants it, and the town believes in them by that, less when they took that
// resource from it lately and less again when the town isn't theirs. Taking from a store counts against the town's
// owner and is remembered of the taker.

#define LOCATOR_IMPLEMENTATIONS

#include "ResourceStoreSystem.h"

#include <cmath>

#include <algorithm>
#include <chrono>

#include <LNDFile.h>
#include <entt/core/hashed_string.hpp>
#include <fmt/format.h>
#include <glm/gtx/euler_angles.hpp>

#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Audio/AudioManagerInterface.h"
#include "ECS/Archetypes/PotArchetype.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/MagicForest.h"
#include "ECS/Components/MagicPile.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/MiracleImpression.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/ResourceLastTaken.h"
#include "ECS/Components/ResourcePile.h"
#include "ECS/Components/StoragePit.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Map.h"
#include "ECS/PhysicsClasses.h"
#include "ECS/Registry.h"
#include "ECS/StoreRules.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/Systems/TownDesireSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/AreaEffect.h"
#include "Magic/ResourcePiles.h"
#include "Magic/TownBelief.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace piles = openblack::magic::piles;

namespace
{
/// The in-game bank's wood mulch, four samples played one after another as wood goes into a store
/// The game's map is 512 cells each way
constexpr int k_MapCells = 512;
constexpr uint32_t k_MulchSample = 155;
constexpr uint32_t k_MulchSamples = 4;
/// The piles a storage pit holds its food and its wood in, for how much one can hold
constexpr auto k_PitFoodPile = PotInfo::StoragePitFoodPile;
constexpr auto k_PitWoodPile = PotInfo::WoodPile_1;

ecs::Registry& Entities()
{
	return Locator::entitiesRegistry::value();
}

const GPotInfo& PotInfoOf(PotInfo type)
{
	return Locator::infoConstants::value().pot.at(static_cast<size_t>(type));
}

uint32_t TickMs()
{
	return static_cast<uint32_t>(
	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

void PlayInGame(uint32_t sample, glm::vec3 position)
{
	if (Locator::audio::has_value())
	{
		Locator::audio::value().PlaySoundEffect(entt::hashed_string(fmt::format("InGame.sad/{}", sample).c_str()).value(),
		                                        position);
	}
}

/// The thud of a pile, where it is
void Thud(ResourceType type, uint32_t amount, glm::vec3 position)
{
	PlayInGame(piles::PileSoundSample(type, amount, TickMs()), position);
}

/// The pile a storage pit keeps its food or wood in, made empty where the pit's model says if it has none
entt::entity StorePile(entt::entity pit, entt::entity& slot, PotInfo type, size_t place)
{
	auto& registry = Entities();
	if (registry.Valid(slot))
	{
		return slot;
	}
	const auto& meshes = Locator::resources::value().GetMeshes();
	const auto& mesh = registry.Get<const Mesh>(pit);
	if (!meshes.Contains(mesh.id))
	{
		return entt::null;
	}
	const auto& metrics = meshes.Handle(mesh.id)->GetExtraMetrics();
	if (place >= metrics.size())
	{
		return entt::null;
	}
	// Turned as the pit is, which is turned about the vertical
	const auto& transform = registry.Get<const Transform>(pit);
	const float yaw = std::atan2(transform.rotation[0].z, transform.rotation[0].x);
	const auto offset = glm::vec3(glm::mat4(transform.rotation) * metrics.at(place)[3]);
	slot = ecs::archetypes::PotArchetype::CreateEmpty(transform.position + offset, yaw, type);
	return slot;
}

/// The town an abode belongs to
entt::entity TownOf(entt::entity store)
{
	auto& registry = Entities();
	const auto* abode = registry.TryGet<const Abode>(store);
	entt::entity town = entt::null;
	if (abode != nullptr)
	{
		registry.Each<const Town>([&](entt::entity entity, const Town& candidate) {
			if (candidate.id == abode->townId)
			{
				town = entity;
			}
		});
	}
	return town;
}

TownDesireInfo DesireFor(ResourceType type)
{
	return type == ResourceType::Food ? TownDesireInfo::ForFood : TownDesireInfo::ForWood;
}

float Desire(entt::entity town, ResourceType type)
{
	return Locator::townDesireSystem::has_value() ? Locator::townDesireSystem::value().RecomputeDesire(town, DesireFor(type))
	                                              : 0.0f;
}

/// A player's alignment moves by a change to a store, scaled as giving or taking and damped by how far it already is
/// that way
void MoveAlignment(PlayerNames player, int64_t amount, float value)
{
	if (!Locator::alignmentSystem::has_value() || player == PlayerNames::NEUTRAL)
	{
		return;
	}
	const auto& town = Locator::infoConstants::value().town;
	const float multiplier = amount < 1 ? town.takeResourceAligmnetChangeMultiplier : town.giveResourceAligmnetChangeMultiplier;
	auto& alignment = Locator::alignmentSystem::value();
	alignment.AddPendingAlignment(player, magic::DampAlignmentChange(value * multiplier, alignment.GetPlayerAlignment(player)));
}

uint32_t& CountOf(Abode& abode, ResourceType type)
{
	return type == ResourceType::Food ? abode.foodAmount : abode.woodAmount;
}

bool IsMushroom(MobileObjectInfo type)
{
	return type == MobileObjectInfo::Champi || type == MobileObjectInfo::MagicMushroom || type == MobileObjectInfo::Toadstool;
}

/// Whether a point is on the land's map
bool InMap(glm::vec3 point)
{
	if (!Locator::terrainSystem::has_value())
	{
		return false;
	}
	const auto extent = Locator::terrainSystem::value().GetExtent();
	return point.x >= extent.minimum.x && point.z >= extent.minimum.y && point.x < extent.maximum.x &&
	       point.z < extent.maximum.y;
}

float LandHeight(glm::vec2 xz)
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(xz) : 0.0f;
}

/// Whether a point is in the water, as off the land is
bool IsWater(glm::vec3 point)
{
	if (!Locator::terrainSystem::has_value() || point.x < 0.0f || point.z < 0.0f)
	{
		return true;
	}
	const auto cell = piles::CellOf({point.x, point.z});
	if (cell.x >= k_MapCells || cell.y >= k_MapCells)
	{
		return true;
	}
	const auto* landCell = Locator::terrainSystem::value().FindCell(glm::u16vec2(cell));
	return landCell == nullptr || landCell->properties.hasWater != 0;
}

/// The half width and depth of an entity's model, unscaled
glm::vec2 HalfExtentOf(const Mesh& mesh)
{
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(mesh.id))
	{
		return glm::vec2(0.0f);
	}
	const auto size = meshes.Handle(mesh.id)->GetBoundingBox().Size();
	return glm::vec2(size.x, size.z) * 0.5f;
}
} // namespace

ResourceStoreSystemInterface::ObjectResource ResourceStoreSystem::ResourceOf(entt::entity object) const
{
	auto& registry = Entities();
	if (!registry.Valid(object) || !Locator::infoConstants::has_value())
	{
		return {};
	}
	const auto& info = Locator::infoConstants::value();
	const auto* transform = registry.TryGet<const Transform>(object);
	const float scale = transform != nullptr ? transform->scale.x : 1.0f;
	const float life = ecs::world_objects::LifeOf(object);
	if (const auto* tree = registry.TryGet<const Tree>(object))
	{
		// A forest miracle's tree is worth more than an ordinary one
		const auto* magic = registry.TryGet<const MagicTree>(object);
		const float multiplier = magic != nullptr ? magic->woodMultiplier : 1.0f;
		const float balance = registry.Context().mapScriptGlobals.landBalance.at(5);
		return {ResourceType::Wood,
		        ecs::store_rules::TreeWood(life, multiplier, info.tree.at(static_cast<size_t>(tree->type)).woodValue, scale,
		                                   balance)};
	}
	if (const auto* dead = registry.TryGet<const DeadTree>(object))
	{
		return {ResourceType::Wood, ecs::store_rules::DeadTreeWood(
		                                scale, info.tree.at(static_cast<size_t>(dead->type)).woodValue, dead->woodMultiplier)};
	}
	if (const auto* still = registry.TryGet<const MobileStatic>(object))
	{
		const auto& row = info.mobileStatic.at(static_cast<size_t>(still->type));
		if (!physics_classes::IsFenceModel(row.meshId))
		{
			return {};
		}
		return {ResourceType::Wood, ecs::store_rules::FenceWood(life, row.woodValue, scale)};
	}
	if (const auto* mobile = registry.TryGet<const MobileObject>(object))
	{
		if (!IsMushroom(mobile->type))
		{
			return {};
		}
		const auto food = info.mobileObject.at(static_cast<size_t>(mobile->type)).foodValue;
		return {ResourceType::Food, food > 0.0f ? static_cast<uint32_t>(food) : 0u};
	}
	if (const auto* animal = registry.TryGet<const Animal>(object))
	{
		const auto& row = info.animal.at(static_cast<size_t>(animal->type));
		return {ResourceType::Food, ecs::store_rules::AnimalFood(row.foodValue, static_cast<uint32_t>(row.foodType))};
	}
	if (const auto* pot = registry.TryGet<const Pot>(object))
	{
		return {PotInfoOf(pot->type).resourceType, pot->amount};
	}
	return {};
}

std::optional<entt::entity> ResourceStoreSystem::StoreOf(entt::entity pile) const
{
	std::optional<entt::entity> store;
	Entities().Each<const StoragePit>([pile, &store](entt::entity pit, const StoragePit& data) {
		if (data.foodPile == pile || std::ranges::find(data.woodPiles, pile) != data.woodPiles.end())
		{
			store = pit;
		}
	});
	return store;
}

bool ResourceStoreSystem::IsStore(entt::entity store, ResourceType type) const
{
	auto& registry = Entities();
	if (!registry.Valid(store) || (type != ResourceType::Food && type != ResourceType::Wood && type != ResourceType::Any))
	{
		return false;
	}
	if (registry.AllOf<StoragePit, Abode>(store))
	{
		return true;
	}
	// A pile stores only its own resource, and only through the store it is part of
	// TODO(stores): worship sites store food, workshops wood, and a building wood while it has a building site; openblack
	// has none of those stores yet
	if (const auto* pot = registry.TryGet<const Pot>(store))
	{
		const auto own = PotInfoOf(pot->type).resourceType;
		const auto structure = StoreOf(store);
		return structure.has_value() && (type == own || type == ResourceType::Any) && IsStore(*structure, type);
	}
	return false;
}

uint32_t ResourceStoreSystem::AddToPile(entt::entity pile, ResourceType type, uint32_t amount)
{
	auto& registry = Entities();
	auto* pot = registry.TryGet<Pot>(pile);
	if (pot == nullptr)
	{
		return 0;
	}
	const auto& info = PotInfoOf(pot->type);
	if (piles::Thuds(pot->type))
	{
		Thud(type, amount, registry.Get<const Transform>(pile).position);
	}
	const auto taken = piles::AmountTaken(pot->amount, amount, pot->maxAmount, piles::IsCapped(info.nextPotForResource));
	pot->amount += taken;
	return taken;
}

uint32_t ResourceStoreSystem::FillPit(entt::entity store, ResourceType type, uint32_t amount)
{
	auto& registry = Entities();
	auto* abode = registry.TryGet<Abode>(store);
	auto* pit = registry.TryGet<StoragePit>(store);
	if (abode == nullptr || pit == nullptr)
	{
		return 0;
	}
	const auto& abodeInfo = Locator::infoConstants::value().abode.at(static_cast<size_t>(abode->type));
	uint32_t left = amount;
	if (type == ResourceType::Food)
	{
		// Its one pile of food
		const auto pile = StorePile(store, pit->foodPile, abodeInfo.potForResourceFood, pit->woodPiles.size());
		if (pile != entt::null && left > 0)
		{
			left -= AddToPile(pile, type, left);
		}
	}
	else
	{
		// Its piles of wood, each in turn
		size_t place = 0;
		for (auto potType = abodeInfo.potForResourceWood;
		     potType != PotInfo::_COUNT && place < pit->woodPiles.size() && left > 0;
		     potType = PotInfoOf(potType).nextPotForResource, ++place)
		{
			const auto pile = StorePile(store, pit->woodPiles.at(place), potType, place);
			if (pile != entt::null)
			{
				left -= AddToPile(pile, type, left);
			}
		}
	}
	return amount - left;
}

uint32_t ResourceStoreSystem::AddToStore(entt::entity store, ResourceType type, uint32_t amount,
                                         std::optional<PlayerNames> giver, bool /*poisoned*/)
{
	auto& registry = Entities();
	// A store's pile passes what it is given to its store
	if (const auto structure = registry.AllOf<Pot>(store) ? StoreOf(store) : std::nullopt)
	{
		return AddToStore(*structure, type, amount, giver, false);
	}
	auto* abode = registry.TryGet<Abode>(store);
	if (abode == nullptr || !registry.AllOf<StoragePit>(store) || (type != ResourceType::Food && type != ResourceType::Wood))
	{
		return 0;
	}
	// TODO(stores): a poisoned gift poisons the pile it goes into; openblack has no poison on piles yet
	const auto added = FillPit(store, type, amount);
	const auto town = TownOf(store);
	if (!giver.has_value() || town == entt::null)
	{
		CountOf(*abode, type) += added;
		return added;
	}
	// The town wants less of what it was given; that, against how lately the giver took it, is what the gift counts for
	const float before = Desire(town, type);
	CountOf(*abode, type) += added;
	const float after = Desire(town, type);
	const auto& townInfo = Locator::infoConstants::value().town;
	const auto* lastTaken = registry.TryGet<const ResourceLastTaken>(town);
	const auto now = Locator::time::has_value() ? Locator::time::value().GetTurn() : 0u;
	const auto taken =
	    lastTaken != nullptr ? lastTaken->turn.at(static_cast<size_t>(*giver)).at(static_cast<size_t>(type)) : std::nullopt;
	float value = ecs::store_rules::LastTakenModifier(taken, now, townInfo.maxGameturnsForBeliefAfterRemovingFromStoragePit) *
	              (before - after);
	MoveAlignment(*giver, added, value);
	if (registry.Get<const Town>(town).owner != *giver)
	{
		value *= townInfo.multiplierForNonOwnerAddingResource;
	}
	if (auto* impression = registry.TryGet<TownImpression>(town))
	{
		magic::town_belief::Add(impression->belief, *giver, value * townInfo.multiplierForAddingResourceToTown);
	}
	// TODO(stores): the giver's creature may copy the giving; openblack's creature has no copying of gifts yet
	return added;
}

uint32_t ResourceStoreSystem::RemovedFromStore(entt::entity store, ResourceType type, uint32_t amount,
                                               std::optional<PlayerNames> taker)
{
	auto& registry = Entities();
	auto* abode = registry.TryGet<Abode>(store);
	if (abode == nullptr)
	{
		return 0;
	}
	const auto town = TownOf(store);
	auto& count = CountOf(*abode, type);
	if (town == entt::null)
	{
		const auto removed = std::min(count, amount);
		count -= removed;
		return removed;
	}
	const float before = Desire(town, type);
	const auto removed = std::min(count, amount);
	count -= removed;
	if (!taker.has_value())
	{
		return removed;
	}
	// The taker is remembered, and the town's owner is blamed for what the town now wants
	auto& lastTaken = registry.AllOf<ResourceLastTaken>(town) ? registry.Get<ResourceLastTaken>(town)
	                                                          : registry.Assign<ResourceLastTaken>(town);
	lastTaken.turn.at(static_cast<size_t>(*taker)).at(static_cast<size_t>(type)) =
	    Locator::time::has_value() ? std::optional(Locator::time::value().GetTurn()) : std::nullopt;
	const float after = Desire(town, type);
	MoveAlignment(registry.Get<const Town>(town).owner, -static_cast<int64_t>(amount), before - after);
	return removed;
}

uint32_t ResourceStoreSystem::TakeFromPit(entt::entity store, ResourceType type, uint32_t amount,
                                          std::optional<PlayerNames> taker)
{
	auto& registry = Entities();
	auto* pit = registry.TryGet<StoragePit>(store);
	if (pit == nullptr)
	{
		return 0;
	}
	uint32_t taken = 0;
	const auto takeFrom = [&](entt::entity pile) {
		if (amount == 0 || !registry.Valid(pile))
		{
			return;
		}
		auto& pot = registry.Get<Pot>(pile);
		const auto from = std::min(amount, pot.amount);
		pot.amount -= from;
		taken += from;
		amount -= from;
	};
	if (type == ResourceType::Food)
	{
		takeFrom(pit->foodPile);
	}
	else if (type == ResourceType::Wood)
	{
		// The last of its wood piles first
		for (auto pile = pit->woodPiles.rbegin(); pile != pit->woodPiles.rend(); ++pile)
		{
			takeFrom(*pile);
		}
	}
	if (taken != 0)
	{
		RemovedFromStore(store, type, taken, taker);
	}
	return taken;
}

uint32_t ResourceStoreSystem::TakeFromPile(entt::entity pile, ResourceType type, uint32_t amount,
                                           std::optional<PlayerNames> taker)
{
	auto& registry = Entities();
	auto* pot = registry.TryGet<Pot>(pile);
	if (pot == nullptr)
	{
		return 0;
	}
	const auto store = StoreOf(pile);
	if (!store.has_value())
	{
		// A pile on its own goes once it has nothing left
		const auto taken = std::min(amount, pot->amount);
		pot->amount -= taken;
		if (pot->amount == 0)
		{
			ecs::world_objects::Remove(pile);
		}
		return taken;
	}
	// What is over what the store's piles can hold isn't this pile's to give
	const auto* abode = registry.TryGet<const Abode>(*store);
	const bool wood = type == ResourceType::Wood;
	const auto held = abode != nullptr ? (wood ? abode->woodAmount : abode->foodAmount) : 0u;
	const auto over =
	    ecs::store_rules::AmountOverMaximum(wood, held, PotInfoOf(wood ? k_PitWoodPile : k_PitFoodPile).maxAmountInPot);
	const auto asked = ecs::store_rules::AskedOfPile(amount, over);
	uint32_t taken = 0;
	if (asked != 0)
	{
		taken = std::min(asked, pot->amount);
		pot->amount -= taken;
	}
	if (taken != 0)
	{
		RemovedFromStore(*store, type, taken, taker);
	}
	// What is still wanted comes from the store's other piles
	if (taken < amount)
	{
		taken += TakeFromPit(*store, type, amount - taken, taker);
	}
	return taken;
}

bool ResourceStoreSystem::TakeObject(entt::entity store, entt::entity object, std::optional<PlayerNames> giver)
{
	auto& registry = Entities();
	if (!registry.Valid(store) || !registry.Valid(object))
	{
		return false;
	}
	const auto resource = ResourceOf(object);
	if (resource.type == ResourceType::None || !IsStore(store, resource.type))
	{
		return false;
	}
	// TODO(stores): a thing thrown into a storage pit by the local player shows the help for giving; openblack has no
	// help system yet
	const auto added = AddToStore(store, resource.type, resource.amount, giver, false);
	const auto* transform = registry.TryGet<const Transform>(object);
	const auto at = transform != nullptr ? transform->position : glm::vec3(0.0f);
	// TODO(stores): the advisor's resource-drop sound for the local player; openblack has no advisor yet
	static_cast<void>(added);
	// Wood that isn't a pot mulches as it goes in
	if (resource.type == ResourceType::Wood && !registry.AllOf<Pot>(object))
	{
		_mulch = (_mulch + 1) % k_MulchSamples;
		PlayInGame(k_MulchSample + _mulch, at);
	}
	ecs::world_objects::LeaveGhost(object);
	ecs::world_objects::Remove(object);
	// A storage pit calls its people to what was put in it
	if (registry.AllOf<StoragePit>(store) && Locator::reactionSystem::has_value())
	{
		const auto& position = registry.Get<const Transform>(store).position;
		Locator::reactionSystem::value().Create({.initiator = store,
		                                         .type = Reaction::ReactToHandPuttingStuffInStoragePit,
		                                         .player = giver.value_or(PlayerNames::NEUTRAL),
		                                         .position = position});
	}
	return true;
}

bool ResourceStoreSystem::PourAt(ResourceType type, glm::vec3 point, uint32_t amount, bool speedUp, PlayerNames player)
{
	if (amount == 0 || (type != ResourceType::Food && type != ResourceType::Wood) || !InMap(point))
	{
		return false;
	}
	auto& registry = Locator::entitiesRegistry::value();
	const glm::vec2 at {point.x, point.z};
	const auto near = [&](entt::entity entity, float multiplier) {
		const auto& transform = registry.Get<const Transform>(entity);
		float radius = piles::RadiusOnGround(HalfExtentOf(registry.Get<const Mesh>(entity)), transform.scale.x);
		// A pile of food reaches as far as it is raised
		if (const auto* pot = registry.TryGet<const Pot>(entity); pot != nullptr && type == ResourceType::Food)
		{
			radius *= piles::ProportionRaised(type, pot->amount, PotInfoOf(pot->type).maxAmountInPot);
		}
		return glm::distance(at, glm::vec2(transform.position.x, transform.position.z)) <= radius * multiplier;
	};

	const auto giver = player != PlayerNames::NEUTRAL ? std::optional(player) : std::nullopt;
	uint32_t left = amount;
	const auto origin = map_coords::CellOf(at);
	for (const auto& step : piles::k_SearchCells)
	{
		if (left == 0)
		{
			break;
		}
		// Everything in the cell, as the cell keeps it: whatever stores this, or is a pile of it, takes what it will
		for (const auto entity : Locator::entitiesMap::value().GetAllInCell(origin + step))
		{
			if (left == 0)
			{
				break;
			}
			if (!registry.Valid(entity))
			{
				continue;
			}
			if (registry.AllOf<StoragePit, Abode>(entity))
			{
				if (near(entity, piles::k_StoreReachMultiplier))
				{
					left -= AddToStore(entity, type, left, giver, false);
				}
			}
			else if (const auto* pot = registry.TryGet<const Pot>(entity);
			         pot != nullptr && registry.AllOf<ResourcePile>(entity) && PotInfoOf(pot->type).resourceType == type)
			{
				if (near(entity, piles::k_PotReachMultiplier))
				{
					left -= AddToPile(entity, type, left);
				}
			}
		}
	}
	if (left == 0 || IsWater(point))
	{
		return left != amount;
	}

	// What is left makes a new pile, which thuds as it lands
	const auto potType = type == ResourceType::Food ? PotInfo::MagicFood : PotInfo::MagicWood;
	point.y = LandHeight(at);
	const auto created = ecs::archetypes::PotArchetype::Create(point, 0.0f, potType, static_cast<int32_t>(left));
	if (created == entt::null)
	{
		return left != amount;
	}
	registry.Get<Transform>(created).scale =
	    glm::vec3(type == ResourceType::Food ? piles::k_MagicFoodScale : piles::k_MagicWoodScale);
	registry.Assign<MagicPile>(created, type, player);
	Thud(type, left, point);
	// A power-up's food speeds up the people who take from the new pile, and sparkles over it for as long as it lasts
	auto& pile = registry.Get<ResourcePile>(created);
	if (speedUp && type == ResourceType::Food)
	{
		pile.speedUp = true;
		if (Locator::particleSystem::has_value())
		{
			pile.speedUpVisual =
			    Locator::particleSystem::value().StartSpotVisual(SpotVisualType::PilefoodSpeedup, point, -1, created, 1.0f);
		}
	}
	// The people come to look at it
	const auto reaction = PotInfoOf(potType).associatedReaction;
	if (reaction != Reaction::None && Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().Create({.initiator = created,
		                                         .type = reaction,
		                                         .player = player,
		                                         .position = point,
		                                         .impressiveValue = PotInfoOf(potType).impressiveValue,
		                                         .power = 1.0f,
		                                         .magicType = MagicType::None,
		                                         .casterCreature = entt::null});
	}
	return true;
}

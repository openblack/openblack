/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "EditorEntities.h"

#include <cstring>

#include <algorithm>

#include <fmt/format.h>
#include <glm/gtx/euler_angles.hpp>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "Creature/LeashRules.h"
#include "ECS/Archetypes/AbodeArchetype.h"
#include "ECS/Archetypes/CreatureArchetype.h"
#include "ECS/Archetypes/FeatureArchetype.h"
#include "ECS/Archetypes/MobileObjectArchetype.h"
#include "ECS/Archetypes/MobileStaticArchetype.h"
#include "ECS/Archetypes/TownArchetype.h"
#include "ECS/Archetypes/TreeArchetype.h"
#include "ECS/Archetypes/VillagerArchetype.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureLocomotion.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/StoragePit.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/LeashSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/TownSystemInterface.h"
#include "EditorMath.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/DispenserRules.h"
#include "Magic/MagicTables.h"
#include "Resources/ResourcesInterface.h"

namespace openblack::editor
{

using namespace ecs::components;
using ecs::archetypes::CreatureArchetype;

namespace
{
template <typename Info, size_t N>
std::vector<std::string> NamesFrom(const std::array<Info, N>& table)
{
	std::vector<std::string_view> raw;
	raw.reserve(N);
	for (const auto& info : table)
	{
		raw.emplace_back(info.debugString.data(), strnlen(info.debugString.data(), info.debugString.size()));
	}
	auto names = ReadableNames(raw);
	for (size_t i = 0; i < names.size(); ++i)
	{
		if (names.at(i).empty())
		{
			names.at(i) = fmt::format("Type {}", i);
		}
	}
	return names;
}

/// The miracles' names by magic type, as the tables spell them made readable, for those a dispenser can give
std::vector<std::string> MiracleNames(const InfoConstants& info)
{
	const auto miracles = magic::DispensableMiracles();
	const auto last = std::ranges::max(miracles, {}, [](MagicType type) { return static_cast<uint32_t>(type); });
	std::vector<std::string> names(static_cast<size_t>(last) + 1);
	for (const auto type : miracles)
	{
		const auto& raw = magic::GetMagicEffectInfo(info, type).debugString;
		names.at(static_cast<size_t>(type)) = TitleCase(std::string_view(raw.data(), strnlen(raw.data(), raw.size())));
	}
	return names;
}

/// How high a bubble put down by the editor floats over the land
constexpr float k_BubbleHeight = 3.0f;

constexpr std::array<std::string_view, static_cast<size_t>(CreatureType::_COUNT)> k_SpeciesNames {
    "Unknown",    "Cow",        "Tiger", "Leopard", "Wolf", "Lion",     "Horse", "Tortoise", "Zebra",
    "Brown Bear", "Polar Bear", "Sheep", "Chimp",   "Ogre", "Mandrill", "Rhino", "Gorilla",  "Giant Ape",
};

std::optional<AbodeInfo> AbodeInfoOfMesh(entt::id_type meshId)
{
	if (!Locator::infoConstants::has_value())
	{
		return std::nullopt;
	}
	const auto& abodes = Locator::infoConstants::value().abode;
	for (size_t i = 0; i < abodes.size(); ++i)
	{
		if (resources::HashIdentifier(abodes.at(i).meshId) == meshId)
		{
			return static_cast<AbodeInfo>(i);
		}
	}
	return std::nullopt;
}

std::optional<VillagerInfo> VillagerInfoOf(const Villager& villager)
{
	if (!Locator::infoConstants::has_value())
	{
		return std::nullopt;
	}
	const auto& villagers = Locator::infoConstants::value().villager;
	for (size_t i = 0; i < villagers.size(); ++i)
	{
		const auto& info = villagers.at(i);
		if (info.tribeType == villager.tribe && info.villagerNumber == villager.number)
		{
			return static_cast<VillagerInfo>(i);
		}
	}
	return std::nullopt;
}

/// The town a building goes to: the nearest, or a new one of its tribe where it stands
uint32_t TownFor(glm::vec3 position, AbodeInfo type)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (Locator::townSystem::has_value())
	{
		if (const auto town = Locator::townSystem::value().FindClosestTown(position); town != entt::null)
		{
			return registry.Get<Town>(town).id;
		}
	}
	uint32_t id = 0;
	for (const auto& [townId, entity] : registry.Context().towns)
	{
		id = std::max(id, townId + 1);
	}
	const auto tribe = Locator::infoConstants::value().abode.at(static_cast<size_t>(type)).tribeType;
	ecs::archetypes::TownArchetype::Create(static_cast<int>(id), position, PlayerNames::PLAYER_ONE,
	                                       tribe == Tribe::NONE ? Tribe::CELTIC : tribe);
	return id;
}
} // namespace

std::optional<NameTables> NameTables::Load()
{
	if (!Locator::infoConstants::has_value())
	{
		return std::nullopt;
	}
	const auto& info = Locator::infoConstants::value();
	NameTables tables;
	for (size_t i = 1; i < k_SpeciesNames.size(); ++i)
	{
		tables.creatures.emplace_back(k_SpeciesNames.at(i));
	}
	tables.villagers = NamesFrom(info.villager);
	tables.buildings = NamesFrom(info.abode);
	tables.trees = NamesFrom(info.tree);
	tables.features = NamesFrom(info.feature);
	tables.mobileObjects = NamesFrom(info.mobileObject);
	tables.mobileStatics = NamesFrom(info.mobileStatic);
	tables.miracles = MiracleNames(info);
	return tables;
}

const std::vector<std::string>& NameTables::Of(PlaceKind kind) const
{
	switch (kind)
	{
	case PlaceKind::Creature:
		return creatures;
	case PlaceKind::Villager:
		return villagers;
	case PlaceKind::Building:
		return buildings;
	case PlaceKind::Tree:
		return trees;
	case PlaceKind::Feature:
		return features;
	case PlaceKind::MobileObject:
		return mobileObjects;
	case PlaceKind::Dispenser:
	case PlaceKind::MiracleBubble:
		return miracles;
	case PlaceKind::MobileStatic:
	default:
		return mobileStatics;
	}
}

std::string_view NameTables::NameOf(PlaceKind kind, int32_t type) const
{
	// The creatures' list starts at the first species
	const auto index = kind == PlaceKind::Creature ? type - 1 : type;
	const auto& names = Of(kind);
	return index >= 0 && static_cast<size_t>(index) < names.size() ? std::string_view(names.at(static_cast<size_t>(index)))
	                                                               : std::string_view("Unknown");
}

std::string_view SpeciesName(CreatureType species)
{
	const auto index = static_cast<size_t>(species);
	return index < k_SpeciesNames.size() ? k_SpeciesNames.at(index) : "Unknown";
}

EntityKind KindOf(const ecs::Registry& registry, entt::entity entity)
{
	if (registry.AllOf<Creature>(entity))
	{
		return EntityKind::Creature;
	}
	if (registry.AllOf<Villager>(entity))
	{
		return EntityKind::Villager;
	}
	if (registry.AllOf<Town>(entity))
	{
		return EntityKind::Town;
	}
	if (registry.AnyOf<SpellDispenser, OneOffSpellSeed>(entity))
	{
		return EntityKind::Miracle;
	}
	if (registry.AllOf<Abode>(entity))
	{
		return EntityKind::Building;
	}
	if (registry.AllOf<Field>(entity))
	{
		return EntityKind::Field;
	}
	if (registry.AllOf<Tree>(entity))
	{
		return EntityKind::Tree;
	}
	if (registry.AllOf<MobileStatic>(entity))
	{
		return EntityKind::MobileStatic;
	}
	if (registry.AllOf<Feature>(entity))
	{
		return EntityKind::Feature;
	}
	if (registry.AllOf<MobileObject>(entity))
	{
		return EntityKind::MobileObject;
	}
	if (registry.AllOf<Pot>(entity))
	{
		return EntityKind::Store;
	}
	if (registry.AllOf<Mobile>(entity))
	{
		return EntityKind::Animal;
	}
	return EntityKind::Other;
}

std::optional<PlaceItem> ItemOf(const ecs::Registry& registry, entt::entity entity)
{
	if (const auto* creature = registry.TryGet<Creature>(entity))
	{
		return PlaceItem {.kind = PlaceKind::Creature, .type = static_cast<int32_t>(creature->species)};
	}
	if (const auto* villager = registry.TryGet<Villager>(entity))
	{
		if (const auto type = VillagerInfoOf(*villager))
		{
			return PlaceItem {.kind = PlaceKind::Villager, .type = static_cast<int32_t>(*type)};
		}
		return std::nullopt;
	}
	if (const auto* dispenser = registry.TryGet<SpellDispenser>(entity))
	{
		return PlaceItem {.kind = PlaceKind::Dispenser, .type = static_cast<int32_t>(dispenser->magicType)};
	}
	if (const auto* orb = registry.TryGet<OneOffSpellSeed>(entity))
	{
		if (orb->magicType == MagicType::None)
		{
			return std::nullopt;
		}
		return PlaceItem {.kind = PlaceKind::MiracleBubble, .type = static_cast<int32_t>(orb->magicType)};
	}
	if (registry.AllOf<Abode>(entity))
	{
		if (const auto* mesh = registry.TryGet<Mesh>(entity))
		{
			if (const auto type = AbodeInfoOfMesh(mesh->id))
			{
				return PlaceItem {.kind = PlaceKind::Building, .type = static_cast<int32_t>(*type)};
			}
		}
		return std::nullopt;
	}
	if (const auto* tree = registry.TryGet<Tree>(entity))
	{
		return PlaceItem {.kind = PlaceKind::Tree, .type = static_cast<int32_t>(tree->type)};
	}
	if (const auto* mobileStatic = registry.TryGet<MobileStatic>(entity))
	{
		return PlaceItem {.kind = PlaceKind::MobileStatic, .type = static_cast<int32_t>(mobileStatic->type)};
	}
	if (const auto* feature = registry.TryGet<Feature>(entity))
	{
		return PlaceItem {.kind = PlaceKind::Feature, .type = static_cast<int32_t>(feature->type)};
	}
	if (const auto* object = registry.TryGet<MobileObject>(entity))
	{
		return PlaceItem {.kind = PlaceKind::MobileObject, .type = static_cast<int32_t>(object->type)};
	}
	return std::nullopt;
}

std::string LabelOf(const ecs::Registry& registry, const NameTables& names, entt::entity entity)
{
	if (const auto* town = registry.TryGet<Town>(entity))
	{
		return fmt::format("Town {}", town->id);
	}
	if (const auto* villager = registry.TryGet<Villager>(entity))
	{
		const auto tribe = static_cast<size_t>(villager->tribe);
		const auto number = static_cast<size_t>(villager->number);
		return fmt::format("{} {}{}", tribe < k_TribeStrs.size() ? TitleCase(k_TribeStrs.at(tribe)) : "",
		                   number < k_VillagerNumberStrs.size() ? TitleCase(k_VillagerNumberStrs.at(number)) : "Villager",
		                   villager->lifeStage == Villager::LifeStage::Child ? " (child)" : "");
	}
	if (const auto* creature = registry.TryGet<Creature>(entity))
	{
		return fmt::format("{} of player {}", SpeciesName(creature->species), static_cast<int>(creature->owner) + 1);
	}
	if (const auto item = ItemOf(registry, entity))
	{
		return std::string(names.NameOf(item->kind, item->type));
	}
	if (registry.AllOf<Field>(entity))
	{
		return "Field";
	}
	if (const auto* pot = registry.TryGet<Pot>(entity))
	{
		return fmt::format("Store {}/{}", pot->amount, pot->maxAmount);
	}
	return "Entity";
}

std::optional<AxisAlignedBoundingBox> MeshBox(uint32_t meshId)
{
	if (!Locator::resources::has_value())
	{
		return std::nullopt;
	}
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(meshId))
	{
		return std::nullopt;
	}
	const auto mesh = meshes.Handle(meshId);
	if (!mesh)
	{
		return std::nullopt;
	}
	return mesh->GetBoundingBox();
}

std::optional<uint32_t> MeshOf(const PlaceItem& item)
{
	if (!Locator::infoConstants::has_value())
	{
		return std::nullopt;
	}
	const auto& info = Locator::infoConstants::value();
	const auto index = static_cast<size_t>(item.type);
	switch (item.kind)
	{
	case PlaceKind::Creature:
		return creature::GetIdFromType(static_cast<CreatureType>(item.type), creature::CreatureBody::Appearance::Base);
	case PlaceKind::Villager:
		return index < info.villager.size() ? std::optional(resources::HashIdentifier(info.villager.at(index).highDetail))
		                                    : std::nullopt;
	case PlaceKind::Building:
		return index < info.abode.size() ? std::optional(resources::HashIdentifier(info.abode.at(index).meshId)) : std::nullopt;
	case PlaceKind::Tree:
		return index < info.tree.size() ? std::optional(resources::HashIdentifier(info.tree.at(index).normal)) : std::nullopt;
	case PlaceKind::Feature:
		return index < info.feature.size() ? std::optional(resources::HashIdentifier(info.feature.at(index).meshId))
		                                   : std::nullopt;
	case PlaceKind::MobileObject:
		return index < info.mobileObject.size() ? std::optional(resources::HashIdentifier(info.mobileObject.at(index).meshId))
		                                        : std::nullopt;
	case PlaceKind::MobileStatic:
		return index < info.mobileStatic.size() ? std::optional(resources::HashIdentifier(info.mobileStatic.at(index).meshId))
		                                        : std::nullopt;
	case PlaceKind::Dispenser:
	case PlaceKind::MiracleBubble:
		// Made by the magic system; no model to show while placing
		return std::nullopt;
	}
	return std::nullopt;
}

std::optional<AxisAlignedBoundingBox> WorldBoundsOf(const ecs::Registry& registry, entt::entity entity)
{
	const auto* transform = registry.TryGet<Transform>(entity);
	if (transform == nullptr)
	{
		return std::nullopt;
	}
	if (const auto* mesh = registry.TryGet<Mesh>(entity))
	{
		if (const auto box = MeshBox(mesh->id))
		{
			return WorldBox(*box, transform->position, transform->rotation, transform->scale);
		}
	}
	// Something with no mesh to measure is a small box where it stands
	constexpr float k_HalfSize = 1.0f;
	return AxisAlignedBoundingBox {
	    .minima = transform->position - glm::vec3(k_HalfSize, 0.0f, k_HalfSize),
	    .maxima = transform->position + glm::vec3(k_HalfSize, 2.0f * k_HalfSize, k_HalfSize),
	};
}

float HeightOf(const ecs::Registry& registry, entt::entity entity)
{
	const auto bounds = WorldBoundsOf(registry, entity);
	return bounds.has_value() ? std::max(bounds->Size().y, 1.0f) : 2.0f;
}

entt::entity Place(const PlaceItem& item, glm::vec3 position, float yawRadians)
{
	using namespace ecs::archetypes;
	switch (item.kind)
	{
	case PlaceKind::Creature:
	{
		const auto species = static_cast<CreatureType>(item.type);
		const auto entity =
		    CreatureArchetype::Create(position, PlayerNames::PLAYER_ONE, species, 0, yawRadians,
		                              CreatureArchetype::StartScale(species), CreatureArchetype::StartBody(species));
		// Placed to try things out, it knows every leash, as creatures made by the original's debug tools do
		if (Locator::leashSystem::has_value())
		{
			for (const auto type : creature_leash::k_Types)
			{
				Locator::leashSystem::value().SetKnown(entity, type, true);
			}
		}
		return entity;
	}
	case PlaceKind::Villager:
	{
		constexpr uint32_t k_AdultAge = 30;
		const auto entity = VillagerArchetype::Create(position, position, static_cast<VillagerInfo>(item.type), k_AdultAge);
		Turn(entity, yawRadians);
		return entity;
	}
	case PlaceKind::Building:
	{
		const auto type = static_cast<AbodeInfo>(item.type);
		constexpr uint32_t k_Store = 100;
		return AbodeArchetype::Create(TownFor(position, type), position, type, yawRadians, 1.0f, k_Store, k_Store);
	}
	case PlaceKind::Tree:
		return TreeArchetype::Create(0, position, static_cast<TreeInfo>(item.type), true, yawRadians, 1.0f, 1.0f);
	case PlaceKind::Feature:
		return FeatureArchetype::Create(position, static_cast<FeatureInfo>(item.type), yawRadians, 1.0f);
	case PlaceKind::MobileObject:
		return MobileObjectArchetype::Create(position, static_cast<MobileObjectInfo>(item.type), yawRadians, 1.0f);
	case PlaceKind::MobileStatic:
		return MobileStaticArchetype::Create(position, static_cast<MobileStaticInfo>(item.type), 0.0f, 0.0f, yawRadians, 0.0f,
		                                     1.0f);
	case PlaceKind::Dispenser:
		return Locator::magicSystem::has_value()
		           ? Locator::magicSystem::value().CreateDispenser(position, static_cast<MagicType>(item.type), yawRadians)
		           : entt::null;
	case PlaceKind::MiracleBubble:
		return Locator::magicSystem::has_value()
		           ? Locator::magicSystem::value().CreateOneOffSeedFor(position + glm::vec3(0.0f, k_BubbleHeight, 0.0f),
		                                                               static_cast<MagicType>(item.type))
		           : entt::null;
	}
	return entt::null;
}

std::optional<entt::entity> Duplicate(entt::entity entity, glm::vec2 offset)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto item = ItemOf(registry, entity);
	const auto* transform = registry.TryGet<Transform>(entity);
	if (!item.has_value() || transform == nullptr)
	{
		return std::nullopt;
	}
	const glm::vec2 point {transform->position.x + offset.x, transform->position.z + offset.y};
	const glm::vec3 position {point.x, LandHeight(point), point.y};
	if (const auto* creature = registry.TryGet<Creature>(entity))
	{
		// A creature's copy has its body as it is now
		return CreatureArchetype::Create(
		    position, creature->owner, creature->species, creature->mind, YawOf(transform->rotation), creature->size,
		    {.alignment = creature->alignment, .fatness = creature->fatness, .strength = creature->strength});
	}
	const auto copy = Place(*item, position, 0.0f);
	if (copy != entt::null)
	{
		if (auto* copied = registry.TryGet<Transform>(copy))
		{
			copied->rotation = transform->rotation;
			copied->scale = transform->scale;
			registry.SetDirty();
		}
	}
	return copy;
}

void Remove(entt::entity entity)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(entity))
	{
		return;
	}
	// A dispenser goes with its bubble and swirl, a bubble with its seed
	if (Locator::magicSystem::has_value() && Locator::magicSystem::value().Remove(entity))
	{
		return;
	}
	if (const auto* pit = registry.TryGet<StoragePit>(entity))
	{
		std::vector<entt::entity> piles(pit->woodPiles.begin(), pit->woodPiles.end());
		piles.push_back(pit->foodPile);
		for (const auto pile : piles)
		{
			if (pile != entt::null && registry.Valid(pile))
			{
				registry.Destroy(pile);
			}
		}
	}
	if (const auto* town = registry.TryGet<Town>(entity))
	{
		registry.Context().towns.erase(town->id);
	}
	registry.Destroy(entity);
}

void MoveTo(entt::entity entity, glm::vec3 position)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* transform = registry.TryGet<Transform>(entity);
	if (transform == nullptr)
	{
		return;
	}
	const auto delta = position - transform->position;
	transform->position = position;
	if (auto* fixed = registry.TryGet<Fixed>(entity))
	{
		fixed->boundingCenter += glm::vec2(delta.x, delta.z);
	}
	// A dispenser's bubble goes with it; a bubble floats where it is put, its seed with it
	if (auto* dispenser = registry.TryGet<SpellDispenser>(entity))
	{
		dispenser->orbPosition += delta;
		if (dispenser->orb != entt::null && registry.Valid(dispenser->orb))
		{
			MoveTo(dispenser->orb, registry.Get<Transform>(dispenser->orb).position + delta);
		}
	}
	if (auto* orb = registry.TryGet<OneOffSpellSeed>(entity))
	{
		orb->position += delta;
		if (orb->seedGraphic != entt::null && registry.Valid(orb->seedGraphic))
		{
			registry.Get<Transform>(orb->seedGraphic).position += delta;
		}
	}
	if (auto* locomotion = registry.TryGet<CreatureLocomotion>(entity))
	{
		if (Locator::creatureLocomotionSystem::has_value())
		{
			Locator::creatureLocomotionSystem::value().Stop(entity);
		}
		// The creature is placed between where it was and is going each frame, so both are where it is put
		locomotion->fromPosition = position;
		locomotion->toPosition = position;
	}
	registry.SetDirty();
}

void Turn(entt::entity entity, float radians)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* transform = registry.TryGet<Transform>(entity);
	if (transform == nullptr)
	{
		return;
	}
	transform->rotation = YawRotation(radians) * transform->rotation;
	if (auto* locomotion = registry.TryGet<CreatureLocomotion>(entity))
	{
		const auto heading = YawOf(transform->rotation);
		locomotion->heading = heading;
		locomotion->targetHeading = heading;
		locomotion->fromHeading = heading;
		locomotion->toHeading = heading;
	}
	registry.SetDirty();
}

float LandHeight(glm::vec2 point)
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(point) : 0.0f;
}

} // namespace openblack::editor

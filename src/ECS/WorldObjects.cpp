/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "WorldObjects.h"

#include <algorithm>
#include <vector>

#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/AnimatedStatic.h"
#include "ECS/Components/AtHome.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/DestructionGhost.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Fire.h"
#include "ECS/Components/Flowers.h"
#include "ECS/Components/Forest.h"
#include "ECS/Components/Indestructible.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/MagicFireBall.h"
#include "ECS/Components/MagicForest.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/TeleportStone.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/TownAggression.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AnimalSystemInterface.h"
#include "ECS/Systems/CreatureFightSystemInterface.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/Implementations/VillagerFire.h"
#include "ECS/Systems/Implementations/VillagerHome.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/TownAggression.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Physics/DamageMesh.h"
#include "Physics/LivingRules.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;

namespace
{
/// A building that is built and whose town stands: one whose repair the town would see to
bool IsBuiltBuildingOfATown(const Registry& registry, entt::entity object)
{
	const auto* abode = registry.TryGet<const Abode>(object);
	if (abode == nullptr)
	{
		return false;
	}
	if (const auto* progress = registry.TryGet<const BuildProgress>(object); progress != nullptr && progress->built < 1.0f)
	{
		return false;
	}
	bool townStands = false;
	registry.Each<const Town>(
	    [&townStands, abode](entt::entity, const Town& town) { townStands = townStands || town.id == abode->townId; });
	return townStands;
}

/// A villager's health out of this is its life
constexpr float k_VillagerHealthScale = 100.0f;
/// An object with no model stands this big
constexpr float k_DefaultRadius = 0.5f;
constexpr float k_DefaultHeight = 1.0f;

template <typename Array, typename Index>
const GObjectInfo* Row(const Array& rows, Index index)
{
	const auto i = static_cast<size_t>(index);
	return i < rows.size() ? &rows[i] : nullptr;
}

/// The people at home in a building come out and decide again what to do
void EmptyBuilding(entt::entity abode)
{
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<entt::entity> inside;
	registry.Each<const Villager, const AtHome>([&](entt::entity villager, const Villager& person, const AtHome&) {
		if (person.abode == abode)
		{
			inside.push_back(villager);
		}
	});
	for (const auto villager : inside)
	{
		villager_home::LeaveHome(villager);
		if (auto* action = registry.TryGet<LivingAction>(villager);
		    action != nullptr && Locator::livingActionSystem::has_value())
		{
			Locator::livingActionSystem::value().VillagerSetState(*action, LivingAction::Index::Top,
			                                                      VillagerStates::DecideWhatToDo, true);
		}
	}
}
} // namespace

const GAbodeInfo* world_objects::AbodeInfoOf(entt::entity object)
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* abode = registry.TryGet<const Abode>(object);
	if (abode == nullptr || !Locator::infoConstants::has_value())
	{
		return nullptr;
	}
	const auto* mesh = registry.TryGet<const Mesh>(object);
	const GAbodeInfo* first = nullptr;
	for (const auto& info : Locator::infoConstants::value().abode)
	{
		if (info.abodeNumber != abode->type)
		{
			continue;
		}
		if (mesh != nullptr && resources::HashIdentifier(info.meshId) == mesh->id)
		{
			return &info;
		}
		if (first == nullptr)
		{
			first = &info;
		}
	}
	return first;
}

const GObjectInfo* world_objects::InfoOf(entt::entity object)
{
	if (!Locator::infoConstants::has_value())
	{
		return nullptr;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object))
	{
		return nullptr;
	}
	const auto& info = Locator::infoConstants::value();
	if (const auto* villager = registry.TryGet<const Villager>(object))
	{
		return Row(info.villager, GVillagerInfo::Find(villager->tribe, villager->number));
	}
	if (const auto* creature = registry.TryGet<const Creature>(object))
	{
		return Row(info.creature, creature::InfoRow(creature->species));
	}
	if (const auto* tree = registry.TryGet<const Tree>(object))
	{
		return Row(info.tree, tree->type);
	}
	if (registry.AllOf<DeadTree>(object))
	{
		return Row(info.mobileStatic, MobileStaticInfo::DeadTree);
	}
	if (const auto* field = registry.TryGet<const Field>(object))
	{
		return Row(info.fieldType, field->type);
	}
	if (registry.AllOf<Abode>(object))
	{
		return AbodeInfoOf(object);
	}
	if (const auto* forest = registry.TryGet<const BigForest>(object))
	{
		return Row(info.bigForest, forest->type);
	}
	if (const auto* animated = registry.TryGet<const AnimatedStatic>(object))
	{
		return Row(info.animatedStatic, animated->type);
	}
	if (const auto* feature = registry.TryGet<const Feature>(object))
	{
		return Row(info.feature, feature->type);
	}
	if (const auto* pot = registry.TryGet<const Pot>(object))
	{
		return Row(info.pot, pot->type);
	}
	if (const auto* still = registry.TryGet<const MobileStatic>(object))
	{
		return Row(info.mobileStatic, still->type);
	}
	if (const auto* mobile = registry.TryGet<const MobileObject>(object))
	{
		return Row(info.mobileObject, mobile->type);
	}
	return nullptr;
}

world_objects::Size world_objects::SizeOf(entt::entity object)
{
	const auto& registry = Locator::entitiesRegistry::value();
	Size size {.radius = k_DefaultRadius, .height = k_DefaultHeight};
	const auto* transform = registry.TryGet<const Transform>(object);
	const auto* mesh = registry.TryGet<const Mesh>(object);
	if (transform == nullptr || mesh == nullptr || !Locator::resources::has_value())
	{
		return size;
	}
	auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(mesh->id))
	{
		return size;
	}
	const auto box = meshes.Handle(mesh->id)->GetBoundingBox().Size() * transform->scale;
	size.radius = std::max(box.x, box.z) * 0.5f;
	size.height = box.y;
	return size;
}

float world_objects::LifeOf(entt::entity object)
{
	const auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object))
	{
		return 0.0f;
	}
	if (const auto* needs = registry.TryGet<const CreatureNeeds>(object))
	{
		return needs->needs.life;
	}
	if (const auto* life = registry.TryGet<const ObjectLife>(object))
	{
		return life->life;
	}
	if (const auto* villager = registry.TryGet<const Villager>(object))
	{
		return static_cast<float>(villager->health) / k_VillagerHealthScale;
	}
	return 1.0f;
}

float world_objects::ReduceLife(entt::entity object, float damage)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object))
	{
		return 0.0f;
	}
	// A field burns its food and keeps its life
	if (auto* field = registry.TryGet<Field>(object))
	{
		if (const auto* info = Row(Locator::infoConstants::value().fieldType, field->type))
		{
			const auto& type = static_cast<const GFieldTypeInfo&>(*info);
			field->crop.food = std::max(field->crop.food - damage * type.totalFoodInField, 0.0f);
		}
		return 1.0f;
	}
	if (auto* needs = registry.TryGet<CreatureNeeds>(object))
	{
		needs->needs.life = std::max(needs->needs.life - damage, 0.0f);
		// A creature with no life left is knocked out
		if (needs->needs.life <= 0.0f && Locator::creatureFightSystem::has_value() &&
		    !Locator::creatureFightSystem::value().IsKnockedOut(object))
		{
			Locator::creatureFightSystem::value().KnockOut(object);
		}
		return needs->needs.life;
	}
	// A building not yet built loses what is built of it rather than life, and all its life once nothing is
	if (auto* progress = registry.TryGet<BuildProgress>(object);
	    progress != nullptr && progress->built < 1.0f && registry.AnyOf<Abode, SpellDispenser>(object))
	{
		progress->built = progress->built - damage <= 0.0f ? 0.0f : progress->built - damage;
		if (progress->built != 0.0f)
		{
			return LifeOf(object);
		}
		damage = LifeOf(object);
	}
	// Villagers keep their life to a fraction of their health, so that small hurts add up
	const float before = LifeOf(object);
	auto& life = registry.AllOf<ObjectLife>(object) ? registry.Get<ObjectLife>(object)
	                                                : registry.Assign<ObjectLife>(object, ObjectLife {.life = before});
	life.life = std::max(life.life - damage, 0.0f);
	if (auto* villager = registry.TryGet<Villager>(object))
	{
		villager->health = static_cast<uint32_t>(std::ceil(life.life * k_VillagerHealthScale));
		CountInjury(object, before, life.life);
	}
	if (registry.AllOf<Abode>(object) && damage > 0.0f)
	{
		// Its people come out of a building that is being hurt
		EmptyBuilding(object);
	}
	// A built building of a town left under its full life gets a site for its repair, which starts from a little less
	// than the life it is left with
	if (life.life < 1.0f && damage > 0.0f && IsBuiltBuildingOfATown(registry, object))
	{
		registry.AssignOrReplace<RepairSite>(object, RepairSite {.startLife = physics::damage::RepairStartLife(life.life)});
	}
	return life.life;
}

bool world_objects::IsBuilding(entt::entity object)
{
	return Locator::entitiesRegistry::value().AnyOf<Abode, SpellDispenser, Field>(object);
}

bool world_objects::CanBeDestroyedBySpell(entt::entity object)
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto can = [&registry, object] {
		// Nor anything a script made indestructible
		if (!registry.Valid(object) ||
		    registry.AnyOf<Creature, Field, Temple, TeleportStone, OneOffSpellSeed, Indestructible>(object))
		{
			return false;
		}
		if (const auto* abode = registry.TryGet<const Abode>(object); abode != nullptr && abode->type == AbodeNumber::Totem)
		{
			return false;
		}
		if (const auto* pot = registry.TryGet<const Pot>(object))
		{
			return pot->amount != 0;
		}
		return true;
	}();
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "World: #{} {} be destroyed by a miracle", entt::to_integral(object),
	                    can ? "may" : "may not");
	return can;
}

bool world_objects::IsVillager(entt::entity object)
{
	return Locator::entitiesRegistry::value().AllOf<Villager>(object);
}

bool world_objects::IsCreature(entt::entity object)
{
	return Locator::entitiesRegistry::value().AllOf<Creature>(object);
}

void world_objects::Destroy(entt::entity object)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object) || IsCreature(object))
	{
		return;
	}
	if (IsBuilding(object))
	{
		// It is left standing with no life, its people out
		ReduceLife(object, LifeOf(object));
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "World: building #{} left standing with no life", entt::to_integral(object));
		return;
	}
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "World: #{} destroyed", entt::to_integral(object));
	Remove(object);
}

void world_objects::LeaveGhost(entt::entity object)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* mesh = registry.TryGet<const Mesh>(object);
	const auto* transform = registry.TryGet<const Transform>(object);
	if (mesh == nullptr || transform == nullptr)
	{
		return;
	}
	auto model = glm::translate(glm::mat4(1.0f), transform->position) * glm::mat4(transform->rotation);
	model = glm::scale(model, transform->scale);
	const auto ghost = registry.Create();
	registry.Assign<DestructionGhost>(ghost, DestructionGhost {.mesh = mesh->id, .model = model});
}

void world_objects::DestroyedByEffect(entt::entity object, std::optional<EffectDeath> death)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object) || IsCreature(object))
	{
		return;
	}
	if (registry.AnyOf<Abode, SpellDispenser>(object))
	{
		// A ghost of it flickers out where it stood, and it goes
		LeaveGhost(object);
		Remove(object);
		return;
	}
	if (registry.AllOf<Villager>(object))
	{
		// Any effect's death counts as killed by a spell, put down to the effect's player
		villager_fire::DieByEffect(object, death.has_value() ? std::optional(villager_fire::DeathCause {
		                                                           .reason = DeathReason::Spell,
		                                                           .killer = death->killer,
		                                                           .weight = death->weight,
		                                                       })
		                                                     : std::nullopt);
		return;
	}
	// An animal falls dead rather than vanishing; a miracle's fades out
	if (registry.AllOf<Animal>(object) && Locator::animalSystem::has_value())
	{
		if (registry.AllOf<SpellAnimal>(object))
		{
			Locator::animalSystem::value().StartFading(object);
		}
		else
		{
			Locator::animalSystem::value().SetDying(object);
		}
		return;
	}
	if (auto* field = registry.TryGet<Field>(object))
	{
		// Its crop is gone, and its fire with it
		field->crop.timesSown = 0;
		field->crop.age = 0.0f;
		field->crop.food = 0.0f;
		if (Locator::fireSystem::has_value())
		{
			Locator::fireSystem::value().Forget(object);
		}
		return;
	}
	Destroy(object);
}

void world_objects::Remove(entt::entity object)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object))
	{
		return;
	}
	if (Locator::fireSystem::has_value())
	{
		Locator::fireSystem::value().Forget(object);
	}
	if (const auto* villager = registry.TryGet<const Villager>(object))
	{
		villager_home::LeaveHome(object);
		if (villager->abode != entt::null && registry.Valid(villager->abode))
		{
			if (auto* abode = registry.TryGet<Abode>(villager->abode))
			{
				abode->inhabitants.erase(object);
			}
		}
	}
	registry.Destroy(object);
	registry.SetDirty();
}

void world_objects::AttackTown(entt::entity object, float damage, PlayerNames aggressor)
{
	if (!Locator::infoConstants::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	const auto& info = Locator::infoConstants::value();
	entt::entity town = entt::null;
	const GObjectInfo* kind = nullptr;
	if (const auto* villager = registry.TryGet<const Villager>(object))
	{
		town = villager->town;
		kind = InfoOf(object);
	}
	else if (const auto* abode = registry.TryGet<const Abode>(object))
	{
		registry.Each<const Town>([&](entt::entity entity, const Town& candidate) {
			if (candidate.id == abode->townId)
			{
				town = entity;
			}
		});
		if (town != entt::null)
		{
			if (const auto* tribe = registry.TryGet<const Tribe>(town))
			{
				const auto row = static_cast<size_t>(GAbodeInfo::Find(*tribe, abode->type));
				kind = row < info.abode.size() ? &info.abode.at(row) : nullptr;
			}
		}
	}
	if (town == entt::null || !registry.Valid(town) || kind == nullptr)
	{
		return;
	}
	const float amount = ecs::town_aggression::AggressionFromDamage(damage, kind->aggressorValue);
	if (amount == 0.0f)
	{
		return;
	}
	auto* aggression = registry.TryGet<TownAggression>(town);
	if (aggression == nullptr)
	{
		aggression = &registry.Assign<TownAggression>(town);
	}
	const bool owner = registry.Get<const Town>(town).owner == aggressor;
	const uint32_t turn = Locator::time::has_value() ? static_cast<uint32_t>(Locator::time::value().GetTurn()) : 0;
	ecs::town_aggression::Attacked(aggression->record, aggressor, owner, amount, info.town.firstTimeDamageDoneAddition, turn);
}

void world_objects::CountInjury(entt::entity villager, float before, float after)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* person = registry.TryGet<const Villager>(villager);
	if (person == nullptr || !registry.Valid(person->town))
	{
		return;
	}
	auto* town = registry.TryGet<Town>(person->town);
	if (town == nullptr)
	{
		return;
	}
	const int change = physics::living::InjuredChange(before, after);
	if (change > 0)
	{
		++town->injured;
	}
	else if (change < 0 && town->injured > 0)
	{
		--town->injured;
	}
}

float world_objects::IncreaseLife(entt::entity object, float amount)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object))
	{
		return 0.0f;
	}
	const float before = LifeOf(object);
	auto& life = registry.AllOf<ObjectLife>(object) ? registry.Get<ObjectLife>(object)
	                                                : registry.Assign<ObjectLife>(object, ObjectLife {.life = before});
	life.life = std::min(life.life + amount, 1.0f);
	return life.life;
}

bool world_objects::CanBeCrushed(entt::entity object)
{
	const auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object))
	{
		return false;
	}
	return registry.AnyOf<Villager, Creature, Animal, Abode, Tree, DeadTree, Feature, Field, AnimatedStatic, BigForest,
	                      MobileStatic, Flowers>(object);
}

std::optional<PlayerNames> world_objects::PlayerOf(entt::entity object)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object))
	{
		return std::nullopt;
	}
	const auto townOwner = [&registry](auto matches) -> std::optional<PlayerNames> {
		std::optional<PlayerNames> owner;
		registry.Each<const Town>([&](entt::entity entity, const Town& town) {
			if (matches(entity, town))
			{
				owner = town.owner;
			}
		});
		return owner;
	};
	if (const auto* creature = registry.TryGet<const Creature>(object))
	{
		return creature->owner;
	}
	if (const auto* ball = registry.TryGet<const MagicFireBall>(object))
	{
		return ball->hasPlayer ? ball->player : PlayerNames::NEUTRAL;
	}
	if (const auto* villager = registry.TryGet<const Villager>(object))
	{
		const auto* town = registry.Valid(villager->town) ? registry.TryGet<const Town>(villager->town) : nullptr;
		return town != nullptr ? std::optional(town->owner) : std::nullopt;
	}
	if (const auto* field = registry.TryGet<const Field>(object))
	{
		return townOwner([field](entt::entity, const Town& town) { return static_cast<int>(town.id) == field->town; });
	}
	if (const auto* abode = registry.TryGet<const Abode>(object))
	{
		return townOwner([abode](entt::entity, const Town& town) { return town.id == abode->townId; })
		    .value_or(PlayerNames::NEUTRAL);
	}
	if (const auto* magic = registry.TryGet<const MagicTree>(object))
	{
		const auto* forest = registry.Valid(magic->forest) ? registry.TryGet<const MagicForest>(magic->forest) : nullptr;
		return forest != nullptr ? std::optional(forest->player) : std::nullopt;
	}
	if (registry.AnyOf<Tree, DeadTree>(object))
	{
		return std::nullopt;
	}
	return PlayerNames::NEUTRAL;
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "MagicSystem.h"

#include <cmath>

#include <algorithm>
#include <chrono>
#include <numbers>

#include <LNDFile.h>
#include <entt/core/hashed_string.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "3D/WaterRings.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Camera/Camera.h"
#include "Common/GUtilsAngle.h"
#include "Common/GUtilsDistance.h"
#include "Common/GameRandom.h"
#include "Creature/CreatureAudio.h"
#include "Creature/CreatureLocomotion.h"
#include "Creature/PerceivedDesires.h"
#include "ECS/Archetypes/PotArchetype.h"
#include "ECS/Archetypes/SpellSeedArchetype.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/AnimatedStatic.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureLocomotion.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/CreatureSpells.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/Forest.h"
#include "ECS/Components/MagicFireBall.h"
#include "ECS/Components/MagicPile.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Player.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/PrayerPower.h"
#include "ECS/Components/Spell.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/SpellSeed.h"
#include "ECS/Components/StoragePit.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/TownAggression.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/CreatureFightSystemInterface.h"
#include "ECS/Systems/CreatureHandSystemInterface.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/ForestSystemInterface.h"
#include "ECS/Systems/GestureEventsInterface.h"
#include "ECS/Systems/InfluenceSystemInterface.h"
#include "ECS/Systems/LeashSystemInterface.h"
#include "ECS/Systems/MagicShieldSystemInterface.h"
#include "ECS/Systems/MiracleFxSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "ECS/Systems/TeleportSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/Systems/WaterRingSystemInterface.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "FileSystem/FileSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/DispenserRules.h"
#include "Magic/MagicTables.h"
#include "Magic/MiracleDeeds.h"
#include "Magic/SpellLifetime.h"
#include "Magic/SpellSeedRules.h"
#include "Magic/WaterRules.h"
#include "MagicLiving.h"
#include "Particles/ParticleEffect.h"
#include "Particles/ParticleMiracleMaths.h"
#include "Particles/ParticleShields.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
constexpr float k_TurnSeconds = std::chrono::duration<float>(TimeSystemInterface::k_TurnDuration).count();
constexpr float k_TurnsPerSecond = 1.0f / k_TurnSeconds;
/// A creature with nothing to look at looks ahead, along its heading
constexpr float k_LookAheadMetres = 10.0f;
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;

/// The hand's speed is smoothed over about this many seconds
constexpr float k_HandSpeedSmoothing = 0.1f;
/// The kind of building a dispenser is
constexpr auto k_DispenserBuilding = AbodeInfo::NorseSpellDispenser;
/// The size a miracle cast from the hand takes without a gesture: a sizing miracle its tables' usual size, any other
/// the plain size
constexpr float k_PlainHandMagnitude = 1.0f;
/// The id of the bubble's model in the resource cache
constexpr auto k_BubbleMeshId = OneOffSpellSeed::k_MeshId;
/// How far round a point a struck shield is looked for, to show its spark
constexpr float k_ShieldStrikeMargin = 2.0f;
/// A bubble is easier to tap than its model is big
constexpr float k_OrbTapLeeway = 1.3f;
constexpr float k_DefaultOrbRadius = 2.5f;

// The game's world
/// A villager's health out of this is its life
constexpr float k_VillagerHealthScale = 100.0f;
/// The colours of the rings the water's drops leave on the land, picked at random
constexpr std::array<uint32_t, 5> k_RippleColours = {0xFF80CBC5u, 0xFF8599C5u, 0xFFBA97B2u, 0xFFB9CA86u, 0xFFBD9C8Au};
/// A tree rustles as the water grows it, one of these at random
constexpr std::array<audio::SoundId, 9> k_TreeGrowSounds {
    audio::SoundId::G_TreeGrow_01_1, audio::SoundId::G_TreeGrow_02_1, audio::SoundId::G_TreeGrow_03_1,
    audio::SoundId::G_TreeGrow_04_1, audio::SoundId::G_TreeGrow_01_2, audio::SoundId::G_TreeGrow_02_2,
    audio::SoundId::G_TreeGrow_03_2, audio::SoundId::G_TreeGrow_04_2, audio::SoundId::G_TreeGrow_01_3};
/// The water reaches a field as though it were this wide round its middle
constexpr float k_FieldWaterRadius = 5.0f;

ecs::Registry& EntityRegistry()
{
	return Locator::entitiesRegistry::value();
}

const GObjectInfo* InfoOfLiving(entt::entity entity)
{
	return magic_living::InfoOf(entity);
}

/// A living thing's life, 0 to 1, none for anything else
std::optional<float> LifeOf(entt::entity entity)
{
	return magic_living::LifeOf(entity);
}

void SetLife(entt::entity entity, float life)
{
	magic_living::SetLife(entity, life);
}

/// Calls the function with every living thing (creatures, villagers and animals) and where it stands
template <typename F>
void EachLiving(F&& function)
{
	const auto& registry = EntityRegistry();
	registry.Each<const Creature, const Transform>(
	    [&](entt::entity entity, const Creature&, const Transform& transform) { function(entity, transform.position); });
	registry.Each<const Villager, const Transform>(
	    [&](entt::entity entity, const Villager&, const Transform& transform) { function(entity, transform.position); });
	registry.Each<const Animal, const Transform>(
	    [&](entt::entity entity, const Animal&, const Transform& transform) { function(entity, transform.position); });
}

/// The alignment kind of what isn't living, by what it is
AlignmentType AlignmentKindOf(entt::entity entity)
{
	const auto& registry = EntityRegistry();
	if (registry.AnyOf<Tree>(entity))
	{
		return AlignmentType::Plant;
	}
	if (registry.AnyOf<Abode>(entity))
	{
		return AlignmentType::Building;
	}
	if (registry.AnyOf<Feature>(entity))
	{
		return AlignmentType::Feature;
	}
	return AlignmentType::MobileObject;
}

/// How an object stands for an effect: where, how big, its life and its defence
std::optional<magic::EffectReceiver> ReceiverOf(entt::entity entity)
{
	const auto& registry = EntityRegistry();
	const auto* transform = registry.TryGet<const Transform>(entity);
	if (transform == nullptr)
	{
		return std::nullopt;
	}
	magic::EffectReceiver receiver {.entity = entity, .position = transform->position, .radius = 0.5f, .height = 1.0f};
	if (const auto* mesh = registry.TryGet<const Mesh>(entity))
	{
		auto& meshes = Locator::resources::value().GetMeshes();
		if (meshes.Contains(mesh->id))
		{
			const auto size = meshes.Handle(mesh->id)->GetBoundingBox().Size() * transform->scale;
			receiver.radius = std::max(size.x, size.z) * 0.5f;
			receiver.height = size.y;
		}
	}
	if (const auto* info = InfoOfLiving(entity))
	{
		receiver.life = LifeOf(entity);
		receiver.defence = magic_living::DefenceOf(entity);
		receiver.alignmentType = info->alignmentType;
		receiver.crushable = true;
	}
	else if (const auto* objectInfo = ecs::world_objects::InfoOf(entity))
	{
		// Every object loses life to a crush or a hit by its own defence, but a field: crushing or hitting it takes nothing
		if (!registry.AnyOf<Field>(entity))
		{
			receiver.life = ecs::world_objects::LifeOf(entity);
		}
		receiver.defence = magic::EffectDefence::From(*objectInfo);
		receiver.alignmentType = objectInfo->alignmentType;
		receiver.crushable = ecs::world_objects::CanBeCrushed(entity);
	}
	else
	{
		receiver.alignmentType = AlignmentKindOf(entity);
		receiver.crushable = ecs::world_objects::CanBeCrushed(entity);
	}
	return receiver;
}

/// The player whose object was harmed remembers the harm against the player whose miracle did it
void AddDamageFrom(PlayerNames owner, PlayerNames from, float damage)
{
	const auto index = static_cast<size_t>(from);
	EntityRegistry().Each<Player>([&](entt::entity, Player& player) {
		if (player.name == owner && index < player.damageFrom.size())
		{
			player.damageFrom.at(index) += damage;
		}
	});
}

/// What a miracle does to an object it takes the last of the life from: a creature that can die faints and one that
/// can't has all its life back, anything else is destroyed as its kind is
void DestroyedByEffect(entt::entity entity)
{
	auto& registry = EntityRegistry();
	if (const auto* creature = registry.TryGet<const Creature>(entity))
	{
		if (!creature->canDie)
		{
			SetLife(entity, 1.0f);
		}
		else if (Locator::creatureFightSystem::has_value())
		{
			Locator::creatureFightSystem::value().ForceFaint(entity);
		}
		return;
	}
	ecs::world_objects::DestroyedByEffect(entity);
}

/// The alignment of whoever an effect comes from, which damps how far it moves
float AlignmentOf(const magic::EffectSource& source)
{
	const auto& registry = EntityRegistry();
	if (source.casterCreature != entt::null && registry.Valid(source.casterCreature))
	{
		if (const auto* creature = registry.TryGet<const Creature>(source.casterCreature))
		{
			return creature->alignment;
		}
	}
	return Locator::alignmentSystem::has_value() ? Locator::alignmentSystem::value().GetPlayerAlignment(source.player) : 0.0f;
}

/// How tall a model stands at a scale
float HeightOfMesh(entt::id_type mesh, float scale)
{
	auto& meshes = Locator::resources::value().GetMeshes();
	return meshes.Contains(mesh) ? meshes.Handle(mesh)->GetBoundingBox().Size().y * scale : 0.0f;
}
} // namespace

// The game's world

float GameMagicWorld::LandHeight(glm::vec2 xz) const
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(xz) : 0.0f;
}

bool GameMagicWorld::InBounds(glm::vec3 point) const
{
	if (!Locator::terrainSystem::has_value())
	{
		return false;
	}
	const auto extent = Locator::terrainSystem::value().GetExtent();
	return point.x >= extent.minimum.x && point.z >= extent.minimum.y && point.x < extent.maximum.x &&
	       point.z < extent.maximum.y;
}

bool GameMagicWorld::IsDryLand(glm::vec3 point) const
{
	if (!InBounds(point) || point.x < 0.0f || point.z < 0.0f)
	{
		return false;
	}
	// Land is dry where it stands above the sea's level, by its cell's altitude
	constexpr float k_CellSize = 10.0f;
	constexpr uint8_t k_LowestDryAltitude = 4;
	const auto cell = glm::u16vec2(glm::floor(glm::vec2(point.x, point.z) / k_CellSize));
	const auto* landCell = Locator::terrainSystem::value().FindCell(cell);
	return landCell != nullptr && landCell->altitude >= k_LowestDryAltitude;
}

bool GameMagicWorld::IsLand(glm::vec3 point) const
{
	if (!InBounds(point) || point.x < 0.0f || point.z < 0.0f)
	{
		return false;
	}
	// Land is a cell without water in it
	constexpr float k_CellSize = 10.0f;
	const auto cell = glm::u16vec2(glm::floor(glm::vec2(point.x, point.z) / k_CellSize));
	const auto* landCell = Locator::terrainSystem::value().FindCell(cell);
	return landCell != nullptr && landCell->properties.hasWater == 0;
}

bool GameMagicWorld::InInfluence(PlayerNames player, glm::vec3 point) const
{
	return Locator::influenceSystem::has_value() && Locator::influenceSystem::value().PlayerInfluence(player, point) > 0.0f;
}

std::optional<glm::vec3> GameMagicWorld::PositionOf(entt::entity object) const
{
	const auto& registry = EntityRegistry();
	if (!registry.Valid(object))
	{
		return std::nullopt;
	}
	const auto* transform = registry.TryGet<const Transform>(object);
	return transform != nullptr ? std::optional(transform->position) : std::nullopt;
}

bool GameMagicWorld::ApplyEffect(entt::entity object, const magic::EffectValues& values, const magic::EffectSource& source)
{
	if (!EntityRegistry().Valid(object))
	{
		return false;
	}
	const auto receiver = ReceiverOf(object);
	if (!receiver.has_value())
	{
		return false;
	}
	// A creature takes some effects its own way, such as a heal in a fight
	if (magic_living::TakesEffectItsOwnWay(object, values, source))
	{
		return true;
	}
	const auto& info = Locator::infoConstants::value();
	const std::array outcomes {magic::ApplyEffectTo(values, *receiver, info.alignment,
	                                                info.player.applyEffectAlignmentChangeAddition, AlignmentOf(source))};
	Apply(values, outcomes, source);
	return receiver->life.has_value();
}

std::vector<entt::entity> GameMagicWorld::ApplyEffectAt(glm::vec3 point, const magic::EffectValues& values,
                                                        const magic::EffectSource& source)
{
	// Everything in the cells round the point that the effect reaches takes it
	auto receivers = ReceiversIn(magic::EffectCellsAround(point, values.radius));
	// A creature takes some effects its own way, such as a heal in a fight
	std::erase_if(receivers, [&](const magic::EffectReceiver& receiver) {
		return magic::Reaches(values, point, receiver) && magic_living::TakesEffectItsOwnWay(receiver.entity, values, source);
	});
	const auto& info = Locator::infoConstants::value();
	const auto outcomes = magic::ApplyEffectToAll(values, point, receivers, info.alignment,
	                                              info.player.applyEffectAlignmentChangeAddition, AlignmentOf(source));
	Apply(values, outcomes, source);
	std::vector<entt::entity> reached;
	reached.reserve(outcomes.size());
	std::ranges::transform(outcomes, std::back_inserter(reached), &magic::EffectOutcome::entity);
	return reached;
}

std::vector<magic::EffectReceiver> GameMagicWorld::ReceiversIn(const magic::CellRange& cells)
{
	auto& registry = EntityRegistry();
	const auto& map = Locator::entitiesMap::value();
	std::vector<magic::EffectReceiver> receivers;
	// x by x, and in each x z by z; in each cell everything as the cell keeps it, a building once for every cell it
	// covers
	for (int x = cells.first.x; x <= cells.last.x; ++x)
	{
		for (int z = cells.first.y; z <= cells.last.y; ++z)
		{
			for (const auto entity : map.GetAllInCell({x, z}))
			{
				if (!registry.Valid(entity) ||
				    !registry.AnyOf<Creature, Villager, Animal, Tree, Abode, Feature, Pot, Field, DeadTree, MobileStatic,
				                    MobileObject, AnimatedStatic, BigForest>(entity))
				{
					continue;
				}
				if (auto receiver = ReceiverOf(entity))
				{
					receivers.push_back(*receiver);
				}
			}
		}
	}
	return receivers;
}

void GameMagicWorld::Apply(const magic::EffectValues& values, std::span<const magic::EffectOutcome> outcomes,
                           const magic::EffectSource& source)
{
	auto& registry = EntityRegistry();
	const float burn = values[magic::EffectKind::Burn];
	const bool byCreature = source.casterCreature != entt::null && registry.Valid(source.casterCreature) &&
	                        registry.AllOf<Creature>(source.casterCreature);
	float alignment = 0.0f;
	for (const auto& outcome : outcomes)
	{
		if (!registry.Valid(outcome.entity))
		{
			continue;
		}
		// A burn heats what it reaches, which may catch and burn; a negative one, as water's, cools what is hot
		if (burn != 0.0f && Locator::fireSystem::has_value())
		{
			Locator::fireSystem::value().ApplyBurn(outcome.entity, burn, source.player);
		}
		if (outcome.lifeAfter.has_value())
		{
			if (LifeOf(outcome.entity).has_value())
			{
				SetLife(outcome.entity, *outcome.lifeAfter);
			}
			else
			{
				// Anything else living takes the heal, then the harm, as its kind takes it: a building hurt sends its
				// people out
				if (outcome.healed > 0.0f)
				{
					ecs::world_objects::IncreaseLife(outcome.entity, outcome.healed);
				}
				if (outcome.damaged > 0.0f)
				{
					ecs::world_objects::ReduceLife(outcome.entity, outcome.damaged);
				}
			}
		}
		magic_living::AfterEffect(outcome.entity, values, source);
		alignment += outcome.alignmentChange;
		// Harm done to a town's people or buildings counts as an attack on the town by whoever did it
		if (outcome.aggression)
		{
			ecs::world_objects::AttackTown(outcome.entity, outcome.damaged + outcome.burnt, source.player);
		}
		// A player's miracle, not a creature's, is remembered against them by the player whose object it harmed
		const auto owner = ecs::world_objects::PlayerOf(outcome.entity);
		if (!byCreature && owner.has_value())
		{
			AddDamageFrom(*owner, source.player, outcome.damaged + outcome.burnt);
		}
		// The people nearby react to what was crushed, once for each thing: to the creature that crushed it if a creature
		// cast the miracle, else to the thing itself, for the thing's player
		if (outcome.crushed && Locator::reactionSystem::has_value() &&
		    !Locator::reactionSystem::value().HasReaction(outcome.entity))
		{
			const auto initiator = byCreature ? source.casterCreature : outcome.entity;
			if (const auto position = PositionOf(initiator))
			{
				Locator::reactionSystem::value().Create({.initiator = initiator,
				                                         .type = Reaction::ReactToObjectCrushed,
				                                         .player = owner.value_or(PlayerNames::NEUTRAL),
				                                         .position = *position,
				                                         .playerless = !owner.has_value(),
				                                         .onCast = true});
			}
		}
		// Brought from some life to none it is destroyed, by the miracle's creature if one cast it
		const bool destroyed = outcome.destroyed;
		if (destroyed && byCreature)
		{
			registry.Get<Creature>(source.casterCreature).objectsDestroyed += 1.0f;
		}
		if (destroyed)
		{
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Magic: a miracle took the last of {}'s life",
			                    entt::to_integral(outcome.entity));
			DestroyedByEffect(outcome.entity);
		}
	}
	if (alignment == 0.0f)
	{
		return;
	}
	// What a creature's miracle did moves its own alignment, what a player's did the player's, over the turns to come
	if (byCreature)
	{
		registry.Get<Creature>(source.casterCreature).pendingAlignment += alignment;
	}
	else if (source.player != PlayerNames::NEUTRAL && Locator::alignmentSystem::has_value())
	{
		Locator::alignmentSystem::value().AddPendingAlignment(source.player, alignment);
	}
}

void GameMagicWorld::Water(const magic::WaterDrop& drop)
{
	auto& registry = EntityRegistry();
	// The objects whose edge the drop falls within reach of across the ground take the water: trees grow, fields are
	// sown and ripen, and the people come to see a fire put out. They are looked for in the nine cells round the drop's,
	// in a spiral out from it; one spanning several of them takes the water in each.
	if (Locator::entitiesMap::has_value())
	{
		const auto& map = Locator::entitiesMap::value();
		const auto start = glm::ivec2(ecs::MapInterface::GetGridCell(drop.position));
		glm::ivec2 offset {0, 0};
		map_coords::Spiral spiral;
		constexpr int k_DropCells = 9;
		std::vector<entt::entity> objects;
		for (int step = 0; step < k_DropCells; ++step)
		{
			const auto cell = start + offset;
			if (map_coords::InBounds(cell))
			{
				const auto id = ecs::MapInterface::CellId(cell);
				objects.assign(map.GetFixedInGridCell(id).begin(), map.GetFixedInGridCell(id).end());
				objects.insert(objects.end(), map.GetMobileInGridCell(id).begin(), map.GetMobileInGridCell(id).end());
				for (const auto object : objects)
				{
					const auto* transform = registry.Valid(object) ? registry.TryGet<const Transform>(object) : nullptr;
					if (transform == nullptr)
					{
						continue;
					}
					// A field counts as 5 m across whatever its size; anything else by its model
					const float across = gutils::GetDistance(transform->position, drop.position);
					const float radius =
					    registry.AllOf<Field>(object) ? k_FieldWaterRadius : ecs::world_objects::SizeOf(object).radius;
					if (across - radius < drop.reach)
					{
						WaterObject(object, drop);
					}
				}
			}
			const auto& next = spiral.Next();
			offset += glm::ivec2(next.x, next.z);
		}
	}
	if (drop.ringGrowth.has_value() && Locator::waterRingSystem::has_value() && Locator::gameRandom::has_value())
	{
		auto& random = Locator::gameRandom::value();
		const auto colour = k_RippleColours.at(random.GameRand(static_cast<uint32_t>(k_RippleColours.size())));
		Locator::waterRingSystem::value().Add(
		    {.position = drop.position, .growth = *drop.ringGrowth, .angle = random.GameFloatRand(k_TwoPi), .argb = colour});
	}
}

void GameMagicWorld::WaterObject(entt::entity object, const magic::WaterDrop& drop)
{
	auto& registry = EntityRegistry();
	// Something burning: the people come to watch the water put it out, one reaction for each miracle at a time
	if (Locator::fireSystem::has_value() && Locator::fireSystem::value().IsOnFire(object) &&
	    Locator::reactionSystem::has_value() && drop.spell != entt::null)
	{
		auto& reactions = Locator::reactionSystem::value();
		auto& watching = _puttingOutFire[drop.spell];
		if (watching == 0 || !reactions.IsActive(watching))
		{
			watching = reactions.Create({.initiator = drop.spell,
			                             .type = Reaction::ReactToMagicWaterPuttingOutFire,
			                             .player = drop.source.player,
			                             .position = drop.position,
			                             .onCast = true});
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Water: the people come to watch #{}'s fire put out, reaction {}",
			                    entt::to_integral(object), watching);
		}
	}
	const auto& info = Locator::infoConstants::value();
	if (auto* field = registry.TryGet<Field>(object))
	{
		// The caster's creature may learn to water the crops from watching. (It would copy the plain water whichever
		// was cast, but creatures don't cast miracles here yet.)
		if (drop.source.player != PlayerNames::NEUTRAL && Locator::creatureMindSystem::has_value())
		{
			Locator::creatureMindSystem::value().PlayerDid(magic::k_DeedCastWaterOnCrops, drop.position, object,
			                                               drop.source.player);
		}
		if (Locator::fireSystem::has_value() && Locator::fireSystem::value().IsOnFire(object))
		{
			return;
		}
		const auto& type = info.fieldType.at(static_cast<size_t>(field->type));
		magic::WaterCrop crop {.timesSown = field->crop.timesSown, .age = field->crop.age, .food = field->crop.food};
		magic::WaterField(crop, {.timesToSow = type.timesToSow,
		                         .ageRipe = type.ageRecolt,
		                         .totalFood = type.totalFoodInField,
		                         .effectOfWater = type.effectOfWaterSpell});
		field->crop.timesSown = crop.timesSown;
		field->crop.age = crop.age;
		field->crop.food = crop.food;
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Water: field #{} sown {} times, age {:.0f}, food {:.1f}",
		                    entt::to_integral(object), crop.timesSown, crop.age, crop.food);
		return;
	}
	if (auto* tree = registry.TryGet<Tree>(object))
	{
		const auto& type = info.tree.at(static_cast<size_t>(tree->type));
		auto& transform = registry.Get<Transform>(object);
		const auto grown = magic::WaterTree(
		    transform.scale.y, tree->maxSize,
		    {.growthAmount = type.growthAmount, .waterAccelerator = type.waterSpellAcceleratorMultiplier}, drop.extreme);
		if (grown.scale != transform.scale.y)
		{
			const float ratio = grown.scale / std::max(transform.scale.y, 1e-4f);
			transform.scale = glm::vec3(grown.scale);
			if (auto* fixed = registry.TryGet<Fixed>(object))
			{
				fixed->boundingRadius *= ratio;
			}
			tree->maxSize = grown.target;
			registry.SetDirty();
			// It rustles as it grows
			if (Locator::audio::has_value() && Locator::gameRandom::has_value())
			{
				const auto sample =
				    k_TreeGrowSounds.at(Locator::gameRandom::value().LocalRand(static_cast<int32_t>(k_TreeGrowSounds.size())));
				Locator::audio::value().StartSoundEffect(static_cast<entt::id_type>(sample),
				                                         {.position = transform.position, .owner = object});
			}
		}
		else if (!grown.canGrow && !drop.extreme)
		{
			PlantNear(object, drop);
		}
		else
		{
			tree->maxSize = grown.target;
		}
	}
}

bool GameMagicWorld::CanBeDestroyedBySpell(entt::entity object) const
{
	return ecs::world_objects::CanBeDestroyedBySpell(object);
}

void GameMagicWorld::PlantNear(entt::entity tree, const magic::WaterDrop& drop)
{
	if (!Locator::forestSystem::has_value())
	{
		return;
	}
	// The tree's forest gains a young tree beside it, when it is not too soon after the last any forest gained
	const auto planted = Locator::forestSystem::value().AddTreeNear(tree);
	// Planting a tree is good. (The game also counts it in the player's statistics and the alignment's history, which
	// aren't kept here yet.)
	if (planted.has_value() && drop.source.player != PlayerNames::NEUTRAL && Locator::alignmentSystem::has_value())
	{
		auto& alignment = Locator::alignmentSystem::value();
		const float change = magic::DampAlignmentChange(Locator::infoConstants::value().player.treePullPutAlignmentChange,
		                                                alignment.GetPlayerAlignment(drop.source.player));
		alignment.AddPendingAlignment(drop.source.player, change);
	}
}

void GameMagicWorld::PlayerAffected(entt::entity object, MagicType type, PlayerNames player)
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(object) || !ReceiverOf(object).has_value() || !Locator::creatureMindSystem::has_value())
	{
		return;
	}
	const auto* transform = registry.TryGet<const Transform>(object);
	if (transform == nullptr)
	{
		return;
	}
	// The town nearest what was reached, within reach, by the game's distance; the first found wins a tie
	const auto here = map_coords::FromMetres({transform->position.x, transform->position.z});
	float best = magic::k_DeedTownReach;
	const ecs::components::Town* nearest = nullptr;
	registry.Each<const ecs::components::Town, const Transform>(
	    [&](entt::entity, const ecs::components::Town& town, const Transform& at) {
		    const float distance = gutils::GetDistanceInMetres(here, map_coords::FromMetres({at.position.x, at.position.z}));
		    if (distance < best)
		    {
			    best = distance;
			    nearest = &town;
		    }
	    });
	const auto& effect = magic::GetMagicEffectInfo(Locator::infoConstants::value(), type);
	const bool onFire = Locator::fireSystem::has_value() && Locator::fireSystem::value().IsOnFire(object);
	// Worship sites and buildings going up aren't on the land here yet, so neither is ever what was reached
	const auto deed = magic::MiracleDeed(type, effect.perceivedPlayerDesire.at(0) == CreatureDesires::Anger,
	                                     nearest != nullptr && nearest->owner != player,
	                                     {.field = registry.AllOf<Field>(object),
	                                      .onFire = onFire,
	                                      .storagePit = registry.AllOf<StoragePit>(object),
	                                      .worshipSite = false,
	                                      .buildingBeingBuilt = false});
	if (deed != magic::k_NoDeed)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Magic: miracle {} by player {} shows deed {} on #{}", static_cast<int>(type),
		                    static_cast<int>(player), deed, entt::to_integral(object));
		Locator::creatureMindSystem::value().PlayerDid(deed, transform->position, object, player);
	}
}

std::vector<entt::entity> GameMagicWorld::HealTargets(glm::vec3 point, float radius, size_t maximum) const
{
	return magic_living::HealTargets(point, radius, maximum);
}

void GameMagicWorld::CurePoison(entt::entity object)
{
	magic_living::CurePoison(object);
}

void GameMagicWorld::ProcessTurn()
{
	// The miracles that have gone no longer keep their watchers
	std::erase_if(_puttingOutFire, [](const auto& entry) { return !EntityRegistry().Valid(entry.first); });
}

void GameMagicWorld::Reset()
{
	_puttingOutFire.clear();
}

// The links between a miracle and its effect

/// What a miracle's particle effect tells it, and asks of it
class MagicSystem::SpellLink final: public particles::SpellSink
{
public:
	SpellLink(MagicSystem& system, entt::entity spell)
	    : _system(system)
	    , _spell(spell)
	{
	}
	bool SpellEvent(const particles::SpellEventInfo& event) override { return _system.OnSpellEvent(_spell, event); }
	[[nodiscard]] int PowerUpLevel() const override
	{
		const auto* spell = EntityRegistry().TryGet<const components::Spell>(_spell);
		return spell != nullptr ? magic::GetPowerUpLevel(Locator::infoConstants::value(), spell->magicType)
		                        : magic::k_BasePowerUpLevel;
	}
	[[nodiscard]] bool IsMyInterfaceCasting() const override
	{
		const auto* spell = EntityRegistry().TryGet<const components::Spell>(_spell);
		return spell != nullptr && spell->fromLocalHand;
	}
	[[nodiscard]] bool IsHumanPlayerCasting() const override
	{
		const auto* spell = EntityRegistry().TryGet<const components::Spell>(_spell);
		return spell != nullptr && spell->humanCasting;
	}
	[[nodiscard]] bool IsCreatureCasting() const override
	{
		const auto* spell = EntityRegistry().TryGet<const components::Spell>(_spell);
		return spell != nullptr && spell->caster.kind == components::SpellCaster::Kind::Creature;
	}
	[[nodiscard]] bool IsScriptCasting() const override
	{
		// The neutral player casts by script
		const auto* spell = EntityRegistry().TryGet<const components::Spell>(_spell);
		return spell != nullptr && spell->caster.kind == components::SpellCaster::Kind::Player &&
		       spell->caster.player == PlayerNames::NEUTRAL;
	}
	[[nodiscard]] int CreatureSpellKind() const override
	{
		const auto* spell = EntityRegistry().TryGet<const components::Spell>(_spell);
		const auto* info =
		    spell != nullptr ? magic::GetMagicInfoAs<GMagicCreatureSpellInfo>(Locator::infoConstants::value(), spell->magicType)
		                     : nullptr;
		return info != nullptr ? static_cast<int>(info->creatureReceiveSpellType) : -1;
	}
	[[nodiscard]] entt::entity Spell() const override { return _spell; }
	[[nodiscard]] float TribalPower() const override
	{
		const auto* spell = EntityRegistry().TryGet<const components::Spell>(_spell);
		return spell != nullptr ? _system.TribalPower(*spell) : 1.0f;
	}
	[[nodiscard]] std::optional<float> RainAmount() const override
	{
		const auto* spell = EntityRegistry().TryGet<const components::Spell>(_spell);
		if (spell == nullptr)
		{
			return std::nullopt;
		}
		const auto* storm = magic::GetMagicInfoAs<GMagicStormAndTornadoInfo>(Locator::infoConstants::value(), spell->magicType);
		return storm != nullptr ? std::optional(storm->rainAmount) : std::nullopt;
	}
	[[nodiscard]] std::optional<float> MaxDirectionChange() const override
	{
		const auto* spell = EntityRegistry().TryGet<const components::Spell>(_spell);
		const auto* info =
		    spell != nullptr ? magic::GetMagicInfoAs<GMagicCreatureSpellInfo>(Locator::infoConstants::value(), spell->magicType)
		                     : nullptr;
		return info != nullptr ? std::optional(info->maxDirnChangeWhenCtrCasting) : std::nullopt;
	}
	void CloseDown() override { _system.CloseDown(_spell); }
	void MoveTo(glm::vec3 position) override
	{
		if (auto* spell = EntityRegistry().TryGet<components::Spell>(_spell))
		{
			spell->position = position;
		}
	}
	[[nodiscard]] float EffectRadius() const override
	{
		const auto* spell = EntityRegistry().TryGet<const components::Spell>(_spell);
		return spell != nullptr ? magic::GetMagicEffectInfo(Locator::infoConstants::value(), spell->magicType).radius
		                        : particles::SpellSink::EffectRadius();
	}

private:
	MagicSystem& _system;
	entt::entity _spell;
};

/// The held miracle's in-hand effect: this computer's player holds it, and it sends nothing back
class MagicSystem::HandEffectLink final: public particles::SpellSink
{
public:
	explicit HandEffectLink(const MagicSystem& system)
	    : _system(system)
	{
	}
	bool SpellEvent(const particles::SpellEventInfo& /*event*/) override { return false; }
	[[nodiscard]] int PowerUpLevel() const override
	{
		if (const auto held = _system.GetHeldSeed())
		{
			if (const auto* seed = EntityRegistry().TryGet<const SpellSeed>(*held))
			{
				return seed->powerUp;
			}
		}
		return magic::k_BasePowerUpLevel;
	}
	[[nodiscard]] bool IsMyInterfaceCasting() const override { return true; }
	[[nodiscard]] bool IsHumanPlayerCasting() const override { return true; }

private:
	const MagicSystem& _system;
};

// The system

MagicSystem::MagicSystem()
    : _handEffectLink(std::make_unique<HandEffectLink>(*this))
{
	for (size_t p = 0; p < _players.size(); ++p)
	{
		// A player pays for their miracles from their prayer power
		_players.at(p) = std::make_unique<magic::PlayerSpellCaster>(static_cast<PlayerNames>(p),
		                                                            [this](PlayerNames player) { return PrayerOf(player); });
	}
	for (auto& powers : _tribalPowers)
	{
		powers.fill(1.0f);
	}
}

MagicSystem::~MagicSystem() = default;

const InfoConstants& MagicSystem::Info() const
{
	return Locator::infoConstants::value();
}

magic::SpellCasterInterface* MagicSystem::CasterOf(const Spell& spell)
{
	switch (spell.caster.kind)
	{
	case SpellCaster::Kind::Player:
		// A miracle from a globe's seed is the player's own, which they top up with nothing
		if (spell.caster.withoutIcon)
		{
			_globeCaster.Bind(spell.caster.player);
			return &_globeCaster;
		}
		return _players.at(static_cast<size_t>(spell.caster.player)).get();
	case SpellCaster::Kind::Object:
		return EntityRegistry().Valid(spell.caster.entity) ? &_objectCaster : nullptr;
	case SpellCaster::Kind::Creature:
		// A creature pays with its body while it is there
		if (!EntityRegistry().Valid(spell.caster.entity))
		{
			return nullptr;
		}
		_creatureCaster.Bind(spell.caster.entity, spell.magicType);
		return &_creatureCaster;
	case SpellCaster::Kind::None:
		break;
	}
	return nullptr;
}

std::array<float, magic::k_TribeCount> MagicSystem::PlayerTribalMultipliers(PlayerNames player) const
{
	// The players' tribal power multipliers come with worship; until then every tribe's is 1 unless the testbed sets it
	return _tribalPowers.at(static_cast<size_t>(player));
}

void MagicSystem::SetTribalPower(PlayerNames player, Tribe tribe, float power)
{
	if (tribe != Tribe::NONE && static_cast<size_t>(tribe) < magic::k_TribeCount)
	{
		_tribalPowers.at(static_cast<size_t>(player)).at(static_cast<size_t>(tribe)) = power;
	}
}

float MagicSystem::PlayerTribalPower(PlayerNames player, MagicType type) const
{
	return magic::GetTribalPower(magic::GetMagicEffectInfo(Info(), type), PlayerTribalMultipliers(player));
}

float MagicSystem::TribalPower(const Spell& spell) const
{
	// A creature's miracle takes its player's tribal power
	return spell.caster.kind == SpellCaster::Kind::Player || spell.caster.kind == SpellCaster::Kind::Creature
	           ? PlayerTribalPower(spell.caster.player, spell.magicType)
	           : 1.0f;
}

float MagicSystem::SeedPower(const Spell& spell) const
{
	const auto& registry = EntityRegistry();
	if (spell.seed != entt::null && registry.Valid(spell.seed))
	{
		if (const auto* seed = registry.TryGet<const SpellSeed>(spell.seed))
		{
			return seed->power;
		}
	}
	return 1.0f;
}

void MagicSystem::AddEffectTarget(const Spell& spell, entt::entity target)
{
	if (spell.effect != ParticleSystemInterface::k_NoEffect)
	{
		Locator::particleSystem::value().AddTarget(spell.effect, target);
	}
}

std::optional<entt::entity> MagicSystem::ShieldAt(glm::vec3 point, float margin)
{
	const auto shield = Locator::particleSystem::value().FindShield(point, margin);
	if (!shield || shield->spell == entt::null)
	{
		return std::nullopt;
	}
	return shield->spell;
}

void MagicSystem::StrikeShield(entt::entity shieldSpell, glm::vec3 point)
{
	if (const auto shield = Locator::particleSystem::value().FindShield(point, k_ShieldStrikeMargin);
	    shield && shield->spell == shieldSpell)
	{
		particles::StrikeShield(*shield, point);
	}
}

entt::entity MagicSystem::SpellEntity(const Spell& spell) const
{
	return EntityRegistry().ToEntity(spell);
}

Spell* MagicSystem::FindSpell(entt::entity spell)
{
	auto& registry = EntityRegistry();
	return registry.Valid(spell) ? registry.TryGet<Spell>(spell) : nullptr;
}

float MagicSystem::GameRandom(float max)
{
	return Locator::gameRandom::has_value() ? Locator::gameRandom::value().GameFloatRand(max) : 0.0f;
}

entt::entity MagicSystem::CastAtPoint(MagicType type, PlayerNames player, glm::vec3 point, const magic::SpellCastData& cast,
                                      const particles::ProcessInfo& info)
{
	return Cast(type, {.kind = SpellCaster::Kind::Player, .player = player, .entity = entt::null}, point, cast, info);
}

entt::entity MagicSystem::Cast(MagicType type, const components::SpellCaster& caster, glm::vec3 point,
                               const magic::SpellCastData& cast, const particles::ProcessInfo& info)
{
	const auto player = caster.player;
	if (!Locator::infoConstants::has_value() || static_cast<size_t>(type) >= magic::k_MagicTypeCount || type == MagicType::None)
	{
		return entt::null;
	}
	// The caster player's creature, seeing it, takes it the player wants what the miracle answers; the player's last cast
	// is remembered
	Empathise(type, player, point);
	RecordCast(type, player, point);
	auto& registry = EntityRegistry();
	const auto entity = registry.Create();
	auto& spell = registry.Assign<Spell>(entity);
	spell.magicType = type;
	spell.spellClass = magic::ClassOf(type);
	spell.caster = caster;
	spell.humanCasting = caster.kind == SpellCaster::Kind::Player && player == PlayerNames::PLAYER_ONE;
	magic::SetChants(spell.chants, cast.chants);
	spell.duration = cast.duration;
	spell.magnitude = cast.magnitude;
	spell.maxObjectsToCreate = cast.maxObjectsToCreate;
	spell.position = point;
	spell.castPosition = point;
	spell.originalCastPosition = point;
	spell.direction = info.direction;
	spell.processInfo = info;
	magic::spells::Prepare(*this, spell);

	auto link = std::make_unique<SpellLink>(*this, entity);
	const auto particleType = magic::spells::ParticleTypeOf(*this, spell);
	spell.hasParticleType = particleType != ParticleType::None;
	auto& particles = Locator::particleSystem::value();
	spell.effect = particles.StartForSpell(particleType, point, spell.direction, spell.magnitude, *link);
	if (spell.effect != ParticleSystemInterface::k_NoEffect)
	{
		particles.SetPlayer(spell.effect, static_cast<int>(player));
		if (auto* effect = particles.Find(spell.effect))
		{
			effect->SetProcessInfo(spell.processInfo);
		}
	}
	_links.insert_or_assign(entity, std::move(link));
	_spells.push_front(entity);
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Magic: miracle {} cast at ({}, {})", static_cast<int>(type), point.x, point.z);
	// A miracle that is its particle effect is nothing if the effect can't start: the cast is refused
	if (magic::CastRefusedWithoutEffect(spell.hasParticleType, spell.effect != ParticleSystemInterface::k_NoEffect))
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "Magic: miracle {} refused, its effect couldn't start", static_cast<int>(type));
		Delete(entity);
		return entt::null;
	}
	if (!magic::spells::Start(*this, registry.Get<Spell>(entity)))
	{
		Delete(entity);
		return entt::null;
	}
	// One without an effect starts at its point by itself
	if (!registry.Get<Spell>(entity).hasParticleType)
	{
		OnSpellEvent(entity, {.type = particles::SpellEventInfo::Type::InitWithoutEffect, .position = point});
	}
	if (auto* started = FindSpell(entity);
	    started != nullptr && magic::GetMagicEffectInfo(Info(), type).createReactionOnCast != 0)
	{
		ReactToSpell(*started, true);
	}
	return FindSpell(entity) != nullptr ? entity : entt::null;
}

entt::entity MagicSystem::CastOnObject(MagicType type, PlayerNames player, entt::entity target,
                                       const magic::SpellCastData& cast, const particles::ProcessInfo& info)
{
	return CastOn(type, {.kind = SpellCaster::Kind::Player, .player = player, .entity = entt::null}, target, cast, info);
}

entt::entity MagicSystem::CastOn(MagicType type, const components::SpellCaster& caster, entt::entity target,
                                 const magic::SpellCastData& cast, const particles::ProcessInfo& info)
{
	const auto position = _world.PositionOf(target);
	if (!position.has_value() || !CanCastOn(type, target))
	{
		return entt::null;
	}
	const auto entity = Cast(type, caster, *position, cast, info);
	if (entity == entt::null)
	{
		return entt::null;
	}
	auto& spell = EntityRegistry().Get<Spell>(entity);
	spell.target = target;
	AddEffectTarget(spell, target);
	if (spell.spellClass == magic::SpellClass::Creature)
	{
		ReceiveCreatureSpell(target, entity, spell);
	}
	return entity;
}

// The creature spells' casting, receiving and turns are in MagicCreatureSpells.cpp

void MagicSystem::CloseDown(entt::entity entity)
{
	auto* spell = FindSpell(entity);
	if (spell == nullptr)
	{
		return;
	}
	if (!spell->closedDown)
	{
		StopHandGrain(*spell);
	}
	spell->closedDown = true;
	if (spell->effect != ParticleSystemInterface::k_NoEffect)
	{
		Locator::particleSystem::value().CloseDown(spell->effect);
	}
}

bool MagicSystem::CanCastAt(MagicType type, PlayerNames player, glm::vec3 point)
{
	return Locator::infoConstants::has_value() && magic::spells::CanCastAt(*this, type, player, point, _ignoreInfluence);
}

bool MagicSystem::SpellEvent(entt::entity entity, const particles::SpellEventInfo& event)
{
	return OnSpellEvent(entity, event);
}

void MagicSystem::PayForSpell(entt::entity entity, float cost)
{
	if (auto* spell = FindSpell(entity); spell != nullptr && !spell->closedDown)
	{
		magic::PayFor(spell->chants, magic::spells::RulesOf(*this, *spell), CasterOf(*spell), cost);
	}
}

entt::entity MagicSystem::CreateTeleportStone(const Spell& spell)
{
	return Locator::teleportSystem::has_value() ? Locator::teleportSystem::value().CreateStone(
	                                                  spell.castPosition, spell.caster.player, EntityRegistry().ToEntity(spell))
	                                            : entt::null;
}

bool MagicSystem::CanPlaceTeleportStone(glm::vec3 point) const
{
	return !Locator::teleportSystem::has_value() || Locator::teleportSystem::value().CanPlaceStone(point);
}

bool MagicSystem::OnSpellEvent(entt::entity entity, const particles::SpellEventInfo& event)
{
	auto* spell = FindSpell(entity);
	return spell != nullptr && magic::spells::OnEvent(*this, *spell, event);
}

particles::ProcessInfo MagicSystem::HandInfo() const
{
	return {
	    .handPosition = _hand.handPosition,
	    .cameraForward = _hand.cameraForward,
	    .direction = _handVelocity,
	    .power = 1.0f,
	    .enabled = true,
	    .spin = _handSpin.spin,
	};
}

void MagicSystem::Maintain(entt::entity entity, Spell& spell)
{
	if (magic::AgeOneTurn(spell.age, spell.duration, k_TurnSeconds))
	{
		CloseDown(entity);
	}
	// Its caster gone, it closes down and is left as it is
	if (CasterOf(spell) == nullptr)
	{
		CloseDown(entity);
		return;
	}
	if (spell.target != entt::null && !EntityRegistry().Valid(spell.target))
	{
		CloseDown(entity);
	}
	// A storm's swirl plays only while whoever cast it is still there
	if (spell.castEffect != ParticleSystemInterface::k_NoEffect && CasterOf(spell) == nullptr)
	{
		Locator::particleSystem::value().Delete(spell.castEffect);
		spell.castEffect = ParticleSystemInterface::k_NoEffect;
	}
	spell.processInfo.enabled = true;
	// A creature's miracle flows from its hands as they move; a locked miracle follows the hand; anything else stays
	// where it was cast
	if (spell.caster.kind == SpellCaster::Kind::Creature)
	{
		UpdateCreatureCast(spell);
	}
	else if (spell.castFromHand && spell.seed != entt::null)
	{
		// A miracle held in the hand follows it, keeping the spin it was cast with: it is cast on the land under the hand
		// itself, which hangs a little short of where the cursor points
		const float power = spell.processInfo.power;
		const float spin = spell.processInfo.spin;
		spell.processInfo = HandInfo();
		spell.processInfo.power = power;
		spell.processInfo.spin = spin;
		const auto& hand = _hand.handPosition;
		spell.castPosition = {hand.x, _world.LandHeight({hand.x, hand.z}), hand.z};
		LetGoIfOutsideCastRule(spell);
	}
	else if (magic::spells::FollowsHand(*this, spell))
	{
		// A flock keeps taking the hand's sweep while it makes its animals, keeping the spin it was cast with
		const float power = spell.processInfo.power;
		const float spin = spell.processInfo.spin;
		spell.processInfo = HandInfo();
		spell.processInfo.power = power;
		spell.processInfo.spin = spin;
	}
	if (!spell.closedDown)
	{
		const float strength = magic::spells::StrengthOf(*this, spell);
		magic::PayForOneTurn(spell.chants, magic::spells::RulesOf(*this, spell), CasterOf(spell));
		_grid.Mark({spell.position.x, spell.position.z});
		spell.processInfo.power = strength;
	}
	else
	{
		spell.processInfo.enabled = false;
		spell.processInfo.power = 0.0f;
	}
}

bool MagicSystem::Process(entt::entity entity, Spell& spell)
{
	if (!spell.closedDown)
	{
		if (auto* caster = CasterOf(spell))
		{
			magic::Recharge(spell.chants, magic::spells::RulesOf(*this, spell), *caster);
		}
		if (!(spell.processInfo.power > 0.0f))
		{
			CloseDown(entity);
		}
	}
	if (spell.effect != ParticleSystemInterface::k_NoEffect &&
	    !Locator::particleSystem::value().ProcessForSpell(spell.effect, spell.processInfo, k_TurnSeconds))
	{
		spell.effect = ParticleSystemInterface::k_NoEffect;
		// Its effect over, the people's reactions to it go, even while what it made keeps it
		if (Locator::reactionSystem::has_value())
		{
			Locator::reactionSystem::value().RemoveFrom(entity);
		}
		spell.reaction = 0;
	}
	// The effect's events may have closed it down or cast others: look it up again
	auto* after = FindSpell(entity);
	if (after == nullptr)
	{
		return false;
	}
	magic::spells::ProcessTurn(*this, *after);
	if (after->reaction != 0 && Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().Move(after->reaction, after->position, after->movement,
		                                      magic::spells::StrengthOf(*this, *after));
	}
	// It lives while its effect does, or until it closes down without one, or while what its kind made keeps it
	return magic::FateOf({
	           .hasParticleType = after->hasParticleType,
	           .effectRunning = after->effect != ParticleSystemInterface::k_NoEffect,
	           .closedDown = after->closedDown,
	           .keptByKind = magic::spells::KeptByKind(*this, *after),
	       }) == magic::SpellFate::Continue;
}

void MagicSystem::Delete(entt::entity entity)
{
	auto& registry = EntityRegistry();
	if (auto* spell = FindSpell(entity))
	{
		if (spell->effect != ParticleSystemInterface::k_NoEffect)
		{
			Locator::particleSystem::value().Delete(spell->effect);
		}
		if (spell->castEffect != ParticleSystemInterface::k_NoEffect)
		{
			Locator::particleSystem::value().Delete(spell->castEffect);
		}
		if (spell->seed != entt::null && registry.Valid(spell->seed))
		{
			if (auto* seed = registry.TryGet<SpellSeed>(spell->seed); seed != nullptr && seed->spell == entity)
			{
				seed->spell = entt::null;
				// A seed linked to its miracle goes with it, out of the hand if it is held
				if (spell->seed == _held)
				{
					LetGoOfSeed(true);
				}
				else
				{
					registry.Destroy(spell->seed);
				}
			}
		}
		if (Locator::reactionSystem::has_value())
		{
			Locator::reactionSystem::value().RemoveFrom(entity);
		}
		registry.Destroy(entity);
	}
	_links.erase(entity);
	std::erase(_spells, entity);
}

void MagicSystem::ProcessTurn()
{
	if (!Locator::infoConstants::has_value() || !Locator::particleSystem::has_value())
	{
		return;
	}
	auto& registry = EntityRegistry();
	++_turn;
	_world.ProcessTurn();
	_grid.Fade();
	ProcessDispensers();
	ProcessCreatureSpells();
	KeepCreatureBeams();

	// The held seed waits until it is ready
	if (_held.has_value())
	{
		if (!registry.Valid(*_held))
		{
			_held.reset();
		}
		else
		{
			auto& seed = registry.Get<SpellSeed>(*_held);
			++seed.turnsInHand;
			const bool wasReady = seed.ready;
			seed.ready =
			    seed.ready || magic::SeedReadyAfter(seed.turnsInHand, k_TurnSeconds, Info().spellSystem.delayBeforeSeedActive);
			// Becoming ready calls off a press made too early
			if (seed.ready && !wasReady)
			{
				Do(magic::Cancel(_input));
			}
			// The hand looks again at how it holds the seed, and the miracle shows in the hand once ready
			ShowHeldSeed();
			// A locked miracle whose prayer power or time ran out is over, and its seed with it
			if (_held.has_value() && _input.state == magic::CastInput::State::Locked)
			{
				const auto spellEntity = registry.Get<SpellSeed>(*_held).spell;
				const auto* spell = FindSpell(spellEntity);
				if (spellEntity != entt::null && (spell == nullptr || spell->closedDown))
				{
					LetGoOfSeed(true);
				}
			}
			// While the button stays down a locked miracle is applied where the hand is; moved somewhere it may not be
			// cast it isn't applied there, and stays locked until the button comes up
			if (_held.has_value() && _input.state == magic::CastInput::State::Locked)
			{
				// Where it may not be applied this turn it waits; the miracle itself lets go only once the hand points
				// outside its cast rule
				const bool valid = _input.onObject ? HeldSeedTarget().has_value() : HandPointValid();
				if (const auto apply = magic::Tick(_input, _turn, valid); apply.cast.has_value())
				{
					CastHeldSeed(*apply.cast);
				}
			}
		}
	}

	for (const auto entity : std::vector<entt::entity>(_spells.begin(), _spells.end()))
	{
		if (auto* spell = FindSpell(entity))
		{
			Maintain(entity, *spell);
		}
	}
	for (const auto entity : std::vector<entt::entity>(_spells.begin(), _spells.end()))
	{
		auto* spell = FindSpell(entity);
		if (spell == nullptr || !Process(entity, *spell))
		{
			Delete(entity);
		}
	}
	// The pour lifting and tipping the hand steps once the miracles have had their turn
	magic::StepPour(_pour, k_TurnSeconds);
}

void MagicSystem::Update(float /*seconds*/)
{
	auto& registry = EntityRegistry();
	TakeGestures();
	KeepCreatureSpellSounds();
	// The hum of an armed seed follows the hand
	if (_holdLoop != entt::null && Locator::audio::has_value())
	{
		Locator::audio::value().SetEmitterPosition(_holdLoop, _hand.handPosition);
	}
	// A seed out of the hand stays with its miracle
	registry.Each<SpellSeed, Transform>([&](entt::entity, const SpellSeed& seed, Transform& transform) {
		if (seed.followsSpell)
		{
			if (const auto* spell = FindSpell(seed.spell))
			{
				transform.position = spell->position;
			}
		}
	});
	// The held seed itself is placed in the hand as the hand is posed, and its in-hand effect with it
	// (PlaceHandEffect)
	if (_held.has_value() && registry.Valid(*_held))
	{
		// The miracle shown in the hand steps every frame by the frame's game time in whole milliseconds, at least one
		// even while the game is paused
		auto& seed = registry.Get<SpellSeed>(*_held);
		if (seed.handEffect != ParticleSystemInterface::k_NoEffect && Locator::particleSystem::has_value())
		{
			const auto milliseconds =
			    Locator::time::has_value() ? std::max<int64_t>(Locator::time::value().GetFrameGameTime().count(), 1) : 1;
			const auto info = HandEffectInfo(seed);
			auto& particles = Locator::particleSystem::value();
			if (!particles.ProcessForSpell(seed.handEffect, info, static_cast<float>(milliseconds) * 0.001f))
			{
				seed.handEffect = ParticleSystemInterface::k_NoEffect;
			}
			_handAtStep = info.handPosition;
			particles.SetDrawOffset(seed.handEffect, glm::vec3(0.0f));
		}
	}
	// A miracle held from this computer's hand, such as the lightning, keeps up with the hand between turns
	if (Locator::particleSystem::has_value())
	{
		for (const auto entity : _spells)
		{
			if (const auto* spell = FindSpell(entity); spell != nullptr && spell->castFromHand && spell->fromLocalHand &&
			                                           spell->effect != ParticleSystemInterface::k_NoEffect)
			{
				// The rain cloud, or the grains and logs, of a sprinkled miracle keep to their limit above the land
				const bool sprinkles =
				    spell->spellClass == magic::SpellClass::Water || spell->spellClass == magic::SpellClass::Resource;
				const auto clamped = [this, sprinkles](glm::vec3 point) {
					if (sprinkles)
					{
						point.y = std::min(point.y, _world.LandHeight({point.x, point.z}) + particles::k_MaximumSprinkleHeight);
					}
					return point;
				};
				Locator::particleSystem::value().SetDrawOffset(spell->effect, clamped(_hand.handPosition) -
				                                                                  clamped(spell->processInfo.handPosition));
			}
		}
	}
}

void MagicSystem::Reset()
{
	_spells.clear();
	_links.clear();
	StopHoldLoop();
	_held.reset();
	_input = {};
	_actionDown = false;
	_pour = {};
	_rainWatchers.clear();
	_circle.reset();
	_turn = 0;
	_driven.reset();
	_lastHandPosition.reset();
	_handVelocity = glm::vec3(0.0f);
	_handEffectPoint.reset();
	_handScale = 1.0f;
	for (auto& powers : _tribalPowers)
	{
		powers.fill(1.0f);
	}
	_lastHandResult = HandResult::None;
	_world.Reset();
	_grid.Clear();
	_creatureCasts.clear();
}

// Dispensers and one-shot miracles

entt::entity MagicSystem::CreateDispenser(glm::vec3 position, MagicType type, float yAngleRadians)
{
	if (!Locator::infoConstants::has_value())
	{
		return entt::null;
	}
	position.y = _world.LandHeight({position.x, position.z});
	const auto entity = archetypes::SpellDispenserArchetype::Create(position, type, k_DispenserBuilding, yAngleRadians, 1.0f);
	auto& dispenser = EntityRegistry().Get<SpellDispenser>(entity);
	if (Locator::particleSystem::has_value())
	{
		dispenser.effect = Locator::particleSystem::value().Start(ParticleType::SpelldispenserVortex, position, 1.0f, true);
	}
	// Its first bubble comes once it has counted its period, as every later one does
	return entity;
}

void MagicSystem::ChargeDispenser(entt::entity dispenser)
{
	auto& registry = EntityRegistry();
	if (const auto* component = registry.TryGet<const SpellDispenser>(dispenser);
	    component != nullptr && component->orb == entt::null && component->magicType != MagicType::None)
	{
		MakeOrb(dispenser);
	}
}

void MagicSystem::SetDispenserPeriod(entt::entity dispenser, float seconds)
{
	if (auto* component = EntityRegistry().TryGet<SpellDispenser>(dispenser))
	{
		component->timer.period = magic::PeriodTurns(seconds, TimeSystemInterface::k_TurnDuration);
		component->timer.active = component->timer.period != 0;
	}
}

entt::entity MagicSystem::CreateOneOffSeed(glm::vec3 position, SpellSeedType seed, int powerUp, float multiplier)
{
	return archetypes::OneOffSpellSeedArchetype::Create(position, seed, powerUp, multiplier);
}

entt::entity MagicSystem::CreateOneOffSeedFor(glm::vec3 position, MagicType type)
{
	const auto seed = magic::FindFirstSpellSeedForMagicType(Info(), type);
	if (!seed.has_value())
	{
		return entt::null;
	}
	const auto step = magic::GetPowerUpGesture(magic::GetSpellSeedInfo(Info(), *seed), type);
	const auto orb = CreateOneOffSeed(position, *seed, step.level, 1.0f);
	if (orb != entt::null)
	{
		EntityRegistry().Get<OneOffSpellSeed>(orb).magicType = type;
	}
	return orb;
}

entt::entity MagicSystem::MakeOrb(entt::entity entity)
{
	auto& registry = EntityRegistry();
	auto& dispenser = registry.Get<SpellDispenser>(entity);
	const auto& transform = registry.Get<const Transform>(entity);
	const auto& mesh = registry.Get<const Mesh>(entity);
	const auto position = magic::OrbPosition(transform.position, HeightOfMesh(mesh.id, transform.scale.y));
	const auto magicType = dispenser.magicType;
	const auto orb = CreateOneOffSeedFor(position, magicType);
	auto& after = registry.Get<SpellDispenser>(entity);
	after.orb = orb;
	after.orbPosition = position;
	if (orb != entt::null)
	{
		registry.Get<OneOffSpellSeed>(orb).dispenser = entity;
		Locator::particleSystem::value().StartSpotVisual(SpotVisualType::MagicObjectCreated, position, std::nullopt,
		                                                 entt::null);
	}
	return orb;
}

void MagicSystem::ProcessDispensers()
{
	auto& registry = EntityRegistry();
	std::vector<entt::entity> making;
	registry.Each<SpellDispenser>([&](entt::entity entity, SpellDispenser& dispenser) {
		const bool hasOrb = dispenser.orb != entt::null;
		bool stillThere = false;
		if (hasOrb && registry.Valid(dispenser.orb))
		{
			if (const auto* orb = registry.TryGet<const OneOffSpellSeed>(dispenser.orb))
			{
				stillThere = magic::OrbStillThere(orb->position, dispenser.orbPosition);
			}
		}
		switch (magic::StepDispenser(dispenser.timer, hasOrb, stillThere, dispenser.magicType != MagicType::None))
		{
		case magic::DispenserStep::OrbTaken:
			dispenser.orb = entt::null;
			break;
		case magic::DispenserStep::MakeOrb:
			making.push_back(entity);
			break;
		case magic::DispenserStep::Wait:
			break;
		}
	});
	for (const auto entity : making)
	{
		MakeOrb(entity);
	}
}

bool MagicSystem::Remove(entt::entity entity)
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(entity))
	{
		return false;
	}
	if (const auto* dispenser = registry.TryGet<const SpellDispenser>(entity))
	{
		if (Locator::particleSystem::has_value() && dispenser->effect != 0)
		{
			Locator::particleSystem::value().Delete(dispenser->effect);
		}
		DestroyOrb(dispenser->orb);
		registry.Destroy(entity);
		return true;
	}
	if (registry.AllOf<OneOffSpellSeed>(entity))
	{
		DestroyOrb(entity);
		return true;
	}
	return false;
}

void MagicSystem::DestroyOrb(entt::entity orb)
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(orb))
	{
		return;
	}
	registry.Destroy(orb);
}

entt::entity MagicSystem::GiveSeedToHand(PlayerNames player, SpellSeedType seedType, int powerUp, float multiplier)
{
	if (_held.has_value() || !Locator::infoConstants::has_value())
	{
		return entt::null;
	}
	const auto entity = archetypes::SpellSeedArchetype::Create(_hand.handPosition, seedType, player, powerUp, multiplier);
	if (entity == entt::null)
	{
		return entt::null;
	}
	auto& registry = EntityRegistry();
	auto& seed = registry.Get<SpellSeed>(entity);
	const auto& seedInfo = magic::GetSpellSeedInfo(Info(), seedType);
	const auto type = magic::GetMagicTypeFromPowerUpLevel(seedInfo, powerUp);
	// Charged in full, for nothing, and ready at once; made at the player's icon for the seed if they have one
	seed.chantStore = magic::ChantNeeded(magic::GetChantsRequiredToCreate(Info(), type), 0.0f);
	seed.origin = magic::SeedOrigin::Bubble;
	seed.hasIcon = PlayerHasSpellIcon(player, seedType);
	seed.ready = magic::ReadyAtOnce(seed.origin);
	seed.power = magic::SeedPower(seed.chantStore, magic::GetChantsRequiredToCreate(Info(), type));
	_held = entity;
	_input = {};
	// The hand takes hold of it afresh, its pour at rest; a tribe's power behind it rings the hand with its name
	_pour = {};
	StartHandEffect(entity);
	ShowHeldSeed();
	if (const auto tribe = TribalPowerTribe(player, type); tribe.has_value() && Locator::miracleFxSystem::has_value())
	{
		Locator::miracleFxSystem::value().StartTribalPowerRing(*tribe);
	}
	// The hand takes it with its bands and bracelets
	if (Locator::miracleFxSystem::has_value())
	{
		Locator::miracleFxSystem::value().SeedInHand(powerUp, powerUp);
	}
	return entity;
}

entt::entity MagicSystem::SummonSeed(PlayerNames player, SpellSeedType seedType, int powerUp)
{
	const auto entity = GiveSeedToHand(player, seedType, powerUp, 1.0f);
	if (entity == entt::null)
	{
		return entt::null;
	}
	auto& seed = EntityRegistry().Get<SpellSeed>(entity);
	const auto type = magic::GetMagicTypeFromPowerUpLevel(magic::GetSpellSeedInfo(Info(), seedType), powerUp);
	const float cost = magic::GetChantsRequiredToCreate(Info(), type);
	// The worship charges it from the player's prayer power, as much as there is, and it isn't ready at once
	auto* store = PrayerOf(player);
	seed.chantStore = store != nullptr ? magic::ChargeSeed(*store, cost) : 0.0f;
	seed.power = magic::SeedPower(seed.chantStore, cost);
	seed.origin = magic::SeedOrigin::Worship;
	seed.hasIcon = true;
	seed.ready = magic::ReadyAtOnce(seed.origin);
	seed.turnsInHand = 0;
	// Its miracle runs in the hand from now, but shows only once it is ready
	ShowHeldSeed();
	return entity;
}

bool MagicSystem::TapOrb(entt::entity orb)
{
	auto& registry = EntityRegistry();
	const auto component = registry.Get<const OneOffSpellSeed>(orb);
	if (GiveSeedToHand(PlayerNames::PLAYER_ONE, component.seedType, component.powerUp, component.multiplier) == entt::null)
	{
		return false;
	}
	// The pop is heard where the hand took it
	if (Locator::audio::has_value())
	{
		Locator::audio::value().PlaySoundEffect(static_cast<entt::id_type>(audio::SoundId::G_SpellBubblePop_04),
		                                        _hand.handPosition);
	}
	DestroyOrb(orb);
	_lastHandResult = HandResult::TookMiracle;
	return true;
}

std::optional<entt::entity> MagicSystem::OrbAlong(glm::vec3 origin, glm::vec3 direction) const
{
	const auto& registry = EntityRegistry();
	auto& meshes = Locator::resources::value().GetMeshes();
	const auto bubble = k_BubbleMeshId.value();
	glm::vec3 centre(0.0f);
	float radius = k_DefaultOrbRadius;
	if (meshes.Contains(bubble))
	{
		const auto box = meshes.Handle(bubble)->GetBoundingBox();
		centre = box.Center();
		radius = std::max({box.Size().x, box.Size().y, box.Size().z}) * 0.5f;
	}
	std::optional<entt::entity> nearest;
	float best = std::numeric_limits<float>::max();
	registry.Each<const OneOffSpellSeed, const Transform>(
	    [&](entt::entity entity, const OneOffSpellSeed& orb, const Transform& transform) {
		    const auto middle = orb.position + centre * transform.scale;
		    const float reach = radius * transform.scale.x * k_OrbTapLeeway;
		    const float along = glm::dot(middle - origin, direction);
		    if (along <= 0.0f)
		    {
			    return;
		    }
		    const auto closest = origin + direction * along;
		    if (glm::distance(closest, middle) < reach && along < best)
		    {
			    best = along;
			    nearest = entity;
		    }
	    });
	return nearest;
}

// The hand

void MagicSystem::UpdateHand(const HandFrame& mouseFrame, float seconds)
{
	auto frame = _driven.has_value() ? *_driven : mouseFrame;
	// The hand a scenario puts is placed from there as the mouse's hand is: kept where a pour began, lifted by the pour
	// and by what it holds
	if (_driven.has_value())
	{
		frame.handPosition = mouseFrame.handPosition;
	}
	// A miracle coming to the hand starts its movement and spin afresh
	const auto held = _held.value_or(entt::null);
	if (held != _spinningSeed)
	{
		_spinningSeed = held;
		_handSpin = {};
		if (held != entt::null)
		{
			_handVelocity = glm::vec3(0.0f);
		}
	}
	if (_lastHandPosition.has_value() && seconds > 0.0f)
	{
		const auto step = frame.handPosition - *_lastHandPosition;
		_handVelocity = magic::FilterHandVelocity(_handVelocity, step / seconds, seconds);
		// While it holds a miracle, how the way it moves turns is the spin it gives the miracle
		if (held != entt::null)
		{
			magic::StepHandSpin(_handSpin, step, _handVelocity, seconds);
		}
	}
	_lastHandPosition = frame.handPosition;
	_hand = frame;
}

void MagicSystem::StartHandEffect(entt::entity entity)
{
	auto& registry = EntityRegistry();
	auto& seed = registry.Get<SpellSeed>(entity);
	const auto& seedInfo = magic::GetSpellSeedInfo(Info(), seed.seedType);
	const auto type = magic::GetMagicTypeFromPowerUpLevel(seedInfo, seed.powerUp);
	auto& particles = Locator::particleSystem::value();
	// Only this computer's: it steps with the frames it draws, so it draws on its own random numbers
	const auto info = HandEffectInfo(seed);
	seed.handEffect = particles.StartForSpell(magic::GetMagicInfo(Info(), type).particleTypeInHand, info.handPosition,
	                                          glm::vec3(0.0f), _handScale, *_handEffectLink, false);
	if (seed.handEffect != ParticleSystemInterface::k_NoEffect)
	{
		// Drawn at once just after the hand, rather than sorted with everything else
		particles.SetDrawPath(seed.handEffect, particles::draw::DrawPath::Immediate);
		particles.SetPlayer(seed.handEffect, static_cast<int>(seed.player));
		if (auto* effect = particles.Find(seed.handEffect))
		{
			effect->SetProcessInfo(info);
			effect->SetHidden(!seed.ready);
		}
		_handAtStep = info.handPosition;
	}
}

void MagicSystem::StopHandEffect(SpellSeed& seed)
{
	if (seed.handEffect != ParticleSystemInterface::k_NoEffect && Locator::particleSystem::has_value())
	{
		Locator::particleSystem::value().Delete(seed.handEffect);
	}
	seed.handEffect = ParticleSystemInterface::k_NoEffect;
}

void MagicSystem::StartHoldLoop()
{
	StopHoldLoop();
	if (Locator::audio::has_value() && _held.has_value())
	{
		_holdLoop = Locator::audio::value().StartSoundEffect(
		    static_cast<entt::id_type>(audio::SoundId::G_HandGesture_02),
		    {.position = _hand.handPosition, .playType = audio::PlayType::Repeat, .owner = *_held});
	}
}

void MagicSystem::StopHoldLoop()
{
	// Cut at once, as the hand's hum is
	if (_holdLoop != entt::null && Locator::audio::has_value() && Locator::audio::value().EmitterExists(_holdLoop))
	{
		Locator::audio::value().StopEmitter(_holdLoop);
	}
	_holdLoop = entt::null;
}

void MagicSystem::LetGoOfSeed(bool destroy)
{
	auto& registry = EntityRegistry();
	StopHoldLoop();
	magic::StopPour(_pour);
	_input = {};
	if (_held.has_value() && Locator::miracleFxSystem::has_value())
	{
		Locator::miracleFxSystem::value().SeedLeftHand();
		Locator::miracleFxSystem::value().StopTribalPowerRing();
	}
	if (_held.has_value() && registry.Valid(*_held))
	{
		auto& seed = registry.Get<SpellSeed>(*_held);
		StopHandEffect(seed);
		if (destroy)
		{
			if (auto* spell = FindSpell(seed.spell); spell != nullptr && spell->seed == *_held)
			{
				spell->seed = entt::null;
			}
			registry.Destroy(*_held);
		}
	}
	_held.reset();
}

MagicType MagicSystem::HeldMagicType() const
{
	if (!_held.has_value() || !EntityRegistry().Valid(*_held))
	{
		return MagicType::None;
	}
	const auto& seed = EntityRegistry().Get<const SpellSeed>(*_held);
	return magic::GetMagicTypeFromPowerUpLevel(magic::GetSpellSeedInfo(Info(), seed.seedType), seed.powerUp);
}

std::optional<entt::entity> MagicSystem::HeldSeedTarget() const
{
	if (!_held.has_value() || !EntityRegistry().Valid(*_held))
	{
		return std::nullopt;
	}
	const auto& seed = EntityRegistry().Get<const SpellSeed>(*_held);
	if (magic::GetSpellSeedInfo(Info(), seed.seedType).castOnObject == 0 || !Locator::creatureHandSystem::has_value())
	{
		return std::nullopt;
	}
	const auto target = Locator::creatureHandSystem::value().CreatureUnderCursor();
	if (!target.has_value() || !CanCastOn(HeldMagicType(), *target))
	{
		return std::nullopt;
	}
	return target;
}

void MagicSystem::LetGoIfOutsideCastRule(const Spell& spell)
{
	// Only the local hand's own miracle, held locked in it, by a player's hand; over nothing the hand keeps it
	if (!spell.fromLocalHand || spell.caster.kind != SpellCaster::Kind::Player || !_held.has_value() || spell.seed != *_held ||
	    _input.state != magic::CastInput::State::Locked || !_hand.point.has_value())
	{
		return;
	}
	if (!magic::spells::MeetsCastRule(*this, spell.magicType, spell.caster.player, *_hand.point, _ignoreInfluence))
	{
		Do(magic::Cancel(_input));
	}
}

bool MagicSystem::HandPointValid() const
{
	if (!_held.has_value() || !_hand.point.has_value() || !EntityRegistry().Valid(*_held))
	{
		return false;
	}
	const auto& seed = EntityRegistry().Get<const SpellSeed>(*_held);
	const auto point = *_hand.point;
	auto& self = const_cast<MagicSystem&>(*this);
	// Every cast from the hand needs the hand in the player's influence, whatever the miracle's own rule
	if (!_ignoreInfluence && !_world.InInfluence(seed.player, point))
	{
		return false;
	}
	return self.CanCastAt(HeldMagicType(), seed.player, point);
}

entt::entity MagicSystem::CastHeldSeed(magic::CastTarget target)
{
	auto& registry = EntityRegistry();
	if (!_held.has_value() || !registry.Valid(*_held))
	{
		return entt::null;
	}
	auto& seed = registry.Get<SpellSeed>(*_held);
	// A locked miracle still running is applied again where it is: it follows the hand
	if (const auto* running = FindSpell(seed.spell); running != nullptr && !running->closedDown)
	{
		return seed.spell;
	}
	const auto& seedInfo = magic::GetSpellSeedInfo(Info(), seed.seedType);
	const auto type = HeldMagicType();
	std::optional<entt::entity> object;
	glm::vec3 point(0.0f);
	// Without a gesture to size it, a miracle takes the plain size
	float size = k_PlainHandMagnitude;
	if (target == magic::CastTarget::Object)
	{
		object = HeldSeedTarget();
		if (!object.has_value())
		{
			return entt::null;
		}
	}
	else if (seedInfo.sizingGesture == GestureType::Circle)
	{
		// The storms and shields go where the circle was drawn, as big as it was, while it is remembered
		if (!_circle.has_value() || _turn - _circle->turn > magic::k_CircleTurns)
		{
			_circle.reset();
			_lastHandResult = HandResult::NoCircle;
			return entt::null;
		}
		point = _circle->centre;
		size = _circle->radius;
		if (!_ignoreInfluence && !_world.InInfluence(seed.player, point))
		{
			return entt::null;
		}
		if (!CanCastAt(type, seed.player, point))
		{
			return entt::null;
		}
	}
	else
	{
		if (!HandPointValid())
		{
			return entt::null;
		}
		point = *_hand.point;
	}
	auto cast = magic::SeedCastData(Info(), type, seed.seedType, seed.castMultiplier, size);
	cast.maxObjectsToCreate = seed.storedMaxObjects;
	const bool resuming = seed.storedChants >= 0.0f;
	if (resuming)
	{
		cast.chants = seed.storedChants;
	}
	const auto heldEntity = *_held;
	// A seed made at an icon is topped up by its worship; one made without is the player's own
	const SpellCaster caster {
	    .kind = SpellCaster::Kind::Player, .player = seed.player, .entity = entt::null, .withoutIcon = !seed.hasIcon};
	const auto spellEntity =
	    object.has_value() ? CastOn(type, caster, *object, cast, HandInfo()) : Cast(type, caster, point, cast, HandInfo());
	if (spellEntity == entt::null)
	{
		return entt::null;
	}
	if (seedInfo.sizingGesture == GestureType::Circle)
	{
		_circle.reset();
	}
	auto& spell = registry.Get<Spell>(spellEntity);
	auto& after = registry.Get<SpellSeed>(heldEntity);
	spell.fromLocalHand = true;
	// Only a miracle held in the hand follows it
	spell.castFromHand = seedInfo.castType == SpellCastType::SpellCastInHand;
	spell.seed = heldEntity;
	if (resuming)
	{
		spell.age = after.storedAge;
	}
	after.spell = spellEntity;
	after.hasCast = true;
	after.storedChants = -1.0f;
	AfterSeedCast(after, type);
	// Food and wood lift and tip the hand as they pour, keeping it where it began; the water raises it eight units and
	// lowers it again over eight seconds, over and over as it rains, the hand free to move
	if (spell.castFromHand && spell.spellClass == magic::SpellClass::Resource)
	{
		magic::StartPour(_pour, magic::k_FoodWoodPour, _hand.handPosition);
	}
	else if (spell.castFromHand && spell.spellClass == magic::SpellClass::Water)
	{
		magic::StartPour(_pour, magic::k_WaterPour, _hand.handPosition);
	}
	return spellEntity;
}

void MagicSystem::AfterCast(entt::entity spellEntity)
{
	auto& registry = EntityRegistry();
	if (!_held.has_value() || !registry.Valid(*_held))
	{
		return;
	}
	auto& seed = registry.Get<SpellSeed>(*_held);
	const auto& seedInfo = magic::GetSpellSeedInfo(Info(), seed.seedType);
	const auto fate =
	    magic::SeedAfterCastOf(seedInfo.isKeptInHand != 0, seedInfo.deleteSeedOnceCast != 0, seedInfo.seedFollowsSpell != 0);
	if (fate == magic::SeedAfterCast::StaysInHand)
	{
		return;
	}
	// Leaving the hand, it shows the cast succeeded where it was cast
	if (const auto* spell = FindSpell(spellEntity); spell != nullptr && Locator::particleSystem::has_value())
	{
		Locator::particleSystem::value().StartSpotVisual(SpotVisualType::SpellSucceedCast, spell->castPosition, std::nullopt,
		                                                 entt::null);
	}
	if (fate == magic::SeedAfterCast::Deleted)
	{
		LetGoOfSeed(true);
		return;
	}
	// Bound to its miracle, out of the hand, and gone with it
	const auto entity = *_held;
	StopHandEffect(seed);
	seed.followsSpell = true;
	registry.Remove<Mesh>(entity);
	LetGoOfSeed(false);
}

void MagicSystem::FailCast()
{
	const auto point = _hand.point.value_or(_hand.handPosition);
	if (Locator::particleSystem::has_value())
	{
		Locator::particleSystem::value().StartSpotVisual(SpotVisualType::SpellFailCast, point, std::nullopt, entt::null);
	}
	if (Locator::audio::has_value())
	{
		Locator::audio::value().PlaySoundEffect(static_cast<entt::id_type>(audio::SoundId::G_SpellCastFailure), std::nullopt);
	}
	if (_lastHandResult != HandResult::NoCircle)
	{
		_lastHandResult = HandResult::CantCastThere;
	}
}

void MagicSystem::Do(const magic::CastActions& actions)
{
	if (actions.notReady)
	{
		_lastHandResult = HandResult::NotReady;
	}
	if (actions.stopHoldLoop)
	{
		StopHoldLoop();
	}
	if (actions.startHoldLoop)
	{
		StartHoldLoop();
		_lastHandResult = HandResult::Readied;
	}
	if (actions.unlock)
	{
		UnlockHeldMiracle();
	}
	if (actions.fail)
	{
		FailCast();
	}
	if (actions.notReady)
	{
		_lastHandResult = HandResult::NotReady;
	}
	if (actions.cast.has_value())
	{
		const bool locked = _input.state == magic::CastInput::State::Locked;
		const auto spell = CastHeldSeed(*actions.cast);
		if (spell == entt::null)
		{
			_input = {};
			FailCast();
			return;
		}
		_lastHandResult = locked ? HandResult::CastHeld : HandResult::Cast;
		AfterCast(spell);
	}
}

bool MagicSystem::TapAction()
{
	if (!_hand.overWorld || !Locator::infoConstants::has_value() || _held.has_value())
	{
		return false;
	}
	if (const auto orb = OrbAlong(_hand.rayOrigin, _hand.rayDirection))
	{
		return TapOrb(*orb);
	}
	if (const auto ball = FireBallAlong(_hand.rayOrigin, _hand.rayDirection))
	{
		return CatchFireBall(*ball);
	}
	return false;
}

std::optional<entt::entity> MagicSystem::FireBallAlong(glm::vec3 origin, glm::vec3 direction) const
{
	std::optional<entt::entity> nearest;
	float best = std::numeric_limits<float>::max();
	// Each ball the hand may hold now, a sphere of its size where its particle was last placed
	EntityRegistry().Each<const MagicFireBall>([&](entt::entity entity, const MagicFireBall& ball) {
		if (!ball.handTarget.has_value())
		{
			return;
		}
		const float along = glm::dot(*ball.handTarget - origin, direction);
		if (along <= 0.0f)
		{
			return;
		}
		const auto closest = origin + direction * along;
		if (glm::distance(closest, *ball.handTarget) < ball.radius && along < best)
		{
			best = along;
			nearest = entity;
		}
	});
	return nearest;
}

bool MagicSystem::CatchFireBall(entt::entity ball)
{
	auto& registry = EntityRegistry();
	const auto& fireBall = registry.Get<const MagicFireBall>(ball);
	// Only another's ball is caught: the player's own slips through the hand. The hand is this computer's player's.
	constexpr auto k_Catcher = PlayerNames::PLAYER_ONE;
	if (fireBall.hasPlayer && fireBall.player == k_Catcher)
	{
		return false;
	}
	// It becomes a fireball seed in the hand, charged in full and ready, and the ball goes
	if (GiveSeedToHand(k_Catcher, SpellSeedType::Fire, magic::k_BasePowerUpLevel, 1.0f) == entt::null)
	{
		return false;
	}
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Magic: the hand caught fireball #{}", entt::to_integral(ball));
	if (Locator::fireSystem::has_value())
	{
		Locator::fireSystem::value().Forget(ball);
	}
	registry.Destroy(ball);
	return true;
}

bool MagicSystem::AbsorbFireBall(entt::entity ball)
{
	auto& registry = EntityRegistry();
	if (!_held.has_value() || !registry.Valid(*_held))
	{
		return false;
	}
	auto& seed = registry.Get<SpellSeed>(*_held);
	if (!seed.ready || seed.seedType != SpellSeedType::Fire)
	{
		return false;
	}
	// Every fireball takes the table's first row
	seed.power *= 1.0f + Info().magicFireBall.at(0).catchIncreaseFactor;
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Magic: the fire seed took in fireball #{}, its power now {:.4f}",
	                    entt::to_integral(ball), seed.power);
	if (Locator::fireSystem::has_value())
	{
		Locator::fireSystem::value().Forget(ball);
	}
	registry.Destroy(ball);
	return true;
}

bool MagicSystem::PressAction()
{
	if (!_hand.overWorld || !Locator::infoConstants::has_value() || !_held.has_value())
	{
		return false;
	}
	_actionDown = true;
	// A ready fire seed put to a fireball, anyone's, takes it in: the seed grows stronger and stays in the hand
	if (const auto ball = FireBallAlong(_hand.rayOrigin, _hand.rayDirection); ball.has_value() && AbsorbFireBall(*ball))
	{
		return true;
	}
	const auto& seed = EntityRegistry().Get<const SpellSeed>(*_held);
	const auto& seedInfo = magic::GetSpellSeedInfo(Info(), seed.seedType);
	// Outside the player's influence a press does nothing at all
	const magic::PressContext context {
	    .inInfluence = IsHandInInfluence(),
	    .seedReady = seed.ready,
	    .onValidObject = HeldSeedTarget().has_value(),
	    .pointValid = HandPointValid() || seedInfo.sizingGesture == GestureType::Circle,
	    .turn = _turn,
	};
	Do(magic::Press(_input, {.castType = seedInfo.castType, .castOnObject = seedInfo.castOnObject != 0}, context));
	return true;
}

void MagicSystem::ReleaseAction()
{
	_actionDown = false;
	if (_held.has_value())
	{
		Do(magic::Release(_input));
	}
}

void MagicSystem::UnlockHeldMiracle()
{
	auto& registry = EntityRegistry();
	magic::StopPour(_pour);
	if (!_held.has_value() || !registry.Valid(*_held))
	{
		return;
	}
	auto& seed = registry.Get<SpellSeed>(*_held);
	bool enough = true;
	if (auto* spell = FindSpell(seed.spell))
	{
		// The seed keeps what the miracle has left, and how long it has run, for another go
		seed.storedChants = spell->chants.chants;
		seed.storedAge = spell->age;
		seed.storedMaxObjects = spell->maxObjectsToCreate;
		enough = !spell->closedDown && magic::spells::HasEnoughForRecast(Info(), *spell);
		spell->seed = entt::null;
		CloseDown(seed.spell);
	}
	seed.spell = entt::null;
	_lastHandResult = HandResult::Released;
	if (magic::GetSpellSeedInfo(Info(), seed.seedType).deleteSeedOnceCast != 0 || !enough)
	{
		LetGoOfSeed(true);
	}
}

void MagicSystem::DiscardHeldSeed()
{
	if (!_held.has_value())
	{
		return;
	}
	Do(magic::Cancel(_input));
	auto& registry = EntityRegistry();
	if (_held.has_value() && registry.Valid(*_held))
	{
		// What the seed still holds goes back to the player's worship
		const auto& seed = registry.Get<const SpellSeed>(*_held);
		if (auto* store = PrayerOf(seed.player))
		{
			magic::ReturnPrayer(*store, magic::SeedRefund(seed.hasIcon, seed.chantStore, seed.storedChants, seed.hasCast));
		}
	}
	if (Locator::audio::has_value())
	{
		constexpr uint32_t k_ShakeVolume = 35;
		Locator::audio::value().StartSoundEffect(static_cast<entt::id_type>(audio::SoundId::G_ShakeHand_01),
		                                         {.volume = k_ShakeVolume});
	}
	// A band flies off the hand
	if (Locator::miracleFxSystem::has_value())
	{
		Locator::miracleFxSystem::value().SeedShakenOff();
	}
	LetGoOfSeed(true);
	_lastHandResult = HandResult::Discarded;
}

void MagicSystem::PowerUpHeldSeed(int level)
{
	auto& registry = EntityRegistry();
	if (!_held.has_value() || !registry.Valid(*_held))
	{
		return;
	}
	auto& seed = registry.Get<SpellSeed>(*_held);
	const auto& seedInfo = magic::GetSpellSeedInfo(Info(), seed.seedType);
	// Only a seed made at an icon, not yet cast, can be powered up, and only to a level it has
	const auto slot = static_cast<size_t>(level + 1);
	if (!seed.hasIcon || seed.hasCast || level < 0 || slot >= seedInfo.magicTypes.size() ||
	    seedInfo.magicTypes.at(slot) == MagicType::None)
	{
		return;
	}
	const auto type = seedInfo.magicTypes.at(slot);
	const float cost = magic::GetChantsRequiredToCreate(Info(), type);
	auto* store = PrayerOf(seed.player);
	// The worship charges what more it needs, or takes back what it no longer does
	if (store != nullptr)
	{
		if (cost > seed.chantStore)
		{
			seed.chantStore += magic::DrawPrayer(*store, cost - seed.chantStore);
		}
		else
		{
			magic::ReturnPrayer(*store, seed.chantStore - cost);
			seed.chantStore = cost;
		}
	}
	const int previous = seed.powerUp;
	seed.powerUp = level;
	seed.power = magic::SeedPower(seed.chantStore, cost);
	// Its in-hand effect starts afresh at the new level, shown once the seed is ready
	StopHandEffect(seed);
	StartHandEffect(*_held);
	ShowHeldSeed();
	// A bracelet more, bands flying on and the announcer
	if (Locator::miracleFxSystem::has_value())
	{
		Locator::miracleFxSystem::value().SeedInHand(level, previous);
	}
	_lastHandResult = HandResult::PoweredUp;
}

void MagicSystem::TakeGestures()
{
	if (!Locator::gestureEvents::has_value())
	{
		return;
	}
	for (const auto& event : Locator::gestureEvents::value().TakeEvents())
	{
		switch (event.kind)
		{
		case GestureEvent::Kind::Circle:
		{
			auto centre = event.centre;
			centre.y = _world.LandHeight({centre.x, centre.z});
			_circle = Circle {.centre = centre, .radius = event.radius, .turn = _turn};
			break;
		}
		case GestureEvent::Kind::PowerUp:
			PowerUpHeldSeed(event.powerUpLevel);
			break;
		case GestureEvent::Kind::Scribble:
		case GestureEvent::Kind::Shake:
			DiscardHeldSeed();
			break;
		}
	}
}

PrayerPower* MagicSystem::PrayerOf(PlayerNames player) const
{
	auto& registry = EntityRegistry();
	PrayerPower* found = nullptr;
	registry.Each<const Player, PrayerPower>([&](entt::entity, const Player& component, PrayerPower& prayer) {
		if (component.name == player)
		{
			found = &prayer;
		}
	});
	return found;
}

MagicSystemInterface::HandCastState MagicSystem::GetHandCastState() const
{
	HandCastState state {.state = _input.state,
	                     .holding = _actionDown,
	                     .velocity = _handVelocity,
	                     .throwSpeed = particles::maths::HandThrow(_handVelocity).speed,
	                     .pour = magic::PourPoseAt(_pour, 1.0f),
	                     .pointValid = HandPointValid()};
	if (_held.has_value() && EntityRegistry().Valid(*_held) && Locator::infoConstants::has_value())
	{
		const auto& seed = EntityRegistry().Get<const SpellSeed>(*_held);
		state.ready = seed.ready;
		state.readyIn =
		    seed.ready
		        ? 0.0f
		        : std::max(Info().spellSystem.delayBeforeSeedActive - static_cast<float>(seed.turnsInHand) * k_TurnSeconds,
		                   0.0f);
		state.origin = seed.origin;
		state.powerUp = seed.powerUp;
		state.chantStore = seed.chantStore;
		state.storedChants = seed.storedChants;
	}
	if (_circle.has_value())
	{
		state.circleCentre = _circle->centre;
		state.circleRadius = _circle->radius;
		state.circleSecondsLeft =
		    std::max(static_cast<float>(magic::k_CircleTurns) - static_cast<float>(_turn - _circle->turn), 0.0f) *
		    k_TurnSeconds;
	}
	return state;
}

void MagicSystem::Empathise(MagicType type, PlayerNames player, glm::vec3 point)
{
	auto& registry = EntityRegistry();
	const auto creature =
	    Locator::leashSystem::has_value() ? Locator::leashSystem::value().PlayersCreature(player) : std::nullopt;
	auto* mind = creature.has_value() && registry.Valid(*creature) ? registry.TryGet<CreatureMindState>(*creature) : nullptr;
	const auto* at = mind != nullptr ? registry.TryGet<const Transform>(*creature) : nullptr;
	if (at == nullptr)
	{
		return;
	}
	// It sees what lies within two thirds of a half turn of where it looks, its head's look if it looks at something
	const auto* animation = registry.TryGet<const CreatureAnimation>(*creature);
	const auto* moving = registry.TryGet<const CreatureLocomotion>(*creature);
	const glm::vec2 here(at->position.x, at->position.z);
	uint16_t look = 0;
	if (animation != nullptr && animation->lookAt.has_value())
	{
		look = gutils::GetAngleFromXZ(here, glm::vec2(animation->lookAt->x, animation->lookAt->z));
	}
	else if (moving != nullptr)
	{
		const auto ahead = creature_locomotion::DirectionOf(moving->heading);
		look = gutils::GetAngleFromXZ(here, here + ahead * k_LookAheadMetres);
	}
	const bool sameCell =
	    map_coords::Cell(map_coords::FromMetres(here)) == map_coords::Cell(map_coords::FromMetres(glm::vec2(point.x, point.z)));
	if (!creature_perceived_desires::CanSeePos(look, gutils::GetAngleFromXZ(here, glm::vec2(point.x, point.z)), sameCell))
	{
		return;
	}
	const auto& effect = magic::GetMagicEffectInfo(Info(), type);
	for (const auto desire : effect.perceivedPlayerDesire)
	{
		creature_perceived_desires::Increase(mind->perceivedDesires, static_cast<size_t>(desire), 1.0f);
	}
	creature_perceived_desires::IncreaseTown(mind->perceivedDesires, static_cast<size_t>(effect.townDesireBeingHelped), 1.0f);
}

void MagicSystem::RecordCast(MagicType type, PlayerNames player, glm::vec3 point)
{
	EntityRegistry().Each<components::Player>([&](entt::entity, components::Player& record) {
		if (record.name == player)
		{
			record.lastCast = components::Player::Cast {.position = point, .type = type, .turn = _turn};
			++record.castsOfType.at(static_cast<size_t>(type));
		}
	});
}

void MagicSystem::ReactToSpell(Spell& spell, bool onCast)
{
	const auto& effect = magic::GetMagicEffectInfo(Info(), spell.magicType);
	if (effect.reactionType == Reaction::None || !Locator::reactionSystem::has_value())
	{
		return;
	}
	// A reaction that has ended lets the miracle's next event make another
	if (spell.reaction != 0 && !Locator::reactionSystem::value().Find(spell.reaction).has_value())
	{
		spell.reaction = 0;
	}
	if (spell.reaction != 0)
	{
		return;
	}
	const auto found = std::ranges::find_if(
	    _spells, [&spell](entt::entity e) { return EntityRegistry().Valid(e) && EntityRegistry().TryGet<Spell>(e) == &spell; });
	if (found == _spells.end())
	{
		return;
	}
	const auto entity = *found;
	spell.reaction = Locator::reactionSystem::value().Create({
	    .initiator = entity,
	    .type = effect.reactionType,
	    .player = spell.caster.player,
	    .position = spell.position,
	    .impressiveValue = effect.impressiveValue,
	    .power = 1.0f,
	    .strength = magic::spells::StrengthOf(*this, spell),
	    .magicType = spell.magicType,
	    .casterCreature = spell.caster.kind == SpellCaster::Kind::Creature ? spell.caster.entity : entt::null,
	    .onCast = onCast,
	});
}

// What the miracles' objects in the world do for them

void MagicSystem::ShieldStruck(const Spell& shield, bool destroyed)
{
	if (Locator::magicShieldSystem::has_value())
	{
		Locator::magicShieldSystem::value().ShieldStruck(EntityRegistry().ToEntity(shield), destroyed);
	}
}

bool MagicSystem::HasWorldObjects(const Spell& spell) const
{
	const auto entity = EntityRegistry().ToEntity(spell);
	if (spell.spellClass == magic::SpellClass::Shield)
	{
		return Locator::magicShieldSystem::has_value() && Locator::magicShieldSystem::value().HasObject(entity);
	}
	if (spell.spellClass == magic::SpellClass::Forest)
	{
		return Locator::forestSystem::has_value() && Locator::forestSystem::value().HasTrees(entity);
	}
	return false;
}

bool MagicSystem::PlantForest(Spell& spell)
{
	return Locator::forestSystem::has_value() &&
	       Locator::forestSystem::value().Plant(EntityRegistry().ToEntity(spell), spell, TribalPower(spell)) > 0;
}

bool MagicSystem::ForestCanGrowAt(glm::vec3 point) const
{
	return !Locator::forestSystem::has_value() || Locator::forestSystem::value().CanGrowAt(point);
}

bool MagicSystem::SendSpellEvent(entt::entity spell, const particles::SpellEventInfo& event)
{
	return OnSpellEvent(spell, event);
}

float MagicSystem::ForcePayForSpell(entt::entity entity, float cost)
{
	auto* spell = FindSpell(entity);
	if (spell == nullptr)
	{
		return 0.0f;
	}
	return magic::PayFor(spell->chants, magic::spells::RulesOf(*this, *spell), CasterOf(*spell), cost, magic::Refill::Whole);
}

float MagicSystem::SpellStrength(entt::entity entity)
{
	const auto* spell = FindSpell(entity);
	return spell != nullptr ? magic::spells::StrengthOf(*this, *spell) : 0.0f;
}

// For the debug window

void MagicSystem::StartCastEffect(Spell& spell, ParticleType type)
{
	auto& particles = Locator::particleSystem::value();
	// It plays out by itself, the same on every machine, as big as the miracle and heading the way it was cast
	spell.castEffect = particles.Start(type, spell.castPosition, spell.magnitude, true);
	if (auto* effect = particles.Find(spell.castEffect))
	{
		effect->SetDirection(spell.direction);
		effect->SetPlayer(static_cast<int>(spell.caster.player));
		effect->SetProcessInfo(spell.processInfo);
	}
}

void MagicSystem::RainOnFire(const glm::vec3& point)
{
	if (!Locator::reactionSystem::has_value())
	{
		return;
	}
	auto& reactions = Locator::reactionSystem::value();
	auto& registry = EntityRegistry();
	bool done = false;
	registry.Each<Spell>([&](entt::entity entity, Spell& spell) {
		if (done || spell.spellClass != magic::SpellClass::StormAndTornado)
		{
			return;
		}
		auto& watching = _rainWatchers[entity];
		if (watching != 0 && reactions.IsActive(watching))
		{
			return;
		}
		if (glm::distance(glm::vec2(point.x, point.z), glm::vec2(spell.position.x, spell.position.z)) < spell.magnitude)
		{
			watching = reactions.Create({.initiator = entity,
			                             .type = Reaction::ReactToMagicWaterPuttingOutFire,
			                             .player = spell.caster.player,
			                             .position = spell.position,
			                             .onCast = true});
			done = true;
		}
	});
}

std::optional<entt::entity> MagicSystem::SpellAt(MagicType type, glm::vec3 point, float radius) const
{
	// The miracles are kept newest first
	for (const auto entity : _spells)
	{
		const auto* spell = EntityRegistry().TryGet<const Spell>(entity);
		if (spell != nullptr && spell->magicType == type && gutils::GetDistanceInMetres(spell->position, point) < radius)
		{
			return entity;
		}
	}
	return std::nullopt;
}

std::vector<MagicSystemInterface::SpellInfo> MagicSystem::GetSpells() const
{
	std::vector<SpellInfo> result;
	if (!Locator::infoConstants::has_value())
	{
		return result;
	}
	auto& self = const_cast<MagicSystem&>(*this);
	for (const auto entity : _spells)
	{
		const auto* spell = EntityRegistry().TryGet<const Spell>(entity);
		if (spell == nullptr)
		{
			continue;
		}
		result.push_back({
		    .entity = entity,
		    .magicType = spell->magicType,
		    .player = spell->caster.player,
		    .age = spell->age,
		    .duration = spell->duration,
		    .chants = spell->chants.chants,
		    .initialChants = spell->chants.initialChants,
		    .strength = magic::spells::StrengthOf(self, *spell),
		    .upkeep = magic::spells::CostToMaintain(Info(), *spell),
		    .closing = spell->closedDown,
		    .fromHand = spell->castFromHand,
		    .effect = spell->effect,
		    .position = spell->position,
		});
	}
	return result;
}

std::vector<MagicSystemInterface::DispenserInfo> MagicSystem::GetDispensers() const
{
	std::vector<DispenserInfo> result;
	EntityRegistry().Each<const SpellDispenser, const Transform>(
	    [&](entt::entity entity, const SpellDispenser& dispenser, const Transform& transform) {
		    result.push_back({
		        .entity = entity,
		        .magicType = dispenser.magicType,
		        .position = transform.position,
		        .hasOrb = dispenser.orb != entt::null,
		        .tick = dispenser.timer.tick,
		        .period = dispenser.timer.period,
		        .active = dispenser.timer.active,
		    });
	    });
	return result;
}

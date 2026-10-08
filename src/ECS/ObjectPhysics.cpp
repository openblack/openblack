/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ObjectPhysics.h"

#include <cmath>

#include <algorithm>
#include <array>
#include <numbers>

#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Common/GUtilsAngle.h"
#include "Common/GUtilsDistance.h"
#include "Common/GameRandom.h"
#include "ECS/Archetypes/DeadTreeArchetype.h"
#include "ECS/Archetypes/MobileStaticArchetype.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/CarriedByTornado.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/FallingRoots.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Fire.h"
#include "ECS/Components/ForestMember.h"
#include "ECS/Components/Indestructible.h"
#include "ECS/Components/MagicForest.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/ScriptControl.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/TempleExterior.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/TownArtefact.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/CreatureSight.h"
#include "ECS/Map.h"
#include "ECS/PhysicsEntry.h"
#include "ECS/PhysicsGround.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/ExplosionSystemInterface.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/ForestSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "ECS/Systems/ResourceStoreSystemInterface.h"
#include "ECS/Systems/ScriptObjectsSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/AreaEffect.h"
#include "Physics/ObjectRules.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
namespace objects = openblack::physics::objects;

namespace
{
Registry& Entities()
{
	return Locator::entitiesRegistry::value();
}

/// The rock-tapping sounds, the hand's and the creature's smash, each four in turn
constexpr std::array<audio::SoundId, 4> k_HandRockTaps = {audio::SoundId::G_RockTap_01_1, audio::SoundId::G_RockTap_02_1,
                                                          audio::SoundId::G_RockTap_03_1, audio::SoundId::G_RockTap_04_1};
constexpr std::array<audio::SoundId, 4> k_CreatureRockSmashes = {audio::SoundId::G_RockTap_01, audio::SoundId::G_RockTap_02,
                                                                 audio::SoundId::G_RockTap_03, audio::SoundId::G_RockTap_04};
constexpr std::array<audio::SoundId, 3> k_TreePlantings = {audio::SoundId::G_PlantTree_01, audio::SoundId::G_PlantTree_02,
                                                           audio::SoundId::G_PlantTree_03};

void PlaySound(audio::SoundId sound, glm::vec3 point)
{
	if (Locator::audio::has_value())
	{
		Locator::audio::value().PlaySoundEffect(static_cast<entt::id_type>(sound), point);
	}
}

/// The next of a set of sounds played in turn, moving the turn on
template <size_t N>
audio::SoundId NextInTurn(const std::array<audio::SoundId, N>& sounds, uint8_t& next)
{
	const auto sound = sounds.at(next);
	next = static_cast<uint8_t>((next + 1) % N);
	return sound;
}

/// Whether a fixed thing belongs to a town or is part of a temple: a building or a field only when it has a town, a
/// building standing outside any town not counting
bool OfTown(const Registry& registry, entt::entity thing)
{
	if (registry.AnyOf<Temple, TempleExterior, TempleEntrance>(thing))
	{
		return true;
	}
	bool found = false;
	if (const auto* abode = registry.TryGet<const Abode>(thing))
	{
		registry.Each<const Town>(
		    [&found, abode](entt::entity, const Town& town) { found = found || town.id == abode->townId; });
	}
	else if (const auto* field = registry.TryGet<const Field>(thing))
	{
		registry.Each<const Town>(
		    [&found, field](entt::entity, const Town& town) { found = found || static_cast<int>(town.id) == field->town; });
	}
	return found;
}

/// The scenic forest of a thing's town, when it belongs to a town that has one
std::optional<uint32_t> TownScenicForest(const Registry& registry, entt::entity thing)
{
	std::optional<int64_t> townId;
	if (const auto* abode = registry.TryGet<const Abode>(thing))
	{
		townId = abode->townId;
	}
	else if (const auto* field = registry.TryGet<const Field>(thing))
	{
		townId = field->town;
	}
	std::optional<uint32_t> forest;
	if (townId.has_value())
	{
		registry.Each<const Town>([&forest, townId](entt::entity, const Town& town) {
			if (static_cast<int64_t>(town.id) == *townId)
			{
				forest = town.scenicForest;
			}
		});
	}
	return forest;
}

/// The tree takes root again: a white puff of smoke at its foot, then it finds a forest to join among what stands round it
void Replant(const PhysicsEntry* entry, entt::entity tree, const LandIslandInterface& land)
{
	auto& registry = Entities();
	// It stands on the land again, at no height above it
	auto& placed = registry.Get<Transform>(tree).position;
	placed.y = land.GetHeightAt({placed.x, placed.z});
	const auto position = placed;
	const glm::vec3 foot = position;
	if (Locator::explosionSystem::has_value())
	{
		Locator::explosionSystem::value().AddSmoke(foot, objects::k_ReplantSmokeSize, objects::k_ReplantSmokeColour);
	}
	// TODO(force-feedback): the player who dropped it feels the planting (force feedback effect 0x2E); openblack has none
	// The things standing round it, in a spiral out from its own cell until the cells are too far away
	objects::ForestSearch search;
	const auto own = map_coords::FromMetres({position.x, position.z});
	auto coords = own;
	map_coords::Spiral spiral;
	for (int step = 0; step < objects::k_ReplantSearchCells; ++step)
	{
		if (gutils::GetDistanceInMetres(own, coords) > objects::k_ReplantSearchReach)
		{
			break;
		}
		const glm::ivec2 cell = map_coords::Cell(coords);
		if (map_coords::InBounds(cell) && Locator::entitiesMap::has_value())
		{
			for (const auto thing : Locator::entitiesMap::value().GetFixedInGridCell(MapInterface::CellId(cell)))
			{
				if (thing == tree || !registry.Valid(thing) || !registry.AllOf<Transform>(thing))
				{
					continue;
				}
				const auto& at = registry.Get<const Transform>(thing).position;
				const float edge = gutils::GetDistanceInMetres(map_coords::FromMetres({at.x, at.z}), own) -
				                   world_objects::SizeOf(thing).radius;
				const auto* member = registry.AllOf<Tree>(thing) ? registry.TryGet<const ForestMember>(thing) : nullptr;
				// Near a town it joins the town's scenic forest, if the town has one
				if (!search.Meet({.edgeDistance = edge,
				                  .ofTown = OfTown(registry, thing),
				                  .townForest = TownScenicForest(registry, thing),
				                  .forest = member != nullptr ? std::optional(member->forest) : std::nullopt}))
				{
					break;
				}
			}
		}
		map_coords::AddCells(coords, spiral.Next());
	}
	registry.Remove<ForestMember>(tree);
	if (const auto forest = search.Forest())
	{
		registry.Assign<ForestMember>(tree, *forest);
	}
	else if (search.StartsForest() && Locator::forestSystem::has_value())
	{
		registry.Assign<ForestMember>(tree,
		                              Locator::forestSystem::value().MakeLandForest(std::nullopt, position, entt::null, false));
	}
	// Away from towns, the player sees the forest grow
	if (!search.NearTown() && Locator::particleSystem::has_value())
	{
		auto& particles = Locator::particleSystem::value();
		const auto effect = particles.StartSpotVisual(SpotVisualType::ForestCreated, position, objects::k_ReplantVisualTurns,
		                                              entt::null, objects::k_ReplantVisualMagnitude);
		if (effect != systems::ParticleSystemInterface::k_NoEffect && entry != nullptr && entry->player.has_value())
		{
			particles.SetPlayer(effect, static_cast<int>(*entry->player));
		}
	}
	// The player who planted it is seen to, by their creature and by the world
	if (entry == nullptr || !entry->player.has_value())
	{
		return;
	}
	const auto player = *entry->player;
	if (Locator::creatureMindSystem::has_value())
	{
		Locator::creatureMindSystem::value().PlayerDid(objects::k_DeedPlantTree, position, tree, player);
	}
	if (Locator::alignmentSystem::has_value() && Locator::infoConstants::has_value() && player != PlayerNames::NEUTRAL)
	{
		auto& alignment = Locator::alignmentSystem::value();
		const float change = Locator::infoConstants::value().player.treePullPutAlignmentChange;
		alignment.AddPendingAlignment(player, magic::DampAlignmentChange(change, alignment.GetPlayerAlignment(player)));
	}
}

/// The people come for a dead tree's wood, called by its player, or by no player at all
void CallForWood(entt::entity deadTree, std::optional<PlayerNames> player)
{
	const auto* transform = Entities().TryGet<const Transform>(deadTree);
	if (transform == nullptr || !Locator::reactionSystem::has_value())
	{
		return;
	}
	Locator::reactionSystem::value().Create({.initiator = deadTree,
	                                         .type = Reaction::ReactToWood,
	                                         .player = player.value_or(PlayerNames::NEUTRAL),
	                                         .position = transform->position,
	                                         .playerless = !player.has_value()});
}

/// The tree falls dead where it came down: a dead tree of its kind and size, turned as it lies, with its fire and its
/// wood's worth; the tree goes
entt::entity BecomeDeadTree(const PhysicsEntry* entry, entt::entity tree)
{
	auto& registry = Entities();
	const auto& transform = registry.Get<const Transform>(tree);
	const auto type = registry.Get<const Tree>(tree).type;
	const auto* magic = registry.TryGet<const MagicTree>(tree);
	const auto dead = archetypes::DeadTreeArchetype::Create(transform.position, type, 0.0f, transform.scale.x);
	registry.Get<Transform>(dead).rotation = transform.rotation;
	registry.Get<DeadTree>(dead).woodMultiplier = magic != nullptr ? magic->woodMultiplier : 1.0f;
	// The first time it is drawn its roots drop from it
	registry.Assign<DropsRoots>(dead);
	// It lies where it came down, in the cells there
	if (Locator::entitiesMap::has_value())
	{
		Locator::entitiesMap::value().Refile(dead);
	}
	if (Locator::fireSystem::has_value())
	{
		Locator::fireSystem::value().MoveFire(tree, dead);
	}
	// A dead tree made from a tree always has a player, the neutral one when no one threw it, and calls people as theirs
	CallForWood(dead, entry != nullptr && entry->player.has_value() ? *entry->player : PlayerNames::NEUTRAL);
	// A script holding the tree holds the dead tree in its place
	if (registry.AllOf<InScript>(tree) && Locator::scriptObjects::has_value())
	{
		Locator::scriptObjects::value().Replace(tree, dead);
		registry.AssignOrReplace<InScript>(dead);
		if (registry.AllOf<ScriptControlled>(tree))
		{
			registry.AssignOrReplace<ScriptControlled>(dead);
		}
	}
	world_objects::Remove(tree);
	return dead;
}
} // namespace

entt::entity object_physics::EndTree(systems::DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity tree,
                                     bool insert)
{
	auto& registry = Entities();
	const bool hasBody = entry != nullptr;
	// A tree a tornado carries ends nothing: it stays as it is until it is let go
	if (registry.AllOf<CarriedByTornado>(tree))
	{
		return tree;
	}
	if (!registry.AllOf<InPhysics>(tree) || !Locator::terrainSystem::has_value())
	{
		return dynamics.EndPhysicsAsObject(tree, insert, hasBody);
	}
	if (!insert)
	{
		return dynamics.EndPhysicsAsObject(tree, false, hasBody);
	}
	const auto& land = Locator::terrainSystem::value();
	const auto& position = registry.Get<const Transform>(tree).position;
	const PhysicsGround ground(land);
	const bool burning = registry.AllOf<Fire>(tree);
	switch (objects::TreeLandingOf(entry != nullptr && entry->Has(PhysicsEntry::k_Landed), registry.AllOf<MagicTree>(tree),
	                               ground.IsLand({position.x, position.z}), burning))
	{
	case objects::TreeLanding::Replanted:
		Replant(entry, tree, land);
		[[fallthrough]];
	case objects::TreeLanding::Stays:
		return dynamics.EndPhysicsAsObject(tree, true, hasBody);
	case objects::TreeLanding::Dies:
		break;
	}
	return BecomeDeadTree(entry, tree);
}

entt::entity object_physics::EndDeadTree(systems::DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity deadTree,
                                         bool insert)
{
	ConsiderArtefact(entry, deadTree, insert);
	const auto kept = dynamics.EndPhysicsAsObject(deadTree, insert, entry != nullptr);
	// A felled tree comes down as a fixed thing does: it called people to its wood as it fell
	const auto* data = Entities().TryGet<const DeadTree>(deadTree);
	if (kept == deadTree && Entities().Valid(deadTree) && (data == nullptr || !data->felled))
	{
		CallForWood(deadTree, entry != nullptr ? entry->player : std::nullopt);
	}
	return kept;
}

entt::entity object_physics::EndPot(systems::DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity pot,
                                    bool insert)
{
	auto& registry = Entities();
	const auto* data = registry.TryGet<const Pot>(pot);
	const bool handful = data != nullptr && (data->type == PotInfo::HandWood || data->type == PotInfo::HandFood);
	if (!insert || !handful || !Locator::terrainSystem::has_value() || !Locator::resourceStoreSystem::has_value())
	{
		return dynamics.EndPhysicsAsObject(pot, insert, entry != nullptr);
	}
	const auto& position = registry.Get<const Transform>(pot).position;
	const PhysicsGround ground(Locator::terrainSystem::value());
	if (!ground.IsLand({position.x, position.z}))
	{
		// In the water it floats on as a thing
		return dynamics.EndPhysicsAsObject(pot, insert, entry != nullptr);
	}
	// On the land it spills into the stores and piles round it, or makes a pile, and flickers out
	auto& stores = Locator::resourceStoreSystem::value();
	const auto resource = stores.ResourceOf(pot);
	stores.PourAt(resource.type, position, resource.amount, false,
	              entry != nullptr ? entry->player.value_or(PlayerNames::NEUTRAL) : PlayerNames::NEUTRAL, resource.poisoned);
	world_objects::LeaveGhost(pot);
	world_objects::Remove(pot);
	return pot;
}

void object_physics::TreeDropSound(entt::entity tree, uint64_t ticks)
{
	if (!Entities().AllOf<Tree>(tree))
	{
		return;
	}
	// One of three planting sounds, chosen by the system clock
	const auto& position = Entities().Get<const Transform>(tree).position;
	PlaySound(k_TreePlantings.at(ticks % k_TreePlantings.size()), position);
}

bool object_physics::IsBonfire(entt::entity object)
{
	const auto* mobile = Entities().TryGet<const MobileStatic>(object);
	return mobile != nullptr && mobile->type == MobileStaticInfo::Bonfire;
}

bool object_physics::IsRock(entt::entity object)
{
	if (IsBonfire(object))
	{
		return true;
	}
	const auto* mobile = Entities().TryGet<const MobileStatic>(object);
	return mobile != nullptr && Locator::infoConstants::has_value() &&
	       Locator::infoConstants::value().mobileStatic.at(static_cast<size_t>(mobile->type)).mobileType ==
	           MobileStaticInfo::Rock;
}

void object_physics::KnockRock(systems::DynamicsSystemInterface& dynamics, entt::entity rock, const ImpactInfo& impact)
{
	auto& registry = Entities();
	// A bonfire is a kind of rock that is never worn by knocks
	if (IsBonfire(rock))
	{
		return;
	}
	const bool struckByRock = impact.hitBy != entt::null && registry.Valid(impact.hitBy) && IsRock(impact.hitBy);
	const auto wear = objects::RockWear(impact.g, world_objects::SizeOf(rock).height, struckByRock);
	if (!wear.has_value())
	{
		return;
	}
	auto& life = registry.AllOf<ObjectLife>(rock) ? registry.Get<ObjectLife>(rock) : registry.Assign<ObjectLife>(rock);
	const float worn = life.life - *wear;
	// An indestructible rock's life is never let down to where it would break
	if (registry.AllOf<Indestructible>(rock) && worn <= objects::k_RockBreakLife)
	{
		return;
	}
	life.life = worn;
	if (life.life < objects::k_RockBreakLife)
	{
		SplitRock(dynamics, rock);
	}
}

void object_physics::SplitRock(systems::DynamicsSystemInterface& dynamics, entt::entity rock)
{
	auto& registry = Entities();
	if (!registry.Valid(rock) || !Locator::gameRandom::has_value() || !Locator::terrainSystem::has_value())
	{
		return;
	}
	const auto& land = Locator::terrainSystem::value();
	const auto transform = registry.Get<const Transform>(rock);
	const auto type = registry.Get<const MobileStatic>(rock).type;
	// The halves lie either side of it, across a line at a random angle
	const float angle = Locator::gameRandom::value().GameFloatRand(2.0f * std::numbers::pi_v<float>);
	const float scale = transform.scale.x * objects::k_RockHalfScale;
	const auto offset = gutils::GetPosFromAngle(angle, world_objects::SizeOf(rock).radius * objects::k_RockHalfSpread);
	// A flying rock's halves fly on as it flew, with half its spin; a resting rock's start still. One the physics has lost
	// track of doesn't break.
	glm::vec3 velocity(0.0f);
	glm::vec3 spin(0.0f);
	if (registry.AllOf<InPhysics>(rock))
	{
		const auto* entry = dynamics.Find(rock);
		if (entry == nullptr || entry->body == nullptr)
		{
			return;
		}
		velocity = entry->body->velocity;
		spin = entry->body->angularMomentum * objects::k_RockHalfSpin;
	}
	const auto heading = objects::HeadingOnly(transform.rotation);
	const auto at = map_coords::FromWorld(land, transform.position);
	for (const auto& place : {at + offset, at - offset})
	{
		const auto half =
		    archetypes::MobileStaticArchetype::Create(map_coords::ToWorld(land, place), type, 0.0f, 0.0f, 0.0f, 0.0f, scale);
		registry.Get<Transform>(half).rotation = heading;
		registry.AssignOrReplace<ObjectLife>(half, ObjectLife {.life = 1.0f});
		const auto started = dynamics.InitialisePhysics(half, {.velocity = velocity, .add = true});
		if (started.entry != nullptr && started.entry->body != nullptr)
		{
			started.entry->body->angularMomentum = spin;
		}
		if (Locator::fireSystem::has_value())
		{
			Locator::fireSystem::value().CopyFire(rock, half);
		}
	}
	world_objects::Remove(rock);
}

bool object_physics::CanTapRock(entt::entity rock)
{
	// A bonfire is never broken by a tap
	return IsRock(rock) && !IsBonfire(rock) && objects::RockBreaksWhenTapped(world_objects::SizeOf(rock).height);
}

void object_physics::TapRock(systems::DynamicsSystemInterface& dynamics, entt::entity rock, glm::vec3 handPoint,
                             std::optional<PlayerNames> player)
{
	auto& registry = Entities();
	const auto point = registry.Get<const Transform>(rock).position;
	SplitRock(dynamics, rock);
	PlaySound(NextInTurn(k_HandRockTaps, registry.Context().nextHandRockTap), handPoint);
	// The player's creature sees the player playing with the rock
	if (player.has_value())
	{
		creature_sight::EmpathiseWithPlayer(*player, CreatureDesires::ToPlay, objects::k_TapEmpathy, point);
	}
}

void object_physics::SmashRock(systems::DynamicsSystemInterface& dynamics, entt::entity rock)
{
	auto& registry = Entities();
	const auto point = registry.Get<const Transform>(rock).position;
	PlaySound(NextInTurn(k_CreatureRockSmashes, registry.Context().nextCreatureRockSmash), point);
	SplitRock(dynamics, rock);
}

entt::entity object_physics::FellTree(systems::DynamicsSystemInterface& dynamics, entt::entity tree, entt::entity feller)
{
	auto& registry = Entities();
	const auto* transform = registry.TryGet<const Transform>(tree);
	const auto* feature = registry.TryGet<const Tree>(tree);
	const auto* from = registry.TryGet<const Transform>(feller);
	if (transform == nullptr || feature == nullptr || from == nullptr)
	{
		return entt::null;
	}
	const float height = world_objects::SizeOf(tree).height;
	// A dead tree of the tree's kind and size, as it stands, the neutral player's
	const auto felled = archetypes::DeadTreeArchetype::Create(transform->position, feature->type, 0.0f, transform->scale.x);
	registry.Get<Transform>(felled).rotation = transform->rotation;
	registry.Get<DeadTree>(felled).felled = true;
	const auto fall = physics::objects::FellingOf(
	    height, glm::vec2(transform->position.x - from->position.x, transform->position.z - from->position.z));
	const auto started = dynamics.InitialisePhysics(felled, {.velocity = fall.velocity,
	                                                         .spin = fall.spin,
	                                                         .thrower = feller,
	                                                         .player = std::nullopt,
	                                                         .add = true,
	                                                         .fromHand = false});
	if (started.entry != nullptr)
	{
		// It is set on the land, turned to its slope, then raised clear of what is under it; villagers pass through it,
		// and it sounds as it topples
		dynamics.SettleOnLand(*started.entry, false, true);
		started.entry->flags |= PhysicsEntry::k_PushedByLiving;
		dynamics.RaiseClearOfWhatIsUnder(*started.entry);
		started.entry->kind = PhysicsEntry::Kind::FelledTree;
	}
	CallForWood(felled, PlayerNames::NEUTRAL);
	return felled;
}

namespace
{
/// The nearest thing of a kind within a reach of a point, as the game finds one: the cells in a spiral out from the
/// point's own, at least three by three of them and as many as twice the reach covers; a thing counts when it is
/// strictly within the reach and strictly nearer than the nearest so far, so the first met wins a tie; and once one is
/// found the walk ends at the first cell farther than half as far again as it, and ten more
template <typename Kind>
entt::entity FindNearest(const map_coords::MapCoords& from, float reach, entt::entity except, Kind isKind)
{
	if (!Locator::entitiesMap::has_value())
	{
		return entt::null;
	}
	const auto& map = Locator::entitiesMap::value();
	auto& registry = Entities();
	auto side = static_cast<int32_t>(std::ceil((reach + reach) / map_coords::k_CellSize));
	side = std::max(side, 3);
	int32_t cells = side * side;
	entt::entity nearest = entt::null;
	float best = 0.0f;
	auto coords = from;
	map_coords::Spiral spiral;
	while (cells != 0 && (nearest == entt::null || gutils::GetDistanceInMetres(from, coords) <= best * 1.5f + 10.0f))
	{
		if (const glm::ivec2 cell = map_coords::Cell(coords); map_coords::InBounds(cell))
		{
			for (const auto thing : map.GetAllInCell(cell))
			{
				if (thing == except || !registry.Valid(thing) || !isKind(thing))
				{
					continue;
				}
				const auto* place = registry.TryGet<const Transform>(thing);
				if (place == nullptr)
				{
					continue;
				}
				const float distance =
				    gutils::GetDistanceInMetres(from, map_coords::FromMetres({place->position.x, place->position.z}));
				if (distance < reach && (distance < best || nearest == entt::null))
				{
					nearest = thing;
					best = distance;
				}
			}
		}
		--cells;
		map_coords::AddCells(coords, spiral.Next());
	}
	return nearest;
}
} // namespace

void object_physics::ConsiderArtefact(const PhysicsEntry* entry, entt::entity object, bool insert)
{
	auto& registry = Entities();
	// Only a gentle put-down from a hand, by a player who isn't neutral, of a thing worth making an artefact of
	if (!insert || entry == nullptr || !entry->player.has_value() || *entry->player == PlayerNames::NEUTRAL ||
	    !entry->Has(PhysicsEntry::k_FromHand) || !entry->Has(PhysicsEntry::k_Landed))
	{
		return;
	}
	const auto* info = world_objects::InfoOf(object);
	const auto* transform = registry.TryGet<const Transform>(object);
	// Nor a thing a script holds
	if (info == nullptr || transform == nullptr || !(info->artifactMultiplier > 0.0f) || registry.AllOf<InScript>(object))
	{
		return;
	}
	// The nearest building within reach, found as the game finds the nearest of a kind of thing: a spiral of cells out
	// from its own, nearest strictly first, until the cells lie well beyond the nearest found
	// TODO(worship): a worship site is suitable as well, and a nearer one takes the artefact; openblack has none yet
	const auto own = map_coords::FromMetres({transform->position.x, transform->position.z});
	const auto nearest = FindNearest(own, physics::objects::k_ArtefactReach, object,
	                                 [&registry](entt::entity thing) { return registry.AnyOf<Abode, SpellDispenser>(thing); });
	if (nearest == entt::null)
	{
		return;
	}
	// TODO(dispensers): a spell dispenser belongs to its town in the game; openblack's keep no town, so none takes it
	const auto* abode = registry.TryGet<const Abode>(nearest);
	if (abode == nullptr)
	{
		return;
	}
	entt::entity town = entt::null;
	const auto townId = abode->townId;
	registry.Each<const Town>([&town, townId](entt::entity entity, const Town& data) {
		if (data.id == townId)
		{
			town = entity;
		}
	});
	if (town == entt::null)
	{
		return;
	}
	const auto player = *entry->player;
	// An artefact already made, worth enough, impresses another town: its people look at it
	if (const auto* record = registry.TryGet<const TownArtefact>(object);
	    record != nullptr && physics::objects::ArtefactWillImpress(record->value, record->town == town) &&
	    Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().Create(
		    {.initiator = object, .type = Reaction::LookAtObject, .player = player, .position = transform->position});
	}
	// It becomes the town's artefact, worth what it impresses villagers when it is new
	if (auto* record = registry.TryGet<TownArtefact>(object))
	{
		record->town = town;
	}
	else
	{
		registry.Assign<TownArtefact>(object,
		                              TownArtefact {.town = town, .player = player, .value = info->villagerImpressiveValue});
	}
	if (Locator::creatureMindSystem::has_value())
	{
		Locator::creatureMindSystem::value().PlayerDid(physics::objects::k_DeedMakeArtefact, transform->position, object,
		                                               player);
	}
}

void object_physics::ArtefactTaken(entt::entity object, PlayerNames player)
{
	if (auto* record = Entities().TryGet<TownArtefact>(object))
	{
		record->town = entt::null;
		record->player = player;
	}
}

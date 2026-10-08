/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "GameHandGrabWorld.h"

#include <cmath>

#include <algorithm>

#include <entt/core/hashed_string.hpp>
#include <fmt/format.h>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Audio/AudioManagerInterface.h"
#include "Common/GameRandom.h"
#include "ECS/Archetypes/PotArchetype.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/MapCellResident.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Player.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/StoragePit.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Map.h"
#include "ECS/PhysicsClasses.h"
#include "ECS/PhysicsEntry.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/AnimalSystemInterface.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/ExplosionSystemInterface.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/InfluenceSystemInterface.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/PickingSystemInterface.h"
#include "ECS/Systems/PlayerSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "ECS/VillagerMemory.h"
#include "ECS/WorldObjects.h"
#include "Hand/HandGrabRules.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/AreaEffect.h"
#include "Physics/Body.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// A pulled-up tree's hole of roots is this share of its width across
constexpr float k_RootsHoleShare = 0.3f;

/// The particles of a handful poured out of the hand: food and wood
constexpr auto k_PourFood = ParticleType::FoodPutdown;
constexpr auto k_PourWood = ParticleType::WoodPutdown;
/// The particles of what is scooped streaming into the hand: food and wood
constexpr auto k_ScoopFood = ParticleType::FoodPickup;
constexpr auto k_ScoopWood = ParticleType::WoodPickup;
/// The scooping sound of the in-game bank, for food and for wood, played each game turn at a pitch rising with the scoop
constexpr uint32_t k_ScoopSample = 44;
constexpr uint32_t k_ScoopWoodSample = 98;
constexpr float k_ScoopPitchStart = 60.0f;
constexpr float k_ScoopPitchRise = 180.0f;

const graphics::L3DMesh* MeshOf(const Registry& registry, entt::entity object)
{
	const auto* mesh = registry.TryGet<const Mesh>(object);
	if (mesh == nullptr || !Locator::resources::has_value())
	{
		return nullptr;
	}
	auto& meshes = Locator::resources::value().GetMeshes();
	return meshes.Contains(mesh->id) ? &*meshes.Handle(mesh->id) : nullptr;
}

const InfoConstants* Info()
{
	return Locator::infoConstants::has_value() ? &Locator::infoConstants::value() : nullptr;
}

PhysicsEntry* EntryOf(entt::entity object)
{
	return Locator::dynamicsSystem::has_value() ? Locator::dynamicsSystem::value().Find(object) : nullptr;
}
} // namespace

Registry& GameHandGrabWorld::Entities()
{
	return Locator::entitiesRegistry::value();
}

entt::entity GameHandGrabWorld::Hand() const
{
	if (!Locator::handSystem::has_value())
	{
		return entt::null;
	}
	return Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(HandSystemInterface::Side::Left)];
}

PlayerNames GameHandGrabWorld::HandPlayer() const
{
	// The hands on the screen are the local player's
	return Locator::playerSystem::has_value() ? Locator::playerSystem::value().GetLocalPlayer() : PlayerNames::PLAYER_ONE;
}

std::optional<entt::entity> GameHandGrabWorld::ObjectUnderCursor([[maybe_unused]] glm::vec3 rayOrigin,
                                                                 [[maybe_unused]] glm::vec3 rayDirection) const
{
	// The object the interface picked under the cursor as the last frame was drawn
	if (!Locator::pickingSystem::has_value())
	{
		return std::nullopt;
	}
	return Locator::pickingSystem::value().GetPick().object;
}

bool GameHandGrabWorld::InInfluence(PlayerNames player, glm::vec3 point) const
{
	return Locator::influenceSystem::has_value() && Locator::influenceSystem::value().PlayerInfluence(player, point) > 0.0f;
}

bool GameHandGrabWorld::InBounds(glm::vec3 point) const
{
	return map_coords::InBounds(point);
}

glm::vec3 GameHandGrabWorld::LandNormalAt(glm::vec3 point) const
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetNormalAt({point.x, point.z})
	                                           : glm::vec3(0.0f, 1.0f, 0.0f);
}

hand_grab::HandGrabWorldInterface::Size GameHandGrabWorld::SizeOf(entt::entity object) const
{
	const auto size = world_objects::SizeOf(object);
	return {.radius = size.radius, .height = size.height};
}

float GameHandGrabWorld::WeightOf(entt::entity object) const
{
	const auto* info = world_objects::InfoOf(object);
	const auto* transform = Locator::entitiesRegistry::value().TryGet<const Transform>(object);
	if (info == nullptr || transform == nullptr)
	{
		return 0.0f;
	}
	return physics_classes::Weight(info->weight, transform->scale.x);
}

float GameHandGrabWorld::LifeOf(entt::entity object) const
{
	return world_objects::LifeOf(object);
}

bool GameHandGrabWorld::IsFlying(entt::entity object) const
{
	return Locator::dynamicsSystem::has_value() && Locator::dynamicsSystem::value().IsFlying(object);
}

bool GameHandGrabWorld::SpeciesAllowsPickUp(entt::entity animal) const
{
	return Locator::animalSystem::has_value() && Locator::animalSystem::value().CanPlayerPickUp(animal);
}

MobileStaticInfo GameHandGrabWorld::StaticKindOf(MobileStaticInfo type) const
{
	const auto* info = Info();
	const auto index = static_cast<size_t>(type);
	return info != nullptr && index < info->mobileStatic.size() ? info->mobileStatic[index].mobileType : MobileStaticInfo::None;
}

MeshId GameHandGrabWorld::StaticMeshOf(MobileStaticInfo type) const
{
	const auto* info = Info();
	const auto index = static_cast<size_t>(type);
	return info != nullptr && index < info->mobileStatic.size() ? info->mobileStatic[index].meshId : MeshId::Dummy;
}

bool GameHandGrabWorld::IsLoosePot(PotInfo type) const
{
	const auto* info = Info();
	const auto index = static_cast<size_t>(type);
	return info != nullptr && index < info->pot.size() && info->pot[index].potType == PotType::Pot;
}

bool GameHandGrabWorld::IsOfRockMaterial(entt::entity object) const
{
	const auto* info = Info();
	if (info == nullptr)
	{
		return false;
	}
	return physics_classes::Classify(Locator::entitiesRegistry::value(), object, *info, {}).row == physics::MaterialRow::Rock;
}

bool GameHandGrabWorld::IsSexuallyActive(entt::entity villager) const
{
	const auto* data = Locator::entitiesRegistry::value().TryGet<const Villager>(villager);
	const auto* info = Info();
	if (data == nullptr || info == nullptr)
	{
		return false;
	}
	const auto& kind = info->villager.at(static_cast<size_t>(GVillagerInfo::Find(data->tribe, data->number)));
	return kind.startHavingSexAge <= data->age && data->age < kind.stopHavingSexAge;
}

std::optional<PlayerNames> GameHandGrabWorld::PlayerOf(entt::entity object) const
{
	return world_objects::PlayerOf(object);
}

void GameHandGrabWorld::LeavePhysicsAndMap(entt::entity object)
{
	if (Locator::dynamicsSystem::has_value())
	{
		Locator::dynamicsSystem::value().RemoveObject(object, false, false);
	}
	Locator::entitiesRegistry::value().Remove<MapCellResident, MapCellMover, InPhysics>(object);
}

void GameHandGrabWorld::CreateReaction(const ReactionRequest& request)
{
	if (Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().Create(
		    {.initiator = request.initiator, .type = request.type, .player = request.player, .position = request.position});
	}
}

void GameHandGrabWorld::RemoveReactions(entt::entity initiator, Reaction type)
{
	if (Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().RemoveFrom(initiator, type);
	}
}

void GameHandGrabWorld::FireStartedMoving(entt::entity object, bool inHand)
{
	if (Locator::fireSystem::has_value())
	{
		Locator::fireSystem::value().StartedMoving(object, inHand);
	}
}

void GameHandGrabWorld::HeatHeld(entt::entity object)
{
	if (Locator::fireSystem::has_value())
	{
		Locator::fireSystem::value().HeatHeldObject(object);
	}
}

void GameHandGrabWorld::PlaySample(uint32_t sample, glm::vec3 position)
{
	if (Locator::audio::has_value())
	{
		Locator::audio::value().PlaySoundEffect(entt::hashed_string(fmt::format("InGame.sad/{}", sample).c_str()).value(),
		                                        position);
	}
}

uint32_t GameHandGrabWorld::LocalRandom(uint32_t count)
{
	if (count == 0 || !Locator::gameRandom::has_value())
	{
		return 0;
	}
	return Locator::gameRandom::value().LocalRand(static_cast<int32_t>(count)) % count;
}

void GameHandGrabWorld::VillagerIntoHand(entt::entity villager)
{
	auto* action = Locator::entitiesRegistry::value().TryGet<LivingAction>(villager);
	if (action == nullptr || !Locator::livingActionSystem::has_value())
	{
		return;
	}
	villager_memory::StorePreviousState(*action);
	Locator::livingActionSystem::value().VillagerSetState(*action, LivingAction::Index::Top, VillagerStates::InHand, false);
}

void GameHandGrabWorld::AnimalIntoOwnFlock(entt::entity animal)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* data = registry.TryGet<Animal>(animal);
	if (data == nullptr || !Locator::animalSystem::has_value())
	{
		return;
	}
	auto* flock = registry.TryGet<Flock>(data->flock);
	if (flock == nullptr)
	{
		return;
	}
	// It goes into a flock of its own, as wide as the one it left, which goes once it has no one left
	// TODO(hand): a flock a script holds stays even when it is left empty (openblack's scripts hold no flocks)
	const auto old = data->flock;
	std::erase(flock->members, animal);
	const auto& position = registry.Get<const Transform>(animal).position;
	const auto own = Locator::animalSystem::value().CreateFlock(glm::vec2(position.x, position.z), flock->domainRadius,
	                                                            flock->flockDistance);
	registry.Get<Flock>(own).members.push_back(animal);
	data->flock = own;
	if (registry.Get<const Flock>(old).members.empty())
	{
		registry.Destroy(old);
	}
}

void GameHandGrabWorld::TreeUprooted(PlayerNames player, entt::entity /*tree*/)
{
	// Pulling up a tree is bad, as planting one is good
	if (player == PlayerNames::NEUTRAL || !Locator::alignmentSystem::has_value() || Info() == nullptr)
	{
		return;
	}
	auto& alignment = Locator::alignmentSystem::value();
	alignment.AddPendingAlignment(
	    player, magic::DampAlignmentChange(-Info()->player.treePullPutAlignmentChange, alignment.GetPlayerAlignment(player)));
}

void GameHandGrabWorld::LeaveRootsHole(entt::entity tree)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* transform = registry.TryGet<const Transform>(tree);
	const auto* mesh = MeshOf(registry, tree);
	if (transform == nullptr || mesh == nullptr || !Locator::explosionSystem::has_value())
	{
		return;
	}
	// The hole is as wide as the tree's model is wide and deep, a share of it, and turned as the tree was
	const auto half = mesh->GetBoundingBox().Size() * 0.5f;
	const float scale = (half.x + half.z) * transform->scale.x * k_RootsHoleShare;
	const float yaw = std::atan2(-transform->rotation[0].z, transform->rotation[0].x);
	Locator::explosionSystem::value().AddRubble(transform->position, yaw, scale);
}

void GameHandGrabWorld::RemovePotReaction(entt::entity pot)
{
	const auto* data = Locator::entitiesRegistry::value().TryGet<const Pot>(pot);
	const auto* info = Info();
	if (data == nullptr || info == nullptr || !Locator::reactionSystem::has_value())
	{
		return;
	}
	const auto reaction = info->pot.at(static_cast<size_t>(data->type)).associatedReaction;
	if (reaction != Reaction::None)
	{
		Locator::reactionSystem::value().RemoveFrom(pot, reaction);
	}
}

void GameHandGrabWorld::SetUpPotReaction(entt::entity pot, PlayerNames player)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* data = registry.TryGet<const Pot>(pot);
	const auto* transform = registry.TryGet<const Transform>(pot);
	const auto* info = Info();
	if (data == nullptr || transform == nullptr || info == nullptr || !Locator::reactionSystem::has_value())
	{
		return;
	}
	auto& reactions = Locator::reactionSystem::value();
	const auto reaction = info->pot.at(static_cast<size_t>(data->type)).associatedReaction;
	if (reaction != Reaction::None && !reactions.HasReaction(pot))
	{
		reactions.Create({.initiator = pot, .type = reaction, .player = player, .position = transform->position});
	}
}

hand_grab::HandGrabWorldInterface::Pose GameHandGrabWorld::PoseOf(entt::entity object) const
{
	const auto* transform = Locator::entitiesRegistry::value().TryGet<const Transform>(object);
	if (transform == nullptr)
	{
		return {};
	}
	auto axes = transform->rotation;
	for (glm::length_t column = 0; column < 3; ++column)
	{
		axes[column] *= transform->scale[column];
	}
	return {.axes = axes, .origin = transform->position};
}

void GameHandGrabWorld::SetPose(entt::entity object, const Pose& pose)
{
	auto* transform = Locator::entitiesRegistry::value().TryGet<Transform>(object);
	if (transform == nullptr)
	{
		return;
	}
	for (glm::length_t column = 0; column < 3; ++column)
	{
		const float length = glm::length(pose.axes[column]);
		transform->rotation[column] = length > 0.0f ? pose.axes[column] / length : pose.axes[column];
		transform->scale[column] = length;
	}
	transform->position = pose.origin;
	Locator::entitiesRegistry::value().SetDirty();
}

glm::mat3 GameHandGrabWorld::UnstretchedAxes(entt::entity object, const glm::mat3& axes) const
{
	const auto* transform = Locator::entitiesRegistry::value().TryGet<const Transform>(object);
	// Its own size is the one across, which stretching up never changes
	const float scale = transform != nullptr ? transform->scale.x : 1.0f;
	glm::mat3 result = axes;
	for (glm::length_t column = 0; column < 3; ++column)
	{
		const float length = glm::length(axes[column]);
		result[column] = length > 0.0f ? axes[column] * (scale / length) : axes[column];
	}
	return result;
}

std::optional<hand_grab::HandGrabWorldInterface::Pose> GameHandGrabWorld::ReleasePose(entt::entity object, bool alignToSlope)
{
	if (!Locator::dynamicsSystem::has_value())
	{
		return std::nullopt;
	}
	const auto pose = Locator::dynamicsSystem::value().ReleasePose(object, alignToSlope);
	if (!pose.has_value())
	{
		return std::nullopt;
	}
	// The body's axes are of unit length: at the thing's own size again
	return Pose {.axes = UnstretchedAxes(object, pose->first), .origin = pose->second};
}

void GameHandGrabWorld::PourPot(entt::entity pot, PlayerNames player)
{
	const auto facts = PotFactsOf(pot);
	if (!facts.has_value())
	{
		return;
	}
	// It pours out of the hand where the hand is
	const auto hand = PoseOf(Hand()).origin;
	if (Locator::particleSystem::has_value() && player == HandPlayer())
	{
		// TODO(hand): a poisoned handful pours poisoned food, particles 111 (openblack keeps no poisoned pots)
		const auto particles = facts->resource == ResourceType::Wood ? k_PourWood : k_PourFood;
		Locator::particleSystem::value().Start(particles, hand, 1.0f);
	}
	// What it holds goes to the stores and piles of it about the point, or makes a pile there; in the water it is lost
	PourAt(facts->resource, hand, facts->amount, player);
	UseUp(pot);
}

std::optional<hand_grab::HandGrabWorldInterface::PotFacts> GameHandGrabWorld::PotFactsOf(entt::entity pot) const
{
	const auto* data = Locator::entitiesRegistry::value().TryGet<const Pot>(pot);
	const auto* info = Info();
	if (data == nullptr || info == nullptr)
	{
		return std::nullopt;
	}
	const auto& kind = info->pot.at(static_cast<size_t>(data->type));
	// A pile of food is scooped into a handful of food, a pile of wood into one of wood
	return PotFacts {.potType = kind.potType,
	                 .resource = kind.resourceType,
	                 .handful = kind.resourceType == ResourceType::Wood ? PotInfo::HandWood : PotInfo::HandFood,
	                 .amount = data->amount};
}

hand_grab::ScoopFacts GameHandGrabWorld::ScoopFactsOf(PotInfo handful) const
{
	const auto* info = Info();
	if (info == nullptr)
	{
		return {};
	}
	const auto& kind = info->pot.at(static_cast<size_t>(handful));
	return {.initial = kind.amountPickedUpInitially,
	        .perTurn = kind.amountPickedUpPerTurn,
	        .perTurnEnd = kind.amountPickedUpPerTurnEnd,
	        .maxPickedUp = kind.maxAmountCanBePickedUp,
	        .rampSeconds = kind.multiPickUpRampTime};
}

uint32_t GameHandGrabWorld::TakeFromPile(entt::entity pile, uint32_t amount)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* pot = registry.TryGet<Pot>(pile);
	if (pot == nullptr)
	{
		return 0;
	}
	const auto taken = std::min(amount, pot->amount);
	pot->amount -= taken;
	// A store's pile stays, its store counting what it lost; any other goes once it has nothing left
	entt::entity store = entt::null;
	registry.Each<const StoragePit>([pile, &store](entt::entity pit, const StoragePit& data) {
		if (data.foodPile == pile || std::ranges::find(data.woodPiles, pile) != data.woodPiles.end())
		{
			store = pit;
		}
	});
	if (auto* abode = store != entt::null ? registry.TryGet<Abode>(store) : nullptr)
	{
		const auto facts = PotFactsOf(pile);
		auto& counted = facts.has_value() && facts->resource == ResourceType::Wood ? abode->woodAmount : abode->foodAmount;
		counted -= std::min(counted, taken);
	}
	else if (pot->amount == 0)
	{
		world_objects::Remove(pile);
	}
	return taken;
}

entt::entity GameHandGrabWorld::MakeHandful(PotInfo type, glm::vec3 position, uint32_t amount)
{
	return archetypes::PotArchetype::Create(position, 0.0f, type, static_cast<int32_t>(amount));
}

std::optional<uint32_t> GameHandGrabWorld::StartScoopStream(ResourceType resource, glm::vec3 source)
{
	if (!Locator::particleSystem::has_value())
	{
		return std::nullopt;
	}
	// TODO(hand): poisoned food streams as particles 108 and a fish farm's fish as 109 (openblack has neither)
	return Locator::particleSystem::value().Start(resource == ResourceType::Wood ? k_ScoopWood : k_ScoopFood, source, 1.0f);
}

void GameHandGrabWorld::StopScoopStream(uint32_t stream)
{
	if (Locator::particleSystem::has_value())
	{
		Locator::particleSystem::value().CloseDown(stream);
	}
}

void GameHandGrabWorld::PlayScoopSound(ResourceType resource, glm::vec3 hand, float ramp)
{
	if (!Locator::audio::has_value())
	{
		return;
	}
	// Wood rattles in, anything else pours; its pitch rises as the scoop ramps up
	const uint32_t sample = resource == ResourceType::Wood ? k_ScoopWoodSample : k_ScoopSample;
	const auto pitch = static_cast<uint32_t>(ramp * k_ScoopPitchRise + k_ScoopPitchStart);
	Locator::audio::value().StartSoundEffect(entt::hashed_string(fmt::format("InGame.sad/{}", sample).c_str()).value(),
	                                         {.position = hand, .pitchPercent = pitch});
}

float GameHandGrabWorld::LandHeightAt(glm::vec3 point) const
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt({point.x, point.z}) : 0.0f;
}

bool GameHandGrabWorld::StoresResource(entt::entity store, ResourceType resource) const
{
	// A storage pit stores any resource
	return (resource == ResourceType::Food || resource == ResourceType::Wood) &&
	       Locator::entitiesRegistry::value().AllOf<StoragePit, Abode>(store);
}

uint32_t GameHandGrabWorld::AddToStore(entt::entity store, ResourceType resource, uint32_t amount)
{
	// TODO(hand): the player's creature may copy the giving, and the advisor says the resource was dropped
	return _resources.StoreResource(store, resource, amount);
}

void GameHandGrabWorld::PourAt(ResourceType resource, glm::vec3 point, uint32_t amount, PlayerNames player)
{
	// TODO(hand): the player's creature may copy the giving to each store, and the advisor says the resource was dropped
	_resources.AddResource(resource, point, amount, false, player);
}

void GameHandGrabWorld::UseUp(entt::entity object)
{
	world_objects::LeaveGhost(object);
	world_objects::Remove(object);
}

FromHandResult GameHandGrabWorld::LetGoFromHand(entt::entity object, const FromHand& release)
{
	return Locator::dynamicsSystem::has_value() ? Locator::dynamicsSystem::value().LetGoFromHand(object, release)
	                                            : FromHandResult {};
}

bool GameHandGrabWorld::PlayerHasNoWindResistance(PlayerNames player) const
{
	if (!Locator::playerSystem::has_value())
	{
		return false;
	}
	const auto entity = Locator::playerSystem::value().GetPlayer(player);
	const auto* data = Locator::entitiesRegistry::value().TryGet<const Player>(entity);
	return data != nullptr && data->windResistance != 0;
}

std::optional<hand_grab::HandGrabWorldInterface::BodyFacts> GameHandGrabWorld::BodyOf(entt::entity object) const
{
	const auto* entry = EntryOf(object);
	if (entry == nullptr || entry->body == nullptr)
	{
		return std::nullopt;
	}
	return BodyFacts {.mass = entry->body->Mass(), .speed = glm::length(entry->body->velocity)};
}

void GameHandGrabWorld::TwistBody(entt::entity object, glm::vec3 torque)
{
	if (auto* entry = EntryOf(object); entry != nullptr && entry->body != nullptr)
	{
		entry->body->externalTorque += torque;
	}
}

void GameHandGrabWorld::DropDrag(entt::entity object)
{
	if (auto* entry = EntryOf(object); entry != nullptr && entry->body != nullptr)
	{
		entry->body->SetDrag(0.0f);
	}
}

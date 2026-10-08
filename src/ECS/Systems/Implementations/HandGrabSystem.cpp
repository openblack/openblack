/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "HandGrabSystem.h"

#include <algorithm>
#include <set>

#include <fmt/format.h>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <spdlog/spdlog.h>

#include "3D/AllMeshes.h"
#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/AtHome.h"
#include "ECS/Components/CarriedByTornado.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/MapCellResident.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/StreetLantern.h"
#include "ECS/Components/TeleportStone.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AnimalSystemInterface.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
using hand_grab::GrabKind;

namespace
{
/// How far along the line through the cursor the hand looks for things to take
constexpr float k_SearchLength = 2000.0f;
/// The line is walked in steps of this many metres, looking in the map's cells along it
constexpr float k_SearchStep = 5.0f;
/// Only where the line runs this low over the land can it meet something standing on it
constexpr float k_SearchHeight = 80.0f;

/// The hand cries of the people, as many of each as there are: a child's, a woman's and a man's
constexpr uint32_t k_ChildCries = 180;
constexpr uint32_t k_WomanCries = 194;
constexpr uint32_t k_ManCries = 187;
constexpr uint32_t k_CriesEach = 7;
/// A tree pulled up creaks, one of three
constexpr uint32_t k_TreeCreaks = 32;
constexpr uint32_t k_TreeCreaksEach = 3;

Registry& Entities()
{
	return Locator::entitiesRegistry::value();
}

bool IsAvailable(entt::entity object)
{
	return object != entt::null && Entities().Valid(object);
}

entt::entity PlayerHand()
{
	if (!Locator::handSystem::has_value())
	{
		return entt::null;
	}
	return Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(HandSystemInterface::Side::Left)];
}

/// A sound of the in-game bank where a thing is
void PlaySample(uint32_t sample, glm::vec3 position)
{
	if (!Locator::audio::has_value())
	{
		return;
	}
	Locator::audio::value().PlaySoundEffect(entt::hashed_string(fmt::format("InGame.sad/{}", sample).c_str()).value(),
	                                        position);
}

/// One of a run of samples, at random
uint32_t RandomSample(uint32_t first, uint32_t count)
{
	return first + (Locator::gameRandom::has_value()
	                    ? Locator::gameRandom::value().LocalRand(static_cast<int32_t>(count)) % count
	                    : 0u);
}

/// The model a thing is drawn with, none without one
const graphics::L3DMesh* MeshOf(entt::entity object)
{
	const auto* mesh = Entities().TryGet<const Mesh>(object);
	if (mesh == nullptr || !Locator::resources::has_value())
	{
		return nullptr;
	}
	auto& meshes = Locator::resources::value().GetMeshes();
	return meshes.Contains(mesh->id) ? &*meshes.Handle(mesh->id) : nullptr;
}

/// Where a line first meets a thing's drawn model, as a distance along the line; none when it misses it
std::optional<float> HitModel(const graphics::L3DMesh& mesh, const glm::mat3& axes, glm::vec3 origin, glm::vec3 rayOrigin,
                              glm::vec3 rayDirection)
{
	// The line in the model's own space, whose distances along it are the world's
	if (glm::determinant(axes) == 0.0f)
	{
		return std::nullopt;
	}
	const auto inverse = glm::inverse(axes);
	const auto localOrigin = inverse * (rayOrigin - origin);
	const auto localDirection = inverse * rayDirection;
	// First its box
	const auto box = mesh.GetBoundingBox();
	float enter = 0.0f;
	float leave = k_SearchLength * 2.0f;
	for (glm::length_t axis = 0; axis < 3; ++axis)
	{
		if (localDirection[axis] == 0.0f)
		{
			if (localOrigin[axis] < box.minima[axis] || localOrigin[axis] > box.maxima[axis])
			{
				return std::nullopt;
			}
			continue;
		}
		float near = (box.minima[axis] - localOrigin[axis]) / localDirection[axis];
		float far = (box.maxima[axis] - localOrigin[axis]) / localDirection[axis];
		if (near > far)
		{
			std::swap(near, far);
		}
		enter = std::max(enter, near);
		leave = std::min(leave, far);
		if (enter > leave)
		{
			return std::nullopt;
		}
	}
	// A model moved by bones keeps its points about its bones, so its box stands for it
	// TODO(hand): test the posed triangles of boned models (people and animals) as they are drawn
	if (mesh.IsBoned())
	{
		return enter;
	}
	// Then the triangles it is drawn with nearest
	std::optional<float> nearest;
	for (const auto& subMesh : mesh.GetSubMeshes())
	{
		if (subMesh->IsPhysics() || (subMesh->GetFlags().lodMask & 1u) == 0)
		{
			continue;
		}
		const auto& surface = subMesh->GetSurface();
		for (size_t i = 0; i + 2 < surface.indices.size(); i += 3)
		{
			const auto hit =
			    hand_grab::RayTriangle(localOrigin, localDirection, surface.positions[surface.indices[i]],
			                           surface.positions[surface.indices[i + 1]], surface.positions[surface.indices[i + 2]]);
			if (hit.has_value() && (!nearest.has_value() || *hit < *nearest))
			{
				nearest = hit;
			}
		}
	}
	return nearest;
}

/// The statics with kinds of their own the hand never takes: lanterns, bonfires, shields, vortices and teleport stones
bool StaticOfItsOwn(MobileStaticInfo type)
{
	switch (type)
	{
	case MobileStaticInfo::StreetLantern:
	case MobileStaticInfo::CountryLantern:
	case MobileStaticInfo::Bonfire:
	case MobileStaticInfo::PhysicalShield:
	case MobileStaticInfo::Vortex:
	case MobileStaticInfo::Teleport:
		return true;
	default:
		return false;
	}
}

MobileStaticInfo StaticRowType(MobileStaticInfo type)
{
	if (!Locator::infoConstants::has_value())
	{
		return MobileStaticInfo::None;
	}
	const auto& rows = Locator::infoConstants::value().mobileStatic;
	const auto index = static_cast<size_t>(type);
	return index < rows.size() ? rows[index].mobileType : MobileStaticInfo::None;
}

MeshId StaticMesh(MobileStaticInfo type)
{
	if (!Locator::infoConstants::has_value())
	{
		return MeshId::Dummy;
	}
	const auto& rows = Locator::infoConstants::value().mobileStatic;
	const auto index = static_cast<size_t>(type);
	return index < rows.size() ? rows[index].meshId : MeshId::Dummy;
}
} // namespace

HandGrab* HandGrabSystem::Grab()
{
	const auto hand = PlayerHand();
	if (!IsAvailable(hand))
	{
		return nullptr;
	}
	auto& registry = Entities();
	if (auto* grab = registry.TryGet<HandGrab>(hand))
	{
		return grab;
	}
	return &registry.Assign<HandGrab>(hand);
}

const HandGrab* HandGrabSystem::Grab() const
{
	const auto hand = PlayerHand();
	return IsAvailable(hand) ? Entities().TryGet<const HandGrab>(hand) : nullptr;
}

GrabKind HandGrabSystem::KindOf(entt::entity object) const
{
	const auto& registry = Entities();
	if (registry.AllOf<Creature>(object))
	{
		return GrabKind::None;
	}
	if (registry.AllOf<Villager>(object))
	{
		return GrabKind::Villager;
	}
	if (registry.AllOf<Animal>(object))
	{
		return GrabKind::Animal;
	}
	if (registry.AllOf<Tree>(object))
	{
		return GrabKind::Tree;
	}
	if (registry.AllOf<DeadTree>(object))
	{
		return GrabKind::DeadTree;
	}
	// A one-shot miracle's bubble is the magic's to take
	if (registry.AnyOf<OneOffSpellSeed, StreetLantern, TeleportStone>(object))
	{
		return GrabKind::None;
	}
	if (const auto* pot = registry.TryGet<const Pot>(object))
	{
		// Piles are scooped from, not picked up
		if (!Locator::infoConstants::has_value())
		{
			return GrabKind::None;
		}
		const auto& rows = Locator::infoConstants::value().pot;
		const auto index = static_cast<size_t>(pot->type);
		return index < rows.size() && rows[index].potType == PotType::Pot ? GrabKind::MobileObject : GrabKind::None;
	}
	if (const auto* still = registry.TryGet<const MobileStatic>(object))
	{
		if (StaticOfItsOwn(still->type))
		{
			return GrabKind::None;
		}
		return StaticRowType(still->type) == MobileStaticInfo::Rock ? GrabKind::Rock : GrabKind::MobileStatic;
	}
	if (const auto* mobile = registry.TryGet<const MobileObject>(object))
	{
		return mobile->type == MobileObjectInfo::LumpOfPoo ? GrabKind::Poo : GrabKind::MobileObject;
	}
	return GrabKind::None;
}

hand_grab::HoldFacts HandGrabSystem::HoldOfObject(entt::entity object) const
{
	const auto& registry = Entities();
	const auto size = world_objects::SizeOf(object);
	const auto* still = registry.TryGet<const MobileStatic>(object);
	const auto type = still != nullptr ? still->type : MobileStaticInfo::None;
	return hand_grab::HoldOf(KindOf(object), type, still != nullptr ? StaticMesh(type) : MeshId::Dummy, size.height,
	                         size.radius);
}

std::optional<entt::entity> HandGrabSystem::ObjectAlong(glm::vec3 origin, glm::vec3 direction) const
{
	if (!Locator::entitiesMap::has_value() || glm::length(direction) == 0.0f)
	{
		return std::nullopt;
	}
	direction = glm::normalize(direction);
	auto& registry = Entities();
	const auto& map = Locator::entitiesMap::value();

	// What stands in the map's cells along the line where it runs low over the land, and what flies
	std::set<entt::entity> candidates;
	std::set<std::pair<int, int>> looked;
	const float landLimit = Locator::terrainSystem::has_value() ? 0.0f : -1.0f;
	for (float t = 0.0f; t <= k_SearchLength; t += k_SearchStep)
	{
		const auto point = origin + direction * t;
		if (landLimit >= 0.0f && point.y - Locator::terrainSystem::value().GetHeightAt({point.x, point.z}) > k_SearchHeight)
		{
			continue;
		}
		const auto cell = map_coords::CellOf(glm::vec2(point.x, point.z));
		for (int dx = -1; dx <= 1; ++dx)
		{
			for (int dz = -1; dz <= 1; ++dz)
			{
				const glm::ivec2 near(static_cast<int>(cell.x) + dx, static_cast<int>(cell.y) + dz);
				if (!looked.emplace(near.x, near.y).second)
				{
					continue;
				}
				for (const auto entity : map.GetAllInCell(near))
				{
					candidates.insert(entity);
				}
			}
		}
		if (Locator::terrainSystem::has_value() && point.y < Locator::terrainSystem::value().GetHeightAt({point.x, point.z}))
		{
			// The line has gone into the land: nothing beyond can be seen
			break;
		}
	}
	if (Locator::dynamicsSystem::has_value())
	{
		Locator::dynamicsSystem::value().ForEachEntry([&candidates](const PhysicsEntry& entry) {
			if (entry.IsFlying())
			{
				candidates.insert(entry.entity);
			}
		});
	}

	std::optional<entt::entity> nearest;
	float best = k_SearchLength;
	for (const auto entity : candidates)
	{
		if (!IsAvailable(entity) || registry.AnyOf<InHand, Creature>(entity))
		{
			continue;
		}
		const auto* mesh = MeshOf(entity);
		const auto* transform = registry.TryGet<const Transform>(entity);
		if (mesh == nullptr || transform == nullptr)
		{
			continue;
		}
		// A thing in flight is where it is drawn
		auto axes = transform->rotation * glm::mat3(glm::scale(glm::mat4(1.0f), transform->scale));
		auto position = transform->position;
		if (const auto* drawn = registry.TryGet<const PhysicsDrawPose>(entity))
		{
			axes = drawn->axes;
			position = drawn->origin;
		}
		const auto hit = HitModel(*mesh, axes, position, origin, direction);
		if (hit.has_value() && *hit < best)
		{
			best = *hit;
			nearest = entity;
		}
	}
	return nearest;
}

bool HandGrabSystem::Press(glm::vec3 rayOrigin, glm::vec3 rayDirection, uint32_t nowMs, uint32_t turn)
{
	auto* grab = Grab();
	if (grab == nullptr)
	{
		return false;
	}
	switch (grab->state)
	{
	case HandGrab::State::Holding:
		// Ready to throw: the spring takes hold of the hand where it is, at rest
		grab->state = HandGrab::State::ReadyToThrow;
		grab->spring.Start(grab->lastTarget);
		return true;
	case HandGrab::State::Grabbing:
	case HandGrab::State::ReadyToThrow:
		return true;
	case HandGrab::State::Empty:
		break;
	}

	const auto object = ObjectAlong(rayOrigin, rayDirection);
	if (!object.has_value())
	{
		return false;
	}
	auto& registry = Entities();
	const auto kind = KindOf(*object);
	const auto* action = registry.TryGet<const LivingAction>(*object);
	const bool hiding = action != nullptr && action->states[static_cast<size_t>(LivingAction::Index::Top)] ==
	                                             static_cast<uint8_t>(VillagerStates::GoAndHideInNearbyBuilding);
	const bool speciesAllows = kind == GrabKind::Animal && Locator::animalSystem::has_value() &&
	                           Locator::animalSystem::value().CanPlayerPickUp(*object);
	const hand_grab::Holdable holdable {
	    .kind = kind,
	    .available = IsAvailable(*object),
	    .atHome = registry.AllOf<AtHome>(*object),
	    .inHand = registry.AllOf<InHand>(*object),
	    .hiding = hiding,
	    .speciesAllows = speciesAllows,
	    .radius = world_objects::SizeOf(*object).radius,
	    .isRock = kind == GrabKind::Rock,
	};
	const bool inInfluence = !Locator::magicSystem::has_value() || Locator::magicSystem::value().IsHandInInfluence();
	if (!hand_grab::PassesGate({.spaceInHand = true,
	                            .alreadyInHand = registry.AllOf<InHand>(*object),
	                            .valid = hand_grab::ValidForPlaceInHand(holdable),
	                            .cannotBePickedUp = false,
	                            .carried = registry.AllOf<CarriedByTornado>(*object),
	                            .inInfluence = inInfluence}))
	{
		// TODO(hand): a press the hand can't take is a tap on the thing (clicking and activating)
		return false;
	}
	grab->state = HandGrab::State::Grabbing;
	grab->object = *object;
	grab->pressMs = nowMs;
	grab->pressTurn = turn;
	// Something in flight is caught only once the button has been held a while; anything else is pulled at once
	grab->waits = Locator::dynamicsSystem::has_value() && Locator::dynamicsSystem::value().IsFlying(*object);
	grab->pullSeconds = 0.0f;
	return true;
}

std::optional<entt::entity> HandGrabSystem::Release(uint32_t nowMs, uint32_t turn)
{
	auto* grab = Grab();
	if (grab == nullptr)
	{
		return std::nullopt;
	}
	switch (grab->state)
	{
	case HandGrab::State::Grabbing:
	{
		// Let go before the thing came free: it stays where it is, and a short press was a tap
		const auto object = grab->object;
		const bool tap = hand_grab::ElapsedMs(nowMs, grab->pressMs, turn, grab->pressTurn) <= hand_grab::k_GrabWaitMs;
		Empty(*grab);
		return tap && IsAvailable(object) ? std::optional(object) : std::nullopt;
	}
	case HandGrab::State::ReadyToThrow:
	{
		// Let go out of the map or out of the player's influence, the hand keeps hold
		const bool inInfluence = !Locator::magicSystem::has_value() || Locator::magicSystem::value().IsHandInInfluence();
		if (!map_coords::InBounds(grab->lastTarget) || !inInfluence)
		{
			grab->state = HandGrab::State::Holding;
			return std::nullopt;
		}
		LetGo(*grab, grab->spring.Velocity(), false);
		return std::nullopt;
	}
	case HandGrab::State::Holding:
	case HandGrab::State::Empty:
		break;
	}
	return std::nullopt;
}

void HandGrabSystem::Take(HandGrab& grab, entt::entity object)
{
	auto& registry = Entities();
	auto position = registry.Get<const Transform>(object).position;
	const bool wasFlying = Locator::dynamicsSystem::has_value() && Locator::dynamicsSystem::value().IsFlying(object);
	const auto kind = KindOf(object);

	// Out of the physics without landing, and out of the map's cells
	if (Locator::dynamicsSystem::has_value())
	{
		Locator::dynamicsSystem::value().RemoveObject(object, false, false);
		position = registry.Get<const Transform>(object).position;
	}
	registry.Remove<MapCellResident, MapCellMover, InPhysics>(object);
	if (Locator::reactionSystem::has_value())
	{
		auto& reactions = Locator::reactionSystem::value();
		reactions.RemoveFrom(object, Reaction::ReactToFlyingObject);
		// The people and animals about the hand see it pick something up
		reactions.Create({.initiator = PlayerHand(),
		                  .type = Reaction::ReactToHandPickUp,
		                  .player = PlayerNames::PLAYER_ONE,
		                  .position = position});
	}
	// A burning thing keeps burning in the hand, out of its blaze; the people round it flee one that isn't a villager
	if (Locator::fireSystem::has_value())
	{
		Locator::fireSystem::value().StartedMoving(object, !registry.AllOf<Villager>(object));
	}
	// TODO(hand): a firefly sitting on what is picked up is taken off it for a reward (no fireflies yet)

	// Its pick-up sound: a tree pulled from the ground creaks instead, and people cry out
	if (!(kind == GrabKind::Tree || kind == GrabKind::DeadTree) || wasFlying)
	{
		PlaySample(static_cast<uint32_t>(10), position);
	}
	if (const auto* villager = registry.TryGet<const Villager>(object);
	    villager != nullptr && world_objects::LifeOf(object) > 0.0f)
	{
		const uint32_t first = villager->lifeStage == Villager::LifeStage::Child ? k_ChildCries
		                       : villager->sex == Villager::Sex::FEMALE          ? k_WomanCries
		                                                                         : k_ManCries;
		PlaySample(RandomSample(first, k_CriesEach), position);
	}

	// The thing's own part of going into the hand
	if (auto* action = registry.TryGet<LivingAction>(object); action != nullptr && registry.AllOf<Villager>(object))
	{
		// TODO(hand): a villager in the middle of making love lets its partner go, and an adult of the player's own tribe
		// in the hand alarms those about it
		if (Locator::livingActionSystem::has_value())
		{
			auto& living = Locator::livingActionSystem::value();
			const auto top = living.VillagerGetState(*action, LivingAction::Index::Top);
			if (top != VillagerStates::InHand)
			{
				living.VillagerSetState(*action, LivingAction::Index::Previous, top, true);
			}
			living.VillagerSetState(*action, LivingAction::Index::Top, VillagerStates::InHand, false);
		}
	}
	if (auto* animal = registry.TryGet<Animal>(object); animal != nullptr && Locator::animalSystem::has_value())
	{
		// An animal taken from its flock goes into a flock of its own, as wide as the one it left
		if (auto* flock = registry.TryGet<Flock>(animal->flock))
		{
			std::erase(flock->members, object);
			const auto own = Locator::animalSystem::value().CreateFlock(glm::vec2(position.x, position.z), flock->domainRadius,
			                                                            flock->flockDistance);
			registry.Get<Flock>(own).members.push_back(object);
			animal->flock = own;
		}
	}
	if (kind == GrabKind::Tree && !wasFlying)
	{
		PlaySample(RandomSample(k_TreeCreaks, k_TreeCreaksEach), position);
		// TODO(hand): pulling up a tree counts towards the player's alignment
	}

	registry.AssignOrReplace<InHand>(object, InHand {.hand = PlayerHand()});
	grab.state = HandGrab::State::Holding;
	grab.object = object;
	grab.hold = HoldOfObject(object);
	const auto size = world_objects::SizeOf(object);
	grab.pickUpLowering = hand_grab::PickUpLowering(grab.hold.loweringMultiplier, size.height, _handSize);
	grab.lastPickedUp = object;
	registry.SetDirty();
}

void HandGrabSystem::LetGo(HandGrab& grab, glm::vec3 velocity, bool forced)
{
	auto& registry = Entities();
	const auto object = grab.object;
	Empty(grab);
	if (!IsAvailable(object))
	{
		return;
	}
	registry.Remove<InHand>(object);
	if (!Locator::dynamicsSystem::has_value())
	{
		return;
	}
	auto& dynamics = Locator::dynamicsSystem::value();
	if (!forced)
	{
		// It starts from where it is drawn in the hand, lifted out of the land, and laid along the slope when let go
		// slowly, unless it is a tree
		const bool anyTree = registry.AnyOf<Tree, DeadTree>(object);
		if (const auto pose = dynamics.ReleasePose(object, !(anyTree || hand_grab::IsFastRelease(velocity))))
		{
			auto& transform = registry.Get<Transform>(object);
			auto axes = pose->first;
			for (glm::length_t column = 0; column < 3; ++column)
			{
				const float length = glm::length(axes[column]);
				axes[column] = length > 0.0f ? axes[column] / length : axes[column];
			}
			transform.rotation = axes;
			transform.position = pose->second;
		}
	}
	// TODO(hand): a pot let go slowly is poured out where it is (scooping), and the held thing applied to what is under
	// the hand (a store, a building site, an altar) before it is thrown
	const auto result = dynamics.InitialisePhysicsFromHand(object, {
	                                                                   .velocity = velocity,
	                                                                   .player = PlayerNames::PLAYER_ONE,
	                                                                   .dontReplant = forced,
	                                                               });
	grab.lastDropped = object;
	if (result.entry != nullptr && !result.landed)
	{
		// The hand gives it a twist a little after it lets go
		grab.released = object;
		grab.releaseSpinMs = static_cast<int32_t>(hand_grab::k_ReleaseSpinDelayMs);
	}
	registry.SetDirty();
}

void HandGrabSystem::Empty(HandGrab& grab)
{
	grab.state = HandGrab::State::Empty;
	grab.object = entt::null;
	grab.waits = false;
	grab.pullSeconds = 0.0f;
	grab.rise = 0.0f;
}

glm::vec3 HandGrabSystem::UpdateFrame(const Frame& frame)
{
	_handSize = frame.handSize;
	_cursorGround = frame.cursorGround;
	auto* grab = Grab();
	if (grab == nullptr)
	{
		return frame.target;
	}

	// The twist of a throw, once the hand has moved on a little
	if (grab->releaseSpinMs.has_value())
	{
		*grab->releaseSpinMs -= static_cast<int32_t>(frame.gameMs);
		if (*grab->releaseSpinMs < 0)
		{
			grab->releaseSpinMs.reset();
			auto* entry =
			    Locator::dynamicsSystem::has_value() ? Locator::dynamicsSystem::value().Find(grab->released) : nullptr;
			if (entry != nullptr && entry->body != nullptr && frame.cursorGround.has_value())
			{
				const auto moved = *frame.cursorGround - grab->lastTarget;
				entry->body->externalTorque +=
				    hand_grab::ReleaseSpinTorque(entry->body->Mass(), glm::length(entry->body->velocity), moved);
			}
			grab->released = entt::null;
		}
	}

	switch (grab->state)
	{
	case HandGrab::State::Empty:
		grab->rise = 0.0f;
		return frame.target;
	case HandGrab::State::Grabbing:
	{
		if (!IsAvailable(grab->object))
		{
			Empty(*grab);
			return frame.target;
		}
		if (grab->waits)
		{
			if (hand_grab::ElapsedMs(frame.nowMs, grab->pressMs, frame.turn, grab->pressTurn) >= hand_grab::k_GrabWaitMs)
			{
				Take(*grab, grab->object);
			}
		}
		else
		{
			// The pull starts once the hand has faded into its pulling pose; anything but a tree comes free at once
			// TODO(hand): a tree is pulled at, leaning and stretching, until it comes free
			grab->pullSeconds += frame.seconds;
			if (grab->pullSeconds >= hand_grab::k_PullBlendSeconds)
			{
				Take(*grab, grab->object);
			}
		}
		return frame.target;
	}
	case HandGrab::State::Holding:
	case HandGrab::State::ReadyToThrow:
		break;
	}

	auto& registry = Entities();
	const auto* tree = registry.TryGet<const Tree>(grab->object);
	const std::optional<float> rooted =
	    tree != nullptr ? std::optional(world_objects::SizeOf(grab->object).height) : std::nullopt;
	grab->rise = hand_grab::HandRise(grab->hold.type, grab->pickUpLowering, frame.handSize, rooted);
	const auto target = frame.target + glm::vec3(0.0f, grab->rise, 0.0f);
	grab->lastTarget = target;
	if (grab->state == HandGrab::State::ReadyToThrow)
	{
		grab->spring.Step(target, frame.gameMs);
		return grab->spring.Position();
	}
	return target;
}

void HandGrabSystem::ProcessTurn()
{
	auto* grab = Grab();
	if (grab == nullptr)
	{
		return;
	}
	if (!IsAvailable(grab->lastPickedUp))
	{
		grab->lastPickedUp = entt::null;
	}
	if (!IsAvailable(grab->lastDropped))
	{
		grab->lastDropped = entt::null;
	}
	if (grab->state != HandGrab::State::Holding && grab->state != HandGrab::State::ReadyToThrow)
	{
		return;
	}
	// What is no longer there leaves the hand
	if (!IsAvailable(grab->object))
	{
		Empty(*grab);
		return;
	}
	// The hold is asked again, as what is held may have changed
	grab->hold = HoldOfObject(grab->object);
	// What is held over a fire catches from it
	if (Locator::fireSystem::has_value())
	{
		Locator::fireSystem::value().HeatHeldObject(grab->object);
	}
}

void HandGrabSystem::ForceDrop()
{
	auto* grab = Grab();
	if (grab == nullptr || (grab->state != HandGrab::State::Holding && grab->state != HandGrab::State::ReadyToThrow))
	{
		return;
	}
	// Put down from where it is held, with no speed, and never planted again
	LetGo(*grab, glm::vec3(0.0f), true);
}

void HandGrabSystem::Reset()
{
	if (auto* grab = Grab())
	{
		*grab = HandGrab {};
	}
	_cursorGround.reset();
}

std::optional<entt::entity> HandGrabSystem::GetHeld() const
{
	const auto* grab = Grab();
	if (grab == nullptr || (grab->state != HandGrab::State::Holding && grab->state != HandGrab::State::ReadyToThrow))
	{
		return std::nullopt;
	}
	return grab->object;
}

bool HandGrabSystem::IsBusy() const
{
	const auto* grab = Grab();
	return grab != nullptr && grab->state != HandGrab::State::Empty;
}

std::optional<HandGrabSystemInterface::HeldPose> HandGrabSystem::GetHeldPose() const
{
	const auto held = GetHeld();
	if (!held.has_value() || !IsAvailable(*held))
	{
		return std::nullopt;
	}
	const auto& grab = *Grab();
	// It hangs its height times its lowering below the hand
	return HeldPose {
	    .object = *held,
	    .hold = grab.hold.type,
	    .hang = grab.hold.loweringMultiplier * world_objects::SizeOf(*held).height,
	    .reach = grab.hold.holdRadius,
	};
}

float HandGrabSystem::GetCursorRaise() const
{
	const auto* grab = Grab();
	return grab != nullptr ? hand_grab::CursorRaise(grab->rise) : 0.0f;
}

/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CreatureObjectActionSystem.h"

#include <cmath>

#include <algorithm>
#include <array>
#include <numbers>
#include <span>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "Creature/CreatureAnimation.h"
#include "Creature/CreatureCatch.h"
#include "Creature/CreatureLayers.h"
#include "Creature/CreatureObjectActions.h"
#include "Creature/CreatureReach.h"
#include "Creature/CreatureRig.h"
#include "Creature/CreatureRoute.h"
#include "Creature/CreatureThrow.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/Ball.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/CreatureObjectAction.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/WallHug.h"
#include "ECS/ObjectPhysics.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AnimalSystemInterface.h"
#include "ECS/Systems/BuildingDamageSystemInterface.h"
#include "ECS/Systems/CreatureAnimationSystemInterface.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/CreaturePhysiologySystemInterface.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"
#include "VillagerPhysics.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;
using openblack::creature::CreatureRig;
using openblack::creature_object_actions::Kind;
using openblack::creature_object_actions::Status;
using Phase = openblack::ecs::components::CreatureObjectAction::Phase;

namespace
{
constexpr float k_TurnSeconds = std::chrono::duration<float>(TimeSystemInterface::k_TurnDuration).count();
/// The meshes look back along their z axis: ahead is -z
constexpr glm::vec3 k_MeshBack {0.0f, 0.0f, 1.0f};
/// It walks or turns this many times to get something in reach before giving up
constexpr uint32_t k_ReachAttempts = 4;
/// Turning to face what it throws at or points at, it tries this many times
constexpr uint32_t k_TurnAttempts = 2;
/// Walking up to something it walks to within this share of the distance to the middle of its reach, and stops no
/// nearer than this share of it
constexpr float k_ApproachFar = 1.0f;
constexpr float k_ApproachNear = 0.6f;
/// Near enough to reach once turned: within this share of the distance to the middle of its reach, either way
constexpr float k_TurnInReachShare = 0.5f;
/// Pointing, the high animation counts fully this far above level, in radians
constexpr float k_PointHighRadians = std::numbers::pi_v<float> / 4.0f;
/// Pointing, the hand is about this share of the creature's height up
constexpr float k_PointFromHeightShare = 0.6f;
/// Nothing smaller than this counts as a distance
constexpr float k_Tiny = 1e-4f;

const CreatureRig* RigOf(const Creature& creature)
{
	const auto& rigs = Locator::resources::value().GetCreatureRigs();
	const auto id = creature::GetRigId(creature.species);
	return rigs.Contains(id) ? &*rigs.Handle(id) : nullptr;
}

const CreatureRig::ActionPoints* PointsOf(const Creature& creature)
{
	const auto* rig = RigOf(creature);
	return rig != nullptr && rig->actionPoints.has_value() ? &*rig->actionPoints : nullptr;
}

glm::mat4 PlacementOf(const Transform& transform)
{
	return creature::PlacementMatrix(transform.position, transform.rotation, transform.scale);
}

/// A point of the world in the creature's own space, kept at the world's scale
glm::vec3 ToLocal(const Transform& transform, const glm::vec3& world)
{
	return glm::transpose(transform.rotation) * (world - transform.position);
}

/// How far round from ahead a point is, along the ground
float BearingTo(const Transform& transform, const glm::vec3& target)
{
	const auto local = ToLocal(transform, target);
	const glm::vec2 ahead {0.0f, -1.0f};
	const glm::vec2 towards {local.x, local.z};
	if (glm::length(towards) < k_Tiny)
	{
		return 0.0f;
	}
	return std::acos(std::clamp(glm::dot(ahead, glm::normalize(towards)), -1.0f, 1.0f));
}

/// The hand a creature acts with: the right, or its mirror
uint32_t HandBone(const CreatureRig::ActionPoints& points, const CreatureAnimation& animation, bool mirrored)
{
	return mirrored && points.rightHand < animation.mirror.size() ? animation.mirror[points.rightHand] : points.rightHand;
}

/// Where the palm is: among the hand bone and the fingers that hang from it, as the body is posed this frame
glm::vec3 PalmOf(uint32_t hand, const CreatureAnimation& animation, const glm::mat4& placement)
{
	auto sum = glm::vec3(creature::PosedBone(hand, animation.boneMatrices, placement)[3]);
	float count = 1.0f;
	const auto& parents = animation.skeleton.parents;
	for (uint32_t bone = 0; bone < parents.size(); ++bone)
	{
		if (parents[bone] == hand)
		{
			sum += glm::vec3(creature::PosedBone(bone, animation.boneMatrices, placement)[3]);
			count += 1.0f;
		}
	}
	return sum / count;
}

/// Where the middle of something is from where it stands, by its mesh's bounds, in its own space at the world's scale
glm::vec3 MiddleOf(const ecs::Registry& registry, entt::entity entity)
{
	const auto* mesh = registry.TryGet<const Mesh>(entity);
	const auto* transform = registry.TryGet<const Transform>(entity);
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (mesh == nullptr || transform == nullptr || !meshes.Contains(mesh->id))
	{
		return glm::vec3(0.0f);
	}
	return meshes.Handle(mesh->id)->GetBoundingBox().Center() * transform->scale;
}

/// Where the right hand is in an animation at a time, in the creature's own space at the world's scale
std::optional<glm::vec3> HandAt(entt::entity creature, const Transform& transform, const CreatureRig::ActionPoints& points,
                                size_t animation, float timeMs)
{
	const auto bone =
	    Locator::creatureAnimationSystem::value().BoneInAnimation(creature, animation, timeMs, points.rightHand, false);
	return bone.has_value() ? std::optional(*bone * transform.scale) : std::nullopt;
}

/// Where the hand takes hold in each of four reaching animations
std::optional<creature_reach::Points> MeasureReach(entt::entity creature, const Transform& transform,
                                                   const CreatureRig::ActionPoints& points,
                                                   const std::array<size_t, creature_reach::k_CornerCount>& animations,
                                                   float timeMs)
{
	creature_reach::Points reach {};
	for (size_t i = 0; i < animations.size(); ++i)
	{
		const auto hand = HandAt(creature, transform, points, animations.at(i), timeMs);
		if (!hand.has_value())
		{
			return std::nullopt;
		}
		reach.at(i) = *hand;
	}
	return reach;
}

float DurationOf(entt::entity creature, size_t animation)
{
	return Locator::creatureAnimationSystem::value().AnimationDuration(creature, animation).value_or(0.0f);
}

bool IsVillager(const ecs::Registry& registry, std::optional<entt::entity> entity)
{
	return entity.has_value() && registry.Valid(*entity) && registry.AllOf<Villager>(*entity);
}

/// How heavy something is, from the game's tables
std::optional<float> WeightOf(const ecs::Registry& registry, entt::entity entity)
{
	if (!Locator::infoConstants::has_value())
	{
		return std::nullopt;
	}
	const auto& info = Locator::infoConstants::value();
	if (const auto* villager = registry.TryGet<const Villager>(entity))
	{
		const auto kind = static_cast<size_t>(GVillagerInfo::Find(villager->tribe, villager->number));
		return kind < info.villager.size() ? std::optional(info.villager.at(kind).weight) : std::nullopt;
	}
	if (const auto* object = registry.TryGet<const MobileObject>(entity))
	{
		const auto kind = static_cast<size_t>(object->type);
		return kind < info.mobileObject.size() ? std::optional(info.mobileObject.at(kind).weight) : std::nullopt;
	}
	if (registry.AllOf<Ball>(entity))
	{
		return info.ball.weight;
	}
	if (const auto* creature = registry.TryGet<const Creature>(entity))
	{
		const auto row = creature::InfoRow(creature->species);
		return row < info.creature.size() ? std::optional(info.creature.at(row).weight) : std::nullopt;
	}
	return std::nullopt;
}

/// Something eaten is gone: out of its home and its town first
void Consume(ecs::Registry& registry, entt::entity food)
{
	if (const auto* villager = registry.TryGet<const Villager>(food))
	{
		if (auto* abode = registry.TryGet<Abode>(villager->abode))
		{
			abode->inhabitants.erase(food);
		}
		if (auto* town = registry.TryGet<Town>(villager->town))
		{
			town->homelessVillagers.erase(food);
		}
	}
	registry.Destroy(food);
}

/// A villager picked up stops wherever it was walking
void StopWalking(ecs::Registry& registry, entt::entity entity)
{
	registry.Remove<MoveStateLinearTag, MoveStateOrbitTag, MoveStateExitCircleTag, MoveStateStepThroughTag,
	                MoveStateFinalStepTag, MoveStateArrivedTag>(entity);
}

/// The blend of the four reaching animations for a target now, mirrored when it lies to the creature's left
struct Reach
{
	bool mirrored;
	creature_reach::Blend blend;
};
Reach ReachFor(const creature_reach::Points& points, const glm::vec3& local)
{
	const bool mirrored = creature_reach::ReachesMirrored(points, local);
	return {.mirrored = mirrored, .blend = creature_reach::Solve(mirrored ? creature_reach::Mirrored(points) : points, local)};
}

void SetSlots(CreatureObjectAction& action, std::span<const size_t> animations, std::span<const float> weights)
{
	action.animationCount = static_cast<uint8_t>(std::min(animations.size(), action.animations.size()));
	for (size_t i = 0; i < action.animationCount; ++i)
	{
		action.animations.at(i) = animations[i];
		action.weights.at(i) = weights[i];
	}
}

/// The body stops whatever else it plays and walking, and the animations start from their beginning
void BeginPlaying(entt::entity creature, CreatureObjectAction& action, CreatureAnimation& animation)
{
	if (Locator::creatureLocomotionSystem::has_value())
	{
		Locator::creatureLocomotionSystem::value().Stop(creature);
	}
	animation.body = {};
	action.phase = Phase::Playing;
	action.timeMs = 0.0f;
	action.durationMs = 0.0f;
	for (size_t i = 0; i < action.animationCount; ++i)
	{
		action.durationMs = std::max(action.durationMs, DurationOf(creature, action.animations.at(i)));
	}
	// Whatever it does at a moment happens by the end at the latest
	action.eventMs = std::min(action.eventMs, std::max(action.durationMs - 1.0f, 0.0f));
}

/// Whether the creature's body is acting out something other than a catch
bool BodyBusy(const CreatureObjectAction* doing)
{
	return doing != nullptr && doing->kind != Kind::Catch && doing->phase == Phase::Playing &&
	       (doing->status == Status::Running || doing->status == Status::Contact);
}

void Fail(CreatureObjectAction& action, std::string reason)
{
	action.status = Status::Failed;
	action.failure = std::move(reason);
}

/// The thing a creature catches is taken into its hand out of the air: it leaves the physics without going back on the
/// map, and a person or animal is held
void TakeInHand(ecs::Registry& registry, entt::entity creature, entt::entity object, const CreatureObjectAction& action,
                const CreatureRig::ActionPoints& points, const CreatureAnimation& animation, const Transform& transform)
{
	const auto& at = registry.Get<const Transform>(object);
	registry.AssignOrReplace<CreatureHeldObject>(
	    creature, CreatureHeldObject {.object = object,
	                                  .mirrored = action.mirrored,
	                                  .bone = HandBone(points, animation, action.mirrored),
	                                  .rotation = glm::transpose(transform.rotation) * at.rotation,
	                                  .middle = MiddleOf(registry, object)});
	registry.AssignOrReplace<HeldByCreature>(object, HeldByCreature {.creature = creature});
	// It leaves the physics with its end, not going back on the map
	if (Locator::dynamicsSystem::has_value() && Locator::dynamicsSystem::value().Find(object) != nullptr)
	{
		Locator::dynamicsSystem::value().RemoveObject(object, false, true);
	}
	if (registry.AllOf<Villager>(object))
	{
		ecs::villager_physics::IntoHand(object);
		StopWalking(registry, object);
	}
	else if (registry.AllOf<Animal>(object) && Locator::animalSystem::has_value())
	{
		Locator::animalSystem::value().IntoHand(object);
	}
}

/// The thing's centre and velocity while it is in the physics, flying or resting there
std::optional<std::pair<glm::vec3, glm::vec3>> FlightOf(entt::entity object)
{
	if (!Locator::dynamicsSystem::has_value())
	{
		return std::nullopt;
	}
	const auto* entry = Locator::dynamicsSystem::value().Find(object);
	if (entry == nullptr || entry->body == nullptr)
	{
		return std::nullopt;
	}
	return std::pair {entry->body->Centre(), entry->body->velocity};
}

/// The flat and high throws blended by how high the target is against where the hand lets go in each
std::array<float, 2> ThrowWeights(entt::entity creature, const Transform& transform, const CreatureRig::ActionPoints& points,
                                  const glm::vec3& target)
{
	const auto slope = [](const glm::vec3& local) { return local.y / std::max(-local.z, k_Tiny); };
	const auto flat = HandAt(creature, transform, points, creature_throw::k_HurlFlat, points.throwMs);
	const auto high = HandAt(creature, transform, points, creature_throw::k_HurlHigh, points.throwMs);
	if (!flat.has_value() || !high.has_value())
	{
		return {1.0f, 0.0f};
	}
	const auto weight = creature_throw::HighThrowWeight(slope(ToLocal(transform, target)), slope(*flat), slope(*high));
	return {1.0f - weight, weight};
}

/// Pointing: low and high to the side the target is on, or with the hand that is free, blended by how high it is
void SetPointing(CreatureObjectAction& action, const Transform& transform, const Creature& body, const CreatureHeldObject* held)
{
	const auto local = ToLocal(transform, action.point);
	// The left side is mirrored in the meshes' space: the right is +x
	const bool right = held != nullptr ? held->mirrored : local.x >= 0.0f;
	const auto height = creature_throw::k_HeightAtSizeOne * body.size * k_PointFromHeightShare;
	const auto elevation = std::atan2(local.y - height, std::max(glm::length(glm::vec2(local.x, local.z)), k_Tiny));
	const auto high = std::clamp(elevation / k_PointHighRadians, 0.0f, 1.0f);
	const std::array<size_t, 2> animations {
	    creature_object_actions::k_PointAnimations.at(right ? 1 : 0),
	    creature_object_actions::k_PointAnimations.at(right ? 3 : 2),
	};
	const std::array<float, 2> weights {1.0f - high, high};
	SetSlots(action, animations, weights);
	action.mirrored = false;
}

/// A vector along the ground turned by an angle
glm::vec2 Turned(glm::vec2 v, float angle)
{
	const auto c = std::cos(angle);
	const auto s = std::sin(angle);
	return {(v.x * c) - (v.y * s), (v.x * s) + (v.y * c)};
}

/// The point to turn to face so that a target lies in the direction of the middle of the reach, on whichever side
/// needs the smaller turn
glm::vec2 FacingToReach(const Transform& transform, const creature_reach::Points& reach, const glm::vec3& target)
{
	const auto from = glm::xz(transform.position);
	const auto towards = glm::xz(target) - from;
	const auto centre = creature_reach::Centre(reach);
	const glm::vec2 ahead {0.0f, -1.0f};
	const auto aheadInWorld = glm::xz(transform.rotation * glm::vec3(0.0f, 0.0f, -1.0f));
	std::optional<glm::vec2> best;
	float bestTurn = 0.0f;
	for (const auto side : {1.0f, -1.0f})
	{
		const glm::vec2 middle {centre.x * side, centre.z};
		if (glm::length(middle) < k_Tiny)
		{
			continue;
		}
		// How far round from ahead the middle of the reach is, and so which way to face for it to lie on the target
		const auto offset = std::atan2((ahead.x * middle.y) - (ahead.y * middle.x), glm::dot(ahead, middle));
		const auto facing = Turned(towards, -offset);
		const auto turn = std::acos(std::clamp(glm::dot(glm::normalize(facing), glm::normalize(aheadInWorld)), -1.0f, 1.0f));
		if (!best.has_value() || turn < bestTurn)
		{
			best = facing;
			bestTurn = turn;
		}
	}
	return from + best.value_or(towards);
}

/// One game turn of walking or turning up to what the creature acts on
void Approach(entt::entity creature, CreatureObjectAction& action, const Creature& body, const Transform& transform,
              CreatureAnimation& animation, const CreatureHeldObject* held)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto& locomotion = Locator::creatureLocomotionSystem::value();
	if (locomotion.IsMoving(creature))
	{
		return;
	}
	const auto* points = PointsOf(body);
	if (points == nullptr)
	{
		Fail(action, "its species has nothing to act with");
		return;
	}

	if (action.kind == Kind::Throw || action.kind == Kind::Point)
	{
		const auto limit = action.kind == Kind::Throw ? creature_object_actions::k_ThrowTurnRadians
		                                              : creature_object_actions::k_PointTurnRadians;
		if (BearingTo(transform, action.point) > limit && action.attempts < k_TurnAttempts)
		{
			++action.attempts;
			locomotion.TurnToFace(creature, glm::xz(action.point));
			return;
		}
		if (action.kind == Kind::Throw)
		{
			action.flightSeconds = creature_throw::FlightTime(glm::distance(transform.position, action.point));
			const auto weights = ThrowWeights(creature, transform, *points, action.point);
			const std::array<size_t, 2> animations {creature_throw::k_HurlFlat, creature_throw::k_HurlHigh};
			SetSlots(action, animations, weights);
			action.mirrored = held != nullptr && held->mirrored;
			action.eventMs = points->throwMs;
		}
		else
		{
			SetPointing(action, transform, body, held);
			action.holdMs = creature_object_actions::k_PointSeconds * 1000.0f;
		}
		BeginPlaying(creature, action, animation);
		return;
	}

	// Walking up to something and reaching for it
	const auto* at = action.target.has_value() && registry.Valid(*action.target)
	                     ? registry.TryGet<const Transform>(*action.target)
	                     : nullptr;
	if (at == nullptr)
	{
		Fail(action, "it has gone");
		return;
	}
	if (!action.reach.has_value())
	{
		const bool pickingUp = action.kind == Kind::PickUp;
		action.eventMs = pickingUp ? points->pickUpMs : points->destroyMs;
		action.reach =
		    MeasureReach(creature, transform, *points,
		                 pickingUp ? creature_reach::k_PickUpAnimations : creature_reach::k_DestroyAnimations, action.eventMs);
		if (!action.reach.has_value())
		{
			// Not posed yet: it tries again next turn
			return;
		}
		action.maxReach = creature_reach::MaxReach(*action.reach);
	}
	const auto local = ToLocal(transform, at->position);
	const auto reach = ReachFor(*action.reach, local);
	if (reach.blend.inRange)
	{
		const auto& animations =
		    action.kind == Kind::PickUp ? creature_reach::k_PickUpAnimations : creature_reach::k_DestroyAnimations;
		SetSlots(action, animations, reach.blend.weights);
		action.mirrored = reach.mirrored;
		BeginPlaying(creature, action, animation);
		return;
	}
	if (action.attempts >= k_ReachAttempts)
	{
		Fail(action, "it couldn't get in reach");
		return;
	}
	++action.attempts;
	const auto centre = creature_reach::Centre(*action.reach);
	const auto ideal = std::max(glm::length(glm::vec2(centre.x, centre.z)), k_Tiny);
	const auto distance = glm::length(glm::vec2(local.x, local.z));
	// Near enough, it only needs to turn so that it lies where its reach is middling
	if (std::abs(distance - ideal) <= ideal * k_TurnInReachShare)
	{
		locomotion.TurnToFace(creature, FacingToReach(transform, *action.reach, at->position));
		return;
	}
	using MoveResult = CreatureLocomotionSystemInterface::MoveResult;
	const auto result = locomotion.MoveTo(creature, glm::xz(at->position), CreatureLocomotionSystemInterface::Pace::Walk,
	                                      ideal * k_ApproachNear, ideal * k_ApproachFar);
	if (result != MoveResult::Started)
	{
		Fail(action, "navigation failed");
	}
}
} // namespace

namespace
{
/// The catch starts with the four catching animations, the thing flying across the land fast enough: the creature turns
/// to face it first when it comes from too far round, and where its hand closes in each animation is measured
bool StartCatch(entt::entity creature, CreatureObjectAction& action, const CreatureRig::ActionPoints& points)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto flight = action.target.has_value() ? FlightOf(*action.target) : std::nullopt;
	const bool hasAnimations = std::ranges::all_of(creature_catch::k_CatchAnimations,
	                                               [creature](size_t clip) { return DurationOf(creature, clip) > 0.0f; });
	if (!hasAnimations || !flight.has_value() ||
	    glm::dot(glm::xz(flight->second), glm::xz(flight->second)) < creature_catch::k_LeastSpeedSquared)
	{
		Fail(action, "it can't catch that");
		return false;
	}
	const auto& transform = registry.Get<const Transform>(creature);
	// It faces back along the thing's flight
	const auto towards = transform.position - glm::vec3(flight->second.x, 0.0f, flight->second.z);
	if (BearingTo(transform, towards) > creature_catch::k_TurnAngle && Locator::creatureLocomotionSystem::has_value())
	{
		Locator::creatureLocomotionSystem::value().TurnToFace(creature, glm::xz(towards));
	}
	if (!Locator::creatureAnimationSystem::has_value())
	{
		Fail(action, "it can't catch that");
		return false;
	}
	// Where the hand closes is measured at the creature's first catch and kept
	if (!registry.AllOf<CatchHands>(creature))
	{
		auto& animations = Locator::creatureAnimationSystem::value();
		CatchHands measured;
		for (size_t i = 0; i < creature_catch::k_CatchAnimations.size(); ++i)
		{
			measured.hands.at(i) =
			    animations
			        .BoneInAnimation(creature, creature_catch::k_CatchAnimations.at(i), points.catchMs, points.rightHand, false)
			        .value_or(glm::vec3(0.0f));
		}
		registry.Assign<CatchHands>(creature, measured);
	}
	action.catchHands = registry.Get<const CatchHands>(creature).hands;
	const std::array<float, 4> weights {0.25f, 0.25f, 0.25f, 0.25f};
	SetSlots(action, creature_catch::k_CatchAnimations, weights);
	action.mirrored = false;
	action.eventMs = points.catchMs;
	action.catching = CreatureObjectAction::Catching::Ready;
	return true;
}

/// Whether a side step towards a catch may be taken: not while the creature moves, and not when the step would end inside
/// a cell it may not stand on, gathered over the block the game checks
bool StepAccepted(entt::entity creature, const Transform& transform, bool mirrored)
{
	if (!Locator::creatureLocomotionSystem::has_value() || !Locator::creatureAnimationSystem::has_value())
	{
		return false;
	}
	auto& locomotion = Locator::creatureLocomotionSystem::value();
	if (locomotion.IsMoving(creature))
	{
		return false;
	}
	const auto travel = Locator::creatureAnimationSystem::value().AnimationTravel(creature, creature_catch::k_CatchStep);
	if (!travel.has_value())
	{
		return false;
	}
	// Where the step ends: the step's travel, the other way across for the left hand, turned and sized as the creature is
	auto step = *travel;
	if (mirrored)
	{
		step.x = -step.x;
	}
	const auto end = transform.position + (transform.rotation * (step * transform.scale));
	// TODO(physics): the game also keeps the step out of the circles of the objects in the block's cells that a creature
	// walks round, as its route planner lays them out for each kind of object; openblack's routes don't lay them out as
	// the game does yet
	// The circles give way to where the creature's route last left it, which is where it stands
	const auto circles = creature_route::StepBlockCircles(locomotion.GetWalkableLand(), creature_route::StepBlock(end),
	                                                      glm::xz(transform.position));
	return !creature_route::InsideAny(glm::xz(end), circles);
}

/// Ready to catch: once it has turned, it waits for the thing, steps across to where it will pass when that is out of
/// reach, and starts the catch when the thing will arrive as the hand closes; behind it or too late, it gives up
void UpdateReady(ecs::Registry& registry, entt::entity creature, const Creature& body, const Transform& transform,
                 CreatureAnimation& animation, CreatureObjectAction& action, float step)
{
	using Catching = CreatureObjectAction::Catching;
	const auto flight = action.target.has_value() && registry.Valid(*action.target) ? FlightOf(*action.target) : std::nullopt;
	if (!flight.has_value())
	{
		action.status = Status::Done;
		animation.slots.clear();
		return;
	}
	const auto* points = PointsOf(body);
	if (action.catching == Catching::Stepping)
	{
		const float duration = DurationOf(creature, creature_catch::k_CatchStep);
		action.timeMs += step;
		if (action.timeMs >= duration)
		{
			action.catching = Catching::Ready;
			action.timeMs = 0.0f;
		}
		animation.slots.clear();
		animation.slots.push_back({.animation = creature_catch::k_CatchStep,
		                           .timeMs = std::clamp(action.timeMs, 0.0f, std::max(duration - 1.0f, 0.0f)),
		                           .weight = 1.0f,
		                           .mirrored = action.mirrored});
		return;
	}
	const bool turning =
	    Locator::creatureLocomotionSystem::has_value() && Locator::creatureLocomotionSystem::value().IsMoving(creature);
	if (turning || points == nullptr)
	{
		return;
	}
	const auto lead = points->catchMs / (1000.0f * creature_layers::PlaybackRate(body.size));
	const auto velocity = glm::transpose(transform.rotation) * flight->second;
	const auto ready =
	    creature_catch::ReadyToCatch(ToLocal(transform, flight->first), velocity, action.catchHands, transform.scale.x, lead);
	switch (ready.readiness)
	{
	case creature_catch::Readiness::Wait:
		// It stands breathing as it waits for the thing to come nearer
		animation.slots.clear();
		animation.slots.push_back(
		    {.animation = creature_layers::animations::k_Stand,
		     .timeMs = static_cast<float>(creature_animation::BreathTime(
		         animation.breathPhase, static_cast<uint32_t>(DurationOf(creature, creature_layers::animations::k_Stand)))),
		     .weight = 1.0f,
		     .mirrored = false});
		break;
	case creature_catch::Readiness::Step:
		if (!StepAccepted(creature, transform, ready.mirrored))
		{
			action.status = Status::Done;
			animation.slots.clear();
			break;
		}
		action.catching = Catching::Stepping;
		action.mirrored = ready.mirrored;
		action.timeMs = 0.0f;
		break;
	case creature_catch::Readiness::Catch:
		action.catching = Catching::Reaching;
		action.mirrored = ready.mirrored;
		action.timeMs = 0.0f;
		break;
	case creature_catch::Readiness::GiveUp:
		action.status = Status::Done;
		animation.slots.clear();
		break;
	}
}

/// Each frame of a catch: the four animations blended by where the thing is against where the hand closes in each, the
/// catch played on while reaching or once caught and played back after a miss, the hand closing on it at its moment
/// unless the blend had to lean too far, and the catch over at either end
void UpdateCatch(ecs::Registry& registry, entt::entity creature, const Creature& body, const Transform& transform,
                 CreatureAnimation& animation, CreatureObjectAction& action, float step)
{
	using Catching = CreatureObjectAction::Catching;
	const auto* points = PointsOf(body);
	const auto flight = action.target.has_value() && registry.Valid(*action.target) ? FlightOf(*action.target) : std::nullopt;
	creature_catch::Blend blend;
	if (flight.has_value())
	{
		blend = creature_catch::Weigh(ToLocal(transform, flight->first), action.catchHands, transform.scale.x, action.mirrored);
		action.catchHeight = blend.height;
	}
	else
	{
		// Once the thing has left the physics, missed or caught, the catch is drawn back to its start: a caught thing
		// leaves on the frame after it is taken, so the creature draws its catch back with the thing in its hand
		blend = creature_catch::WeighWithout(action.catchHeight);
		action.catching = Catching::Missed;
	}
	action.weights = blend.weights;
	const float duration = DurationOf(creature, creature_catch::k_CatchAnimations.front());
	if (action.catching == Catching::Missed)
	{
		if (action.timeMs < step)
		{
			action.status = Status::Done;
		}
		action.timeMs = std::max(action.timeMs - step, 0.0f);
	}
	else
	{
		if (action.timeMs + step >= duration)
		{
			action.status = Status::Done;
		}
		action.timeMs += step;
		if (action.catching == Catching::Reaching && action.timeMs >= action.eventMs && points != nullptr)
		{
			action.eventDone = true;
			if (blend.clamped || !flight.has_value())
			{
				action.catching = Catching::Missed;
			}
			else
			{
				action.catching = Catching::Caught;
				TakeInHand(registry, creature, *action.target, action, *points, animation, transform);
				action.status = Status::Contact;
			}
		}
	}
	animation.slots.clear();
	if (action.status == Status::Done)
	{
		return;
	}
	for (size_t i = 0; i < action.animationCount; ++i)
	{
		animation.slots.push_back({.animation = action.animations.at(i),
		                           .timeMs = std::clamp(action.timeMs, 0.0f, std::max(duration - 1.0f, 0.0f)),
		                           .weight = action.weights.at(i),
		                           .mirrored = action.mirrored});
	}
}
} // namespace

bool CreatureObjectActionSystem::Start(entt::entity creature, CreatureObjectAction action)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(creature) || !registry.AllOf<Creature, CreatureAnimation, Transform>(creature))
	{
		return false;
	}
	const auto& body = registry.Get<const Creature>(creature);
	const auto* held = registry.TryGet<const CreatureHeldObject>(creature);
	action.status = Status::Running;
	action.phase = Phase::Approach;
	switch (creature_object_actions::HandsFor(action.kind))
	{
	case creature_object_actions::Hands::Holding:
		if (held == nullptr)
		{
			Fail(action, "it has nothing in its hand");
		}
		break;
	case creature_object_actions::Hands::Empty:
		if (held != nullptr)
		{
			Fail(action, "its hands are full");
		}
		break;
	case creature_object_actions::Hands::Either:
		break;
	}
	const auto* points = PointsOf(body);
	if (action.status == Status::Running && points == nullptr)
	{
		Fail(action, "its species has nothing to act with");
	}
	if (action.status == Status::Running && action.target.has_value())
	{
		if (!registry.Valid(*action.target) || !registry.AllOf<Transform>(*action.target))
		{
			Fail(action, "there is nothing there");
		}
		else if (action.kind == Kind::PickUp && (!CanPickUp(*action.target) || registry.AllOf<HeldByCreature>(*action.target)))
		{
			Fail(action, "it can't be picked up");
		}
		else if (action.kind == Kind::Destroy && !CanDestroy(*action.target))
		{
			Fail(action, "it can't be knocked down");
		}
	}
	if (action.status == Status::Running && action.kind == Kind::Throw)
	{
		const auto& transform = registry.Get<const Transform>(creature);
		const auto ground = glm::distance(glm::xz(transform.position), glm::xz(action.point));
		if (!creature_throw::FarEnoughToThrow(ground, body.size))
		{
			Fail(action, "too close to throw at");
		}
	}

	if (action.status == Status::Running && points != nullptr)
	{
		// Acting on what it holds starts at once, with the hand it holds it in
		const auto single = [&](size_t animation, float eventMs) {
			const std::array<size_t, 1> animations {animation};
			const std::array<float, 1> weights {1.0f};
			SetSlots(action, animations, weights);
			action.mirrored = held != nullptr && held->mirrored;
			action.eventMs = eventMs;
		};
		bool now = true;
		switch (action.kind)
		{
		case Kind::PutDown:
			single(creature_throw::k_PutDown, points->putDownMs);
			break;
		case Kind::Discard:
			single(creature_throw::k_Discard, points->discardMs);
			break;
		case Kind::Lob:
		{
			// The game times the gentle lob by nothing of its own: it lets go as far through as tossing away does
			const auto discard = DurationOf(creature, creature_throw::k_Discard);
			const auto lob = DurationOf(creature, creature_throw::k_GentleLob);
			single(creature_throw::k_GentleLob, discard > 0.0f ? points->discardMs * lob / discard : points->discardMs);
			break;
		}
		case Kind::Eat:
			single(creature_throw::k_Eat, points->eatMs);
			break;
		case Kind::Keep:
			// The animation was chosen as the action was asked for
			single(action.animations.front(), 0.0f);
			action.eventDone = true;
			break;
		case Kind::PickUp:
		case Kind::Destroy:
		case Kind::Throw:
		case Kind::Point:
			now = false;
			break;
		case Kind::Catch:
			now = StartCatch(creature, action, *points);
			break;
		}
		if (now)
		{
			BeginPlaying(creature, action, registry.Get<CreatureAnimation>(creature));
		}
	}
	const bool started = action.status == Status::Running;
	registry.AssignOrReplace<CreatureObjectAction>(creature, std::move(action));
	return started;
}

bool CreatureObjectActionSystem::Catch(entt::entity creature, entt::entity object)
{
	auto& registry = Locator::entitiesRegistry::value();
	// While its body acts out something else the catch waits, tried again each frame
	if (BodyBusy(registry.TryGet<const CreatureObjectAction>(creature)))
	{
		registry.AssignOrReplace<PendingCatch>(creature, PendingCatch {.object = object});
		return true;
	}
	registry.Remove<PendingCatch>(creature);
	return Start(creature, {.kind = Kind::Catch, .target = object});
}

void CreatureObjectActionSystem::StartPendingCatches()
{
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<std::pair<entt::entity, entt::entity>> due;
	registry.Each<const PendingCatch>([&](entt::entity creature, const PendingCatch& pending) {
		if (!BodyBusy(registry.TryGet<const CreatureObjectAction>(creature)))
		{
			due.emplace_back(creature, pending.object);
		}
	});
	for (const auto& [creature, object] : due)
	{
		registry.Remove<PendingCatch>(creature);
		if (registry.Valid(object))
		{
			Start(creature, {.kind = Kind::Catch, .target = object});
		}
	}
}

bool CreatureObjectActionSystem::PickUp(entt::entity creature, entt::entity object)
{
	return Start(creature, {.kind = Kind::PickUp, .target = object});
}

bool CreatureObjectActionSystem::PutDown(entt::entity creature)
{
	return Start(creature, {.kind = Kind::PutDown});
}

bool CreatureObjectActionSystem::Discard(entt::entity creature)
{
	return Start(creature, {.kind = Kind::Discard});
}

bool CreatureObjectActionSystem::Lob(entt::entity creature)
{
	return Start(creature, {.kind = Kind::Lob});
}

bool CreatureObjectActionSystem::EatHeld(entt::entity creature)
{
	const auto held = GetHeld(creature);
	if (held.has_value() && !FoodValueOf(*held).has_value())
	{
		auto action = CreatureObjectAction {.kind = Kind::Eat};
		Fail(action, "what it holds can't be eaten");
		Locator::entitiesRegistry::value().AssignOrReplace<CreatureObjectAction>(creature, std::move(action));
		return false;
	}
	return Start(creature, {.kind = Kind::Eat, .target = held});
}

bool CreatureObjectActionSystem::Keep(entt::entity creature, size_t animation)
{
	CreatureObjectAction action {.kind = Kind::Keep};
	action.animations.front() = animation;
	return Start(creature, std::move(action));
}

bool CreatureObjectActionSystem::Throw(entt::entity creature, const glm::vec3& target)
{
	return Start(creature, {.kind = Kind::Throw, .point = target});
}

bool CreatureObjectActionSystem::Destroy(entt::entity creature, entt::entity target)
{
	return Start(creature, {.kind = Kind::Destroy, .target = target});
}

bool CreatureObjectActionSystem::PointAt(entt::entity creature, const glm::vec3& point)
{
	return Start(creature, {.kind = Kind::Point, .point = point});
}

void CreatureObjectActionSystem::Cancel(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (registry.Valid(creature))
	{
		registry.Remove<CreatureObjectAction>(creature);
		if (auto* animation = registry.TryGet<CreatureAnimation>(creature))
		{
			animation->slots.clear();
		}
	}
}

void CreatureObjectActionSystem::Drop(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto held = GetHeld(creature);
	if (!held.has_value())
	{
		return;
	}
	const auto* at = registry.TryGet<const Transform>(*held);
	Release(creature, at != nullptr ? at->position : registry.Get<const Transform>(creature).position, glm::vec3(0.0f));
}

void CreatureObjectActionSystem::Release(entt::entity creature, const glm::vec3& position, const glm::vec3& velocity)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* held = registry.TryGet<const CreatureHeldObject>(creature);
	if (held == nullptr)
	{
		return;
	}
	const auto object = held->object;
	registry.Remove<CreatureHeldObject>(creature);
	if (!registry.Valid(object))
	{
		return;
	}
	registry.Remove<HeldByCreature>(object);
	if (auto* transform = registry.TryGet<Transform>(object))
	{
		transform->position = position;
	}
	LetGo(creature, object, velocity);
}

void CreatureObjectActionSystem::LetGo(entt::entity creature, entt::entity object, const glm::vec3& velocity)
{
	if (!Locator::dynamicsSystem::has_value())
	{
		return;
	}
	// It leaves the hand into the physics as a god's hand lets things go, put down or thrown, turning a little about
	// the up axis (the game's turning is the other way round from the physics' own), credited to the creature's player
	constexpr glm::vec3 k_ReleaseTurn {0.0f, -1.0f, 0.0f};
	auto& registry = Locator::entitiesRegistry::value();
	const auto* owner = registry.Valid(creature) ? registry.TryGet<const Creature>(creature) : nullptr;
	Locator::dynamicsSystem::value().LetGoFromHand(object,
	                                               {.velocity = velocity,
	                                                .angularMomentum = k_ReleaseTurn,
	                                                .player = owner != nullptr ? std::optional(owner->owner) : std::nullopt,
	                                                .creature = registry.Valid(creature) ? creature : entt::null});
	// The creature remembers it as the last thing it dropped
	if (registry.Valid(creature))
	{
		registry.AssignOrReplace<CreatureDroppedObject>(creature, CreatureDroppedObject {.object = object});
	}
}

CreatureObjectActionSystem::State CreatureObjectActionSystem::GetState(entt::entity creature) const
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* action = registry.Valid(creature) ? registry.TryGet<const CreatureObjectAction>(creature) : nullptr;
	if (action == nullptr)
	{
		return State::Idle;
	}
	switch (action->status)
	{
	case Status::Running:
	case Status::Contact:
		return State::Busy;
	case Status::Done:
		return State::Done;
	case Status::Failed:
		break;
	}
	return State::Failed;
}

std::optional<float> CreatureObjectActionSystem::GetProgress(entt::entity creature) const
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* action = registry.Valid(creature) ? registry.TryGet<const CreatureObjectAction>(creature) : nullptr;
	if (action == nullptr || action->phase != Phase::Playing ||
	    (action->status != Status::Running && action->status != Status::Contact))
	{
		return std::nullopt;
	}
	if (action->holdMs > 0.0f || action->durationMs <= 0.0f)
	{
		return 0.0f;
	}
	return std::clamp(action->timeMs / action->durationMs, 0.0f, 1.0f);
}

std::optional<entt::entity> CreatureObjectActionSystem::GetHeld(entt::entity creature) const
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* held = registry.Valid(creature) ? registry.TryGet<const CreatureHeldObject>(creature) : nullptr;
	return held != nullptr && registry.Valid(held->object) ? std::optional(held->object) : std::nullopt;
}

std::optional<float> CreatureObjectActionSystem::FoodValueOf(entt::entity object) const
{
	const auto& registry = Locator::entitiesRegistry::value();
	if (!Locator::infoConstants::has_value() || !registry.Valid(object))
	{
		return std::nullopt;
	}
	const auto& info = Locator::infoConstants::value();
	float value = 0.0f;
	if (const auto* villager = registry.TryGet<const Villager>(object))
	{
		const auto kind = static_cast<size_t>(GVillagerInfo::Find(villager->tribe, villager->number));
		value = kind < info.villager.size() ? info.villager.at(kind).foodValue : 0.0f;
	}
	else if (const auto* mobile = registry.TryGet<const MobileObject>(object))
	{
		const auto kind = static_cast<size_t>(mobile->type);
		value = kind < info.mobileObject.size() ? info.mobileObject.at(kind).foodValue : 0.0f;
	}
	else if (const auto* pot = registry.TryGet<const Pot>(object))
	{
		// A pot or pile of food is worth as much as the food in it. A storage pit's pile is eaten from where it lies,
		// which isn't done here.
		const auto kind = static_cast<size_t>(pot->type);
		const bool food = kind < info.pot.size() && info.pot.at(kind).resourceType == ResourceType::Food &&
		                  pot->type != PotInfo::StoragePitFoodPile;
		value = food ? static_cast<float>(pot->amount) : 0.0f;
	}
	return value > 0.0f ? std::optional(value) : std::nullopt;
}

bool CreatureObjectActionSystem::CanPickUp(entt::entity object) const
{
	const auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object) || !registry.AllOf<Transform>(object))
	{
		return false;
	}
	// Of pots and piles, only food can be picked up, to eat
	return registry.AnyOf<MobileObject, Ball, Villager>(object) ||
	       (registry.AllOf<Pot>(object) && FoodValueOf(object).has_value());
}

bool CreatureObjectActionSystem::CanDestroy(entt::entity target) const
{
	const auto& registry = Locator::entitiesRegistry::value();
	return registry.Valid(target) && registry.AllOf<Transform>(target) &&
	       registry.AnyOf<Tree, Abode, MobileObject, Ball>(target);
}

void CreatureObjectActionSystem::ProcessTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!Locator::creatureLocomotionSystem::has_value() || !Locator::creatureAnimationSystem::has_value())
	{
		return;
	}

	// What is held by a creature that is gone, or no longer holds it, falls
	std::vector<entt::entity> loose;
	registry.Each<const HeldByCreature>([&](entt::entity entity, const HeldByCreature& by) {
		const auto* held = registry.Valid(by.creature) ? registry.TryGet<const CreatureHeldObject>(by.creature) : nullptr;
		if (held == nullptr || held->object != entity)
		{
			loose.push_back(entity);
		}
	});
	for (const auto entity : loose)
	{
		registry.Remove<HeldByCreature>(entity);
		LetGo(entt::null, entity, glm::vec3(0.0f));
	}
	std::vector<entt::entity> emptyHanded;
	registry.Each<const CreatureHeldObject>([&](entt::entity entity, const CreatureHeldObject& held) {
		if (!registry.Valid(held.object))
		{
			emptyHanded.push_back(entity);
		}
	});
	for (const auto entity : emptyHanded)
	{
		registry.Remove<CreatureHeldObject>(entity);
	}

	registry.Each<const Creature, const Transform, CreatureAnimation, CreatureObjectAction>(
	    [&](entt::entity entity, const Creature& body, const Transform& transform, CreatureAnimation& animation,
	        CreatureObjectAction& action) {
		    if (action.status == Status::Running && action.phase == Phase::Approach)
		    {
			    Approach(entity, action, body, transform, animation, registry.TryGet<const CreatureHeldObject>(entity));
		    }
	    });

	// Carrying something heavy makes it stronger as it walks about
	registry.Each<const Creature, CreatureNeeds>([&](entt::entity entity, const Creature&, CreatureNeeds& needs) {
		needs.carriedWeight.reset();
		const auto held = GetHeld(entity);
		if (!held.has_value())
		{
			return;
		}
		const auto weight = WeightOf(registry, *held);
		const auto own = WeightOf(registry, entity);
		if (weight.has_value() && own.has_value() && *own > 0.0f)
		{
			needs.carriedWeight = std::clamp(*weight / *own, 0.0f, 1.0f);
		}
	});

	// The town sees what the creature does with fear or respect, and keeps that view a while after
	registry.Each<const Creature>([&](entt::entity entity, const Creature&) {
		const auto* action = registry.TryGet<const CreatureObjectAction>(entity);
		const bool acting = action != nullptr && (action->status == Status::Running || action->status == Status::Contact);
		const auto held = GetHeld(entity);
		const bool holdingVillager = IsVillager(registry, held);
		auto attitude = creature_object_actions::TownAttitude::None;
		if (acting)
		{
			attitude = creature_object_actions::AttitudeTo(action->kind, IsVillager(registry, action->target), holdingVillager);
		}
		else if (holdingVillager)
		{
			attitude = creature_object_actions::TownAttitude::Fear;
		}
		auto* seen = registry.TryGet<CreatureTownAttitude>(entity);
		if (attitude != creature_object_actions::TownAttitude::None)
		{
			registry.AssignOrReplace<CreatureTownAttitude>(
			    entity,
			    CreatureTownAttitude {.attitude = attitude, .secondsLeft = creature_object_actions::AttitudeSeconds(attitude)});
		}
		else if (seen != nullptr && seen->attitude != creature_object_actions::TownAttitude::None)
		{
			seen->secondsLeft -= k_TurnSeconds;
			if (seen->secondsLeft <= 0.0f)
			{
				*seen = {};
			}
		}
	});
}

void CreatureObjectActionSystem::Update(std::chrono::duration<float, std::milli> gameTime)
{
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<const Creature, const Transform, CreatureAnimation, CreatureObjectAction>(
	    [&](entt::entity entity, const Creature& body, const Transform& transform, CreatureAnimation& animation,
	        CreatureObjectAction& action) {
		    if (action.phase != Phase::Playing || (action.status != Status::Running && action.status != Status::Contact))
		    {
			    return;
		    }
		    const float step = gameTime.count() * creature_layers::PlaybackRate(body.size);
		    if (action.kind == Kind::Catch)
		    {
			    using Catching = CreatureObjectAction::Catching;
			    if (action.catching == Catching::Ready || action.catching == Catching::Stepping)
			    {
				    UpdateReady(registry, entity, body, transform, animation, action, step);
			    }
			    if (action.catching != Catching::Ready && action.catching != Catching::Stepping &&
			        action.status != Status::Done)
			    {
				    UpdateCatch(registry, entity, body, transform, animation, action, step);
			    }
			    return;
		    }
		    action.timeMs += step;

		    // Until it has hold, the reach follows what it reaches for, and fails when it gets out of reach
		    if ((action.kind == Kind::PickUp || action.kind == Kind::Destroy) && !action.eventDone && action.reach.has_value())
		    {
			    const auto* at = action.target.has_value() && registry.Valid(*action.target)
			                         ? registry.TryGet<const Transform>(*action.target)
			                         : nullptr;
			    if (at == nullptr)
			    {
				    Fail(action, "it has gone");
				    animation.slots.clear();
				    return;
			    }
			    const auto local = ToLocal(transform, at->position);
			    const auto reach =
			        creature_reach::Solve(action.mirrored ? creature_reach::Mirrored(*action.reach) : *action.reach, local);
			    if (!reach.inRange)
			    {
				    Fail(action, "it got out of reach");
				    animation.slots.clear();
				    return;
			    }
			    action.weights = reach.weights;
		    }

		    if (action.holdMs > 0.0f)
		    {
			    action.holdMs -= gameTime.count();
			    if (action.holdMs <= 0.0f)
			    {
				    action.status = Status::Done;
			    }
		    }
		    else if (action.timeMs >= action.durationMs && (action.eventDone || action.eventMs > action.durationMs))
		    {
			    action.status = Status::Done;
		    }
		    if (action.status == Status::Done)
		    {
			    animation.slots.clear();
			    return;
		    }

		    animation.slots.clear();
		    for (size_t i = 0; i < action.animationCount; ++i)
		    {
			    const auto duration = DurationOf(entity, action.animations.at(i));
			    // Pointing loops; everything else holds its last frame
			    const auto time = action.holdMs > 0.0f && duration > 0.0f
			                          ? std::fmod(action.timeMs, duration)
			                          : std::clamp(action.timeMs, 0.0f, std::max(duration - 1.0f, 0.0f));
			    animation.slots.push_back({.animation = action.animations.at(i),
			                               .timeMs = time,
			                               .weight = action.weights.at(i),
			                               .mirrored = action.mirrored});
		    }
	    });
	StartPendingCatches();
}

void CreatureObjectActionSystem::LateUpdate(std::chrono::duration<float, std::milli> gameTime)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto seconds = std::chrono::duration<float>(gameTime).count();

	// The moments things are taken hold of and let go of, with the hand where the body was posed this frame
	struct Moment
	{
		entt::entity creature;
	};
	std::vector<Moment> moments;
	registry.Each<CreatureObjectAction>([&](entt::entity entity, const CreatureObjectAction& action) {
		if (action.phase == Phase::Playing && action.status == Status::Running && !action.eventDone &&
		    action.timeMs >= action.eventMs)
		{
			moments.push_back({entity});
		}
	});
	for (const auto& moment : moments)
	{
		const auto creature = moment.creature;
		auto& action = registry.Get<CreatureObjectAction>(creature);
		const auto& body = registry.Get<const Creature>(creature);
		const auto& transform = registry.Get<const Transform>(creature);
		const auto& animation = registry.Get<const CreatureAnimation>(creature);
		const auto* points = PointsOf(body);
		action.eventDone = true;
		if (points == nullptr)
		{
			continue;
		}
		const auto handPosition = PalmOf(HandBone(*points, animation, action.mirrored), animation, PlacementOf(transform));
		switch (action.kind)
		{
		case Kind::PickUp:
		{
			const auto object = *action.target;
			if (!registry.Valid(object) || registry.AllOf<HeldByCreature>(object))
			{
				Fail(action, "it has gone");
				break;
			}
			const auto& at = registry.Get<const Transform>(object);
			registry.AssignOrReplace<CreatureHeldObject>(
			    creature, CreatureHeldObject {.object = object,
			                                  .mirrored = action.mirrored,
			                                  .bone = HandBone(*points, animation, action.mirrored),
			                                  .rotation = glm::transpose(transform.rotation) * at.rotation,
			                                  .middle = MiddleOf(registry, object)});
			registry.AssignOrReplace<HeldByCreature>(object, HeldByCreature {.creature = creature});
			// Taken out of the air, it leaves the physics without going back on the map
			if (Locator::dynamicsSystem::has_value() && Locator::dynamicsSystem::value().IsFlying(object))
			{
				Locator::dynamicsSystem::value().RemoveObject(object, false, true);
				// What its landing set, the creature's hand overrides: a person or animal is held
				if (registry.AllOf<Villager>(object))
				{
					villager_physics::IntoHand(object);
				}
				else if (registry.AllOf<Animal>(object) && Locator::animalSystem::has_value())
				{
					Locator::animalSystem::value().IntoHand(object);
				}
			}
			if (registry.AllOf<Villager>(object))
			{
				StopWalking(registry, object);
			}
			action.status = Status::Contact;
			break;
		}
		case Kind::Destroy:
		{
			const auto target = *action.target;
			action.status = Status::Contact;
			if (!registry.Valid(target))
			{
				break;
			}
			// A building is broken about where it stands, and a rock is smashed in two; the blow does nothing to anything
			// else
			if (registry.AnyOf<Abode, SpellDispenser>(target))
			{
				if (Locator::buildingDamageSystem::has_value())
				{
					Locator::buildingDamageSystem::value().Smash(target, creature, body.size);
				}
			}
			else if (object_physics::IsRock(target) && Locator::dynamicsSystem::has_value())
			{
				object_physics::SmashRock(Locator::dynamicsSystem::value(), target);
			}
			break;
		}
		case Kind::PutDown:
			Release(creature, handPosition, glm::vec3(0.0f));
			break;
		case Kind::Discard:
		case Kind::Lob:
		{
			const auto animationIndex = action.animations.front();
			const auto now = HandAt(creature, transform, *points, animationIndex, action.eventMs);
			const auto before =
			    HandAt(creature, transform, *points, animationIndex, action.eventMs - creature_throw::k_HandSpeedSpanMs);
			const auto velocity = now.has_value() && before.has_value()
			                          ? creature_throw::TossVelocity(
			                                creature_throw::HandVelocity(*now, *before, creature_throw::k_HandSpeedSpanMs),
			                                action.mirrored, transform.rotation, creature_throw::k_DiscardSpeedShare)
			                          : glm::vec3(0.0f);
			Release(creature, handPosition, velocity);
			break;
		}
		case Kind::Throw:
			Release(creature, handPosition,
			        creature_throw::ReleaseVelocity(action.point, handPosition, std::max(action.flightSeconds, k_Tiny)));
			break;
		case Kind::Eat:
		{
			const auto held = GetHeld(creature);
			if (!held.has_value())
			{
				break;
			}
			const auto value = FoodValueOf(*held);
			registry.Remove<CreatureHeldObject>(creature);
			Consume(registry, *held);
			if (value.has_value() && Locator::creaturePhysiologySystem::has_value())
			{
				Locator::creaturePhysiologySystem::value().Eat(creature, *value);
			}
			break;
		}
		case Kind::Keep:
		case Kind::Point:
		case Kind::Catch:
			break;
		}
	}

	// What is held rides in the hand, turned with it
	bool moved = false;
	registry.Each<const Creature, const Transform, const CreatureAnimation, const CreatureHeldObject>(
	    [&](const Creature&, const Transform& transform, const CreatureAnimation& animation, const CreatureHeldObject& held) {
		    auto* at = registry.Valid(held.object) ? registry.TryGet<Transform>(held.object) : nullptr;
		    if (at == nullptr)
		    {
			    return;
		    }
		    // Held by its middle in the palm, turned with the creature
		    const auto placement = PlacementOf(transform);
		    at->rotation = transform.rotation * held.rotation;
		    at->position = PalmOf(held.bone, animation, placement) - (at->rotation * held.middle);
		    moved = true;
	    });

	if (moved)
	{
		registry.SetDirty();
	}
}
